/*
 * File:   AlarmService.cc
 * Author: haufs
 *
 * Created on August 5, 2016, 11:30 AM
 */


#include "AlarmService.hh"

#include <boost/foreach.hpp>
#include <fstream>

#include "karabo/io/FileTools.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/PathElement.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/State.hh"
#include "karabo/util/VectorElement.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, karabo::devices::AlarmService)

namespace karabo {

    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::core;
        using namespace boost::placeholders;


        void AlarmService::expectedParameters(Schema& expected) {
            OVERWRITE_ELEMENT(expected)
                  .key("state")
                  .setNewOptions(State::INIT, State::ON)
                  .setNewDefaultValue(State::INIT)
                  .commit();

            OVERWRITE_ELEMENT(expected).key("deviceId").setNewDefaultValue("Karabo_AlarmService").commit();
            // device elements

            OVERWRITE_ELEMENT(expected).key("visibility").setNewDefaultValue<int>(Schema::AccessLevel::ADMIN).commit();

            PATH_ELEMENT(expected)
                  .key("storagePath")
                  .displayedName("Storage path")
                  .description("Path under which this device will persist its data for recovery")
                  .assignmentOptional()
                  .defaultValue("./")
                  .expertAccess()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("flushInterval")
                  .displayedName("Flush interval")
                  .unit(Unit::SECOND)
                  .assignmentOptional()
                  .defaultValue(10)
                  .reconfigurable()
                  .expertAccess()
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("registeredDevices")
                  .displayedName("Registered devices")
                  .description("The devices which are currently registered to this alarm service device")
                  .readOnly()
                  .expertAccess()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("updateTime")
                  .displayedName("Update Time")
                  .description("Time interval between the sending of updates!")
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(500)
                  .reconfigurable()
                  .minInc(100)
                  .maxInc(2000)
                  .commit();
        }


        AlarmService::AlarmService(const karabo::util::Hash& input)
            : karabo::core::Device<>(input),
              m_updateTimer(karabo::net::EventLoop::getIOService()),
              m_alarmIdCounter(0ull) {
            setupSignalsAndSlots();
            KARABO_INITIAL_FUNCTION(initialize);
        }


        AlarmService::~AlarmService() {}


        void AlarmService::preDestruction() {
            m_flushRunning = false;
            m_flushWorker.join();
        }

        void AlarmService::initialize() {
            // Recover previous alarms in case we recovered from a failure or were restarted
            m_flushFilePath = get<std::string>("storagePath") + "/" + getInstanceId() + ".xml";
            reinitFromFile();

            // NOTE: boost::bind() is OK for these handlers because SignalSlotable calls them directly instead
            // of dispatching them via the event loop.
            remote().registerInstanceNewMonitor(boost::bind(&AlarmService::registerNewDevice, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&AlarmService::instanceGoneHandler, this, _1, _2));

            // Switch on instance tracking - which is blocking a while.
            // Note that instanceNew(..) will be called for all instances already in the game.
            remote().enableInstanceTracking();

            // we add a worker thread, which persists alarm state at regular intervals. This data is used when
            // recovering from a alarm service shutdown
            m_flushRunning = true;
            m_flushWorker = boost::thread(&AlarmService::flushRunner, this);

            updateState(State::ON);

            m_updateTimer.expires_from_now(boost::posix_time::milliseconds(get<int>("updateTime")));
            m_updateTimer.async_wait(
                  bind_weak(&karabo::devices::AlarmService::sendAlarmUpdates, this, boost::asio::placeholders::error));
        }


        void AlarmService::sendAlarmUpdates(const boost::system::error_code& err) {
            if (err) {
                KARABO_LOG_FRAMEWORK_ERROR << "Sending alarm update timer was cancelled!";
                return;
            }
            try {
                boost::mutex::scoped_lock lock(m_updateMutex);
                if (!m_updateHash.empty()) {
                    // For this use here, the signal does NOT have to be a system signal (i.e. it may be droppable), at
                    // least what concerns the order of the signal and slot reply since not triggered by a slot call
                    // here.
                    emit("signalAlarmServiceUpdate", getInstanceId(), std::string("alarmUpdate"), m_updateHash);
                    m_updateHash.clear();
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in sendAlarmUpdates(): " << e.detailedMsg();
            }

            // Always restart the timer!
            m_updateTimer.expires_from_now(boost::posix_time::milliseconds(get<int>("updateTime")));
            m_updateTimer.async_wait(
                  bind_weak(&karabo::devices::AlarmService::sendAlarmUpdates, this, boost::asio::placeholders::error));
        }

        void AlarmService::setupSignalsAndSlots() {
            registerSlot<std::string, karabo::util::Hash>(boost::bind(&AlarmService::slotUpdateAlarms, this, _1, _2),
                                                          "slotUpdateAlarms");
            // See comments where this signal is emitted to clarify why this is a SYSTEM_SIGNAL.
            KARABO_SYSTEM_SIGNAL("signalAlarmServiceUpdate", std::string, std::string, karabo::util::Hash)
            registerSlot<karabo::util::Hash>(boost::bind(&AlarmService::slotAcknowledgeAlarm, this, _1),
                                             "slotAcknowledgeAlarm");
            registerSlot(boost::bind(&AlarmService::slotRequestAlarmDump, this), "slotRequestAlarmDump");
            KARABO_SLOT(slotRequestAlarms, karabo::util::Hash);
        }


        void AlarmService::registerNewDevice(const karabo::util::Hash& topologyEntry) {
            // only act upon instance we do not know about yet coming up
            try {
                const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                if (type == "device") {
                    // const ref is fine even for temporary std::string&
                    const std::string& deviceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type)
                                                         ? topologyEntry.get<Hash>(type).begin()->getKey()
                                                         : std::string("?"));

                    if (deviceId == getInstanceId()) return; // prevent registering ourselves.

                    // Connect signal and request current set of alarms afterwards
                    asyncConnect(deviceId, "signalAlarmUpdate", "", "slotUpdateAlarms",
                                 util::bind_weak(&AlarmService::connectedHandler, this, deviceId));
                }
            } catch (const std::exception& e) {
                KARABO_LOG_ERROR << "In registerAlarmWithNewDevice: " << e.what() << ".";
            }
        }


        void AlarmService::connectedHandler(const std::string& deviceId) {
            {
                // Since there is no Device::appendVector(key, vectorItem), need an extra mutex lock to avoid that
                // "registeredDevices" changes in between 'get' and 'set':
                boost::unique_lock<boost::shared_mutex> lock(m_deviceRegisterMutex);
                std::vector<std::string> devices = get<std::vector<std::string> >("registeredDevices");
                // Avoid duplicates that could arise if a device shuts down and comes back
                if (std::find(devices.begin(), devices.end(), deviceId) == devices.end()) {
                    devices.push_back(deviceId);
                    set("registeredDevices", devices);
                }
            }
            // We might have alarms pending from it. We should ask for an update on these alarms.
            boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
            const boost::optional<Hash::Node&> alarmNode = m_alarms.find(deviceId);
            request(deviceId, "slotReSubmitAlarms", alarmNode ? alarmNode->getValue<Hash>() : Hash())
                  // Using slot as reply handler is OK as long as mutexes are used inside
                  // (since  "slot not called in parallel to itself" not guaranteed anymore)
                  .receiveAsync<std::string, Hash>(util::bind_weak(&AlarmService::slotUpdateAlarms, this, _1, _2));
        }


        void AlarmService::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            // Disconnect to switch off the automatic reconnect.
            // This allows a controlled connect/request procedure (in registerNewDevice) in case the device comes back
            asyncDisconnect(instanceId, "signalAlarmUpdate", "", "slotUpdateAlarms",
                            boost::function<void()>(), // no success handler needed
                            []() { // dummy error handler to completely silence the expected time out handling
                            });

            // updates by row
            Hash rowUpdates;

            boost::upgrade_lock<boost::shared_mutex> readLock(m_alarmChangeMutex);
            boost::optional<Hash::Node&> alarmN = m_alarms.find(instanceId);
            if (alarmN) {
                {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Device instance '" << instanceId
                                               << "' disappeared. Setting all pending alarms to acknowledgeable";
                    boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
                    Hash& alarm = alarmN->getValue<Hash>();
                    for (Hash::iterator propIt = alarm.begin(); propIt != alarm.end(); ++propIt) {
                        Hash& property = propIt->getValue<Hash>();
                        for (Hash::iterator aTypeIt = property.begin(); aTypeIt != property.end(); ++aTypeIt) {
                            Hash& typeEntry = aTypeIt->getValue<Hash>();
                            // if a device died all alarms need to be and can be acknowledged
                            typeEntry.set("needsAcknowledging", true);
                            typeEntry.set("acknowledgeable", true);

                            // add as update to row updates;
                            const unsigned long long id = m_alarmsMap_r.find(&(*aTypeIt))->second;
                            rowUpdates.set(boost::lexical_cast<std::string>(id),
                                           addRowUpdate("deviceKilled", typeEntry));
                        }
                    }
                }
                if (!rowUpdates.empty()) {
                    boost::mutex::scoped_lock lock(m_updateMutex);
                    m_updateHash.merge(rowUpdates);
                }
            }
        }


        void AlarmService::slotUpdateAlarms(const std::string& deviceId, const karabo::util::Hash& alarmInfo) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotUpdateAlarms alarmInfo for: " << deviceId << " " << alarmInfo;

            // alarmInfo MUST HAVE "toClear" and "toAdd" keys
            if (!(alarmInfo.has("toClear") && alarmInfo.has("toAdd"))) return;

            Hash rowUpdates;

            removeDeviceAlarms(deviceId, alarmInfo.get<Hash>("toClear"), rowUpdates);
            addDeviceAlarms(deviceId, alarmInfo.get<Hash>("toAdd"), rowUpdates);
            if (!rowUpdates.empty()) {
                boost::mutex::scoped_lock lock(m_updateMutex);
                m_updateHash.merge(rowUpdates);
            }
        }


        karabo::util::Hash AlarmService::addRowUpdate(const std::string& updateType,
                                                      const karabo::util::Hash& entry) const {
            return Hash(updateType, entry);
        }


        void AlarmService::addDeviceAlarms(const std::string& deviceId, const karabo::util::Hash& alarms,
                                           karabo::util::Hash& rowUpdates) {
            // check if any alarms exist for this deviceId
            // in the following, a suffix of "Node" at the end of a variable name signifies a Hash node
            boost::upgrade_lock<boost::shared_mutex> readLock(m_alarmChangeMutex);

            if (!alarms.empty()) {
                boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
                boost::optional<Hash::Node&> deviceAlarmsNode = m_alarms.find(deviceId);

                if (!deviceAlarmsNode) {
                    // These are the first alarm entries for this device. Create a sub-Hash in m_alarms
                    deviceAlarmsNode = m_alarms.set(deviceId, Hash());
                }

                Hash& deviceAlarms = deviceAlarmsNode->getValue<Hash>();

                // Iterate over properties with alarms to add
                for (Hash::const_iterator propertyNameIt = alarms.begin(); propertyNameIt != alarms.end();
                     ++propertyNameIt) {
                    // check if alarms for this property exist
                    boost::optional<Hash::Node&> propertyEntryNode = deviceAlarms.find(propertyNameIt->getKey());
                    if (!propertyEntryNode) {
                        // These are the first alarm entries for this property
                        propertyEntryNode = deviceAlarms.set(propertyNameIt->getKey(), Hash());
                    }

                    // Update the property
                    const Hash& propertyEntryFromUpdate = propertyNameIt->getValue<Hash>();
                    Hash& propertyEntry = propertyEntryNode->getValue<Hash>();
                    std::string lastAlarmType;

                    // Iterate over alarm types for this property
                    for (Hash::const_iterator alarmTypeEntryIt = propertyEntryFromUpdate.begin();
                         alarmTypeEntryIt != propertyEntryFromUpdate.end(); ++alarmTypeEntryIt) {
                        boost::optional<Hash::Node&> existingAlarmTypeEntryNode =
                              propertyEntry.find(alarmTypeEntryIt->getKey());

                        const Timestamp updateTimeStamp =
                              Timestamp::fromHashAttributes(alarmTypeEntryIt->getAttributes());
                        Timestamp existingTimeStamp = updateTimeStamp;

                        unsigned long long id = 0;
                        if (existingAlarmTypeEntryNode) {
                            // alarm exists, we use its first occurance
                            Hash& existingAlarmTypeEntry = existingAlarmTypeEntryNode->getValue<Hash>();
                            existingTimeStamp = Timestamp::fromHashAttributes(
                                  existingAlarmTypeEntry.getAttributes("timeOfFirstOccurrence"));
                            id = m_alarmsMap_r.find(&(*existingAlarmTypeEntryNode))->second;
                        } else {
                            // get the next id if we perform insertion
                            id = m_alarmIdCounter++;
                        }

                        // first set all properties we can simply copy by assigning value of the new entry
                        Hash::Node& newAlarmTypeEntryNode =
                              propertyEntry.set(alarmTypeEntryIt->getKey(), alarmTypeEntryIt->getValue<Hash>());
                        Hash& newAlarmTypeEntry = newAlarmTypeEntryNode.getValue<Hash>();

                        // now those which we needed to modify
                        newAlarmTypeEntry.set("timeOfFirstOccurrence", existingTimeStamp.toIso8601Ext());
                        existingTimeStamp.toHashAttributes(newAlarmTypeEntry.getAttributes("timeOfFirstOccurrence"));
                        newAlarmTypeEntry.set("timeOfOccurrence", updateTimeStamp.toIso8601Ext());
                        updateTimeStamp.toHashAttributes(newAlarmTypeEntry.getAttributes("timeOfOccurrence"));
                        // acknowledgeable is determined by whether an alarm needs acknowledging
                        newAlarmTypeEntry.set("acknowledgeable", !newAlarmTypeEntry.get<bool>("needsAcknowledging"));
                        newAlarmTypeEntry.set("deviceId", deviceId);
                        newAlarmTypeEntry.set("property",
                                              boost::replace_all_copy(propertyEntryNode->getKey(),
                                                                      Validator::kAlarmParamPathSeparator, "."));
                        newAlarmTypeEntry.set("id", id);

                        // update maps
                        m_alarmsMap[id] = &newAlarmTypeEntryNode;
                        m_alarmsMap_r[&newAlarmTypeEntryNode] = id;

                        if (existingAlarmTypeEntryNode) {
                            rowUpdates.set(boost::lexical_cast<std::string>(id),
                                           addRowUpdate("update", newAlarmTypeEntry));
                        } else {
                            rowUpdates.set(boost::lexical_cast<std::string>(id),
                                           addRowUpdate("add", newAlarmTypeEntry));
                        }
                        lastAlarmType = newAlarmTypeEntry.get<std::string>("type");
                    }

                    // Handle global alarm conditions from cpp/python/middlelayer devices
                    if (propertyEntryNode->getKey() == "global") {
                        const AlarmCondition lastAdded = AlarmCondition::fromString(lastAlarmType);
                        // Make all more significant alarm types acknowledgeable
                        makeMoreSignificantAcknowledgeable(propertyEntry, lastAdded, rowUpdates);
                    }
                }
            }
        }


        void AlarmService::removeDeviceAlarms(const std::string& deviceId, const karabo::util::Hash& alarms,
                                              karabo::util::Hash& rowUpdates) {
            boost::upgrade_lock<boost::shared_mutex> readLock(m_alarmChangeMutex);
            boost::optional<Hash::Node&> deviceAlarmsNode = m_alarms.find(deviceId);

            // check if any alarms exist for this deviceId
            if (deviceAlarmsNode) {
                boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
                Hash& deviceAlarms = deviceAlarmsNode->getValue<Hash>();

                // iterate over property names which have alarms to clear
                for (Hash::const_iterator propertyNameIt = alarms.begin(); propertyNameIt != alarms.end();
                     ++propertyNameIt) {
                    boost::optional<Hash::Node&> propertyEntryNode =
                          deviceAlarms.find(propertyNameIt->getKey()); // existing alarms for deviceId.property
                    if (!propertyEntryNode) continue;                  // no alarms for this property

                    Hash& propertyEntry = propertyEntryNode->getValue<Hash>();

                    // iterate over alarm types in this property
                    const std::vector<std::string>& alarmTypesToClear =
                          propertyNameIt->getValue<std::vector<std::string> >();

                    for (auto alarmTypeIt = alarmTypesToClear.begin(); alarmTypeIt != alarmTypesToClear.end();
                         ++alarmTypeIt) {
                        boost::optional<Hash::Node&> alarmTypeEntryNode =
                              propertyEntry.find(*alarmTypeIt); // existing alarm types for property
                        if (!alarmTypeEntryNode) continue;      // no alarm for this property and alarm type

                        // alarm of this type exists for the property
                        Hash& alarmTypeEntry = alarmTypeEntryNode->getValue<Hash>();
                        const unsigned long long id = m_alarmsMap_r.find(&(*alarmTypeEntryNode))->second;
                        if (alarmTypeEntry.get<bool>("needsAcknowledging")) {
                            // if the alarm needs to be acknowledged we allow this now
                            alarmTypeEntry.set("acknowledgeable", true);
                            // add to rowUpdates
                            rowUpdates.set(boost::lexical_cast<std::string>(id),
                                           addRowUpdate("acknowledgeable", alarmTypeEntry));

                        } else {
                            // add as delete to row updates;
                            rowUpdates.set(boost::lexical_cast<std::string>(id),
                                           addRowUpdate("remove", alarmTypeEntry));

                            // Erase the pointers to the m_alarms Hash
                            m_alarmsMap_r.erase(&(*alarmTypeEntryNode));
                            m_alarmsMap.erase(id);

                            // Erase the alarm condition from the m_alarms Hash as it is allowed to silently disappear
                            propertyEntry.erase(*alarmTypeIt);
                        }
                    }

                    if (propertyEntry.empty()) {
                        // When a device property has no remaining alarms, erase it from the m_alarms Hash
                        deviceAlarms.erase(propertyNameIt->getKey());
                    }
                }
            }
        }


        void AlarmService::flushRunner() const {
            while (m_flushRunning) {
                {
                    boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);

                    Hash h("alarms", m_alarms, "nextAlarmId", static_cast<unsigned long long>(m_alarmIdCounter));
                    karabo::io::saveToFile(h, m_flushFilePath);
                }
                boost::this_thread::sleep(boost::posix_time::seconds(this->get<unsigned int>("flushInterval")));
            }
        }


        void AlarmService::reinitFromFile() {
            // nothing to do if file doesn't exist
            if (!boost::filesystem::exists(m_flushFilePath)) return;

            try {
                Hash previousState;
                karabo::io::loadFromFile(previousState, m_flushFilePath);

                boost::unique_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
                m_alarms = previousState.get<Hash>("alarms");

                if (previousState.has("nextAlarmId")) {
                    m_alarmIdCounter = previousState.get<unsigned long long>("nextAlarmId");
                } else {
                    // File likely from older Karabo versions < 2.10.0.
                    // Start one above biggest stored id.
                    for (auto devIdIt = m_alarms.begin(); devIdIt != m_alarms.end(); ++devIdIt) {
                        const Hash& devAlarms = devIdIt->getValue<Hash>();
                        for (auto propIt = devAlarms.begin(); propIt != devAlarms.end(); ++propIt) {
                            const Hash& propAlarms = propIt->getValue<Hash>();
                            for (auto lvlIt = propAlarms.begin(); lvlIt != propAlarms.end(); ++lvlIt) {
                                const Hash& level = lvlIt->getValue<Hash>();
                                const unsigned long long id = level.get<unsigned long long>("id");
                                if (id >= m_alarmIdCounter) m_alarmIdCounter = id + 1ull;
                            }
                        }
                    }
                    KARABO_LOG_WARN << "Stored alarms file lacks 'nextAlarmId' (likely from Karabo version < 2.10.0)."
                                    << " Start with " << m_alarmIdCounter << ".";
                    KARABO_LOG_FRAMEWORK_INFO << "Initialised successfully from file '" << m_flushFilePath << "'.";
                }
            } catch (const std::exception& e) {
                // we go on without updating alarms
                KARABO_LOG_WARN << "Could not load previous alarm state from file " << m_flushFilePath
                                << " even though file exists: " << e.what();
            }

            // send this as init information to Clients
            Hash rowInits;
            boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
            for (Hash::const_iterator it = m_alarms.begin(); it != m_alarms.end(); ++it) {
                const Hash& instance = it->getValue<Hash>();
                // property level
                for (Hash::const_iterator propIt = instance.begin(); propIt != instance.end(); ++propIt) {
                    const Hash& alarmTypes = propIt->getValue<Hash>();
                    // type level
                    for (Hash::const_iterator aTypeIt = alarmTypes.begin(); aTypeIt != alarmTypes.end(); ++aTypeIt) {
                        const Hash& entry = aTypeIt->getValue<Hash>();
                        const unsigned long long id = entry.get<unsigned long long>("id");
                        m_alarmsMap[id] = &(*aTypeIt);
                        m_alarmsMap_r[&(*aTypeIt)] = id;
                        rowInits.set(boost::lexical_cast<std::string>(id), addRowUpdate("init", entry));
                    }
                }
            }
            // For this use here, the signal does NOT have to be a system signal (i.e. it may be droppable), at least
            // what concerns the order of the signal and slot reply since not triggered by a slot call here.
            emit("signalAlarmServiceUpdate", getInstanceId(), std::string("alarmInit"), rowInits);
        }


        void AlarmService::slotAcknowledgeAlarm(const karabo::util::Hash& acknowledgedRows) {
            Hash rowUpdates;
            for (Hash::const_iterator it = acknowledgedRows.begin(); it != acknowledgedRows.end(); ++it) {
                try {
                    const unsigned long long id = boost::lexical_cast<unsigned long long>(it->getKey());

                    const auto mapIter = m_alarmsMap.find(id);
                    if (mapIter == m_alarmsMap.end()) {
                        KARABO_LOG_WARN << "Tried to acknowledge non-existing alarm!";
                        continue;
                    }
                    Hash::Node* entryN = mapIter->second;
                    Hash& entry = entryN->getValue<Hash>();
                    if (entry.get<bool>("acknowledgeable")) {
                        // add as delete to row updates;
                        entry.set("acknowledged", true);
                        rowUpdates.set(boost::lexical_cast<std::string>(id), addRowUpdate("remove", entry));

                        m_alarmsMap_r.erase(&(*entryN));
                        m_alarmsMap.erase(id);
                        const std::string path = entry.get<std::string>("deviceId") + "." +
                                                 boost::replace_all_copy(entry.get<std::string>("property"), ".",
                                                                         Validator::kAlarmParamPathSeparator) +
                                                 "." + entry.get<std::string>("type");
                        m_alarms.erasePath(path, '.');
                    } else {
                        rowUpdates.set(boost::lexical_cast<std::string>(id),
                                       addRowUpdate("refuseAcknowledgement", entry));
                    }
                } catch (const boost::bad_lexical_cast&) {
                    KARABO_LOG_ERROR << "Failed casting " << it->getKey() << " to integer representation";
                }
            }
            // Immediately send out our changes after human interaction!
            if (!rowUpdates.empty()) {
                boost::mutex::scoped_lock lock(m_updateMutex);
                m_updateHash.merge(rowUpdates);
                // To avoid any surprises with order of receival of this signal and the reply to the call to this
                // "slotAcknowledgeAlarm", the signal has to be a KARABO_SYSTEM_SIGNAL.
                emit("signalAlarmServiceUpdate", getInstanceId(), std::string("alarmUpdate"), m_updateHash);
                m_updateHash.clear();
            }
            // Reply that the command has been executed successfully
            reply(Hash("instanceId", getInstanceId(), "success", true, "reason", ""));
        }


        void AlarmService::slotRequestAlarmDump() {
            this->sendAlarmInformation();
        }


        void AlarmService::slotRequestAlarms(const karabo::util::Hash& info) {
            this->sendAlarmInformation();
        }


        void AlarmService::sendAlarmInformation() {
            Hash rowInits;
            boost::shared_lock<boost::shared_mutex> lock(m_alarmChangeMutex);
            for (auto it = m_alarmsMap.begin(); it != m_alarmsMap.end(); ++it) {
                const Hash::Node* entryN = it->second;
                const Hash& entry = entryN->getValue<Hash>();
                rowInits.set(boost::lexical_cast<std::string>(entry.get<unsigned long long>("id")),
                             addRowUpdate("init", entry));
            }
            reply(Hash("instanceId", getInstanceId(), "alarms", rowInits, "success", true, "reason", ""));
        }


        void AlarmService::makeMoreSignificantAcknowledgeable(karabo::util::Hash& propertyAlarms,
                                                              const karabo::util::AlarmCondition& lastAdded,
                                                              karabo::util::Hash& rowUpdates) {
            /*
             * Checking against the following matrix:
             *
             * X marks acknowledgeable
             *                   warn  alarm  interlock
             * normal state        X     X      X
             * warn state          -     X      X
             * alarm state         -     -      X
             * interlock state     -     -      -
             */
            for (Hash::iterator alarmTypeEntryIt = propertyAlarms.begin(); alarmTypeEntryIt != propertyAlarms.end();
                 ++alarmTypeEntryIt) {
                const AlarmCondition alarmType = AlarmCondition::fromString(alarmTypeEntryIt->getKey());
                if (alarmType.isMoreCriticalThan(lastAdded)) {
                    // make this type entry acknowledgeable
                    Hash::Node& existingAlarmTypeEntryNode = propertyAlarms.getNode(alarmTypeEntryIt->getKey());
                    Hash& existingAlarmTypeEntry = existingAlarmTypeEntryNode.getValue<Hash>();

                    if (!existingAlarmTypeEntry.get<bool>("acknowledgeable")) {
                        const unsigned long long id = m_alarmsMap_r[&existingAlarmTypeEntryNode];
                        existingAlarmTypeEntry.set("acknowledgeable",
                                                   existingAlarmTypeEntry.get<bool>("needsAcknowledging"));

                        rowUpdates.set(boost::lexical_cast<std::string>(id),
                                       addRowUpdate("acknowledgeable", existingAlarmTypeEntry));
                    }
                }
            }
        }
    } // namespace devices
} // namespace karabo
