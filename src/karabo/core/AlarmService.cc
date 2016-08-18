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
            
            //device elements
            
            VECTOR_STRING_ELEMENT(expected).key("registeredDevices")
                    .displayedName("Registered devices")
                    .description("The devices which are currently registered to this alarm service device")
                    .readOnly()
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
            //we listen on instance new events to connect to the instances "signalAlarmUpdate" signal
            remote().registerInstanceNewMonitor(boost::bind(&AlarmService::registerAlarmServiceWithNewDevice, this, _1));
            
            updateState(State::NORMAL);
        }
        
        void AlarmService::setupSignalsAndSlots(){     
            KARABO_SYSTEM_SIGNAL2("signalAlarmDeviceStarted", std::string, karabo::util::Hash);
            registerSlot<karabo::util::Hash > (boost::bind(&AlarmService::slotUpdateAlarms, this, _1), "slotUpdateAlarms");
        }
        
        void AlarmService::registerAlarmServiceWithNewDevice(const karabo::util::Hash& topologyEntry){
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
                            
                            connect(instanceId, "signalAlarmUpdate", "", "slotUpdateAlarms");
                            
                            std::vector<std::string> devices = get<std::vector<std::string> >("registeredDevices");
                            devices.push_back(instanceId);
                            set("registeredDevices", devices);
                            
                            //TODO: Implement listening to new instances heartbeats
            
                            
                        } else {
                        //this instance reappeared, we should ask for resubmitting all alarms
                        //TODO: implement this
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
        
        void AlarmService::slotUpdateAlarms(const karabo::util::Hash& alarmInfo){
            const Hash::Node& instanceNode = *alarmInfo.begin(); //we assume only one instance entry here
            const std::string& instance = instanceNode.getKey();
            const Hash& instanceNodeHash = instanceNode.getValue<Hash>();
            
            // get rid of alarm conditions that have passed or can now be acknowledged
            const Hash& toClear = instanceNodeHash.get<Hash>("toClear");
            
            // check if any alarms exist for this instance
            // in the following capital "N" at the end of a variable declarations signifies a Hash node
            boost::optional<Hash::Node&> existingDeviceEntryN = m_alarms.find(instance);
            if(existingDeviceEntryN){
                Hash& existingDeviceEntry = existingDeviceEntryN->getValue<Hash>();
                
                //iterate over properties for this instance
                for(Hash::const_iterator pit = toClear.begin(); pit != toClear.end(); ++pit){
                    boost::optional<Hash::Node&> existingPropEntryN = existingDeviceEntry.find(pit->getKey()); //existing property alarms for instance
                    if(!existingPropEntryN) continue; //no alarm for this property
                    
                    Hash& existingPropEntry = existingPropEntryN->getValue<Hash>();
                   
                    //iterate over alarm types in this property
                    const Hash& alarmTypes = pit->getValue<Hash>();
                   
                    for(Hash::const_iterator atit = alarmTypes.begin(); atit != alarmTypes.end(); ++atit){
                        boost::optional<Hash::Node&> existingTypeEntryN = existingPropEntry.find(atit->getKey()); //existing alarm types for property
                        if(!existingTypeEntryN) continue; //no alarm for this property and alarm type
   
                        //alarm of this type exists for the property
                        Hash& existingTypeEntry = existingTypeEntryN->getValue<Hash>();
                        if(existingTypeEntry.get<bool>("needsAcknowledging")){
                            //if the alarm needs to be acknowledged we allow this now
                            existingTypeEntry.set("acknowledgeable", true);
                           
                        } else {
                           //go ahead and erase the alarm condition as it is allowed to silently disappear
                            existingPropEntry.erase(pit->getKey());
                        
                        }
                    }
                    
                }
            }
            
            //now add new alarms
            const Hash& toAdd = instanceNodeHash.get<Hash>("toAdd");

            if(!toAdd.empty()){
                //if instance appears for first time we add it to the alarm entries
                if(!existingDeviceEntryN){
                    existingDeviceEntryN = m_alarms.set(instance, Hash());
                }
                Hash& existingDeviceEntry = existingDeviceEntryN->getValue<Hash>();

                //iteration over properties with alarms
                for(Hash::const_iterator pit = toAdd.begin(); pit != toAdd.end(); ++pit){
                    //check if alarms for this property exist
                    boost::optional<Hash::Node&> existingPropEntryN = existingDeviceEntry.find(pit->getKey());
                    if(!existingPropEntryN) { 
                        //create node for property if it doesn't exist
                        existingPropEntryN = existingDeviceEntry.set(pit->getKey(), Hash());
                    }
                    
                    //update this property
                    const Hash& updatingPropEntry =  pit->getValue<Hash>(); 
                    Hash& existingPropEntry = existingPropEntryN->getValue<Hash>();

                    //iterates over alarm type of this property
                    for(Hash::const_iterator atit = updatingPropEntry.begin(); atit != updatingPropEntry.end(); ++atit){
                        boost::optional<Hash::Node&> existingTypeEntryN = existingPropEntry.find(atit->getKey());
                        
                        
                        const Timestamp updatedTimeStamp = Timestamp::fromHashAttributes(pit->getAttributes());
                        std::string timeOfOccurrence = updatedTimeStamp.toIso8601();
                        unsigned long long trainOfOccurrence = updatedTimeStamp.getTrainId();
                        std::string timeOfFirstOccurrence = timeOfOccurrence;
                        unsigned long long trainOfFirstOccurrence = trainOfOccurrence;
                        
                        if(existingTypeEntryN) {
                            //alarm exists, we use its first occurance
                            Hash& existingTypeEntry = existingTypeEntryN->getValue<Hash>();
                            timeOfFirstOccurrence =  existingTypeEntry.get<std::string>("timeOfFirstOccurrence");
                            trainOfFirstOccurrence =  existingTypeEntry.get<unsigned long long>("trainOfFirstOccurrence");
                        }
                        
                        //first set all properties we can simply copy by assigning value of the new entry
                        Hash::Node& newEntryN = existingPropEntry.set(atit->getKey(), atit->getValue<Hash>());

                        //now those which we needed to modify
                        Hash& newEntry = newEntryN.getValue<Hash>();
                        newEntry.set("timeOfFirstOccurrence", timeOfFirstOccurrence);
                        newEntry.set("trainOfFirstOccurrence", trainOfFirstOccurrence);
                        newEntry.set("timeOfOccurrence", timeOfOccurrence);
                        newEntry.set("trainOfOccurrence", trainOfOccurrence);
                        // acknowledgeable is determined by whether an alarm needs acknowledging
                        newEntry.get<bool>("needsAcknowledging") ? newEntry.set("acknowledgeable", false) : newEntry.set("acknowledgeable", true);
                        
                
                    }
                }
            }
            
            updateAlarmTable();
            
        }
        
        void AlarmService::updateAlarmTable(){
            std::vector<Hash> tableVector;

            //instance level
            for(Hash::const_iterator it = m_alarms.begin(); it != m_alarms.end(); ++it){
                const std::string& instance = it->getKey();
                const Hash& instances = it->getValue<Hash>();
                
                //property level
                for(Hash::const_iterator pit = instances.begin(); pit != instances.end(); ++pit){
                    const std::string& property = pit->getKey();
                    const Hash& properties = pit->getValue<Hash>();
                    
                    //type level
                    for(Hash::const_iterator atit = properties.begin(); atit != properties.end(); ++atit){
                        const Hash& entry = atit->getValue<Hash>();
                        Hash h;
                        h.set("timeOfOccurrence", entry.get<std::string>("timeOfOccurrence"));
                        h.set("trainOfOccurrence", entry.get<unsigned long long>("trainOfOccurrence"));
                        h.set("timeOfFirstOccurrence", entry.get<std::string>("timeOfFirstOccurrence"));
                        h.set("trainOfFirstOccurrence", entry.get<unsigned long long>("trainOfFirstOccurrence"));
                        h.set("instanceId", instance);
                        h.set("property", property);
                        h.set("type", entry.get<std::string>("type"));
                        h.set("description", entry.get<std::string>("description"));
                        h.set("needsAcknowledging", entry.get<bool>("needsAcknowledging"));
                        h.set("acknowledgeable", entry.get<bool>("acknowledgeable"));
                        tableVector.push_back(h);
                        
                    }
                }
            }
            
            set("currentAlarms", tableVector);
        }
        
    }
}

