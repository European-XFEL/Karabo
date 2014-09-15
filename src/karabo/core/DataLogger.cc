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

        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, DataLogger)

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
                    .setNewDefaultValue(120)
                    .commit();
        }

        DataLogger::DataLogger(const Hash& input) : Device<OkErrorFsm>(input) {
            input.get("deviceToBeLogged", m_deviceToBeLogged);
        }

        DataLogger::~DataLogger() {
            if (m_configStream.is_open())
                m_configStream.close();
        }

        void DataLogger::okStateOnEntry() {
            // Register slots
            SLOT2(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
            SLOT2(slotSchemaUpdated, Schema /*changedSchema*/, string /*deviceId*/);

            connectT(m_deviceToBeLogged, "signalChanged", "", "slotChanged");
            connectT(m_deviceToBeLogged, "signalSchemaUpdated", "", "slotSchemaUpdated");

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
                Schema schema = remote().getDeviceSchemaNoWait(m_deviceToBeLogged);
                Hash hash = remote().getConfigurationNoWait(m_deviceToBeLogged);

                // call slotSchemaUpdated updated by hand
                if (!schema.empty()) slotSchemaUpdated(schema, m_deviceToBeLogged);

                // call slotChanged by hand
                if (!hash.empty()) slotChanged(hash, m_deviceToBeLogged);

            } catch (...) {
                KARABO_RETHROW_AS(KARABO_INIT_EXCEPTION("Could not create new entry for " + m_deviceToBeLogged));
            }
        }

        void DataLogger::slotTagDeviceToBeDiscontinued(const bool wasValidUpToNow, const char reason) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotTagDeviceToBeDiscontinued " << wasValidUpToNow << " '" << reason << "'";
            try {
                if (wasValidUpToNow) {
                    if (m_configStream.is_open()) {
                        Timestamp t;
                        m_configStream << t.getSeconds() << " " << t.getFractionalSeconds() << " " << t.getTrainId() << " . . . . " << reason << "\n";
                        m_configStream.flush();
                    }
                    return;
                }
                string filename = get<string>("directory") + "/" + m_deviceToBeLogged + "_configuration.txt";
                if (!m_configStream.is_open()) {
                    m_configStream.open(filename.c_str(), ios::in | ios::out | ios::app);
                    if (!m_configStream.is_open()) {
                        KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
                    }
                    m_configStream.seekg(0, ios::end); // position to EOF
                    long filesize = m_configStream.tellg(); // get file size
                    m_configStream.seekg(0); // position to 0
                    if (filesize == 0) {
                        Timestamp t;
                        m_fileTimestampSecondsStart = t.getSeconds();
                        m_fileTimestampFractionStart = t.getFractionalSeconds();
                        m_fileTrainIdStart = t.getTrainId();
                    } else {
                        m_configStream >> m_fileTimestampSecondsStart >> m_fileTimestampFractionStart >> m_fileTrainIdStart;
                    }
                }
                Timestamp t;
                m_fileTimestampSecondsEnd = t.getSeconds();
                m_fileTimestampFractionEnd = t.getFractionalSeconds();
                m_fileTrainIdEnd = t.getTrainId();
                m_configStream << m_fileTimestampSecondsEnd << " " << m_fileTimestampFractionEnd << " " << m_fileTrainIdEnd << " . . . . " << reason << "\n";
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_LOGIC_EXCEPTION("Problems tagging " + m_deviceToBeLogged + " to be discontinued"));
            }
        }

        void DataLogger::slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId) {

            if (m_currentSchema.empty()) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Schema for " << deviceId << " still empty";
                return;
            }

            // open "deviceId"_configuration.txt file for append mode
            string filename = get<string>("directory") + "/" + deviceId + "_configuration.txt";
            if (!m_configStream.is_open()) {
                m_configStream.open(filename.c_str(), ios::in | ios::out | ios::app);
                if (!m_configStream.is_open()) {
                    KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
                }
                m_configStream.seekg(0, ios::end); // position to EOF
                long filesize = m_configStream.tellg(); // get file size
                m_configStream.seekg(0); // position to 0
                if (filesize == 0) {
                    Timestamp t;
                    m_fileTimestampSecondsStart = t.getSeconds();
                    m_fileTimestampFractionStart = t.getFractionalSeconds();
                    m_fileTrainIdStart = t.getTrainId();
                } else {
                    m_configStream >> m_fileTimestampSecondsStart >> m_fileTimestampFractionStart >> m_fileTrainIdStart;
                }
            }
            const string user = getSenderInfo("slotChanged")->getUserIdOfSender();
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
                m_fileTimestampSecondsEnd = t.getSeconds();
                m_fileTimestampFractionEnd = t.getFractionalSeconds();
                m_fileTrainIdEnd = t.getTrainId();
                if (m_fileTimestampSecondsEnd < m_fileTimestampSecondsStart
                        || (m_fileTimestampSecondsEnd == m_fileTimestampSecondsStart && m_fileTimestampFractionEnd < m_fileTimestampFractionStart)) {
                    m_fileTimestampSecondsStart = m_fileTimestampSecondsEnd;
                    m_fileTimestampFractionStart = m_fileTimestampFractionEnd;
                    m_fileTrainIdStart = m_fileTrainIdEnd;
                }
                m_configStream << m_fileTimestampSecondsEnd << " " << m_fileTimestampFractionEnd
                        << " " << m_fileTrainIdEnd << " " << path << " " << type << " " << value << " " << user << " V\n";
            }
            long maxFilesize = get<int>("maximumFileSize") * 1000000;
            if (maxFilesize <= m_configStream.tellp()) {
                m_configStream.close();
                m_lastIndex = incrementLastIndex(deviceId);
                string newname = get<string>("directory") + "/" + deviceId + "_configuration_" + toString(m_lastIndex) + ".txt";
                boost::filesystem::rename(filename, newname);
            } else {
                m_configStream.flush();
            }
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
                KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
        }

        int DataLogger::determineLastIndex(const std::string& deviceId) {
            string lastIndexFilename = get<string>("directory") + "/" + deviceId + ".last";
            int idx;
            fstream fs;
            if (!boost::filesystem::exists(lastIndexFilename)) {
                for (size_t i = 1;; i++) {
                    string filename = get<string>("directory") + "/" + deviceId + "_configuration_" + toString(i) + ".txt";
                    if (!boost::filesystem::exists(filename)) {
                        idx = i - 1;
                        break;
                    }
                }
                fs.open(lastIndexFilename.c_str(), ios::out | ios::app);
                fs << idx;
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
            file.seekp(0);
            file << idx << "\n";
            file.close();
            return idx;
        }
    }
}
