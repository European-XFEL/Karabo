/* 
 * File:   AlarmService.cc
 * Author: haufs
 * 
 * Created on August 5, 2016, 11:30 AM
 */

#include "AlarmService.hh"
#include "karabo/util/State.hh"

namespace karabo {
    
   
    
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, AlarmService)


        void AlarmService::expectedParameters(Schema& expected) {
            
            Schema tableRow;
            
            STRING_ELEMENT(tableRow).key("timeOfOccurrence")
                    .displayedName("Occurred at")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("timeOfFirstOccurrence")
                    .displayedName("First occurred at")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("deviceId")
                    .displayedName("Device")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("property")
                    .displayedName("Property")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("severity")
                    .displayedName("Severity")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("description")
                    .displayedName("Description")
                    .readOnly()
                    .commit();
            
            BOOL_ELEMENT(tableRow).key("needsAcknowledging")
                    .displayedName("ack?")
                    .readOnly()
                    .commit();
            
            BOOL_ELEMENT(tableRow).key("acknowledgeable")
                    .displayedName("can ack?")
                    .readOnly()
                    .commit();
            
            BOOL_ELEMENT(tableRow).key("acknowledged")
                    .displayedName("Acknowledged")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();
            
            TABLE_ELEMENT(expected).key("currentAlarms")
                    .displayedName("Current Alarms")
                    .setNodeSchema(tableRow)
                    .assignmentOptional().noDefaultValue()
                    .reconfigurable()
                    .commit();
                  
        }

            


        AlarmService::AlarmService(const karabo::util::Hash& input) :
            karabo::core::Device<>(input){
            KARABO_INITIAL_FUNCTION(initialize);
        }
     
        AlarmService::~AlarmService() {
        }
        
        void AlarmService::initialize() {
            updateState(State::INIT);
            remote().registerInstanceNewMonitor(boost::bind(&AlarmService::registerAlarmWithNewDevice, this, _1));
            //make existing device aware of us
            emit("*", "signalAlarmDevice", getInstanceId());
            updateState(State::NORMAL);
        }
        
        void AlarmService::registerAlarmWithNewDevice(const karabo::util::Hash& topologyEntry){
            //only act upon device we do not know about yet coming up
            
                try {
                    const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                    // const ref is fine even for temporary std::string+
                    if(type == "device"){
                        const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                        if(m_registeredDevices.insert(entry).second){
                            const std::string& instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                                                             topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));
                            KARABO_LOG_FRAMEWORK_INFO << "registerAlarmWithNewDevice --> instanceId: '" << instanceId
                                    << "', type: '" << type << "'";

                            //can later be extended

                            const Hash serviceInfo("instance", getInstanceId());
                            call(instanceId, "slotRegisterAlarmService", serviceInfo);
                            
                        } else {
                        //this device reappeared, we should ask for resubmitting all alarms
                        }
                    }
                
                } catch (const Exception& e) {
                    KARABO_LOG_ERROR << "In registerAlarmWithNewDevice:\n" << e;
                } catch (const std::exception& e) {
                    KARABO_LOG_ERROR << "In registerAlarmWithNewDevice: " << e.what() << ".";
                } catch (...) {
                    KARABO_LOG_ERROR << "Unknown exception in registerAlarmWithNewDevice.";
                }
            
        }
        
        void AlarmService::slotRegisterExistingDevice(const karabo::util::Hash& instanceInfo){
            if(m_registeredDevices.insert(entry).second){
                const std::string& instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                                                 topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));
                KARABO_LOG_FRAMEWORK_INFO << "slotRegisterExistingDevice --> instanceId: '" << instanceId
                        << "', type: '" << type << "'";

                //can later be extended

                const Hash serviceInfo("instance", getInstanceId());
                call(instanceId, "slotRegisterAlarmService", serviceInfo);

            } else {
            //this device reappeared, we should ask for resubmitting all alarms
            }
        }
        
        void AlarmService::slotUpdateAlarms(const karabo::util::Hash& alarmInfo){
            //get rid of alarm conditions that have passed or can now be acknowledged
            const Hash& toClear = alarmInfo.get<Hash>("toClear");
            for(Hash::const_iterator it = toClear.begin(); it != toClear.end(); ++it){
                const std::string& device = it->getKey();
                boost::optional<Hash::Node&> deviceEntryNode = m_alarms.find(device);
                if(!deviceEntryNode) continue; //no alarms present for this device
                
                Hash& deviceEntry = deviceEntryNode->getValue<Hash>();
                const Hash& properties = it->getValue<Hash>();
                for(Hash::const_iterator pit = properties.begin(); pit != properties.end(); ++pit){
                    boost::optional<Hash::Node&> propertyEntryNode = deviceEntry.find(pit->getKey());
                    if(!propertyEntryNode) continue; //no alarm for this property
                    
                    Hash& propertyEntry = propertyEntryNode->getValue<Hash>();
                    if(propertyEntry.get<bool>("needsAcknowledging")){
                        //if the alarm needs to be acknowledged we allow this now
                        propertyEntry.set("acknowledgeable", true);
                    } else {
                       //go ahead and erase the alarm condition
                        deviceEntry.erase(pit->getKey());
                    }
                    
                    
                }
            }
            //now add new alarms
            const Hash& toAdd = alarmInfo.get<Hash>("toAdd");
            for(Hash::const_iterator it = toAdd.begin(); it != toAdd.end(); ++it){
                const std::string& device = it->getKey();
                boost::optional<Hash::Node&> deviceEntryNode = m_alarms.find(device);
                if(!deviceEntryNode){
                    deviceEntryNode = m_alarms.set(device, Hash());
                }
                Hash& deviceEntry = deviceEntryNode->getValue<Hash>();
                const Hash& properties = it->getValue<Hash>();
                for(Hash::const_iterator pit = properties.begin(); pit != properties.end(); ++pit){
                    boost::optional<Hash::Node&> propertyEntryNode = deviceEntry.find(pit->getKey());
                    std::string& firstOccurance = pit->getValue<Hash>().get<std::string>("timeOfFirstOccurrence");
                    if(propertyEntryNode) {
                        firstOccurance =  propertyEntryNode->getValue<Hash>().get<std::string>("timeOfFirstOccurrence");
                    }
                    Hash::Node& newEntry = deviceEntry.set(pit->getKey(), pit->getValue<Hash>());
                    newEntry.getValue<Hash>().set("timeOfFirstOccurrence", firstOccurance);
                }
            }
            
            set("currentAlarms", m_alarms);
        }
    }
}

