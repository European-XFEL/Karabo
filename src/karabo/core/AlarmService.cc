/* 
 * File:   AlarmService.cc
 * Author: haufs
 * 
 * Created on August 5, 2016, 11:30 AM
 */

#include "AlarmService.hh"
#include "karabo/util/State.hh"
#include "karabo/io/TextSerializer.hh"
#include <fstream>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>


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
            
            STRING_ELEMENT(tableRow).key("deviceId")
                    .displayedName("Device")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("property")
                    .displayedName("Property")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("type")
                    .displayedName("Type")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("description")
                    .displayedName("Description")
                    .readOnly()
                    .commit();
            
            BOOL_ELEMENT(tableRow).key("needsAcknowledging")
                    .displayedName("Needs acknowledging")
                    .readOnly()
                    .commit();
            
            BOOL_ELEMENT(tableRow).key("acknowledgeable")
                    .displayedName("Acknowledgeable")
                    .readOnly()
                    .commit();
            
            BOOL_ELEMENT(tableRow).key("acknowledged")
                    .displayedName("Acknowledged")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
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
            m_flushRunning = false;
            m_flushWorker.join();
        }
        
        void AlarmService::initialize() {

            updateState(State::INIT);
            
            setupSignalsAndSlots();
            
                        
            //we listen on instance new events to connect to the instances "signalAlarmUpdate" signal
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

        }
        
        void AlarmService::registerNewDevice(const karabo::util::Hash& topologyEntry){
            //only act upon instance we do not know about yet coming up
            try {
                const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                // const ref is fine even for temporary std::string+
                if(type == "device"){
                    
                    const std::string& deviceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                                                         topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));

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
                boost::upgrade_lock<boost::shared_mutex> readLock(m_alarmChangeMutex);
                boost::optional<Hash::Node&> alarmN = m_alarms.find(instanceId);
                if(alarmN){
                    KARABO_LOG_INFO<<"Device instance '"<<instanceId<<"' disappeared. Setting all pending alarms to acknowledgeable";
                    boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
                    Hash& entry = alarmN->getValue<Hash>();
                    for(Hash::iterator propIt = entry.begin(); propIt != entry.end(); ++propIt){
                        Hash& property = propIt->getValue<Hash>();
                        for(Hash::iterator aTypeIt = property.begin(); aTypeIt != property.end(); ++aTypeIt){
                            Hash& typeEntry = aTypeIt->getValue<Hash>();
                            //if a device died all alarms need to be and can be acknowledged
                            typeEntry.set("needsAcknowledging", true);
                            typeEntry.set("acknowledgeable", true);
                        }
                    }
                    updateAlarmTable();
                }
            }
           
        }
        

        void AlarmService::slotUpdateAlarms(const std::string& deviceId, const karabo::util::Hash& alarmInfo){
            
            //check for sanity of incoming hash
            if(!alarmInfo.has("toClear") || !alarmInfo.has("toAdd")) return;
            
            // get rid of alarm conditions that have passed or can now be acknowledged
            const Hash& toClear = alarmInfo.get<Hash>("toClear");
            
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
                        if(existingTypeEntry.get<bool>("needsAcknowledging")){
                            //if the alarm needs to be acknowledged we allow this now
                            existingTypeEntry.set("acknowledgeable", true);
                           
                        } else {
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
                        
                        
                        const Timestamp updatedTimeStamp = Timestamp::fromHashAttributes(propIt->getAttributes());
                        const std::string timeOfOccurrence = updatedTimeStamp.toIso8601();
                        const unsigned long long trainOfOccurrence = updatedTimeStamp.getTrainId();
                        std::string timeOfFirstOccurrence = timeOfOccurrence;
                        unsigned long long trainOfFirstOccurrence = trainOfOccurrence;
                        
                        if(existingTypeEntryN) {
                            //alarm exists, we use its first occurance
                            Hash& existingTypeEntry = existingTypeEntryN->getValue<Hash>();
                            timeOfFirstOccurrence =  existingTypeEntry.get<std::string>("timeOfFirstOccurrence");
                            trainOfFirstOccurrence =  existingTypeEntry.get<unsigned long long>("trainOfFirstOccurrence");
                        }
                        
                        //first set all properties we can simply copy by assigning value of the new entry
                        Hash::Node& newEntryN = existingPropEntry.set(aTypeIt->getKey(), aTypeIt->getValue<Hash>());

                        //now those which we needed to modify
                        Hash& newEntry = newEntryN.getValue<Hash>();
                        newEntry.set("timeOfFirstOccurrence", timeOfFirstOccurrence);
                        newEntry.set("trainOfFirstOccurrence", trainOfFirstOccurrence);
                        newEntry.set("timeOfOccurrence", timeOfOccurrence);
                        newEntry.set("trainOfOccurrence", trainOfOccurrence);
                        // acknowledgeable is determined by whether an alarm needs acknowledging
                        newEntry.set("acknowledgeable",  !newEntry.get<bool>("needsAcknowledging"));
                        
                
                    }
                }
            }
            
            updateAlarmTable();
            
        }
        
        void AlarmService::updateAlarmTable(){
            std::vector<Hash> tableVector;

            //instance level

            boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
            for(Hash::const_iterator it = m_alarms.begin(); it != m_alarms.end(); ++it){
                const std::string& device = it->getKey();
                const Hash& instances = it->getValue<Hash>();
                
                //property level
                for(Hash::const_iterator propIt = instances.begin(); propIt != instances.end(); ++propIt){
                    const std::string& property = propIt->getKey();
                    const Hash& alarmTypes = propIt->getValue<Hash>();
                    
                    //type level
                    for(Hash::const_iterator aTypeIt = alarmTypes.begin(); aTypeIt != alarmTypes.end(); ++aTypeIt){
                        const Hash& entry = aTypeIt->getValue<Hash>();
                        tableVector.push_back(Hash());
                        Hash& h =  tableVector.back();
                        h.set("timeOfOccurrence", entry.get<std::string>("timeOfOccurrence"));
                        h.set("trainOfOccurrence", entry.get<unsigned long long>("trainOfOccurrence"));
                        h.set("timeOfFirstOccurrence", entry.get<std::string>("timeOfFirstOccurrence"));
                        h.set("trainOfFirstOccurrence", entry.get<unsigned long long>("trainOfFirstOccurrence"));
                        h.set("deviceId", device);
                        h.set("property", property);
                        h.set("type", entry.get<std::string>("type"));
                        h.set("description", entry.get<std::string>("description"));
                        h.set("needsAcknowledging", entry.get<bool>("needsAcknowledging"));
                        h.set("acknowledgeable", entry.get<bool>("acknowledgeable"));
                       
                        
                    }
                }
            }
            
            set("currentAlarms", tableVector);
        }
        
        void AlarmService::preReconfigure(karabo::util::Hash& incomingReconfiguration){
            
            boost::optional<Hash::Node&> alarmConfig = incomingReconfiguration.find("currentAlarms");
            if(alarmConfig){
                std::vector<Hash>& alarmsInTable = alarmConfig->getValue<std::vector<Hash> >();
                for(std::vector<Hash>::iterator it = alarmsInTable.begin(); it != alarmsInTable.end(); ++it){
                    const std::string& deviceId = it->get<std::string>("deviceId");
                    const std::string& property = it->get<std::string>("property");
                    const std::string& type = it->get<std::string>("type");
                    const std::string path = deviceId+"."+property+"."+type;
                    //we make sure that the table is up-to-date-with the internal data structure
                    boost::upgrade_lock<boost::shared_mutex> readLock(m_alarmChangeMutex);
                    const boost::optional<Hash::Node&> entryN = m_alarms.find(path, '.');
                     if(entryN){
                         const Hash& entry = entryN->getValue<Hash>();
                         if(entry.get<bool>("acknowledgeable") && entry.get<bool>("needsAcknowledging") && it->get<bool>("acknowledged")){
                             //remove the entry
                            boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
                            m_alarms.erasePath(path, '.');
                         } 
                     } else {
                         KARABO_LOG_WARN<<"Element in alarm table ("<<deviceId<<":"<<property<<":"<<type<<") does not match any internal alarm entry!";

                     }
                }
                // now remove update to table and update from m_alarms instead
                // this is necessary to keep everything in sync. Even if new alarms pop up during the update
                incomingReconfiguration.erase("currentAlarms");
                updateAlarmTable();
            }
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
            
            //now request all devices to update their alarms
            const Hash runtimeInfo = remote().getSystemInformation();
            const Hash& onlineDevices = runtimeInfo.get<Hash>("device");
            for (Hash::const_iterator it = onlineDevices.begin(); it != onlineDevices.end(); ++it) {
                const Hash::Node& deviceNode = *it;
                // Topology entry as understood by registerNewDevice: Hash with path "device.<deviceId>"
                Hash topologyEntry("device", Hash());
                // Copy node with key "<deviceId>" and attributes into the single Hash in topologyEntry:
                topologyEntry.begin()->getValue<Hash>().setNode(deviceNode);
                registerNewDevice(topologyEntry);
                
                const std::string& deviceId = it->getKey();
                
                boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
                const boost::optional<Hash::Node&> entry = m_alarms.find(deviceId);
                request(deviceId, "slotReSubmitAlarms", entry ? entry->getValue<Hash>() : Hash())
                        .receiveAsync<std::string, Hash>(boost::bind(&AlarmService::slotUpdateAlarms, this, _1, _2));
            }

        }

        
    }
}

