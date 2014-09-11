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

            connectT(m_deviceToBeLogged, "signalChanged", "", "slotChanged");
            connectT(m_deviceToBeLogged, "signalSchemaUpdated", "", "slotSchemaUpdated");

            if (!boost::filesystem::exists(get<string>("directory"))) {
                boost::filesystem::create_directory(get<string>("directory"));
            }

            slotTagDeviceToBeDiscontinued(false, 'L'); // 2nd arg means: device was not valid up to now, 3rd means logger
            refreshDeviceInformation();

            // Register slots
            SLOT2(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
            SLOT2(slotSchemaUpdated, Schema /*changedSchema*/, string /*deviceId*/);
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
            try {
                if (wasValidUpToNow) {
                    if (m_configStream.is_open()) {
                        Timestamp t;
                        m_configStream << t.toTimestamp() << " " << t.getTrainId() << " = = = = " << reason << "\n";
                        m_configStream.flush();
                    }
                    return;
                }
                string filename = get<string>("directory") + "/" + m_deviceToBeLogged + "_configuration.txt";
                if (!m_configStream.is_open()) {
                    m_configStream.open(filename.c_str(), ios::app);
                    if (!m_configStream.is_open()) {
                        KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
                    }
                }
                if (!m_configStream.is_open()) {
                    Timestamp t;
                    m_configStream << t.toTimestamp() << " " << t.getTrainId() << " - - - - " << reason << "\n";
                }

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
                m_configStream.open(filename.c_str(), ios::app);
                if (!m_configStream.is_open()) {
                    KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
                }
            }
            const string user = getSenderInfo("slotChanged")->getUserIdOfSender();
            vector<string> paths;
            configuration.getPaths(paths);
            for (size_t i = 0; i < paths.size(); ++i) {
                const string& path = paths[i];
                const Hash::Node& leafNode = configuration.getNode(path);
                // Skip those elements which should not be archived
                if (!m_currentSchema.has(path) || (m_currentSchema.hasArchivePolicy(path) && (m_currentSchema.getArchivePolicy(path) == Schema::NO_ARCHIVING))) continue;
                string value = leafNode.getValueAs<string>();
                string type = Types::to<ToLiteral>(leafNode.getType());
                Timestamp t = Timestamp::fromHashAttributes(leafNode.getAttributes());

                // [do logging to file]
                m_configStream << t.toTimestamp() << " "
                        << t.getTrainId() << " "
                        << path << " "
                        << type << " "
                        << value << " "
                        << user << " V\n";
            }
            long maxFilesize = get<int>("maximumFileSize");
            if (maxFilesize <= m_configStream.tellp()) {
                m_configStream.close();
                string newname = get<string>("directory") + "/" + deviceId + "_configuration-archive.txt";
                boost::filesystem::rename(filename, newname);
            } else {
                m_configStream.flush();
            }
        }

        void DataLogger::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {
            m_currentSchema = schema;
            string filename = get<string>("directory") + "/" + deviceId + "_schema.txt";
            ofstream fileout(filename.c_str(), ios::app);
            if (fileout.is_open()) {
                Timestamp t;
                TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create("Xml", Hash("Xml.indentation", -1));
                string archive;
                serializer->save(schema, archive);
                fileout << t.toTimestamp() << " " << t.getTrainId() << " " << archive << "\n";
                fileout.close();
            } else
                KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
        }
    }
}
