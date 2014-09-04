/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/io/Input.hh>
#include "DataLogger.hh"
#include "karabo/io/FileTools.hh"

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

        }


        void DataLogger::okStateOnEntry() {
            
            connectT(m_deviceToBeLogged, "signalChanged", "", "slotChanged");
            connectT(m_deviceToBeLogged, "signalSchemaUpdated", "", "slotSchemaUpdated");

            slotTagDeviceToBeDiscontinued(false, 'l'); // 2nd arg means: device was not valid up to now, 3rd means logger
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

             
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_LOGIC_EXCEPTION("Problems tagging " + m_deviceToBeLogged + " to be discontinued"));
            }
        }              


        void DataLogger::slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId) {
            
            if (m_currentSchema.empty()) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Schema for " << deviceId << " still empty";
                return;
            }
            vector<string> paths;
            configuration.getPaths(paths);
            for (size_t i = 0; i < paths.size(); ++i) {
                const string& path = paths[i];
                const Hash::Node& leafNode = configuration.getNode(path);
                // Skip those elements which should not be archived
                if (!m_currentSchema.has(path) || (m_currentSchema.hasArchivePolicy(path) && (m_currentSchema.getArchivePolicy(path) == Schema::NO_ARCHIVING))) continue;
                string value = leafNode.getValueAs<string>();
                Timestamp t = Timestamp::fromHashAttributes(leafNode.getAttributes());

                // [do logging to file]
                
            }
        }
        
        void DataLogger::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {

        }
    }
}
