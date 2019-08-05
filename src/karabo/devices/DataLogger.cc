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


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLogger)

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

            // Do not archive the archivers (would lead to infinite recursion)
            OVERWRITE_ELEMENT(expected).key("archive")
                    .setNewDefaultValue(false)
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


        struct DeviceData {

            KARABO_CLASSINFO(DeviceData, "DataLoggerDeviceData", "2.6")

            enum class InitLevel {

                NONE = 0, /// DeviceData is created
                STARTED, /// connecting to device's signals has started
                CONNECTED, /// all connections are established (and first Schema received in between)
                COMPLETE /// the initial configuration has arrived
            };
            DeviceData(const std::string& deviceId, const std::string& directory);

            ~DeviceData();

            std::string m_deviceToBeLogged; // same as this DeviceData's key in DeviceDataMap

            const std::string m_directory;

            InitLevel m_initLevel;

            karabo::net::Strand::Pointer m_strand;

            karabo::util::Schema m_currentSchema;

            std::fstream m_configStream;

            unsigned int m_lastIndex;
            std::string m_user;
            boost::mutex m_lastTimestampMutex;
            karabo::util::Timestamp m_lastDataTimestamp;
            bool m_updatedLastTimestamp;
            bool m_pendingLogin;

            std::map<std::string, karabo::util::MetaData::Pointer> m_idxMap;
            std::vector<std::string> m_idxprops;
            size_t m_propsize;
            time_t m_lasttime;

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_serializer;
        };

        DeviceData::DeviceData(const std::string& deviceId, const std::string& directory)
            : m_deviceToBeLogged(deviceId)
            , m_directory(directory)
            , m_initLevel(InitLevel::NONE)
            , m_strand(boost::make_shared<karabo::net::Strand>(karabo::net::EventLoop::getIOService()))
            , m_currentSchema()
            , m_configStream()
            , m_lastIndex(0u)
            , m_user(".") //TODO:  Define proper user running a device. The dot is unknown user?
            , m_lastTimestampMutex()
            , m_lastDataTimestamp(Epochstamp(0ull, 0ull), Trainstamp())
            , m_updatedLastTimestamp(false)
            , m_pendingLogin(true)
            , m_idxMap()
            , m_idxprops()
            , m_propsize(0u)
            , m_lasttime(0)
            , m_serializer(TextSerializer<Hash>::create(Hash("Xml.indentation", -1))) {
        }


        DeviceData::~DeviceData() {
            if (m_initLevel != InitLevel::COMPLETE) {
                // We have not yet started logging this device, so nothing to mark about being done.
                return;
            }

            const std::string& deviceId = m_deviceToBeLogged;
            // Mark as logger stopped.
            // Although this destructor is not running on the strand, accessing all members is safe:
            // All other actions touching the members are posted on the strand and have a shared  pointer
            // to the DeviceData - so this destructor can only run when all these actions are done.
            try {
                if (m_configStream.is_open()) {
                    boost::mutex::scoped_lock lock(m_lastTimestampMutex);
                    karabo::util::Timestamp& lastTs = m_lastDataTimestamp;
                    m_configStream << lastTs.toIso8601Ext() << "|" << fixed << lastTs.toTimestamp()
                            << "|" << lastTs.getTrainId() << "|.|||" << m_user << "|LOGOUT\n";
                    m_configStream.flush();
                    std::ostream::pos_type position = m_configStream.tellp();
                    m_configStream.close();
                    if (position >= 0) {
                        string contentPath = m_directory + "/" + deviceId + "/raw/archive_index.txt";
                        ofstream contentStream(contentPath.c_str(), ios::app);
                        contentStream << "-LOG " << lastTs.toIso8601Ext() << " " << fixed << lastTs.toTimestamp()
                                << " " << lastTs.getTrainId() << " " << position << " "
                                << (m_user.empty() ? "." : m_user) << " " << m_lastIndex << "\n";
                        contentStream.close();
                    } else {
                        KARABO_LOG_FRAMEWORK_ERROR << "Error retrieving position of LOGOUT entry in archive with index '"
                                << m_lastIndex << "': skipped writing index entry for " << deviceId;
                    }

                    for (map<string, MetaData::Pointer>::iterator it = m_idxMap.begin(); it != m_idxMap.end(); it++) {
                        MetaData::Pointer mdp = it->second;
                        if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
                    }
                    m_idxMap.clear();
                }
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_LOGIC_EXCEPTION("Problems tagging " + deviceId + " to be discontinued"));
            }
        }


        DataLogger::DataLogger(const Hash& input)
            : karabo::core::Device<>(input)
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
                DeviceDataPointer data = boost::make_shared<DeviceData>(deviceId, get<string>("directory"));
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
                    const DeviceDataPointer& data = pair.second;
                    initConnection(data, counter);
                }
            }

            // Start the flushing
            m_flushDeadline.expires_from_now(boost::posix_time::seconds(m_flushInterval));
            m_flushDeadline.async_wait(util::bind_weak(&DataLogger::flushActor, this, boost::asio::placeholders::error));
        }


        void DataLogger::setupDirectory(const DeviceDataPointer& data) const {

            boost::system::error_code ec;
            if (!boost::filesystem::exists(get<string>("directory") + "/" + data->m_deviceToBeLogged)) {
                boost::filesystem::create_directories(get<string>("directory") + "/" + data->m_deviceToBeLogged, ec);
                if (ec) {
                    const std::string msg("Failed to create directories : " + data->m_deviceToBeLogged + ". code = "
                                          + toString(ec.value()) += " -- " + ec.message());
                    KARABO_LOG_FRAMEWORK_ERROR << msg;
                    throw KARABO_INIT_EXCEPTION(msg);
                }
            }
            if (!boost::filesystem::exists(get<string>("directory") + "/" + data->m_deviceToBeLogged + "/raw")) {
                boost::filesystem::create_directory(get<string>("directory") + "/" + data->m_deviceToBeLogged + "/raw");
            }
            if (!boost::filesystem::exists(get<string>("directory") + "/" + data->m_deviceToBeLogged + "/idx")) {
                boost::filesystem::create_directory(get<string>("directory") + "/" + data->m_deviceToBeLogged + "/idx");
            }

            data->m_lastIndex = determineLastIndex(data->m_deviceToBeLogged);
        }


        void DataLogger::initConnection(const DeviceDataPointer& data,
                                        const boost::shared_ptr<std::atomic<unsigned int> >& counter) {

            const std::string& deviceId = data->m_deviceToBeLogged;
            // First try to establish p2p before connecting signals - i.e. don't to spam the broker with signalChanged.
            if (std::getenv("KARABO_DISABLE_LOGGER_P2P") == NULL) {
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
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Data logging via p2p has been disabled for loggers!";
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


        void DataLogger::handleFailure(const std::string& reason, const DeviceDataPointer& data,
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


        void DataLogger::handleSchemaConnected(const DeviceDataPointer& data,
                                               const boost::shared_ptr<std::atomic<unsigned int> >& counter) {
            const std::string& deviceId = data->m_deviceToBeLogged;
            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Requesting slotGetSchema (receiveAsync) for " << deviceId;

            request(deviceId, "slotGetSchema", false)
                    .receiveAsync<karabo::util::Schema, std::string>
                    (util::bind_weak(&DataLogger::handleSchemaReceived, this, _1, _2, data, counter),
                     util::bind_weak(&DataLogger::handleFailure, this, "receiving schema from", data, counter));
        }


        void DataLogger::handleSchemaReceived(const karabo::util::Schema& schema, const std::string& deviceId,
                                              const DeviceDataPointer& data,
                                              const boost::shared_ptr<std::atomic<unsigned int> >& counter) {
            // We need to store the received schema and then connect to configuration updates.
            // Since the first should not be done concurrently, we just post to the strand here:
            data->m_strand->post(util::bind_weak(&DataLogger::handleSchemaReceived2, this, schema, data, counter));
        }


        void DataLogger::handleSchemaReceived2(const karabo::util::Schema& schema, const DeviceDataPointer& data,
                                               const boost::shared_ptr<std::atomic<unsigned int> >& counter) {

            // Set initial Schema - needed for receiving properly in slotChanged
            handleSchemaUpdated(schema, data);

            // Now connect concurrently both, signalStateChanged and signalChanged, to the same slot.
            asyncConnect({SignalSlotConnection(data->m_deviceToBeLogged, "signalStateChanged", "", "slotChanged"),
                         SignalSlotConnection(data->m_deviceToBeLogged, "signalChanged", "", "slotChanged")},
                         util::bind_weak(&DataLogger::handleConfigConnected, this, data, counter),
                         util::bind_weak(&DataLogger::handleFailure, this, "receiving configuration from", data, counter));
        }


        void DataLogger::handleConfigConnected(const DeviceDataPointer& data,
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
            if (result) {
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
                DeviceDataPointer data = boost::make_shared<DeviceData>(deviceId, get<string>("directory"));
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
                DeviceDataPointer& data = it->second;
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
                    KARABO_LOG_FRAMEWORK_DEBUG << "Ignore slotChanged for " << deviceId
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


        void DataLogger::handleChanged(const karabo::util::Hash& configuration, const std::string& user,
                                       const DeviceDataPointer& data) {

            data->m_user = user; // set under m_strand protection
            const std::string& deviceId = data->m_deviceToBeLogged;

            const bool newPropToIndex = this->updatePropsToIndex(*data);

            // TODO: Define these variables: each of them uses 24 bits
            int expNum = 0x0F0A1A2A;
            int runNum = 0x0F0B1B2B;

            // To write log I need schema - but that has arrived before connecting signal[State]Changed to slotChanged
            // and thus before any data can arrive here in handleChanged.
            vector<string> paths;
            getPathsForConfiguration(configuration, data->m_currentSchema, paths);

            if (newPropToIndex) {
                // DataLogReader got request for history of a property not indexed
                // so far, that means it triggered the creation of an index file
                // for that property. Since we cannot be sure that the index creation
                // has finished when we want to add a new index entry, we close the
                // file and thus won't touch this new index file (but start a new one).
                this->ensureFileClosed(*data);
            }

            for (size_t i = 0; i < paths.size(); ++i) {
                const string& path = paths[i];
                
                // Skip those elements which should not be archived
                if (!data->m_currentSchema.has(path)
                    || (data->m_currentSchema.hasArchivePolicy(path)
                        && (data->m_currentSchema.getArchivePolicy(path) == Schema::NO_ARCHIVING))) {
                    continue;
                }

                const Hash::Node& leafNode = configuration.getNode(path);

                // Check for timestamp ...
                if (!Timestamp::hashAttributesContainTimeInformation(leafNode.getAttributes())) {
                    KARABO_LOG_WARN << "Skip '" << path << "' of '" << deviceId
                            << "' - it lacks time information attributes.";
                    continue;
                }

                Timestamp t = Timestamp::fromHashAttributes(leafNode.getAttributes());
                {
                    // Update time stamp for DeviceData destructor and property "lastUpdatesUtc".
                    // Since the latter is accessing it when not posted on data->m_strand, need mutex protection:
                    boost::mutex::scoped_lock lock(data->m_lastTimestampMutex);
                    if (t.getEpochstamp() > data->m_lastDataTimestamp.getEpochstamp()) {
                        // If mixed timestamps in single message (or arrival in wrong order), always take most recent one.
                        data->m_updatedLastTimestamp = true;
                        data->m_lastDataTimestamp = t;
                    }
                }
                string value = "";   // "value" should be a string, so convert depending on type ...
                if (leafNode.getType() == Types::VECTOR_HASH) {
                    // Represent any vector<Hash> as XML string ...
                    data->m_serializer->save(leafNode.getValue<vector < Hash >> (), value);
                } else if (Types::isVector(leafNode.getType())) {
                    // ... and any other vector as a comma separated text string of vector elements
                    value = toString(leafNode.getValueAs<string,vector>());
                } else {
                    value = leafNode.getValueAs<string>();
                }

                bool newFile = false;
                if (!data->m_configStream.is_open()) {
                    string configName = get<string>("directory") + "/" + deviceId + "/raw/archive_" + toString(data->m_lastIndex) + ".txt";
                    data->m_configStream.open(configName.c_str(), ios::out | ios::app);
                    if (!data->m_configStream.is_open()) {
                        KARABO_LOG_ERROR << "Failed to open \"" << configName << "\". Check permissions.";
                        return;
                    }
                    if (data->m_configStream.tellp() > 0) {
                        // Make sure that the file contains '\n' (newline) at the end of previous round
                        data->m_configStream << '\n';
                    } else
                        newFile = true;
                }

                size_t position = data->m_configStream.tellp(); // get current file size
                const string type = Types::to<ToLiteral>(leafNode.getType());
                data->m_configStream << t.toIso8601Ext() << "|" << fixed << t.toTimestamp() << "|"
                        << t.getTrainId() << "|" << path << "|" << type << "|" << scientific
                        << value << "|" << data->m_user;
                if (data->m_pendingLogin) data->m_configStream << "|LOGIN\n";
                else data->m_configStream << "|VALID\n";

                if (data->m_pendingLogin || newFile) {
                    string contentPath = get<string>("directory") + "/" + deviceId + "/raw/archive_index.txt";
                    ofstream contentStream(contentPath.c_str(), ios::app);
                    if (data->m_pendingLogin) {
                        contentStream << "+LOG ";
                    } else {
                        contentStream << "=NEW ";
                    }

                    contentStream << t.toIso8601Ext() << " " << fixed << t.toTimestamp() << " "
                            << t.getTrainId() << " " << position << " " << (data->m_user.empty() ? "." : data->m_user) << " " << data->m_lastIndex << "\n";
                    contentStream.close();

                    data->m_pendingLogin = false;
                }

                // check if we have property registered
                if (find(data->m_idxprops.begin(), data->m_idxprops.end(), path) == data->m_idxprops.end()) continue;

                MetaData::Pointer& mdp = data->m_idxMap[path]; //Pointer by reference!
                bool first = false;
                if (!mdp) {
                    // a property not yet indexed - create meta data and set file
                    mdp = MetaData::Pointer(new MetaData);
                    mdp->idxFile = get<string>("directory") + "/" + deviceId + "/idx/archive_" + toString(data->m_lastIndex)
                            + "-" + path + "-index.bin";
                    first = true;
                }
                if (!mdp->idxStream.is_open()) {
                    mdp->idxStream.open(mdp->idxFile.c_str(), ios::out | ios::app | ios::binary);
                }
                mdp->record.epochstamp = t.toTimestamp();
                mdp->record.trainId = t.getTrainId();
                mdp->record.positionInRaw = position;
                mdp->record.extent1 = (expNum & 0xFFFFFF);
                mdp->record.extent2 = (runNum & 0xFFFFFF);
                if (first) {
                    mdp->record.extent2 |= (1 << 30);
                }
                mdp->idxStream.write((char*) &mdp->record, sizeof (MetaData::Record));
            }

            long maxFilesize = get<int>("maximumFileSize") * 1000000; // times to 1000000 because maximumFilesSize in MBytes
            long position = data->m_configStream.tellp();
            if (maxFilesize <= position) {
                this->ensureFileClosed(*data);
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


        bool DataLogger::updatePropsToIndex(DeviceData& data) {
            const std::string& deviceId = data.m_deviceToBeLogged;
            boost::filesystem::path propPath(get<string>("directory") + "/" + deviceId + "/raw/properties_with_index.txt");
            if (boost::filesystem::exists(propPath)) {
                const size_t propsize = boost::filesystem::file_size(propPath);
                const time_t lasttime = boost::filesystem::last_write_time(propPath);
                // read prop file only if it was changed
                if (data.m_propsize != propsize || data.m_lasttime != lasttime) {
                    data.m_propsize = propsize;
                    data.m_lasttime = lasttime;
                    // re-read prop file
                    ifstream in(propPath.c_str());
                    string content(propsize, ' ');
                    content.assign((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
                    in.close();
                    data.m_idxprops.clear();
                    boost::split(data.m_idxprops, content, boost::is_any_of("\n"));
                    // Could do more clever gymnastics to check whether content of
                    // data.m_idxprops now really differs from old content...
                    return true;
                }
            }
            return false;
        }


        void DataLogger::ensureFileClosed(DeviceData& data) {
            const std::string& deviceId = data.m_deviceToBeLogged;
            if (data.m_configStream.is_open()) {
                // increment index number for configuration file
                data.m_lastIndex = this->incrementLastIndex(deviceId);
                data.m_configStream.close();
            }

            for (std::map<std::string, MetaData::Pointer>::iterator it = data.m_idxMap.begin(), endIt = data.m_idxMap.end();
                 it != endIt; ++it) {
                MetaData::Pointer mdp = it->second;
                if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
            }
            data.m_idxMap.clear();
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


        void DataLogger::doFlush() {

            // Flush all files, but also hijack this actor to update lastUpdatesUtc
            std::vector<Hash> lastStamps;
            bool updatedAnyStamp = false;
            {
                boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
                lastStamps.reserve(m_perDeviceData.size());
                for (auto& idData : m_perDeviceData) {
                    DeviceDataPointer& data = idData.second;
                    {
                        // To avoid this mutex lock, access to m_lastTimestampMutex would have to be posted on m_strand.
                        // Keep as is since this file based DataLogger is supposed to phase out soon...
                        boost::mutex::scoped_lock lock(data->m_lastTimestampMutex);
                        updatedAnyStamp |= data->m_updatedLastTimestamp;
                        data->m_updatedLastTimestamp = false;
                        const karabo::util::Timestamp& ts = data->m_lastDataTimestamp;
                        lastStamps.push_back(Hash("deviceId", idData.first,
                                                  "lastUpdateUtc", ts.getSeconds() == 0ull ? "" : ts.toFormattedString()));
                    }

                    // We post on strand to exclude parallel access to data->m_configStream and data->m_idxMap
                    data->m_strand->post(karabo::util::bind_weak(&DataLogger::flushOne, this, data));
                }
            }

            if (updatedAnyStamp
                || (lastStamps.empty() && !get<std::vector < Hash >> ("lastUpdatesUtc").empty())) {
                set("lastUpdatesUtc", lastStamps);
            }
        }


        void DataLogger::flushOne(const DeviceDataPointer& data) {
            if (data->m_configStream.is_open()) {
                data->m_configStream.flush();
            }
            for (std::map<string, MetaData::Pointer>::iterator it = data->m_idxMap.begin(); it != data->m_idxMap.end(); ++it) {
                MetaData::Pointer mdp = it->second;
                if (mdp && mdp->idxStream.is_open()) mdp->idxStream.flush();
            }
        }


        void DataLogger::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotSchemaUpdated: Schema for " << deviceId << " arrived...";

            boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
            DeviceDataMap::iterator it = m_perDeviceData.find(deviceId);
            if (it != m_perDeviceData.end()) {
                DeviceDataPointer& data = it->second;
                data->m_strand->post(karabo::util::bind_weak(&DataLogger::handleSchemaUpdated, this, schema, data));
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "slotSchemaUpdated called from non-treated device " << deviceId << ".";
            }
        }


        void DataLogger::handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceDataPointer& data) {

            const std::string& deviceId = data->m_deviceToBeLogged;

            data->m_currentSchema = schema;

            string filename = get<string>("directory") + "/" + deviceId + "/raw/archive_schema.txt";
            fstream fileout(filename.c_str(), ios::out | ios::app);
            if (fileout.is_open()) {
                Timestamp t;
                // Since schema updates are rare, do not store this serialiser as the one for Hash (data->m_serializer):
                TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create(Hash("Xml.indentation", -1));
                string archive;
                serializer->save(schema, archive);
                fileout << t.getSeconds() << " " << t.getFractionalSeconds() << " " << t.getTrainId() << " " << archive << "\n";
                fileout.close();
            } else
                throw KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
        }


        int DataLogger::determineLastIndex(const std::string& deviceId) const {
            string lastIndexFilename = get<string>("directory") + "/" + deviceId + "/raw/archive.last";
            int idx;
            fstream fs;
            if (!boost::filesystem::exists(lastIndexFilename)) {
                for (size_t i = 0;; i++) {
                    string filename = get<string>("directory") + "/" + deviceId + "/raw/archive_" + toString(i) + ".txt";
                    if (!boost::filesystem::exists(filename)) {
                        idx = i;
                        break;
                    }
                }
                fs.open(lastIndexFilename.c_str(), ios::out | ios::app);
                fs << idx << "\n";
            } else {
                fs.open(lastIndexFilename.c_str(), ios::in);
                fs >> idx;
            }
            fs.close();
            return idx;
        }


        int DataLogger::incrementLastIndex(const std::string& deviceId) {
            string lastIndexFilename = get<string>("directory") + "/" + deviceId + "/raw/archive.last";
            int idx;
            if (!boost::filesystem::exists(lastIndexFilename)) {
                idx = determineLastIndex(deviceId);
            }
            fstream file(lastIndexFilename.c_str(), ios::in | ios::out);
            file >> idx;
            if (file.fail()) file.clear();
            ++idx;
            file.seekg(0);
            file << idx << "\n";
            file.close();
            return idx;
        }


    }
}
