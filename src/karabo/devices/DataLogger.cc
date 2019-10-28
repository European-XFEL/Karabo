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
#include "karabo/util/Schema.hh"
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


        //KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLogger)

        void DataLogger::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::NORMAL)
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

            BOOL_ELEMENT(expected).key("useP2p")
                    .displayedName("Use p2p shortcut")
                    .description("Whether to try to use point-to-point instead of broker")
                    .assignmentOptional().defaultValue(false)
                    .commit();

            PATH_ELEMENT(expected).key("directory")
                    .displayedName("Directory")
                    .description("The directory where the log files should be placed")
                    .assignmentMandatory()
                    .commit();

            INT32_ELEMENT(expected).key("maximumFileSize")
                    .displayedName("Maximum file size")
                    .description("After any archived file has reached this size it will be time-stamped and not appended anymore")
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .assignmentMandatory()
                    .commit();

            Schema lastUpdateSchema;
            STRING_ELEMENT(lastUpdateSchema).key("deviceId")
                    .displayedName("Device")
                    .assignmentMandatory()
                    .commit();

            STRING_ELEMENT(lastUpdateSchema).key("lastUpdateUtc")
                    .displayedName("Last Update (UTC)")
                    .assignmentMandatory()
                    .commit();

            TABLE_ELEMENT(expected).key("lastUpdatesUtc")
                    .displayedName("Last Updates (UTC)")
                    .description("Timestamps of last recorded parameter update in UTC (updated in flush interval)")
                    .setColumns(lastUpdateSchema)
                    .assignmentOptional() // should be readOnly!
                    .defaultValue(std::vector<Hash>())
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

            // Slow beats
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();

            SLOT_ELEMENT(expected).key("flush")
                    .displayedName("Flush")
                    .description("Persist buffered data")
                    .allowedStates(State::NORMAL)
                    .commit();
        }


        DeviceData::DeviceData(const karabo::util::Hash& input)
            : m_initLevel(InitLevel::NONE)
            , m_strand(boost::make_shared<karabo::net::Strand>(karabo::net::EventLoop::getIOService())) {
            input.get("deviceToBeLogged", m_deviceToBeLogged);
            input.get("directory", m_directory);
        }


        DeviceData::~DeviceData() {
            if (m_initLevel != InitLevel::COMPLETE) {
                // We have not yet started logging this device, so nothing to mark about being done.
                return;
            }
        }


        DataLogger::DataLogger(const Hash& input)
            : karabo::core::Device<>(input)
            , m_useP2p(false)
            , m_flushDeadline(karabo::net::EventLoop::getIOService())
        {

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
            // When m_perDeviceData will be destructed, all the DeviceData destructors will run and tag
            // the remaining devices as discontinued.
            // Locking mutex maybe not needed since no parallelism anymore (?) - but cannot harm.
            boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
            for (auto it = m_perDeviceData.begin(), itEnd = m_perDeviceData.end(); it != itEnd; ++it) {
                disconnectP2P(it->first);
            }
            // Previously, here was an attempt to flush data to file - but that is not needed:
            // stream object destructors take care that their data arrives on disk.
        }


        void DataLogger::initialize() {
            m_useP2p = get<bool>("useP2p");

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
                DeviceData::Pointer data = createDeviceData(Hash("deviceToBeLogged", deviceId,
                                                                 "directory", get<string>("directory")));
                setupDirectory(data);
                // Locking mutex not yet needed - no parallelism on content of m_perDeviceData yet.
                m_perDeviceData.insert(std::make_pair(deviceId, data));
            }

            // Initiate connection to logged devices - will leave INIT state when all are connected (or failed)
            boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
            auto counter = boost::make_shared<std::atomic<unsigned int>>(m_perDeviceData.size());
            if (0 == *counter) {
                // No devices to log, so declare readiness immediately
                updateState(State::NORMAL);
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
            // First try to establish p2p before connecting signals - i.e. don't to spam the broker with signalChanged.
            if (m_useP2p) {
                // copy to avoid capture of bare 'this'
                auto successHandler = [deviceId] () {
                    KARABO_LOG_FRAMEWORK_INFO << "Going to establish p2p to '" << deviceId << "'";
                };
                auto failureHandler = [deviceId] () {
                    try {
                        throw;
                    } catch (const std::exception& e) {
                        // As of now (2019-06-24), this is expected for middlelayer...
                        KARABO_LOG_FRAMEWORK_WARN << "Cannot establish p2p to '" << deviceId << "' since:\n"
                                << e.what();
                    }
                };
                asyncConnectP2p(deviceId, successHandler, failureHandler);
            }

            // Then connect to schema updates and afterwards request Schema (in other order we might miss an update).
            data->m_initLevel = DeviceData::InitLevel::STARTED;
            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Connecting to " << deviceId << ".slotSchemaUpdated";
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
            stopLogging(deviceId);
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
            // Since the first should not be done concurrently, we just post to the strand here:
            data->m_strand->post(util::bind_weak(&DataLogger::handleSchemaReceived2, this, schema, data, counter));
        }


        void DataLogger::handleSchemaReceived2(const karabo::util::Schema& schema, const DeviceData::Pointer& data,
                                               const boost::shared_ptr<std::atomic<unsigned int> >& counter) {

            // Set initial Schema - needed for receiving properly in slotChanged
            handleSchemaUpdated(schema, data);

            // Now connect concurrently both, signalStateChanged and signalChanged, to the same slot.
            asyncConnect({SignalSlotConnection(data->m_deviceToBeLogged, "signalStateChanged", "", "slotChanged"),
                         SignalSlotConnection(data->m_deviceToBeLogged, "signalChanged", "", "slotChanged")},
                         util::bind_weak(&DataLogger::handleConfigConnected, this, data, counter),
                         util::bind_weak(&DataLogger::handleFailure, this, "receiving configuration from", data, counter));
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
            // Update State once all configured device are connected
            if (--counter == 0) {
                updateState(State::NORMAL);
            }
        }


        bool DataLogger::stopLogging(const std::string& deviceId) {

            // Avoid automatic reconnects -
            // if not all signals connected or device is already dead, this triggers some (delayed) WARNings:
            asyncDisconnect(deviceId, "signalSchemaUpdated", "", "slotSchemaUpdated");
            asyncDisconnect(deviceId, "signalStateChanged", "", "slotChanged");
            asyncDisconnect(deviceId, "signalChanged", "", "slotChanged");

            boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
            const bool result = (m_perDeviceData.erase(deviceId) > 0);
            if (m_useP2p && result) {
                disconnectP2P(deviceId);
            }
            return result;
        }


        void DataLogger::slotTagDeviceToBeDiscontinued(const std::string& reason, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Stop logging '" << deviceId
                                      << "' requested since: " << reason;

            removeFrom(deviceId, "devicesToBeLogged");
            removeFrom(deviceId, "devicesNotLogged"); // just in case it was a problematic one

            // (Try to) Remove device from m_perDeviceData:
            if (!stopLogging(deviceId)) {
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

                // Create data structure and setup directory
                DeviceData::Pointer data = createDeviceData(Hash("deviceToBeLogged", deviceId,
                                                                 "directory", get<string>("directory")));
                setupDirectory(data);
                boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
                m_perDeviceData.insert(std::make_pair(deviceId, data));

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
                } else {
                    // connected, but requested full configuration not yet arrived - ignore these updates
                    KARABO_LOG_FRAMEWORK_INFO << "Ignore slotChanged for " << deviceId
                            << " - not connected or initial full config not yet arrived:\n" << configuration;
                    return;
                }
                // UserId only available in real slot call, before posting to event loop:
                const std::string& user = getSenderInfo("slotChanged")->getUserIdOfSender();
                data->m_strand->post(karabo::util::bind_weak(&DataLogger::handleChanged, this,
                                                             configuration, user, data));
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "slotChanged called from non-treated device " << deviceId << ".";
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


        void DataLogger::getPathsForConfiguration(const karabo::util::Hash& configuration,
                                                  const karabo::util::Schema& schema,
                                                  std::vector<std::string>& paths) const {
            {
                using karabo::util::Epochstamp;

                // Gets the paths for the leaf nodes in the configuration sorted by their order in the schema.
                getLeaves(configuration, schema, paths);

                // Sort the paths by ascending order of their corresponding nodes Epochstamps.
                std::sort(paths.begin(), paths.end(),
                          [&configuration](const std::string& firstPath, const std::string & secondPath) {
                              const Hash::Node& firstNode = configuration.getNode(firstPath);
                          const Hash::Node& secondNode = configuration.getNode(secondPath);
                          Epochstamp firstTime(0, 0);
                          Epochstamp secondTime(0, 0);
                          if (Epochstamp::hashAttributesContainTimeInformation(firstNode.getAttributes())) {
                          firstTime = Epochstamp::fromHashAttributes(firstNode.getAttributes()).toTimestamp();
                              }
                          if (Epochstamp::hashAttributesContainTimeInformation(secondNode.getAttributes())) {
                          secondTime = Epochstamp::fromHashAttributes(secondNode.getAttributes()).toTimestamp();
                              }
                          return (firstTime < secondTime);
                          });
            }
        }


        void DataLogger::flush() {
            // If the related asynchronous operation cannot be cancelled, the flush might already be running
            if (m_flushDeadline.cancel()) {
                doFlush();
                m_flushDeadline.expires_from_now(boost::posix_time::seconds(m_flushInterval));
                m_flushDeadline.async_wait(util::bind_weak(&DataLogger::flushActor, this, boost::asio::placeholders::error));
            }
        }


        void DataLogger::flushActor(const boost::system::error_code& e) {
            if (e == boost::asio::error::operation_aborted) {
                return;
            }
            doFlush();
            // arm timer again
            m_flushDeadline.expires_from_now(boost::posix_time::seconds(m_flushInterval));
            m_flushDeadline.async_wait(util::bind_weak(&DataLogger::flushActor, this, boost::asio::placeholders::error));
        }


        void DataLogger::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotSchemaUpdated: Schema for " << deviceId << " arrived...";

            boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
            DeviceDataMap::iterator it = m_perDeviceData.find(deviceId);
            if (it != m_perDeviceData.end()) {
                DeviceData::Pointer& data = it->second;
                data->m_strand->post(karabo::util::bind_weak(&DataLogger::handleSchemaUpdated, this, schema, data));
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "slotSchemaUpdated called from non-treated device " << deviceId << ".";
            }
        }
    }
}
