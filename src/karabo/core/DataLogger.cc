/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/io/Input.hh>
#include "DataLogger.hh"
#include <karabo/karabo.hpp>
#include <karabo/io/TextSerializer.hh>
#include <karabo/util/Schema.hh>

namespace karabo {
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<karabo::core::OkErrorFsm>, DataLogger)

        void DataLogger::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected).key("deviceToBeLogged")
                    .displayedName("Device to be logged")
                    .description("The device that should be logged by this logger instance")
                    .assignmentMandatory()
                    .commit();

            PATH_ELEMENT(expected).key("directory")
                    .displayedName("Directory")
                    .description("The directory where the log files should be placed")
                    .assignmentMandatory()
                    .commit();

            INT32_ELEMENT(expected).key("maximumFileSize")
                    .displayedName("Maximum file size")
                    .description("After any archived file has reached this size it will be time-stamped and not appended anymore")
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .assignmentMandatory()
                    .commit();

            INT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(60)
                    .reconfigurable()
                    .commit();

            // Do not archive the archivers (would lead to infinite recursion)
            OVERWRITE_ELEMENT(expected).key("archive")
                    .setNewDefaultValue(false)
                    .commit();

            // Hide the loggers from the standard view in clients
            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();

            // Slow beats
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();
        }

        DataLogger::DataLogger(const Hash& input) : karabo::core::Device<karabo::core::OkErrorFsm>(input) {
            input.get("deviceToBeLogged", m_deviceToBeLogged);
        }

        void DataLogger::okStateOnEntry() {

            m_user = "."; //TODO:  Define proper user running a device. The dot is unknown user?
            m_pendingLogin = false;

            // Register slots
            SLOT2(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
            SLOT2(slotSchemaUpdated, Schema /*changedSchema*/, string /*deviceId*/);
            SLOT2(slotTagDeviceToBeDiscontinued, bool /*wasValidUpToNow*/, char /*reason*/);

            connect(m_deviceToBeLogged, "signalChanged", "", "slotChanged", NO_TRACK);
            connect(m_deviceToBeLogged, "signalSchemaUpdated", "", "slotSchemaUpdated", NO_TRACK);

            if (!boost::filesystem::exists(get<string>("directory"))) {
                boost::filesystem::create_directory(get<string>("directory"));
            }

            m_lastIndex = determineLastIndex(m_deviceToBeLogged);

            slotTagDeviceToBeDiscontinued(false, 'L'); // 2nd arg means: device was not valid up to now, 3rd means logger
            refreshDeviceInformation();

        }

        void DataLogger::refreshDeviceInformation() {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "refreshDeviceInformation " << m_deviceToBeLogged;
                requestNoWait(m_deviceToBeLogged, "slotGetSchema", "", "slotSchemaUpdated", false);
                requestNoWait(m_deviceToBeLogged, "slotGetConfiguration", "", "slotChanged");
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_INIT_EXCEPTION("Could not create new entry for " + m_deviceToBeLogged));
            }
        }

        void DataLogger::slotTagDeviceToBeDiscontinued(const bool wasValidUpToNow, const char reason) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotTagDeviceToBeDiscontinued " << wasValidUpToNow << " '" << reason << "'";
            try {
                string filename = get<string>("directory") + "/" + m_deviceToBeLogged + "_configuration_" + toString(m_lastIndex) + ".txt";
                string indexname = get<string>("directory") + "/" + m_deviceToBeLogged + "_index.txt";
                if (wasValidUpToNow) {
                    if (m_configStream.is_open()) {
                        m_configStream << m_lastDataTimestamp.toIso8601Ext() << "|" << fixed << m_lastDataTimestamp.toTimestamp()
                                << "|" << m_lastDataTimestamp.getSeconds() << "|" << m_lastDataTimestamp.getFractionalSeconds()
                                << "|" << m_lastDataTimestamp.getTrainId() << "|.|||" << m_user << "|LOGOUT\n";
                        m_configStream.flush();
                        m_configStream.seekg(0, ios::end);
                        long position = m_configStream.tellg();
                        m_configStream.close();
                        ofstream indexstream(indexname.c_str(), ios::app);
                        indexstream.seekp(0, ios_base::end);
                        indexstream << "-LOG " << m_lastDataTimestamp.toIso8601Ext() << " " << fixed << m_lastDataTimestamp.toTimestamp()
                                << " " << m_lastDataTimestamp.getSeconds() << " " << m_lastDataTimestamp.getFractionalSeconds()
                                << " " << m_lastDataTimestamp.getTrainId() << " " << position << " " << m_user << " " << m_lastIndex << "\n";
                        indexstream.close();
                    }
                    return;
                }

                if (!m_configStream.is_open()) {
                    m_configStream.open(filename.c_str(), ios::in | ios::out | ios::app);
                    if (!m_configStream.is_open()) {
                        throw KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
                    }
                    // Make sure that the file contains '\n' (newline) at the end
                    m_configStream.seekg(0, ios::end);
                    long pos = m_configStream.tellg();
                    if (pos > 0) { // ... file is not empty!
                        string nl;
                        m_configStream.seekg(pos - 1, ios::beg);
                        m_configStream >> nl; // if newline exists the status will "fail" or "eof" because "nl" string is empty
                        if (m_configStream.fail()) m_configStream.clear(); // clear the status otherwise stream will not work
                        m_configStream.seekg(0); // without this statement the writing doesn't work
                        if (!nl.empty()) {
                            m_configStream << "\n";
                            m_configStream.flush();
                        }
                    }
                }
                m_pendingLogin = true;
                int millis = get<int>("flushInterval") * 1000;
                string timerId("flush-to-disk");
                startTimer(millis, boost::bind(&DataLogger::flushHandler, this, boost::static_pointer_cast<SignalSlotable>(shared_from_this()), timerId), timerId);
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_LOGIC_EXCEPTION("Problems tagging " + m_deviceToBeLogged + " to be discontinued"));
            }
        }

        void DataLogger::slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId) {

            if (m_currentSchema.empty()) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Schema for " << deviceId << " still empty";
                return;
            }
            if (!m_configStream.is_open()) {
                return;
            }
            m_user = getSenderInfo("slotChanged")->getUserIdOfSender();
            if (m_user.size() == 0) m_user = "operator";

            vector<string> paths;
            configuration.getPaths(paths);
            for (size_t i = 0; i < paths.size(); ++i) {
                const string& path = paths[i];
                const Hash::Node& leafNode = configuration.getNode(path);
                if (leafNode.getType() == Types::HASH) continue;
                // Skip those elements which should not be archived
                if (!m_currentSchema.has(path) || (m_currentSchema.hasArchivePolicy(path) && (m_currentSchema.getArchivePolicy(path) == Schema::NO_ARCHIVING))) continue;
                string value = leafNode.getValueAs<string>();
                string type = Types::to<ToLiteral>(leafNode.getType());
                Timestamp t = Timestamp::fromHashAttributes(leafNode.getAttributes());
                m_lastDataTimestamp = t;
                if (m_pendingLogin) {
                    m_pendingLogin = false;
                    m_configStream.flush();
                    if (m_configStream.fail()) m_configStream.clear();
                    m_configStream.seekg(0, ios::end); // position to EOF
                    long position = m_configStream.tellg(); // get file size
                    if (position == -1) {
                        string filename = get<string>("directory") + "/" + deviceId + "_configuration_" + toString(m_lastIndex) + ".txt";
                        throw KARABO_IO_EXCEPTION("Failed to position in file \"" + filename + "\"");
                    }
                    m_configStream << t.toIso8601Ext() << "|" << fixed << t.toTimestamp() << "|" << t.getSeconds() << "|" << t.getFractionalSeconds() << "|"
                            << t.getTrainId() << "|" << path << "|" << type << "|" << value << "|" << m_user << "|LOGIN\n";
                    string indexname = get<string>("directory") + "/" + deviceId + "_index.txt";
                    ofstream indexstream(indexname.c_str(), ios::app);
                    indexstream << "+LOG " << t.toIso8601Ext() << " " << fixed << t.toTimestamp() << " " << t.getSeconds() << " "
                            << t.getFractionalSeconds() << " " << t.getTrainId() << " " << position << " " << m_user << " " << m_lastIndex << "\n";
                    indexstream.close();
                } else {
                    m_configStream << t.toIso8601Ext() << "|" << fixed << t.toTimestamp() << "|" << t.getSeconds() << "|" << t.getFractionalSeconds()
                            << "|" << t.getTrainId() << "|" << path << "|" << type << "|" << value << "|" << m_user << "|VALID\n";
                }
            }
            long maxFilesize = get<int>("maximumFileSize") * 1000000; // times to 1000000 because maximumFilesSize in MBytes
            long position = m_configStream.tellp();
            if (maxFilesize <= position) {
                m_configStream.close();
                // increment index number for configuration file
                m_lastIndex = incrementLastIndex(deviceId);
                string filename = get<string>("directory") + "/" + deviceId + "_configuration_" + toString(m_lastIndex) + ".txt";
                m_configStream.open(filename.c_str(), ios::in | ios::out | ios::app);
                if (!m_configStream.is_open()) {
                    throw KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
                }
                // record changing the file into index file
                string indexname = get<string>("directory") + "/" + deviceId + "_index.txt";
                ofstream indexstream(indexname.c_str(), ios::app);
                indexstream << "=NEW " << m_lastDataTimestamp.toIso8601Ext() << " " << fixed << m_lastDataTimestamp.toTimestamp() << " " << m_lastDataTimestamp.getSeconds() << " "
                        << m_lastDataTimestamp.getFractionalSeconds() << " " << m_lastDataTimestamp.getTrainId() << " 0 " << m_user << " " << m_lastIndex << "\n";
                indexstream.close();
            }
        }

        void DataLogger::flushHandler(karabo::xms::SignalSlotable::Pointer pointer, const std::string& id) {
            if (m_configStream.is_open()) {
                m_configStream.flush();
            }
            int millis = get<int>("flushInterval") * 1000;
            startTimer(millis, boost::bind(&DataLogger::flushHandler, this, pointer, id), id);
        }
        
        void DataLogger::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotSchemaUpdated: Schema for " << deviceId << " arrived...";
            m_currentSchema = schema;
            string filename = get<string>("directory") + "/" + deviceId + "_schema.txt";
            fstream fileout(filename.c_str(), ios::out | ios::app);
            if (fileout.is_open()) {
                Timestamp t;
                TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create(Hash("Xml.indentation", -1));
                string archive;
                serializer->save(schema, archive);
                fileout << t.getSeconds() << " " << t.getFractionalSeconds() << " " << t.getTrainId() << " " << archive << "\n";
                fileout.close();
            } else
                throw KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
        }

        int DataLogger::determineLastIndex(const std::string& deviceId) {
            string lastIndexFilename = get<string>("directory") + "/" + deviceId + ".last";
            int idx;
            fstream fs;
            if (!boost::filesystem::exists(lastIndexFilename)) {
                for (size_t i = 0;; i++) {
                    string filename = get<string>("directory") + "/" + deviceId + "_configuration_" + toString(i) + ".txt";
                    if (!boost::filesystem::exists(filename)) {
                        idx = i;
                        break;
                    }
                }
                fs.open(lastIndexFilename.c_str(), ios::out | ios::app);
                fs << idx << "\n";
            } else {
                fs.open(lastIndexFilename.c_str(), ios::in);
                fs >> idx;
            }
            fs.close();
            return idx;
        }

        int DataLogger::incrementLastIndex(const std::string& deviceId) {
            string lastIndexFilename = get<string>("directory") + "/" + deviceId + ".last";
            int idx;
            if (!boost::filesystem::exists(lastIndexFilename)) {
                idx = determineLastIndex(deviceId);
            }
            fstream file(lastIndexFilename.c_str(), ios::in | ios::out);
            file >> idx;
            if (file.fail()) file.clear();
            ++idx;
            file.seekg(0);
            file << idx << "\n";
            file.close();
            return idx;
        }
    }
}
