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
            
            UINT64_ELEMENT(tableRow).key("trainOfOccurrence")
                    .displayedName("Occurred at train")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("timeOfFirstOccurrence")
                    .displayedName("First occurred at")
                    .readOnly()
                    .commit();
            
            UINT64_ELEMENT(tableRow).key("trainOfFirstOccurrence")
                    .displayedName("First occurred at train")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("instanceId")
                    .displayedName("Device")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("property")
                    .displayedName("Property")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("type")
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
            
            setupSignalsAndSlots();
            remote().registerInstanceNewMonitor(boost::bind(&AlarmService::registerAlarmWithNewDevice, this, _1));
            
            //make existing instance aware of us
            //Hash lastKnownAlarms;
            //emit("*", "signalAlarmDeviceStarted", getInstanceId(), lastKnownAlarms);
            updateState(State::NORMAL);
        }
        
        void AlarmService::setupSignalsAndSlots(){     
            KARABO_SYSTEM_SIGNAL2("signalAlarmDeviceStarted", std::string, karabo::util::Hash);
        }
        
        void AlarmService::registerAlarmWithNewDevice(const karabo::util::Hash& topologyEntry){
            //only act upon instance we do not know about yet coming up
            
                try {
                    const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                    // const ref is fine even for temporary std::string+
                    if(type == "device"){
                        const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                        const std::string& instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                                                             topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));
                        if(m_registeredDevices.insert(std::pair<std::string, Hash>(instanceId, entry)).second){
                            
                            KARABO_LOG_FRAMEWORK_INFO << "registerAlarmWithNewDevice --> instanceId: '" << instanceId
                                    << "', type: '" << type << "'";

                            //can later be extended

                            const Hash serviceInfo("instance", getInstanceId());
                            call(instanceId, "slotRegisterAlarmService", serviceInfo);
                            
                        } else {
                        //this instance reappeared, we should ask for resubmitting all alarms
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
        
    }
}

