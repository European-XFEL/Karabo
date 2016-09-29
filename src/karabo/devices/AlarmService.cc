/* 
 * File:   AlarmService.cc
 * Author: haufs
 * 
 * Created on August 5, 2016, 11:30 AM
 */


#include <fstream>

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

#include "karabo/util/State.hh"
#include "karabo/io/TextSerializer.hh"

#include "AlarmService.hh"


namespace karabo {
    
   
    
    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::core;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, AlarmService)


        void AlarmService::expectedParameters(Schema& expected) {
            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_AlarmService")
                    .commit();
            //device elements
            
            PATH_ELEMENT(expected).key("storagePath")
                    .displayedName("Storage path")
                    .description("Path under which this device will persist its data for recovery")
                    .assignmentOptional().defaultValue("./")
                    .expertAccess()
                    .commit();
            
            UINT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(10)
                    .reconfigurable()
                    .expertAccess()
                    .commit();
            
            VECTOR_STRING_ELEMENT(expected).key("registeredDevices")
                    .displayedName("Registered devices")
                    .description("The devices which are currently registered to this alarm service device")
                    .readOnly()
                    .expertAccess()
                    .commit();
                  
        }

        AlarmService::AlarmService(const karabo::util::Hash& input) :
            karabo::core::Device<>(input){
            setupSignalsAndSlots();
            KARABO_INITIAL_FUNCTION(initialize);
        }
     
        AlarmService::~AlarmService() {
            m_flushRunning = false;
            m_flushWorker.join();
        }
        
        void AlarmService::initialize() {

            updateState(State::INIT);
            
            //we listen on instance new events to connect to the instances "signalAlarmUpdate" signal
            trackAllInstances();
            remote().registerInstanceNewMonitor(boost::bind(&AlarmService::registerNewDevice, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&AlarmService::instanceGoneHandler, this, _1, _2));

            
            //try to recover previous alarms in case we recovered from a failure or were restarted
            m_flushFilePath = get<std::string>("storagePath") + "/" + getInstanceId() + ".xml";
            reinitFromFile();
            
            //we add a worker thread, which persists alarm state at regular intervals. This data is used when recovering
            //from a alarm service shutdown
            
            m_flushRunning = true;
            m_flushWorker = boost::thread(&AlarmService::flushRunner, this);
            
            updateState(State::NORMAL);
        }
        
        void AlarmService::setupSignalsAndSlots(){     

            registerSlot<std::string, karabo::util::Hash > (boost::bind(&AlarmService::slotUpdateAlarms, this, _1, _2), "slotUpdateAlarms");
            KARABO_SIGNAL3("signalAlarmServiceUpdate", std::string, std::string, karabo::util::Hash)
            registerSlot<karabo::util::Hash > (boost::bind(&AlarmService::slotAcknowledgeAlarm, this, _1), "slotAcknowledgeAlarm");
            registerSlot(boost::bind(&AlarmService::slotRequestAlarmDump, this), "slotRequestAlarmDump");
            
        }
        
        void AlarmService::registerNewDevice(const karabo::util::Hash& topologyEntry){
            //only act upon instance we do not know about yet coming up
            try {
                const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                // const ref is fine even for temporary std::string+
                if(type == "device"){
                    
                    const std::string& deviceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                                                         topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));
                    
                    if(deviceId == getInstanceId()) return; //prevent registering ourselves.

                    boost::upgrade_lock<boost::shared_mutex> readLock(m_deviceRegisterMutex);
                    std::vector<std::string> devices = get<std::vector<std::string> >("registeredDevices");
                    if (std::find(devices.begin(), devices.end(), deviceId) == devices.end()) {
                        connect(deviceId, "signalAlarmUpdate", "", "slotUpdateAlarms");
                        devices.push_back(deviceId);
                        boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
                        set("registeredDevices", devices);
                    } else {
                        //we've known this instance before and might have alarms pending from it. We should ask
                        //for an update on these alarms.
                        KARABO_LOG_INFO<<"Device '"<<deviceId<<"' reappeared. Asking it to re-submit its alarms!";
                        boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
                        const boost::optional<Hash::Node&> alarmNode = m_alarms.find(deviceId);
                        request(deviceId, "slotReSubmitAlarms", alarmNode ? alarmNode->getValue<Hash>() : Hash())
                                .receiveAsync<std::string, Hash>(boost::bind(&AlarmService::slotUpdateAlarms, this, _1, _2));
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
        
        void AlarmService::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo){
            
            {   
                //updates by row
                Hash rowUpdates;
                
                boost::upgrade_lock<boost::shared_mutex> readLock(m_alarmChangeMutex);
                boost::optional<Hash::Node&> alarmN = m_alarms.find(instanceId);
                if(alarmN){
                    {
                        KARABO_LOG_INFO<<"Device instance '"<<instanceId<<"' disappeared. Setting all pending alarms to acknowledgeable";
                        boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
                        Hash& alarm = alarmN->getValue<Hash>();
                        for(Hash::iterator propIt = alarm.begin(); propIt != alarm.end(); ++propIt){
                            Hash& property = propIt->getValue<Hash>();
                            for(Hash::iterator aTypeIt = property.begin(); aTypeIt != property.end(); ++aTypeIt){
                                Hash& typeEntry = aTypeIt->getValue<Hash>();
                                //if a device died all alarms need to be and can be acknowledged
                                typeEntry.set("needsAcknowledging", true);
                                typeEntry.set("acknowledgeable", true);
                                
                                //add as update to row updates;
                                const unsigned long long id =  m_alarmsMap_r.find(&(*aTypeIt))->second;
                                rowUpdates.set(boost::lexical_cast<std::string>(id), addRowUpdate("deviceKilled",  typeEntry));
 
                            }
                        }
                    }
                    emit("signalAlarmServiceUpdate", getInstanceId(), std::string("alarmUpdate"), rowUpdates);
                }
                
            }
           
        }
        

        void AlarmService::slotUpdateAlarms(const std::string& deviceId, const karabo::util::Hash& alarmInfo){
            
            //check for sanity of incoming hash
            if(!alarmInfo.has("toClear") || !alarmInfo.has("toAdd")) return;
            
            // get rid of alarm conditions that have passed or can now be acknowledged
            const Hash& toClear = alarmInfo.get<Hash>("toClear");
            
            //updates by row
            Hash rowUpdates;
            
            // check if any alarms exist for this deviceId
            // in the following capital "N" at the end of a variable declarations signifies a Hash node
            boost::upgrade_lock<boost::shared_mutex> readLock(m_alarmChangeMutex);
            boost::optional<Hash::Node&> existingDeviceEntryN = m_alarms.find(deviceId);

            if(existingDeviceEntryN){
                boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
                Hash& existingDeviceEntry = existingDeviceEntryN->getValue<Hash>();
                
                //iterate over properties for this deviceId
                for(Hash::const_iterator propIt = toClear.begin(); propIt != toClear.end(); ++propIt){
                    boost::optional<Hash::Node&> existingPropEntryN = existingDeviceEntry.find(propIt->getKey()); //existing property alarms for deviceId
                    if(!existingPropEntryN) continue; //no alarm for this property
                    
                    Hash& existingPropEntry = existingPropEntryN->getValue<Hash>();
                   
                    //iterate over alarm types in this property
                    const std::vector<std::string>& alarmTypes = propIt->getValue<std::vector<std::string> >();
                   
                    for(std::vector<std::string>::const_iterator aTypeIt = alarmTypes.begin(); aTypeIt != alarmTypes.end(); ++aTypeIt){
                        boost::optional<Hash::Node&> existingTypeEntryN = existingPropEntry.find(*aTypeIt); //existing alarm types for property
                        if(!existingTypeEntryN) continue; //no alarm for this property and alarm type
   
                        //alarm of this type exists for the property
                        Hash& existingTypeEntry = existingTypeEntryN->getValue<Hash>();
                        
                        const unsigned long long id =  m_alarmsMap_r.find(&(*existingTypeEntryN))->second;
                        if(existingTypeEntry.get<bool>("needsAcknowledging")){
                            //if the alarm needs to be acknowledged we allow this now
                            existingTypeEntry.set("acknowledgeable", true);
                            //add to rowUpdates
                            rowUpdates.set(boost::lexical_cast<std::string>(id), addRowUpdate("acknowledgeable", existingTypeEntry));   
                           
                        } else {
                           
                            //add as delete to row updates;
                            rowUpdates.set(boost::lexical_cast<std::string>(id), addRowUpdate("remove", existingTypeEntry));
                            
                            m_alarmsMap_r.erase(&(*existingTypeEntryN));
                            m_alarmsMap.erase(id);
                            
                            //go ahead and erase the alarm condition as it is allowed to silently disappear
                            existingPropEntry.erase(*aTypeIt);
                        }
                    }
                    
                    if(existingPropEntry.empty()) existingDeviceEntry.erase(propIt->getKey());
                    
                }
            }
            
            //now add new alarms
            const Hash& toAdd = alarmInfo.get<Hash>("toAdd");

            if(!toAdd.empty()){

                boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
                //if instance appears for first time we add it to the alarm entries

                if(!existingDeviceEntryN){
                    existingDeviceEntryN = m_alarms.set(deviceId, Hash());
                }
                
                Hash& existingDeviceEntry = existingDeviceEntryN->getValue<Hash>();

                //iteration over properties with alarms
                for(Hash::const_iterator propIt = toAdd.begin(); propIt != toAdd.end(); ++propIt){
                    //check if alarms for this property exist
                    boost::optional<Hash::Node&> existingPropEntryN = existingDeviceEntry.find(propIt->getKey());
                    if(!existingPropEntryN) { 
                        //create node for property if it doesn't exist
                        existingPropEntryN = existingDeviceEntry.set(propIt->getKey(), Hash());
                    }
                    
                    //update this property
                    const Hash& updatingPropEntry =  propIt->getValue<Hash>(); 
                    Hash& existingPropEntry = existingPropEntryN->getValue<Hash>();

                    //iterates over alarm type of this property
                    for(Hash::const_iterator aTypeIt = updatingPropEntry.begin(); aTypeIt != updatingPropEntry.end(); ++aTypeIt){
                        boost::optional<Hash::Node&> existingTypeEntryN = existingPropEntry.find(aTypeIt->getKey());
                        
                        
                        const Timestamp updatedTimeStamp = Timestamp::fromHashAttributes(aTypeIt->getAttributes());
                        Timestamp originalTimeStamp = updatedTimeStamp;
                        
                        
                        //get the next id if we perform insertion
                        unsigned long long id = 0;
                        if(!m_alarmsMap.empty()) {
                            id = (--m_alarmsMap.end())->first;
                            id++;
                        }
   
                        if(existingTypeEntryN) {
                            //alarm exists, we use its first occurance
                            Hash& existingTypeEntry = existingTypeEntryN->getValue<Hash>();
                            originalTimeStamp = Timestamp::fromHashAttributes(existingTypeEntry.getAttributes("timeOfFirstOccurrence"));
                            id = m_alarmsMap_r.find(&(*existingTypeEntryN))->second;
                        }
                        
                        //first set all properties we can simply copy by assigning value of the new entry
                        Hash::Node& newEntryN = existingPropEntry.set(aTypeIt->getKey(), aTypeIt->getValue<Hash>());

                        //now those which we needed to modify
                        Hash& newEntry = newEntryN.getValue<Hash>();
                        newEntry.set("timeOfFirstOccurrence", originalTimeStamp.toIso8601Ext());
                        newEntry.set("timeOfOccurrence", updatedTimeStamp.toIso8601Ext());
                        originalTimeStamp.toHashAttributes(newEntry.getAttributes("timeOfFirstOccurrence"));
                        updatedTimeStamp.toHashAttributes(newEntry.getAttributes("timeOfOccurrence"));
                        // acknowledgeable is determined by whether an alarm needs acknowledging
                        newEntry.set("acknowledgeable",  !newEntry.get<bool>("needsAcknowledging"));
                        newEntry.set("deviceId", deviceId);
                        newEntry.set("property", boost::replace_all_copy(existingPropEntryN->getKey(), Validator::kAlarmParamPathSeparator, "."));
                        newEntry.set("id", id);
                        
                        //update maps
                        m_alarmsMap[id] = &newEntryN;
                        m_alarmsMap_r[&newEntryN] = id;
                        
                       
                        if(existingTypeEntryN) {
                            rowUpdates.set(boost::lexical_cast<std::string>(id), 
                                    addRowUpdate("update",  newEntry));
                        } else {
                            rowUpdates.set(boost::lexical_cast<std::string>(id), 
                                    addRowUpdate("add", newEntry));
                        }
                        
                        
                    }
                }
            }
            emit("signalAlarmServiceUpdate", getInstanceId(), std::string("alarmUpdate"), rowUpdates);
            
            
        }
        
        karabo::util::Hash AlarmService::addRowUpdate(const std::string& updateType, const karabo::util::Hash& entry) const {
            return Hash(updateType, entry);
        }

                
        void AlarmService::flushRunner() const {
            TextSerializer<Hash>::Pointer serializer = TextSerializer<Hash>::create("Xml");
            
            
            while(m_flushRunning){
                std::string archive;
                std::ofstream fout;
                
                fout.open(m_flushFilePath, ios::trunc);
                {
                    boost::interprocess::file_lock flock(m_flushFilePath.c_str());
                    boost::interprocess::scoped_lock<boost::interprocess::file_lock> wflock(flock);
                    
                    {

                        boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
                        
                        Hash h("devices", get<std::vector<std::string> >("registeredDevices"), "alarms", m_alarms);
                        serializer->save(h, archive);
                    }
                    fout<<archive;
                    fout.close();
                }
                boost::this_thread::sleep(boost::posix_time::seconds(this->get<unsigned int>("flushInterval")));
            }
        }
        

        void AlarmService::reinitFromFile(){
            //nothing to do if file doesn't exist
            if ( !boost::filesystem::exists(m_flushFilePath) ) return;

            boost::interprocess::file_lock flock(m_flushFilePath.c_str());
            try {
                boost::interprocess::sharable_lock<boost::interprocess::file_lock> shlock(flock);
                std::ifstream fin;
                fin.open(m_flushFilePath);
                std::ostringstream archive;
                std::string input;
                while(fin>>input) archive<<input<<std::endl;
                fin.close();
                TextSerializer<Hash>::Pointer serializer = TextSerializer<Hash>::create("Xml");
                Hash previousState;
                serializer->load(previousState, archive.str());
                
                boost::unique_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
                m_alarms = previousState.get<Hash>("alarms");
                
                
                
                
            } catch (...){
                //we go on without updating alarms
                KARABO_LOG_WARN<<"Could not load previous alarm state from file "<<m_flushFilePath<<" even though file exists!";
            }
            
            //send this as init information to Clients
            Hash rowInits;
            boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
            for(Hash::const_iterator it = m_alarms.begin(); it != m_alarms.end(); ++it){
                const Hash& instance = it->getValue<Hash>();   
                //property level
                for(Hash::const_iterator propIt = instance.begin(); propIt != instance.end(); ++propIt){
                    const Hash& alarmTypes = propIt->getValue<Hash>();
                    //type level
                    for(Hash::const_iterator aTypeIt = alarmTypes.begin(); aTypeIt != alarmTypes.end(); ++aTypeIt){
                        const Hash& entry = aTypeIt->getValue<Hash>();
                        const unsigned long long id = entry.get<unsigned long long>("id");
                        m_alarmsMap[id] = &(*aTypeIt);
                        m_alarmsMap_r[&(*aTypeIt)] = id;
                        rowInits.set(boost::lexical_cast<std::string>(id), addRowUpdate("init",  entry));
                        
                        
                    }
                }
            }
            emit("signalAlarmServiceUpdate", getInstanceId(), std::string("alarmInit"), rowInits);
            
            // trigger instance new handlers for existing devices
            // this will also trigger these devices to resubmit their alarms
            const Hash runtimeInfo = remote().getSystemInformation();
            const Hash& onlineDevices = runtimeInfo.get<Hash>("device");
            for (Hash::const_iterator it = onlineDevices.begin(); it != onlineDevices.end(); ++it) {
                const Hash::Node& deviceNode = *it;
                // Topology entry as understood by registerNewDevice: Hash with path "device.<deviceId>"
                Hash topologyEntry("device", Hash());
                // Copy node with key "<deviceId>" and attributes into the single Hash in topologyEntry:
                topologyEntry.begin()->getValue<Hash>().setNode(deviceNode);
                registerNewDevice(topologyEntry);              
            }

        }
        
        void AlarmService::slotAcknowledgeAlarm(const karabo::util::Hash& acknowledgedRows){
           
            Hash rowUpdates;
            for(Hash::const_iterator it = acknowledgedRows.begin(); it != acknowledgedRows.end(); ++it){
                try {
                    const unsigned long long id = boost::lexical_cast<unsigned long long>(it->getKey());
                
                    const auto mapIter = m_alarmsMap.find(id);
                    if (mapIter == m_alarmsMap.end()) {
                         KARABO_LOG_WARN << "Tried to acknowledge non-existing alarm!";
                         continue;
                    }
                    Hash::Node* entryN = mapIter->second;
                    Hash& entry = entryN->getValue<Hash>();
                    if(entry.get<bool>("acknowledgeable")){

                        //add as delete to row updates;
                        entry.set("acknowledged", true);
                        rowUpdates.set(boost::lexical_cast<std::string>(id), addRowUpdate("remove", entry));
                        
                        m_alarmsMap_r.erase(&(*entryN));
                        m_alarmsMap.erase(id);
                        const std::string path = entry.get<std::string>("deviceId") 
                            + "." + boost::replace_all_copy(entry.get<std::string>("property"), ".", Validator::kAlarmParamPathSeparator) 
                            + "." + entry.get<std::string>("type");
                        m_alarms.erasePath(path, '.');
                    } else {
                        rowUpdates.set(boost::lexical_cast<std::string>(id), addRowUpdate("refuseAcknowledgement",  entry));
                    }
                } catch (const boost::bad_lexical_cast & ){
                    KARABO_LOG_ERROR<<"Failed casting "<<it->getKey()<<" to integer representation";
                }
                
            }
            if(!rowUpdates.empty()) emit("signalAlarmServiceUpdate", getInstanceId(), std::string("alarmUpdate"), rowUpdates);
            
        }
        
        void AlarmService::slotRequestAlarmDump(){
            
            Hash rowInits;
            boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
            for(auto it = m_alarmsMap.begin(); it != m_alarmsMap.end(); ++it){
                const Hash::Node* entryN = it->second;
                const Hash& entry = entryN->getValue<Hash>(); 
                rowInits.set(boost::lexical_cast<std::string>(entry.get<unsigned long long>("id")), addRowUpdate("init", entry));
                
            }
            
            reply(Hash("instanceId", getInstanceId(), "alarms", rowInits));
            
        }
       
    }
}

