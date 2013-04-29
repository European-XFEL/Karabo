/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/io/Input.hh>
#include "MasterDevice.hh"
#include "DeviceServer.hh"
#include "HashDatabase.hh"


namespace karabo {
    namespace core {

        using namespace log4cpp;

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        
        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, MasterDevice)

        MasterDevice::~MasterDevice() {
        }

        void MasterDevice::expectedParameters(Schema& expected) {
        }

        MasterDevice::MasterDevice(const Hash& input) : Device<OkErrorFsm>(input) {
            
            GLOBAL_SLOT1(slotDeviceServerProvideName, string) /* Hostname */

            GLOBAL_SLOT2(slotNewDeviceServerAvailable, string, string) /* Hostname, DeviceServerInstanceId */
            
            GLOBAL_SLOT4(slotNewStandaloneDeviceInstanceAvailable, string, Hash, string, string) /* Hostname, CurrentConfig, DeviceInstanceId, XSD */

            GLOBAL_SLOT3(slotSchemaUpdated, string, string, string)
            
            GLOBAL_SLOT3(slotNewDeviceClassAvailable, string, string, string)
                    
            GLOBAL_SLOT2(slotNewDeviceInstanceAvailable, string, Hash)
                    
            GLOBAL_SLOT1(slotDeviceServerInstanceGone, string) /* DeviceServerInstanceId */
                                                                
            GLOBAL_SLOT2(slotDeviceInstanceGone, string, string) /* DeviceServerInstanceId, DeviceInstanceId */
                    
            GLOBAL_SLOT2(slotSelect, string, string)
            
            GLOBAL_SLOT3(slotCreateNewDeviceClassPlugin, string, string, string)
            
//            SIGNAL1("signalNewNode", Hash)
//            connectN("", "signalNewNode", "*", "slotNewNode");
//            
//            SIGNAL1("signalNewDeviceServerInstance", Hash)
//            connectN("", "signalNewDeviceServerInstance", "*", "slotNewDeviceServerInstance");
//            
//            SIGNAL1("signalNewStandaloneDeviceServerInstance", Hash)
//            connectN("", "signalNewStandaloneDeviceServerInstance","*", "slotNewStandaloneDeviceServerInstance");
//            
//            SIGNAL1("signalNewDeviceClass", Hash)
//            connectN("", "signalNewDeviceClass","*", "slotNewDeviceClass");
//                    
//            SIGNAL1("signalNewDeviceInstance", Hash)
//            connectN("", "signalNewDeviceInstance", "*", "slotNewDeviceInstance");
//            
//            SIGNAL1("signalUpdateDeviceServerInstance", Hash)
//            connectN("", "signalUpdateDeviceServerInstance","*", "slotUpdateDeviceServerInstance");
//            
//            SIGNAL1("signalUpdateDeviceInstance", Hash)
//            connectN("", "signalUpdateDeviceInstance","*", "slotUpdateDeviceInstance");
//            
//            SIGNAL3("signalSchemaUpdatedToGui", string, string, string); // schema, instanceId, classId
//            connectN("", "signalSchemaUpdatedToGui", "*", "slotSchemaUpdatedToGui");
            
             initialize();
        }

        void MasterDevice::initialize() {
            bool exists = KARABO_DB_READ;
            // Check for existing database
            if(exists) {
                KARABO_LOG_DEBUG << "Found existing central database";
                trackInstances();
            } else { // Create new one
                KARABO_LOG_INFO << "No database information found, creating new...";
                KARABO_DB_SETUP
                KARABO_DB_SAVE
            }
        }

        void MasterDevice::trackInstances() {
            vector<Hash> result;
            KARABO_DB_SELECT(result, "instanceId", "DeviceServerInstance", true);
            for (size_t i = 0; i < result.size(); ++i) {
                const string& name = result[i].get<string>("instanceId");
                trackExistenceOfInstance(name);
            }
            result.clear();
            KARABO_DB_SELECT(result, "instanceId", "DeviceInstance", true);
            for (size_t i = 0; i < result.size(); ++i) {
                const string& name = result[i].get<string>("instanceId");
                trackExistenceOfInstance(name);
            }
        }

        void MasterDevice::instanceNotAvailable(const std::string& instanceId) {
            log() << Priority::INFO << "Instance: \"" << instanceId << "\" not available.";
            
            HashDatabase::ResultType ids;
            KARABO_DB_SELECT(ids, "id", "DeviceServerInstance", row.get<string>("instanceId") == instanceId)
            if (ids.size() > 0) {
                deviceServerInstanceNotAvailable(instanceId);
                return;
            }
            
            ids.clear();
            KARABO_DB_SELECT(ids, "id", "DeviceInstance", row.get<string>("instanceId") == instanceId)
            if (ids.size() > 0) {
                deviceInstanceNotAvailable(instanceId);
                return;
            }
        }

        void MasterDevice::deviceServerInstanceNotAvailable(const std::string& devSrvInstId) {
            log() << Priority::INFO << "DeviceServerInstance: \"" << devSrvInstId << "\" not available.";

            Hash keyValue("status", "offline");
            KARABO_DB_UPDATE("DeviceServerInstance", keyValue, row.get<string > ("instanceId") == devSrvInstId);
            // Everything below here is hell & slow and should be replaced by proper DB calls (chained delete etc.) in future
            HashDatabase::ResultType ids;
            KARABO_DB_SELECT(ids, "id,instanceId,status,nodId", "DeviceServerInstance", row.get<string>("instanceId") == devSrvInstId)
            //emit("signalUpdateDeviceServerInstance", ids[0]);
            call("*", "slotUpdateDeviceServerInstance", ids[0]);
            HashDatabase::ResultType devClaIds;
            KARABO_DB_SELECT(devClaIds, "id", "DeviceClass", row.get<unsigned int>("devSerInsId") == ids[0].get<unsigned int>("id"))
            size_t nbDevCla = devClaIds.size();
            for (size_t i = 0; i < nbDevCla; ++i) {
               KARABO_DB_DELETE("DeviceInstance", row.get<unsigned int>("devClaId") == devClaIds[i].get<unsigned int>("id"))
               //KARABO_DB_DELETE("DeviceClass", row.get<unsigned int>("id") == devClaIds[i].get<unsigned int>("id"))
            }
            KARABO_DB_SAVE
        }
        
        void MasterDevice::deviceInstanceNotAvailable(const std::string& devInsId) {
            log() << Priority::INFO << "DeviceInstance: \"" << devInsId << "\" not available.";

            // Everything below here is hell & slow and should be replaced by proper DB calls (chained delete etc.) in future
            HashDatabase::ResultType ids;
            KARABO_DB_SELECT(ids, "id,instanceId,devClaId,schema", "DeviceInstance", row.get<string>("instanceId") == devInsId)
            call("*", "slotUpdateDeviceInstance", ids[0]);
            size_t nbDevIns = ids.size();
            for (size_t i = 0; i < nbDevIns; ++i) {
               KARABO_DB_DELETE("DeviceInstance", row.get<unsigned int>("id") == ids[i].get<unsigned int>("id"))
            }
            
            KARABO_DB_SAVE
        }

        void MasterDevice::instanceAvailableAgain(const std::string& instanceId) {
//            HashDatabase::ResultType ids;
//            KARABO_DB_SELECT(ids, "id,instanceId,status,nodId", "DeviceServerInstance", row.get<string>("instanceId") == devSrvInstId)
//            emit("signalUpdateDeviceServerInstance", ids[0]);
            log() << Priority::INFO << "Instance: " << instanceId << " is back again.";
        }

        void MasterDevice::slotDeviceServerProvideName(const std::string& hostname) {

            log() << Priority::INFO << "Device-server from host \"" << hostname << "\" requests device-server instance Id";

            unsigned int nodId;

            // Check whether the node is already known
            vector<Hash> result;
            KARABO_DB_SELECT(result, "id", "Node", row.get<string > ("name") == hostname);

            if (result.empty()) {
                Hash data("name", hostname);
                nodId = KARABO_DB_INSERT("Node", data);
                data.set("id", nodId);
                //emit("signalNewNode", data);
                call("*", "slotNewNode", data);
            } else {
                result[0].get("id", nodId);
                result.clear();
                KARABO_DB_SELECT(result, "instanceId", "DeviceServerInstance", true);
            }

            string instanceId = hostname + "/DeviceServer/" + karabo::util::toString(result.size());
            this->sanifyDeviceServerInstanceId(instanceId); 
            
            log() << Priority::INFO << "This will be the " << result.size() + 1 << " device-server online on host \"" << hostname << "\"";
            log() << Priority::INFO << "Device-Server instance id will be: " << instanceId;

            Hash data("instanceId", instanceId, "alias", "", "status", "starting", "nodId", nodId);
            unsigned int id = KARABO_DB_INSERT("DeviceServerInstance", data);
            data.set("id", id);
            //emit("signalNewDeviceServerInstance", data);
            call("*", "slotNewDeviceServerInstance", data);
            KARABO_DB_SAVE

            trackExistenceOfInstance(instanceId);
            reply(instanceId);
        }
        
        void MasterDevice::sanifyDeviceServerInstanceId(std::string& originalInstanceId) const {
            for (std::string::iterator it = originalInstanceId.begin(); it != originalInstanceId.end(); ++it) {
                if ((*it) == '.') (*it) = '-';
            }
        }
        
        void MasterDevice::slotNewDeviceServerAvailable(const std::string& hostname, const std::string& devSrvInstId) {
            log() << Priority::INFO << "New device-server from host \"" << hostname << "\" wants to register with id \"" << devSrvInstId << "\"";

            SIGNAL1("answer", string)
            string message;

            vector<Hash> result;
            KARABO_DB_SELECT(result, "id,status,instanceId,nodId", "DeviceServerInstance", row.get<string > ("instanceId") == devSrvInstId); 
            
            if (result.size() == 0) {

                unsigned int nodId;
                // Check whether the node is already known
                vector<Hash> nodeResult;
                KARABO_DB_SELECT(nodeResult, "id", "Node", row.get<string > ("name") == hostname)
                if (nodeResult.empty()) {
                    Hash data("name", hostname);
                    nodId = KARABO_DB_INSERT("Node", data);
                    data.set("id", nodId);
                    //emit("signalNewNode", data);
                    call("*", "slotNewNode", data);
                } else {
                    nodeResult[0].get("id", nodId);
                }
                Hash data("instanceId", devSrvInstId, "alias", "", "status", "online", "nodId", nodId);
                
                unsigned int devSerInsId = KARABO_DB_INSERT("DeviceServerInstance", data);
                data.set("id", devSerInsId);
                //emit("signalNewDeviceServerInstance", data);
                call("*", "slotNewDeviceServerInstance", data);
                trackExistenceOfInstance(devSrvInstId);
                call(devSrvInstId, "slotRegistrationOk", "Your name got accepted, welcome in the team!");

            } else if (result.size() == 1) {
                string status = result[0].get<string > ("status");
                if (status == "starting") {
                    call(devSrvInstId, "slotRegistrationOk", "We are happy to have you in the team!");
                    Hash keyValue("status", "online");
                    KARABO_DB_UPDATE("DeviceServerInstance", keyValue, row.get<string > ("instanceId") == devSrvInstId)
                    Hash data(result[0]);
                    data.merge(keyValue);
                    //emit("signalNewDeviceServerInstance", data);
                    call("*", "slotNewDeviceServerInstance", data);
                } else if (status == "offline") { // back in business
                    call(devSrvInstId, "slotRegistrationOk", "Welcome back!");
                    Hash keyValue("status", "online");
                    KARABO_DB_UPDATE("DeviceServerInstance", keyValue, row.get<string > ("instanceId") == devSrvInstId);
                    connectN("", "answer", devSrvInstId, "slotRegistrationOk");
                    
                    result[0].set("status", "online");
                    //emit("signalUpdateDeviceServerInstance", result[0]);
                    call("*", "slotUpdateDeviceServerInstance", result[0]);
                } else if (status == "online") { // already online (bad)
                    call(devSrvInstId, "slotRegistrationFailed", "Another device-server with the same instance is already online");
                }
                trackExistenceOfInstance(devSrvInstId);
            } else if (result.size() > 1) {
                throw KARABO_LOGIC_EXCEPTION("Internal error: Inconsistent database");
            }
            KARABO_DB_SAVE
        }

        void MasterDevice::slotNewStandaloneDeviceInstanceAvailable(const std::string& hostname, const karabo::util::Hash& deviceConfig, const std::string& devInstId, const std::string& deviceXsd) {
            log() << Priority::INFO << "New standalone device instance from host \"" << hostname << "\" wants to register with id \"" << devInstId << "\"";
            
            string devSrvInstId = "no server (standalone devices)";
            const string& devClassId = deviceConfig.begin()->getKey(); // Root node corresponds to devClassId
            slotNewDeviceServerAvailable(hostname, devSrvInstId);
            slotNewDeviceClassAvailable(devSrvInstId, devClassId, deviceXsd);
            slotNewDeviceInstanceAvailable(devSrvInstId, deviceConfig);
            trackExistenceOfInstance(devInstId);
        }
        
        void MasterDevice::slotNewDeviceClassAvailable(const std::string& devSrvInstId, const std::string& devClassId, const std::string& deviceXsd) {
            log() << Priority::INFO << "New device class \"" << devClassId << "\" on device-server \"" << devSrvInstId << "\" available";
            // Skip myself
            if (devClassId == getClassInfo().getClassId()) return;
            // Skip inbuilt GuiServer
            if (devClassId == "GuiServerDevice") return;
            
            vector<Hash> result;
            KARABO_DB_SELECT(result, "id", "DeviceServerInstance", row.get<string>("instanceId") == devSrvInstId);
            
            unsigned int devSerInsId;
            if (result.size() == 1) {
                result[0].get("id", devSerInsId);
            } else {
                throw KARABO_LOGIC_EXCEPTION("Missing device-server instance");
            }
            
            result.clear();
            KARABO_DB_SELECT(result, "id", "DeviceClass", row.get<string>("name") == devClassId && row.get<unsigned int>("devSerInsId") == devSerInsId);
            if (result.size() == 0) {
                // Insert new deviceClass if not seen before
                Hash data("name", devClassId, "schema", deviceXsd, "devSerInsId", devSerInsId);
                unsigned int id = KARABO_DB_INSERT("DeviceClass", data);
                data.set("id", id);
                //emit("signalNewDeviceClass", data);
                call("*", "slotNewDeviceClass", data);
                KARABO_DB_SAVE
            }
            else
            {
                // Update existing deviceClass
                Hash keyValue("schema", deviceXsd);
                
                size_t nbDevCla = result.size();
                for (size_t i = 0; i < nbDevCla; ++i) {
                    unsigned int id = result[i].get<unsigned int>("id");
                    KARABO_DB_UPDATE("DeviceClass", keyValue, row.get<unsigned int>("id") == id)   
                    Hash data("id", id, "name", devClassId, "schema", deviceXsd, "devSerInsId", devSerInsId);
                    call("*", "slotNewDeviceClass", data);         
                }
                KARABO_DB_SAVE
            }
        }
        
        void MasterDevice::slotNewDeviceInstanceAvailable(const std::string& devSrvInstId, const karabo::util::Hash& deviceConfig) {
            
            const string& devClassId = deviceConfig.begin()->getKey(); // Root node corresponds to devClassId
            
            // Skip myself
            if (devClassId == "MasterDevice" || devClassId == "GuiServerDevice") return;
            
            cout << deviceConfig << endl;
            
            const Hash& config = deviceConfig.get<Hash>(devClassId);
            const string& devInstId = config.get<string>("devInstId");
            
            log() << Priority::INFO << "New device instance \"" << devInstId << "\" on device-server \"" << devSrvInstId << "\" available";
            
            HashDatabase::ResultType result;
            KARABO_DB_SELECT(result, "id", "DeviceServerInstance", row.get<string>("instanceId") == devSrvInstId);
            
            unsigned int devSerInsId;
            if (result.size() == 1) {
                result[0].get("id", devSerInsId);
            } else {
                throw KARABO_LOGIC_EXCEPTION("Missing device-server instance");
            }
            
            result.clear();
            KARABO_DB_SELECT(result, "id,schema", "DeviceClass", row.get<string>("name") == devClassId && row.get<unsigned int>("devSerInsId") == devSerInsId);
            
            unsigned int devClaId;
            string schema;
            if (result.size() == 1) {
                result[0].get("id", devClaId);
                result[0].get("schema", schema);
            } else {
                throw KARABO_LOGIC_EXCEPTION("Missing device-server instance");
            }

            // Insert new deviceInstance
            Hash data("instanceId", devInstId, "configuration", deviceConfig, "devClaId", devClaId, "schema", schema);
            unsigned int id = KARABO_DB_INSERT("DeviceInstance", data);
            KARABO_DB_SAVE
            
            data.set("id", id);
            
            call("*", "slotNewDeviceInstance", data);
        }
        
        void MasterDevice::slotSchemaUpdated(const std::string& schema, const std::string& instanceId, const std::string& classId) {
            // Replace current schema with new schema
            Hash keyValue("schema", schema);
            KARABO_DB_UPDATE("DeviceInstance", keyValue, row.get<string > ("instanceId") == instanceId);
            KARABO_DB_SAVE
            
            //emit("signalSchemaUpdatedToGui", schema, instanceId, classId);
            call("*", "slotSchemaUpdatedToGui", schema, instanceId, classId);
        }
        
        void MasterDevice::slotDeviceServerInstanceGone(const std::string& deviceServerInstanceId) {
            log() << Priority::INFO << deviceServerInstanceId << " is going to die soon";
            deviceServerInstanceNotAvailable(deviceServerInstanceId);
        }
        
        void MasterDevice::slotDeviceInstanceGone(const std::string& deviceServerInstanceId, const std::string& deviceInstanceId) {
            deviceInstanceNotAvailable(deviceInstanceId);
        }
        
//        void MasterDevice::slotGetDeviceServersAndDevices() {
//            
//            Hash hash;
//            HashDatabase::ResultType result;
//            
//            vector<Hash>& deviceServers = hash.bindReference<vector<Hash> >("deviceServers");
//            KARABO_DB_SELECT(result, "instanceId,status", "DeviceServerInstance", true);
//            deviceServers = result;
//            result.clear();
//            
//            vector<Hash >& devices = hash.bindReference<vector<Hash> >("devices");
//            KARABO_DB_SELECT(result, "instanceId", "DeviceInstance", true);
//            devices = result;
//            result.clear();
//            
//            reply(hash);
//        }
        
        void MasterDevice::slotSelect(const std::string& fields, const std::string& table) {
            std::vector<karabo::util::Hash> result;
            KARABO_DB_SELECT(result, fields, table, true);
            reply(result);
            
        }
        
        void MasterDevice::slotCreateNewDeviceClassPlugin(const std::string& devSerInsId, const std::string& devClaId, const std::string& newDevClaId) {
            vector<Hash> result;
            
            KARABO_DB_SELECT(result, "id", "DeviceServerInstance", row.get<string>("instanceId") == devSerInsId);
            unsigned int id;
            if (result.size() == 1) {
                result[0].get("id", id);
            }

            result.clear();
            KARABO_DB_SELECT(result, "id,schema", "DeviceClass", row.get<string>("name") == devClaId && row.get<unsigned int>("devSerInsId") == id);
            
            string schema;
            if (result.size() == 1) {
                result[0].get("schema", schema);
            }

            slotNewDeviceClassAvailable(devSerInsId, newDevClaId, schema);
        }
    }
}
