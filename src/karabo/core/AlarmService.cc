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
            Hash lastKnownAlarms;
            emit("*", "signalAlarmDeviceStarted", getInstanceId(), lastKnownAlarms);
            updateState(State::NORMAL);
        }
        
        void AlarmService::setupSignalsAndSlots(){         
            KARABO_SYSTEM_SIGNAL2("signalAlarmDeviceStarted", std::string, Hash)
        }
        
        void AlarmService::registerAlarmWithNewDevice(const karabo::util::Hash& topologyEntry){
            //only act upon instance we do not know about yet coming up
            
                try {
                    const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                    // const ref is fine even for temporary std::string+
                    if(type == "instance"){
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
        
        void AlarmService::slotRegisterExistingDevice(const karabo::util::Hash& instanceInfo){
            
        }
        
        void AlarmService::slotUpdateAlarms(const karabo::util::Hash& alarmInfo){
            const Hash::Node& instanceNode = *alarmInfo.begin(); //we assume only one instance entry here
            const std::string& instance = instanceNode.getKey();
            const Hash& instanceNodeHash = instanceNode.getValue<Hash>();
            //get rid of alarm conditions that have passed or can now be acknowledged
            const Hash& toClear = instanceNodeHash.get<Hash>("toClear");
            
            //check if any alarms exist for this instance
            boost::optional<Hash::Node&> existingDeviceEntryN = m_alarms.find(instance);
            if(existingDeviceEntryN){
                Hash& existingDeviceEntry = existingDeviceEntryN->getValue<Hash>();
                
                
                //iterate over properties for this instance
                for(Hash::const_iterator pit = toClear.begin(); pit != toClear.end(); ++pit){
                    boost::optional<Hash::Node&> existingPropEntryN = existingDeviceEntry.find(pit->getKey()); //existing property alarms for instance
                    if(!existingPropEntryN) continue; //no alarm for this property
                    
                    Hash& existingPropEntry = existingPropEntryN->getValue<Hash>();
                    //iterate over alarm types in this property
                    const Hash& alarms = pit->getValue<Hash>();
                    for(Hash::const_iterator ait = alarms.begin(); ait != alarms.end(); ++ait){
                        boost::optional<Hash::Node&> existingTypeEntryN = existingPropEntry.find(ait->getKey()); //existing property alarms for instance
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
                    for(Hash::const_iterator ait = updatingPropEntry.begin(); ait != updatingPropEntry.end(); ++ait){
                        boost::optional<Hash::Node&> existingTypeEntryN = existingPropEntry.find(pit->getKey());
          
                        
                        const Timestamp updatedTimeStamp = Timestamp::fromHashAttributes(existingTypeEntryN->getAttributes());
                        std::string timeOfFirstOccurrence = updatedTimeStamp.toIso8601();
                        unsigned long long trainOfFirstOccurrence = updatedTimeStamp.getTrainId();
                        
                        if(existingTypeEntryN) {
                            //alarm exists, we use its first occurance
                            Hash& existingTypeEntry = existingTypeEntryN->getValue<Hash>();
                            timeOfFirstOccurrence =  existingTypeEntry.get<std::string>("timeOfFirstOccurrence");
                            trainOfFirstOccurrence =  existingTypeEntry.get<unsigned long long>("trainOfFirstOccurrence");
                        }
                        

                        Hash::Node& newEntry = existingPropEntry.set(pit->getKey(), ait->getValueAsAny());
                        newEntry.getValue<Hash>().set("timeOfFirstOccurrence", timeOfFirstOccurrence);
                        newEntry.getValue<Hash>().set("trainOfFirstOccurrence", trainOfFirstOccurrence);
                
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
                    for(Hash::const_iterator ait = properties.begin(); ait != properties.end(); ++ait){
                        const Hash& entry = ait->getValue<Hash>();
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

