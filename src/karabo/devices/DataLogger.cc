/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <streambuf>

#include <algorithm>
#include <boost/filesystem.hpp>

#include "karabo/io/Input.hh"
#include "karabo/io/TextSerializer.hh"
#include "karabo/net/Strand.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/Strand.hh"
#include "karabo/util/OverwriteElement.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/TableElement.hh"
#include "karabo/util/VectorElement.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/xms/SlotElement.hh"

#include "DataLogger.hh"

namespace karabo {
    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::xms;


        void DataLogger::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::ON)
                    .setNewDefaultValue(State::INIT)
                    .commit();

            VECTOR_STRING_ELEMENT(expected).key("devicesToBeLogged")
                    .displayedName("Devices to be logged")
                    .description("The devices that should be logged by this logger instance")
                    .assignmentOptional()
                    .defaultValue(std::vector<std::string>())
                    .commit();

            VECTOR_STRING_ELEMENT(expected).key("devicesNotLogged")
                    .displayedName("Devices not logged")
                    .description("The devices that are not (yet or due to connection failures) logged")
                    .readOnly()
                    .initialValue(std::vector<std::string>())
                    .commit();

            Schema lastUpdateSchema;
            STRING_ELEMENT(lastUpdateSchema).key("deviceId")
                    .displayedName("Device")
                    .readOnly()
                    .initialValue(std::string())
                    .commit();

            STRING_ELEMENT(lastUpdateSchema).key("lastUpdateUtc")
                    .displayedName("Last Update (UTC)")
                    .readOnly()
                    .initialValue(std::string())
                    .commit();

            TABLE_ELEMENT(expected).key("lastUpdatesUtc")
                    .displayedName("Last Updates (UTC)")
                    .description("Timestamps of last recorded parameter update in UTC (updated in flush interval)")
                    .setColumns(lastUpdateSchema)
                    .readOnly()
                    .initialValue(std::vector<Hash>())
                    .commit();

            UINT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(60).minInc(1)
                    .commit();

            // Hide the loggers from the standard view in clients
            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue<int>(Schema::AccessLevel::ADMIN)
                    .commit();

            SLOT_ELEMENT(expected).key("flush")
                    .displayedName("Flush")
                    .description("Persist buffered data")
                    .allowedStates(State::ON)
                    .commit();
        }


        DeviceData::DeviceData(const karabo::util::Hash& input)
            : m_deviceToBeLogged(input.get<std::string>("deviceToBeLogged"))
            , m_initLevel(InitLevel::NONE)
            , m_strand(boost::make_shared<karabo::net::Strand>(karabo::net::EventLoop::getIOService()))
            , m_currentSchema()
            , m_user(".")
            , m_lastTimestampMutex()
            , m_lastDataTimestamp(Epochstamp(0ull, 0ull), Trainstamp())
            , m_updatedLastTimestamp(false)
            , m_pendingLogin(true)
            , m_onDataBeforeComplete(0u) {
        }


        DeviceData::~DeviceData() {
        }


        DataLogger::DataLogger(const Hash& input)
            : karabo::core::Device<>(input)
            , m_flushDeadline(karabo::net::EventLoop::getIOService()) {

            // start "flush" actor ...
            input.get("flushInterval", m_flushInterval); // in seconds

            // Register slots in constructor to ensure existence when sending instanceNew
            KARABO_SLOT(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
            KARABO_SLOT(slotSchemaUpdated, Schema /*changedSchema*/, string /*deviceId*/);
            KARABO_SLOT(slotAddDevicesToBeLogged, vector<string> /*deviceIds*/);
            KARABO_SLOT(slotTagDeviceToBeDiscontinued, string /*reason*/, string /*deviceId*/);
            KARABO_SLOT(flush);

            KARABO_INITIAL_FUNCTION(initialize)
        }


        DataLogger::~DataLogger() {
        }


        void DataLogger::preDestruction() {
            std::vector<std::string> devices;
            {
                boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
                for (auto it = m_perDeviceData.begin(), itEnd = m_perDeviceData.end(); it != itEnd; ++it) {
                    devices.push_back(it->first);
                }
            }
            for (auto it = devices.begin(); it != devices.end(); ++it) {
                try {
                    slotTagDeviceToBeDiscontinued("D", *it);
                } catch (const std::exception& e) {
                    // Just go on with other devices in case something is weird...
                    KARABO_LOG_FRAMEWORK_WARN << "Problem cleaning up for " << *it << ": " << e.what();
                }
            }
        }


        void DataLogger::initialize() {

            // Validate that devicesToBeLogged does not contain duplicates
            const auto devsToLog(get<std::vector < std::string >> ("devicesToBeLogged"));
            const std::set<std::string> tester(devsToLog.begin(), devsToLog.end());
            if (tester.size() < devsToLog.size()) {
                throw KARABO_INIT_EXCEPTION("Duplicated ids in configured devicesToBeLogged: " + toString(devsToLog));
            }
            // In the beginning, all are not yet logged (no mutex needed since no parallel action triggered yet):
            set("devicesNotLogged", devsToLog);

            // Create data structures, including directories
            for (const std::string& deviceId : devsToLog) {
                DeviceData::Pointer data = createDeviceData(Hash("deviceToBeLogged", deviceId));
                // Locking mutex not yet needed - no parallelism on content of m_perDeviceData yet.
                m_perDeviceData.insert(std::make_pair(deviceId, data));
            }
            // Schedule Logger specific initialization that may use asyc. logic ...
            initializeLoggerSpecific();
        }


        void DataLogger::initializeLoggerSpecific() {
            // This is default implementation.
            // Put here Logger specific initialization.
            // ...
            // and finally, ...
            startConnection();
        }


        void DataLogger::startConnection() {
            // Initiate connection to logged devices - will leave INIT state when all are connected (or failed)
            boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
            auto counter = boost::make_shared<std::atomic<unsigned int>>(m_perDeviceData.size());
            if (0 == *counter) {
                // No devices to log, so declare readiness immediately.
                if (this->get<karabo::util::State>("state") != State::ERROR) {
                    // We only go to ON state when there's no ERROR condition
                    // signaled by the DataLogger subclass. InfluxDataLogger, for
                    // instance, uses ERROR state to indicate issues with
                    // Influx server availability.
                    updateState(State::ON);
                } else {
                    KARABO_LOG_ERROR << "DataLogger '" << m_instanceId
                        << "' in ERROR state and cannot goto ON state. "
                        << "Current status is '"
                        << this->get<string>("status") << "'";
                }
            } else {
                for (DeviceDataMap::value_type& pair : m_perDeviceData) {
                    const DeviceData::Pointer& data = pair.second;
                    initConnection(data, counter);
                }
            }

            // Start the flushing
            m_flushDeadline.expires_from_now(boost::posix_time::seconds(m_flushInterval));
            m_flushDeadline.async_wait(util::bind_weak(&DataLogger::flushActor, this, boost::asio::placeholders::error));
        }


        void DataLogger::initConnection(const DeviceData::Pointer& data,
                                        const boost::shared_ptr<std::atomic<unsigned int> >& counter) {

            const std::string& deviceId = data->m_deviceToBeLogged;

            // Connect to schema updates and afterwards request Schema (in other order we might miss an update).
            data->m_initLevel = DeviceData::InitLevel::STARTED;
            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Connecting to " << deviceId << ".signalSchemaUpdated";
            asyncConnect(deviceId, "signalSchemaUpdated", "", "slotSchemaUpdated",
                         util::bind_weak(&DataLogger::handleSchemaConnected, this, data, counter),
                         util::bind_weak(&DataLogger::handleFailure, this, "connecting to schema for", data, counter));
            // Final steps until DataLogger is properly initialised for this device are treated in a chain of async handlers:
            // - If signalSchemaUpdated connected, request current schema;
            // - if that arrived, connect to both signal(State)Changed;
            // - if these two are connected, request initial configuration, start flushing and update state.
        }


        void DataLogger::handleFailure(const std::string& reason, const DeviceData::Pointer& data,
                                       const boost::shared_ptr<std::atomic<unsigned int> >& counter) {

            const std::string& deviceId = data->m_deviceToBeLogged;
            try {
                throw; // This will tell us which exception triggered the call to this error handler.
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_INFO << "Failed " << reason << " " << deviceId << ": " << e.what();
            }
            if (counter) checkReady(*counter);
            stopLogging(deviceId, true); // 'true' means retry later!
        }


        void DataLogger::handleSchemaConnected(const DeviceData::Pointer& data,
                                               const boost::shared_ptr<std::atomic<unsigned int> >& counter) {
            const std::string& deviceId = data->m_deviceToBeLogged;
            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Requesting slotGetSchema (receiveAsync) for " << deviceId;

            request(deviceId, "slotGetSchema", false)
                    .receiveAsync<karabo::util::Schema, std::string>
                    (util::bind_weak(&DataLogger::handleSchemaReceived, this, _1, _2, data, counter),
                     util::bind_weak(&DataLogger::handleFailure, this, "receiving schema from", data, counter));
        }


        void DataLogger::handleSchemaReceived(const karabo::util::Schema& schema, const std::string& deviceId,
                                              const DeviceData::Pointer& data,
                                              const boost::shared_ptr<std::atomic<unsigned int> >& counter) {
            // We need to store the received schema and then connect to configuration updates.
            // Since the first should not be done concurrently, we just post to the strand here,
            // adding the best timestamp we can get for this change - 'now' (better would be to receive the stamp
            // from the sender - possibly from the broker message header?):
            data->m_strand->post(util::bind_weak(&DataLogger::handleSchemaReceived2, this, schema, Timestamp(),
                                                 data, counter));
        }


        void DataLogger::handleSchemaReceived2(const karabo::util::Schema& schema, const karabo::util::Timestamp& stamp,
                                               const DeviceData::Pointer& data,
                                               const boost::shared_ptr<std::atomic<unsigned int> >& counter) {

            // Set initial Schema - needed for receiving properly in slotChanged
            data->handleSchemaUpdated(schema, stamp);

            // Now connect concurrently both, signalStateChanged and signalChanged, to the same slot.
            asyncConnect({SignalSlotConnection(data->m_deviceToBeLogged, "signalStateChanged", "", "slotChanged"),
                         SignalSlotConnection(data->m_deviceToBeLogged, "signalChanged", "", "slotChanged")},
                         util::bind_weak(&DataLogger::handleConfigConnected, this, data, counter),
                         util::bind_weak(&DataLogger::handleFailure, this, "connecting to configuration updates",
                                         data, counter));
        }


        void DataLogger::handleConfigConnected(const DeviceData::Pointer& data,
                                               const boost::shared_ptr<std::atomic<unsigned int> >& counter) {

            const std::string& deviceId = data->m_deviceToBeLogged;
            data->m_initLevel = DeviceData::InitLevel::CONNECTED;
            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Requesting " << deviceId << ".slotGetConfiguration (no wait)";
            requestNoWait(deviceId, "slotGetConfiguration", "", "slotChanged");

            if (counter) checkReady(*counter);
        }


        void DataLogger::checkReady(std::atomic<unsigned int>& counter) {
            // Update State once all configured device are connected.
            if (--counter == 0) {
                if (this->get<karabo::util::State>("state") != State::ERROR) {
                    // We only go to ON state when there's no ERROR condition
                    // signaled by the DataLogger subclass. InfluxDataLogger, for
                    // instance, uses ERROR state to indicate issues with
                    // InfluxServer availabilty.
                    updateState(State::ON);
                } else {
                    KARABO_LOG_ERROR << "DataLogger '" << m_instanceId
                        << "' in ERROR state and cannot goto ON state. "
                        << "Current status is '"
                        << this->get<string>("status") << "'";
                }
            }
        }


        bool DataLogger::stopLogging(const std::string& deviceId, bool retry) {

            auto disconnectCounter = boost::make_shared<std::atomic<int>>(3); // three signals to disconnect below
            auto genericHandler = bind_weak(&DataLogger::disconnectHandler, this, _1, deviceId, _2,
                                            retry, disconnectCounter);
            // Use the very long default timeout for the disconnection:
            // If we shall retry to connect, a long timeout gives a better chance that the other end is now capable to
            // accept connections (in the situation that this failed before due to overload).
            asyncDisconnect(deviceId, "signalSchemaUpdated", "", "slotSchemaUpdated",
                            boost::bind(genericHandler, false, "signalSchemaUpdated"), // successHandler for schema
                            boost::bind(genericHandler, true, "signalSchemaUpdated")); // failureHandler for schema
            asyncDisconnect(deviceId, "signalStateChanged", "", "slotChanged",
                            boost::bind(genericHandler, false, "signalStateChanged"), // successHandler for stateChanged
                            boost::bind(genericHandler, true, "signalStateChanged")); // failureHandler for...
            asyncDisconnect(deviceId, "signalChanged", "", "slotChanged",
                            boost::bind(genericHandler, false, "signalChanged"), // ... for changed
                            boost::bind(genericHandler, true, "signalChanged")); // ... for changed

            boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
            auto it = m_perDeviceData.find(deviceId);
            if (it == m_perDeviceData.end()) return false;
            it->second->stopLogging();
            m_perDeviceData.erase(it);

            return true;
        }


        void DataLogger::disconnectHandler(bool isFailure, const std::string& devId, const std::string& signal,
                                           bool retry, const boost::shared_ptr<std::atomic<int>>&counter) {
            if (isFailure) {
                try {
                    throw;
                } catch (const karabo::util::TimeoutException&) {
                    // Silence the expected timeout if stopLogging was called since device went offline
                    karabo::util::Exception::clearTrace();
                } catch (const std::exception& se) {
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to disconnect from " << devId << "." << signal
                            << ": " << se.what();
                }
            }

            // If retry requested, wait until all signals are disconnected (do not bother if that failed);
            if (retry && 0 >= --(*counter)) {
                const std::vector<std::string> devsToLog = get<std::vector < std::string >> ("devicesToBeLogged");
                if (std::find(devsToLog.begin(), devsToLog.end(), devId) == devsToLog.end()) {
                    return; // lost interest in this device
                }
                const std::vector<std::string> devsNotLogged = get<std::vector < std::string >> ("devicesNotLogged");
                if (std::find(devsNotLogged.begin(), devsNotLogged.end(), devId) == devsNotLogged.end()) {
                    // Maybe some other path was taken to connect, e.g. from outside while the attempt to connect
                    // "this time" ran into time out.
                    KARABO_LOG_FRAMEWORK_INFO << "Skip retry to connect to " << devId << " since connection OK.";
                    return;
                }
                KARABO_LOG_FRAMEWORK_INFO << "Retry to connect device " << devId;

                DeviceData::Pointer data = createDeviceData(Hash("deviceToBeLogged", devId));

                boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
                m_perDeviceData.insert(std::make_pair(devId, data));
                m_nonTreatedSlotChanged.erase(devId); // Just in case we received unwanted data before...

                // Init connection to device
                initConnection(data, boost::shared_ptr<std::atomic<unsigned int> >());
            }

        }


        void DataLogger::slotTagDeviceToBeDiscontinued(const std::string& reason, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Stop logging '" << deviceId
                    << "' requested since: " << reason;

            removeFrom(deviceId, "devicesToBeLogged");
            removeFrom(deviceId, "devicesNotLogged"); // just in case it was a problematic one

            // (Try to) Remove device from m_perDeviceData:
            if (!stopLogging(deviceId, false)) {
                // Inform caller about failure by exception
                throw KARABO_LOGIC_EXCEPTION("Device '" + deviceId + "' not treated.");
            }
        }


        void DataLogger::slotAddDevicesToBeLogged(const std::vector<std::string>& deviceIds) {
            // Collect devices that are requested, but which are already logged, to reply them
            std::vector<std::string> badIds;

            // Initiate logging for all of them
            for (const std::string& deviceId : deviceIds) {
                if (!appendTo(deviceId, "devicesToBeLogged")) {
                    badIds.push_back(deviceId);
                    continue;
                }
                // No need to check return value here - everything in 'devicesNotLogged' is also in 'devicesToBeLogged':
                appendTo(deviceId, "devicesNotLogged");

                // Create data structure ... depending on implementation
                DeviceData::Pointer data = createDeviceData(Hash("deviceToBeLogged", deviceId));
                boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
                m_perDeviceData.insert(std::make_pair(deviceId, data));
                m_nonTreatedSlotChanged.erase(deviceId);

                // Init connection to device,
                // using an empty pointer to counter since addition of logged devices at runtime shall not influence State.
                initConnection(data, boost::shared_ptr<std::atomic<unsigned int> >());
            }

            reply(badIds);
        }


        void DataLogger::slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId) {

            boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
            DeviceDataMap::iterator it = m_perDeviceData.find(deviceId);
            if (it != m_perDeviceData.end()) {
                DeviceData::Pointer& data = it->second;
                if (data->m_initLevel == DeviceData::InitLevel::COMPLETE) {
                    // normal case, nothing to do but just log
                } else if (data->m_initLevel == DeviceData::InitLevel::CONNECTED
                           && configuration.has("_deviceId_")) {
                    // configuration is the requested full configuration at the beginning
                    data->m_initLevel = DeviceData::InitLevel::COMPLETE;

                    // Update that now this device is logged (under lock to protect for parallel actions):
                    removeFrom(deviceId, "devicesNotLogged");
                    KARABO_LOG_FRAMEWORK_INFO << "Logging for " << deviceId << " established";
                    data->m_onDataBeforeComplete = 0ul; // reset in case - should not matter
                } else {
                    // connected, but requested full configuration not yet arrived - ignore these updates
                    // Log only 1st, 2nd, 3rd, ..., 9th, 10th, 20th, ..., 90th, 100th, 200th, ..., 900th, 1000th, 2000th, ...
                    //          and finally every millionth time:
                    const unsigned int numLogs = ++(data->m_onDataBeforeComplete);
                    unsigned int modulo = 1'000'000; // a million using C++14 digit separators
                    do {
                        if (numLogs % modulo == 0) {
                            const char* th = (numLogs > 3ul ? "th" : (numLogs == 3ul ? "rd" : (numLogs == 2ul ? "nd" : "st")));
                            KARABO_LOG_FRAMEWORK_INFO << "Ignore slotChanged for " << deviceId << " the " << numLogs
                                    << th << " time - not connected or initial full config not yet arrived";
                            break;
                        }
                        if (numLogs > modulo) break;
                    } while (modulo /= 10u); // integer division will end at zero
                    return;
                }
                // UserId only available in real slot call, before posting to event loop:
                const std::string& user = getSenderInfo("slotChanged")->getUserIdOfSender();
                // See DataLogger::slotSchemaUpdated for  a consideration about using boost::bind with 'data' instead
                // of bind_weak with bare pointer
                data->m_strand->post(karabo::util::bind_weak(&DeviceData::handleChanged, data.get(),
                                                             configuration, user));
            } else {
                // Throttled logging, see above.
                const unsigned int numLogs = ++m_nonTreatedSlotChanged[deviceId]; // prefix increment
                unsigned int modulo = 1'000'000;
                do {
                    if (numLogs % modulo == 0) {
                        const char* th = (numLogs > 3ul ? "th" : (numLogs == 3ul ? "rd" : (numLogs == 2ul ? "nd" : "st")));
                        KARABO_LOG_FRAMEWORK_WARN << "slotChanged called the " << numLogs << th << " time from non-treated device " << deviceId << ".";
                        break;
                    }
                    if (numLogs > modulo) break;
                } while (modulo /= 10u);
            }
        }


        bool DataLogger::removeFrom(const std::string& str, const std::string& vectorProp) {
            // lock mutex to avoid that another thread interferes in between get and set
            boost::mutex::scoped_lock lock(m_changeVectorPropMutex);
            std::vector<std::string> vec = get<std::vector < std::string >> (vectorProp);
            const auto it = std::find(vec.begin(), vec.end(), str);
            if (it != vec.end()) {
                vec.erase(it);
                set(vectorProp, vec);
                return true;
            } else {
                return false;
            }
        }


        bool DataLogger::appendTo(const std::string& str, const std::string& vectorProp) {
            // lock mutex to avoid that another thread interferes in between get and set
            boost::mutex::scoped_lock lock(m_changeVectorPropMutex);
            std::vector<std::string> vec = get<std::vector < std::string >> (vectorProp);
            const auto it = std::find(vec.begin(), vec.end(), str);
            if (it == vec.end()) {
                vec.push_back(str);
                set(vectorProp, vec);
                return true;
            } else {
                return false;
            }
        }


        void DeviceData::getPathsForConfiguration(const karabo::util::Hash& configuration,
                                                  const karabo::util::Schema& schema,
                                                  std::vector<std::string>& paths) const {
            {
                using karabo::util::Epochstamp;

                // Gets the paths for the leaf nodes in the configuration sorted by their order in the schema.
                getLeaves(configuration, schema, paths);

                // Sort the paths by ascending order of their corresponding nodes Epochstamps.
                std::sort(paths.begin(), paths.end(),
                          [&configuration](const std::string &firstPath, const std::string &secondPath) {
                              const Hash::Node &firstNode = configuration.getNode(firstPath);
                              const Hash::Node &secondNode = configuration.getNode(secondPath);
                              Epochstamp firstTime(0, 0);
                              Epochstamp secondTime(0, 0);
                              if (Epochstamp::hashAttributesContainTimeInformation(firstNode.getAttributes())) {
                                  firstTime = Epochstamp::fromHashAttributes(firstNode.getAttributes());
                              }
                              if (Epochstamp::hashAttributesContainTimeInformation(secondNode.getAttributes())) {
                                  secondTime = Epochstamp::fromHashAttributes(secondNode.getAttributes());
                              }
                              return (firstTime < secondTime);
                          });
            }
        }


        void DataLogger::flush() {
            // If the related asynchronous operation cannot be cancelled, the flush might already be running
            // To have full control when the flush is done (and reply that!), we have to try until it succeeds...
            // while (true) should be OK, but we cowardly try only for two seconds...
            int nTries = 2000;
            while (--nTries >= 0) {
                if (m_flushDeadline.cancel()) {
                    updateTableAndFlush(boost::make_shared<SignalSlotable::AsyncReply>(this));
                    m_flushDeadline.expires_from_now(boost::posix_time::seconds(m_flushInterval));
                    m_flushDeadline.async_wait(util::bind_weak(&DataLogger::flushActor, this, boost::asio::placeholders::error));
                    return;
                }
                boost::this_thread::sleep(boost::posix_time::milliseconds(1));
            }
            throw KARABO_TIMEOUT_EXCEPTION("Tried 2000 times to cancel flush timer...");
        }


        void DataLogger::flushActor(const boost::system::error_code& e) {
            if (e == boost::asio::error::operation_aborted) {
                return;
            }
            // Use empty reply pointer here: not inside slot, so no reply handling needed
            updateTableAndFlush(boost::shared_ptr<SignalSlotable::AsyncReply>());
            // arm timer again
            m_flushDeadline.expires_from_now(boost::posix_time::seconds(m_flushInterval));
            m_flushDeadline.async_wait(util::bind_weak(&DataLogger::flushActor, this, boost::asio::placeholders::error));
        }


        void DataLogger::updateTableAndFlush(const boost::shared_ptr<SignalSlotable::AsyncReply>& aReplyPtr) {

            // Update lastUpdatesUtc
            std::vector<Hash> lastStamps;
            bool updatedAnyStamp = false;
            {
                boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
                lastStamps.reserve(m_perDeviceData.size());
                for (auto& idData : m_perDeviceData) {
                    DeviceData::Pointer data = idData.second;
                    {
                        // To avoid this mutex lock, access to m_lastTimestampMutex would have to be posted on m_strand.
                        boost::mutex::scoped_lock lock(data->m_lastTimestampMutex);
                        updatedAnyStamp |= data->m_updatedLastTimestamp;
                        data->m_updatedLastTimestamp = false;
                        const karabo::util::Timestamp& ts = data->m_lastDataTimestamp;
                        Hash h("deviceId", idData.first);
                        // Human readable Epochstamp (except if no updates yet), attributes for machines
                        Hash::Node& node = h.set("lastUpdateUtc", "");
                        if (ts.getSeconds() != 0ull) {
                            node.setValue(ts.toFormattedString()); //"%Y%m%dT%H%M%S"));
                        }
                        ts.getEpochstamp().toHashAttributes(node.getAttributes());
                        lastStamps.push_back(std::move(h));
                    }
                }
            }

            if (updatedAnyStamp
                || (lastStamps.size() != get<std::vector < Hash >> ("lastUpdatesUtc").size())) {
                // If sizes are equal, but devices have changed, then at least one time stamp must have changed as well.
                set("lastUpdatesUtc", lastStamps);
            }

            // And flush
            flushImpl(aReplyPtr);
        }


        void DataLogger::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_INFO << "slotSchemaUpdated: Schema for " << deviceId << " arrived...";

            boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
            DeviceDataMap::iterator it = m_perDeviceData.find(deviceId);
            if (it != m_perDeviceData.end()) {
                Timestamp stamp; // Best time stamp we can get for this schema change (or take it from broker message header?)
                DeviceData::Pointer data = it->second;
                // Or boost::bind the shared pointer instead of bind_weak with this?
                // That would create a potentially dangerous cyclic reference: data has a strand that has a queue that
                // contains a handler that has a pointer to data until processed.
                // The advantage would be a kind of guarantee that our update will be processed,
                // even if data is removed from m_perDeviceData
                data->m_strand->post(karabo::util::bind_weak(&DeviceData::handleSchemaUpdated, data.get(), schema, stamp));
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "slotSchemaUpdated called from non-treated device " << deviceId << ".";
            }
        }
    }
}
