/**
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 *
 * The main logic of the managing of the data loggers and of the devices that they should log is as follows:
 * * Register handlers for new and gone instances before topology gathering starts.
 *   o By that we get rid of an initial treatment of the topology and of everything that comes afterwards. With
 *      multithreading it is difficult to get a clear cut between these two states.
 *   o Now we rely on the fact that the instanceNew handler is called for all instances discovered, be it in the
 *     initial topology gathering or later.
 * * We track a state for each logger server - it consist of
 *   o a LoggerState, i.e. whether the server is still OFFLINE, or whether the server was discovered and therefore
 *     we are INSTANTIATING the logger or whether the logger is RUNNING.
 *   o a "backlog" of all devices that should be logged by this server,
 *   o the devices "beingAdded", i.e. for which we told the logger to take care, but did not yet get confirmation,
 *   o and those "devices" that the logger has acknowledged to take care of.
 * * All actions that work on this state are sequentialised by using a strand to be sure about the state.
 * * Once a server is discovered, the logger is instantiated.
 * * Once a logger is discovered, it is told to log all devices that are found to be logged by it and have been
 *   discovered before
 * * Once a device to be logged is discovered (this could even be a logger), it is checked which logger server is
 *   responsible
 *   o either the logger is told to log this device
 *   o or, if not yet ready, the device is added to the "backlog".
 * * If a device to be logged is gone, its logger is informed to stop logging
 * * If a logger goes down, it is restarted (well, we try - if also the server is down, restart will fail)
 *   before all logged "devices" and those "beingAdded" are put into "backlog".
 * * If a logger server goes down, "devices" and those "beingAdded" are put into "backlog" as well.
 *   o There is currently (2.6.0) no defined order of device gone and server gone - it depends whether a server went
 *     down cleanly or its death was discovered by lack of heartbeats (to be fixed in the DeviceClient).
 * * The case that
 *   o the loggers and the manager work fine,
 *   o but then the manager is shutdown
 *   o and a logged device stops afterwards,
 *   o and then the manager starts again
 *  is handled within the regular checks described in the following.
 *
 * Besides taking care to instruct loggers to log devices, a regular sanity check based on Epochstamps is done:
 * * Every "topologyCheck.interval" minutes, each DataLogger that is running is checked.
 * * This check accesses the "lastUpdatesUtc" tables of all devices:
 *   o If there is an empty time stamp, that should mean that logging of the device in question has just started. The
 *     check procedure is continued with the other devices, but id of the device is noted and in case that during
 *     the next check there is still no timestamp, stop and start of logging that device is enforced.
 *   o Otherwise each timestamp is compared with 'now'.
 * * If the time difference is smaller than "topologyCheck.toleranceLogged", the logging of the device is considered
 *   to be OK.
 * * Otherwise the configuration of the device in question is queried:
 *   o If the time stamp in "lastUpdatesUtc" is not more than "topologyCheck.toleranceDiff" behind the most recent
 *     timestamp of the requested device configuration, the logging is considered OK.
 *   o If it is more behind, this is considered to be a problem and stop and start of logging is enforced.
 *   o If the query for the device configuration fails, the internal book keeping is cross checked whether the device
 *     is really supposed to be logged. If not, the logger is called to stop logging this device. (This situation
 *     can appear if the manager was down when the device went down so the manager could not inform the logger.)
 * * When treatment of all loggers and their devices is finished, a summary of the findings are logged and published
 *   as "topologyCheck.lastCheckResult" and the procedure is triggered again in "topologyCheck.interval" minutes.
 */

#include <vector>
#include <set>
#include <unordered_set>
#include <string>
#include <algorithm>    // std::find, std::max
#include <string.h>     // strlen

#include <boost/asio/deadline_timer.hpp>

#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/StringTools.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/net/EventLoop.hh"

#include "DataLoggerManager.hh"


namespace karabo {
    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using karabo::xms::SLOT_ELEMENT;


        /**
         * Helper function used below:
         * Add object to set<T> at key's position in h - if no such key exists, create one
         */
        template<class T>
        void addToSetOrCreate(Hash& h, const std::string& key, const T& object) {
            if (h.has(key)) {
                h.get<std::set<T> >(key).insert(object);
            } else {
                h.set(key, std::set<T>({object}));
            }
        }


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLoggerManager)


        void DataLoggerManager::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::NORMAL, State::MONITORING)
                    .setNewDefaultValue(State::INIT)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("performanceStatistics.enable")
                    .setNewDefaultValue(true)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue<int>(Schema::AccessLevel::ADMIN)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_DataLoggerManager_0")
                    .commit();

            INT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(40).minInc(1)
                    .reconfigurable()
                    .commit();

            BOOL_ELEMENT(expected).key("enablePerformanceStats")
                    .displayedName("Performance stats on/off")
                    .description("Value of 'performanceStatistics.enable' used when instantiating loggers")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(true) // true will cause alarms when loggers are too slow
                    .commit();

            CHOICE_ELEMENT(expected).key("logger")
                    .displayedName("Logger type")
                    .assignmentOptional().defaultValue("FileDataLogger")
                    .commit();

            NODE_ELEMENT(expected).key("logger.FileDataLogger")
                    .displayedName("FileDataLogger")
                    .description("File based data logging")
                    .commit();

            PATH_ELEMENT(expected).key("logger.FileDataLogger.directory")
                    .displayedName("Directory")
                    .description("The directory where the log files should be placed")
                    .assignmentOptional().defaultValue("karaboHistory")
                    .commit();

            INT32_ELEMENT(expected).key("logger.FileDataLogger.maximumFileSize")
                    .displayedName("Maximum file size")
                    .description("After any archived file has reached this size it will be time-stamped and not appended anymore")
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .assignmentOptional().defaultValue(100)
                    .commit();

            NODE_ELEMENT(expected).key("logger.InfluxDataLogger")
                    .displayedName("InfluxDataLogger")
                    .description("Influxdb based data logging")
                    .commit();

            STRING_ELEMENT(expected).key("logger.InfluxDataLogger.urlWrite")
                    .displayedName("Logger influxdb URL")
                    .description("URL should be given in form: tcp://host:port")
                    .assignmentOptional().defaultValue("tcp://localhost:8086")
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("logger.InfluxDataLogger.urlRead")
                    .displayedName("Reader influxdb URL")
                    .description("URL should be given in form: tcp://host:port")
                    .assignmentOptional().defaultValue("tcp://localhost:8086")
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("logger.InfluxDataLogger.dbname")
                    .displayedName("Database name")
                    .description("Name of the database of the data. If empty, fall back to broker topic.")
                    .assignmentOptional().defaultValue("")
                    .init()
                    .commit();

            UINT32_ELEMENT(expected).key("logger.InfluxDataLogger.maxBatchPoints")
                    .displayedName("Max batch points")
                    .description("Max number of InfluxDB points in the batch")
                    .assignmentOptional().defaultValue(200)
                    .init()
                    .commit();

            BOOL_ELEMENT(expected).key("useP2p")
                    .displayedName("Use p2p shortcut")
                    .description("Whether to instruct loggers to use point-to-point instead of broker")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(false)
                    .commit();

            VECTOR_STRING_ELEMENT(expected).key("serverList")
                    .displayedName("Server list")
                    .description("List of device server IDs where the DataLogger instance run. "
                                 "The load balancing is round-robin. Must not be empty")
                    .init()
                    .minSize(1)
                    .assignmentMandatory()
                    .commit();

            UINT32_ELEMENT(expected).key("timeout")
                    .displayedName("Timeout")
                    .description("Timeout of requests to DataLogger's or during checks")
                    .unit(Unit::SECOND)
                    .metricPrefix(MetricPrefix::MILLI)
                    // Defaults to 15 s.
                    // 2 s. lead to many timeouts. During tests, 4 s. latencies have been observed.
                    // 15 s. adds an extra safety margin.
                    .assignmentOptional().defaultValue(15000)
                    .reconfigurable()
                    .minInc(100).maxInc(60000) // 100 ms to 1 minute
                    .commit();

            NODE_ELEMENT(expected).key("topologyCheck")
                    .displayedName("Topology check")
                    .description("Status and parameters of regular topology checks")
                    .commit();

            SLOT_ELEMENT(expected).key("topologyCheck.slotForceCheck")
                    .displayedName("Force check")
                    .description("Immediately launch a check")
                    .allowedStates(State::NORMAL)
                    .commit();

            STRING_ELEMENT(expected).key("topologyCheck.lastCheckStartedUtc")
                    .displayedName("Check started (UTC)")
                    .description("Last time a check was initiated")
                    .readOnly().initialValue("")
                    .commit();

            STRING_ELEMENT(expected).key("topologyCheck.lastCheckDoneUtc")
                    .displayedName("Check finished (UTC)")
                    .description("Last time a check was finished")
                    .readOnly().initialValue("")
                    .commit();

            STRING_ELEMENT(expected).key("topologyCheck.lastCheckResult")
                    .displayedName("Check result")
                    .description("Result of last running check")
                    .readOnly().initialValue("")
                    .commit();

            UINT32_ELEMENT(expected).key("topologyCheck.interval")
                    .displayedName("Check interval")
                    .description("Interval in between regular checks on the topology whether "
                                 "everything is logged as it should")
                    .unit(Unit::MINUTE)
                    .assignmentOptional().defaultValue(30)
                    .reconfigurable()
                    .minInc(1).maxInc(1440) // min every minute, max every 24 h
                    .commit();

            UINT32_ELEMENT(expected).key("topologyCheck.toleranceLogged")
                    .displayedName("Tolerance logged")
                    .description("How old last logged update may be")
                    .unit(Unit::MINUTE)
                    .assignmentOptional().defaultValue(10)
                    .reconfigurable()
                    .minInc(1).maxInc(60)
                    .commit();

            UINT32_ELEMENT(expected).key("topologyCheck.toleranceDiff")
                    .displayedName("Tolerance diff")
                    .description("Tolerated difference between logged data and latest update when last logged data is old")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(30)
                    .reconfigurable()
                    .minInc(1).maxInc(600)
                    .commit();
            
            // this is in here for 2.9.X only to allow temporarily running two logging systems.
            STRING_ELEMENT(expected).key("loggermap")
                    .displayedName("Logger map file")
                    .assignmentOptional().defaultValue("loggermap.xml")
                    .commit();
        }

        DataLoggerManager::DataLoggerManager(const Hash& input)
            : karabo::core::Device<>(input)
            , m_serverList(input.get<vector<string> >("serverList"))
            , m_serverIndex(0), m_loggerMapFile(input.get<string>("loggermap"))
            , m_strand(boost::make_shared<karabo::net::Strand>(karabo::net::EventLoop::getIOService()))
            , m_topologyCheckTimer(karabo::net::EventLoop::getIOService())
            , m_logger("Unsupported") {

            if (input.has("logger.FileDataLogger")) {
                m_logger = "FileDataLogger";
            } else if (input.has("logger.InfluxDataLogger")) {
                m_logger = "InfluxDataLogger";
            }

            m_loggerMap.clear();
            if (boost::filesystem::exists(m_loggerMapFile)) {
                karabo::io::loadFromFile(m_loggerMap, m_loggerMapFile);
            }

            KARABO_SYSTEM_SIGNAL("signalLoggerMap", Hash /*loggerMap*/);
            KARABO_SLOT(slotGetLoggerMap);
            KARABO_SLOT(topologyCheck_slotForceCheck);

            KARABO_INITIAL_FUNCTION(initialize);
        }

        DataLoggerManager::~DataLoggerManager() {
        }


        void DataLoggerManager::initialize() {

            checkLoggerMap(); // throws if loggerMap and serverList are inconsistent

            // Setup m_loggerData from server list
            const Hash data("state", LoggerState::OFFLINE,
                            "backlog", std::unordered_set<std::string>(),
                            "beingAdded", std::unordered_set<std::string>(),
                            "devices", std::unordered_set<std::string>());
            for (const std::string& server : m_serverList) {
                m_loggerData.set(server, data);
            }

            if (m_logger == "InfluxDataLogger" && get<std::string>("logger.InfluxDataLogger.dbname").empty()) {
                // Initialise DB name from broker topic
                const std::string dbName(getTopic());
                KARABO_LOG_FRAMEWORK_INFO << "Switch to Influx DB name '" << dbName << "'";
                set("logger.InfluxDataLogger.dbname", dbName);
            }

            // Register handlers here
            remote().registerInstanceNewMonitor(boost::bind(&DataLoggerManager::instanceNewHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&DataLoggerManager::instanceGoneHandler, this, _1, _2));

            // Switch on instance tracking - which is blocking a while.
            // Note that instanceNew(..) will be called for all instances already in the game.
            remote().enableInstanceTracking();

            // Publish logger map read from disc. Do that as late as possible in the initialization procedure
            // to give those interested the chance to register their slots after we sent signalInstanceNew.
            {
                boost::mutex::scoped_lock lock(m_loggerMapMutex); // m_loggerMap must not be changed while we process it
                emit<Hash>("signalLoggerMap", m_loggerMap);
            }

            // Start regular topology checks (and update State to NORMAL)
            m_strand->post(bind_weak(&Self::launchTopologyCheck, this));

        }


        void DataLoggerManager::checkLoggerMap() {
            // Check that all servers that are supposed to host DataLoggers are in server list.

            // First get server ids - the values of the logger map.
            std::unordered_set<std::string> serversInMap; // use set to filter out duplications
            {
                boost::mutex::scoped_lock lock(m_loggerMapMutex); // m_loggerMap must not be changed while we process it
                for (Hash::const_iterator it = m_loggerMap.begin(), itEnd = m_loggerMap.end(); it != itEnd; ++it) {
                    serversInMap.insert(it->getValue<std::string>());
                }
            }
            // Now loop and check that all from logger map are also in configured server list
            for (const std::string& serverInMap : serversInMap) {
                if (find(m_serverList.begin(), m_serverList.end(), serverInMap) == m_serverList.end()) {
                    throw KARABO_LOGIC_EXCEPTION("Inconsistent '" + m_loggerMapFile + "' and \"serverList\" configuration: '"
                            + serverInMap + "' is in map, but not in list.");
                }
            }
        }


        void DataLoggerManager::topologyCheck_slotForceCheck() {
            if (m_topologyCheckTimer.cancel() > 0) {
                topologyCheck(boost::system::error_code());
            }
        }


        void DataLoggerManager::launchTopologyCheck() {
            // Publish last results except if in INIT (because then there was no last run!):
            if (getState() != State::INIT) {
                const std::string result(checkSummary());
                KARABO_LOG_FRAMEWORK_INFO << "Check finished - " << result;
                set(Hash("topologyCheck", Hash("lastCheckDoneUtc", Epochstamp().toFormattedString(),
                                               "lastCheckResult", result)));
            }

            updateState(State::NORMAL);
            m_topologyCheckTimer.expires_from_now(boost::posix_time::minutes(get<unsigned int>("topologyCheck.interval")));
            m_topologyCheckTimer.async_wait(bind_weak(&Self::topologyCheck, this, boost::asio::placeholders::error));
        }


        std::string DataLoggerManager::checkSummary() {
            std::stringstream checkResult;
            Hash newCheckStatus;

            bool bad = false;
            if (m_checkStatus.has("offline")) {
                checkResult << "   Offline logger servers: " << toString(m_checkStatus.get<std::set<std::string> >("offline")) << "\n";
                m_checkStatus.erase("offline"); // skip it from loop below!
                bad = true;
            }

            // The remaining keys are serverIds:
            for (const Hash::Node& node : m_checkStatus) {
                const Hash& serverHash = node.getValue<Hash>();
                const std::string& serverId = node.getKey();
                checkResult << "\n   " << serverId << ": ";
                if (serverHash.has("loggerQueryFailed")) {
                    checkResult << "Query to logger failed.\n";
                    bad = true;
                    continue;
                }
                if (serverHash.has("emptyTimestamp")) {
                    const auto& devs = serverHash.get<std::set<std::string> >("emptyTimestamp");
                    checkResult << "\n      Empty time stamps for " << toString(devs);
                    // Keep info for next round, but with new key to check next time whether still empty timestamp:
                    newCheckStatus.set(serverId, Hash("emptyTimestampLast", devs));
                }
                // No need to summarize "emptyTimestampLast" here: It was in summary in last round and
                // - either is fine now,
                // - or is not online anymore,
                // - or appears in "forced" below

                if (serverHash.has("forced")) {
                    const auto& devs = serverHash.get<std::set<std::string> >("forced");
                    checkResult << "\n      Re-enforced logging for " << devs.size() << " devices: " << toString(devs);
                    bad = true;
                }
                if (serverHash.has("detailsRequested")) {
                    const auto& devs = serverHash.get<std::set<std::string> >("detailsRequested");
                    if (devs.size() > 5) {
                        // Prints the first three and the total number of devices.
                        checkResult << "\n      Details requested for "
                                << *std::next(devs.begin(), 0) << ", " << *std::next(devs.begin(), 1) << ", "
                                << *std::next(devs.begin(), 2) << " (and " << devs.size() - 3 << " more devices...)";
                    } else {
                        checkResult << "\n      Details requested for " << toString(devs);
                    }
                }
                if (serverHash.has("deviceQueryFailed")) {
                    const auto& devs = serverHash.get<std::set<std::string> >("deviceQueryFailed");
                    checkResult << "\n      " << devs.size() << " device queries failed: " << toString(devs);
                    bad = true; // Could just being shutdown and logger was not yet aware...
                }
                if (serverHash.has("stopped")) {
                    const auto& devs = serverHash.get<std::set<std::string> >("stopped");
                    checkResult << "\n      " << devs.size() << " devices now offline, logging stopped: " << toString(devs);
                }
            }
            // Clear check status, but keep what is needed for next check
            m_checkStatus = std::move(newCheckStatus);

            const std::string checkResultStr(checkResult.str());
            std::string prefix(bad ? "Found problems:" : "Ok");
            if (!bad) {
                prefix += (checkResultStr.empty() ? "." : ", just note:");
            }
            return (prefix += checkResultStr);
        }

        void DataLoggerManager::topologyCheck(const boost::system::error_code& e) {
            if (e == boost::asio::error::operation_aborted) {
                return;
            }
            KARABO_LOG_FRAMEWORK_INFO << "Launching topology check from state " << getState().name();
            updateState(State::MONITORING);
            set("topologyCheck.lastCheckStartedUtc", Epochstamp().toFormattedString());
            m_strand->post(bind_weak(&Self::topologyCheckOnStrand, this));
        }


        void DataLoggerManager::topologyCheckOnStrand() {
            printLoggerData();

            auto loggerCounter = boost::make_shared<std::atomic<size_t> >(m_loggerData.size());
            const unsigned int timeout = get<unsigned int>("timeout");
            for (const Hash::Node& serverNode : m_loggerData) {
                const std::string& serverId = serverNode.getKey();
                const Hash& serverData = serverNode.getValue<Hash>();
                if (serverData.get<LoggerState>("state") != LoggerState::RUNNING) {
                    KARABO_LOG_FRAMEWORK_INFO << "Skip checking '" << serverId << "' since not running";
                    addToSetOrCreate(m_checkStatus, "offline", serverId);
                    --(*loggerCounter);
                    continue;
                }
                const std::string loggerId(serverIdToLoggerId(serverId));
                KARABO_LOG_FRAMEWORK_DEBUG << "Request config of logger '" << loggerId << "'";
                call(loggerId, "flush"); // Force flushing and thus update of "lastUpdatesUtc"
                auto okHandler = bind_weak(&Self::checkLoggerConfig, this, true, loggerCounter, _1, _2);
                auto failHandler = bind_weak(&Self::checkLoggerConfig, this, false, loggerCounter, Hash(), loggerId);
                request(loggerId, "slotGetConfiguration").timeout(timeout)
                        .receiveAsync<Hash, std::string>(okHandler, failHandler);
            }
            if (0 == *loggerCounter) { // All loggers are not (yet) running
                m_strand->post(bind_weak(&Self::launchTopologyCheck, this));
            }
        }


        void DataLoggerManager::printLoggerData() const {
            std::stringstream out;
            for (const Hash::Node& serverNode : m_loggerData) {
                const Hash& serverHash = serverNode.getValue<Hash>();
                const LoggerState state = serverHash.get<LoggerState>("state");
                out << "\n  Server " << serverNode.getKey() << " is "
                    << (state == LoggerState::OFFLINE ? "offline" : (state == LoggerState::RUNNING ? "running" : "instantiating"));
                for (const std::string& key : std::vector<std::string>({"devices", "backlog", "beingAdded"})) {
                out << "\n    " << key << ": ";
                size_t nChars = 0;
                for (const std::string& id : serverHash.get<std::unordered_set<std::string> >(key)) {
                    // If line gets too long, start a new one - but at least print one id per line:
                    if ((nChars += (id.size())) > 55 && nChars != id.size()) {
                        out << "\n             ";
                        nChars = 0;
                    }
                    out << id << ", ";
                    nChars += 2; // for comma and space
                }
            }
            }

            KARABO_LOG_FRAMEWORK_INFO << "Internal logger info:" << out.str();
        }


        void DataLoggerManager::checkLoggerConfig(bool ok, const boost::shared_ptr<std::atomic<size_t> >& loggerCounter,
                                                  const Hash& config, const std::string& loggerId) {
            // Put on strand to be sequential, but take care that the re-throw trick for exception details
            // will not work anymore.
            std::string errorTxt;
            if (!ok) {
                try {
                    throw;
                } catch (const std::exception& e) {
                    errorTxt = e.what();
                }
            }
            m_strand->post(bind_weak(&Self::checkLoggerConfigOnStrand, this, errorTxt, loggerCounter, config, loggerId));
        }


        void DataLoggerManager::checkLoggerConfigOnStrand(const std::string& errorTxt, const boost::shared_ptr<std::atomic<size_t> >& loggerCounter,
                                                          const Hash& config, const std::string& loggerId) {
            Epochstamp now;
            const std::string serverId(loggerIdToServerId(loggerId));
            if (errorTxt.empty()) {
                const TimeDuration tolerance(0, 0, get<unsigned int>("topologyCheck.toleranceLogged"), 0ull, 0ull); // from minutes
                const std::vector<Hash>& updates = config.get<std::vector < Hash >> ("lastUpdatesUtc");
                auto loggedDevCounter = boost::make_shared<std::atomic<size_t> >(updates.size());
                const unsigned int timeout = get<unsigned int>("timeout");
                std::vector<std::string> idsWithoutTimestamp;
                for (const Hash& row : updates) {
                    const Hash::Node& lastUpdateNode = row.getNode("lastUpdateUtc");
                    const std::string& lastUpdateStr = lastUpdateNode.getValue<std::string>();
                    const std::string& deviceId = row.get<std::string>("deviceId");

                    if (lastUpdateStr.empty() || !Epochstamp::hashAttributesContainTimeInformation(lastUpdateNode.getAttributes())) {
                        // No update yet, so DataLogging for that device likely being started - book-keeping to check next time:
                        idsWithoutTimestamp.push_back(deviceId);
                        // Check the last try:
                        const std::string keyEmpty(serverId + ".emptyTimestampLast");
                        if (m_checkStatus.has(keyEmpty)) {
                            std::set<std::string>& emptyLast = m_checkStatus.get<std::set<std::string> >(keyEmpty);
                            auto it = std::find(emptyLast.begin(), emptyLast.end(), deviceId);
                            if (it != emptyLast.end()) {
                                KARABO_LOG_FRAMEWORK_WARN << "Device '" << deviceId << "' logged by '" << loggerId
                                        << "' still has no last update stamp - force logging";
                                forceDeviceToBeLogged(deviceId);
                                addToSetOrCreate(m_checkStatus, serverId + ".forced", deviceId);
                            }
                        }
                        continue;
                    }
                    const Epochstamp lastUpdate(Epochstamp::fromHashAttributes(lastUpdateNode.getAttributes()));
                    if (now > lastUpdate && now.elapsed(lastUpdate) > tolerance) { // Caveat: TimeDuration is always positive!
                        KARABO_LOG_FRAMEWORK_DEBUG << loggerId << " logged last data from '" << deviceId
                                << "' rather long ago: " << lastUpdateStr << " (UTC)";
                        addToSetOrCreate(m_checkStatus, serverId + ".detailsRequested", deviceId);

                        const unsigned int toleranceSec = std::max(get<unsigned int>("topologyCheck.toleranceDiff"),
                                                                   config.get<unsigned int>("flushInterval"));
                        auto okHandler = bind_weak(&Self::checkDeviceConfig, this, true, loggerCounter, loggerId,
                                                   toleranceSec, loggedDevCounter, lastUpdate, _1, _2);
                        auto failHandler = bind_weak(&Self::checkDeviceConfig, this, false, loggerCounter, loggerId,
                                                     toleranceSec, loggedDevCounter, lastUpdate, Hash(), deviceId);
                        request(deviceId, "slotGetConfiguration").timeout(timeout)
                                .receiveAsync<Hash, std::string>(okHandler, failHandler);
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Device '" << deviceId << "' OK according to logger '"
                                << loggerId << "': " << *loggedDevCounter - 1 << " left ";
                        --(*loggedDevCounter);
                    }
                }
                if (!idsWithoutTimestamp.empty()) {
                    (*loggedDevCounter) -= idsWithoutTimestamp.size();
                    KARABO_LOG_FRAMEWORK_INFO << "Logger lacks last update timestamp of " << toString(idsWithoutTimestamp);
                    m_checkStatus.set(serverId + ".emptyTimestamp",
                                      std::set<std::string>(idsWithoutTimestamp.begin(), idsWithoutTimestamp.end()));
                }

                if (0 == *loggedDevCounter) {
                    // No suspicious devices logged by this logger
                    KARABO_LOG_FRAMEWORK_INFO << "All devices logged by " << loggerId << " are recently logged.";
                    --(*loggerCounter);
                }
            } else {
                // Failure handler
                KARABO_LOG_FRAMEWORK_INFO << "Failed to query configuration of " << loggerId << ": " << errorTxt
                            << "\n logger left: " << *loggerCounter - 1;
                m_checkStatus.set(serverId + ".loggerQueryFailed", true);
                --(*loggerCounter);
            }

            if (0 == *loggerCounter) {
                m_strand->post(bind_weak(&Self::launchTopologyCheck, this));
            }
        }


        void DataLoggerManager::forceDeviceToBeLogged(const std::string& deviceId) {
            goneDeviceToLog(deviceId);
            newDeviceToLog(deviceId);
        }


        void DataLoggerManager::checkDeviceConfig(bool ok, const boost::shared_ptr<std::atomic<size_t> >& loggerCounter,
                                                  const std::string& loggerId, unsigned int toleranceSec,
                                                  const boost::shared_ptr<std::atomic<size_t> >& loggedDevCounter,
                                                  Epochstamp lastUpdateLogger, const Hash& config, const std::string& deviceId) {
            std::string errorTxt;
            if (!ok) {
                // Note that this can e.g. happen when 'deviceId' went down when the manager was offline and then
                // the manager is restarted. The failure would be a timeout then.
                // Trick:
                // For a failure handler, re-throwing the exception does not work when posting further.
                try {
                    throw;
                } catch (const std::exception& e) {
                    errorTxt = e.what();
                }
            }
            m_strand->post(bind_weak(&Self::checkDeviceConfigOnStrand, this, errorTxt, loggerCounter, loggerId, toleranceSec,
                                     loggedDevCounter, lastUpdateLogger, config, deviceId));
        }


        void DataLoggerManager::checkDeviceConfigOnStrand(const std::string& errorTxt, const boost::shared_ptr<std::atomic<size_t> >& loggerCounter,
                                                          const std::string& loggerId, unsigned int toleranceSec,
                                                          const boost::shared_ptr<std::atomic<size_t> >& loggedDevCounter,
                                                          Epochstamp lastUpdateLogger, const Hash& config, const std::string& deviceId) {
            if (errorTxt.empty()) {
                const Epochstamp lastDeviceUpdate = mostRecentEpochstamp(config);
                const TimeDuration tolerance(0, 0, 0, toleranceSec, 0ull);
                if (lastDeviceUpdate > lastUpdateLogger && lastDeviceUpdate.elapsed(lastUpdateLogger) > tolerance) {
                    KARABO_LOG_FRAMEWORK_WARN << deviceId << " had last update at " << lastDeviceUpdate.toFormattedString()
                            << ", but most recent data logged by " << loggerId << " is from "
                            << lastUpdateLogger.toFormattedString() << " - force logging to start again";
                    forceDeviceToBeLogged(deviceId);
                    const std::string serverId(loggerIdToServerId(loggerId));
                    addToSetOrCreate(m_checkStatus, serverId + ".forced", deviceId);
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Last update of " << deviceId << " at " << lastDeviceUpdate.toFormattedString()
                            << ": logger not behind.";
                }
            } else {
                // Failure handler:
                KARABO_LOG_FRAMEWORK_INFO << "Failed to query device configuration of " << deviceId << ": " << errorTxt;
                const std::string serverId(loggerIdToServerId(loggerId));
                addToSetOrCreate(m_checkStatus, serverId + ".deviceQueryFailed", deviceId);
                // If request failed, it maybe that the device went offline and the manager did not notice since
                // it was down at that time and restarted later. In that case the device should not be in the logger
                // data - so better check that!
                // But do not trust remote().getSystemInformation() - it is not synchronised with our m_strand.
                const auto& knownDevs = m_loggerData.get<std::unordered_set<std::string> >(serverId + ".devices");
                if (knownDevs.find(deviceId) == knownDevs.end()) {
                    KARABO_LOG_FRAMEWORK_WARN << "Device " << deviceId << " not known for logger "
                            << loggerId << " - stop logging it!";
                    addToSetOrCreate(m_checkStatus, serverId + ".stopped", deviceId);
                    call(loggerId, "slotTagDeviceToBeDiscontinued", "D", deviceId);
                }
            }

            if (0 == --(*loggedDevCounter)) {
                // Last device of this logger, so decrease also the logger counter
                if (0 == --(*loggerCounter)) {
                    // This was the last logger
                    m_strand->post(bind_weak(&Self::launchTopologyCheck, this));
                }
            }
        }


        karabo::util::Epochstamp DataLoggerManager::mostRecentEpochstamp(const Hash& config, Epochstamp oldStamp) const {

            for (const Hash::Node& node : config) {
                if (Epochstamp::hashAttributesContainTimeInformation(node.getAttributes())) {
                    const Epochstamp stamp(Epochstamp::fromHashAttributes(node.getAttributes()));
                    if (stamp > oldStamp) {
                        oldStamp = stamp;
                    }
                }
                // If it is a Hash, we recurse inside the NODE.
                // A vector<Hash> would be a TABLE_ELEMENT that we treat atomically, so we do not recurse.
                if (node.is<Hash>()) {
                    oldStamp = mostRecentEpochstamp(node.getValue<Hash>(), oldStamp);
                }
            }
            return oldStamp;
        }


        void DataLoggerManager::instantiateReaders(const std::string& serverId) {
            for (unsigned int i = 0; i < DATALOGREADERS_PER_SERVER; ++i) {
                const std::string readerId = DATALOGREADER_PREFIX + toString(i) + "-" + serverId;
                if (!remote().exists(readerId).first) {
                    Hash hash;
                    Hash config;
                    if (m_logger == "FileDataLogger") {
                        hash.set("classId", "FileLogReader");
                        hash.set("deviceId", readerId);
                        config.set("directory", get<string>("logger.FileDataLogger.directory"));
                    } else if (m_logger == "InfluxDataLogger") {
                        hash.set("classId", "InfluxLogReader");
                        hash.set("deviceId", readerId);
                        config.set("url", get<string>("logger.InfluxDataLogger.urlRead"));
                        config.set("dbname", get<string>("logger.InfluxDataLogger.dbname"));
                    }
                    hash.set("configuration", config);
                    KARABO_LOG_FRAMEWORK_INFO
                            << "Trying to instantiate '" << readerId << "' "
                            << "of type '" << m_logger << "' on server '" << serverId << "'";

                    remote().instantiateNoWait(serverId, hash);
                }
            }
        }

        void DataLoggerManager::slotGetLoggerMap() {
            boost::mutex::scoped_lock lock(m_loggerMapMutex); // m_loggerMap must not be changed while we process it
            reply(m_loggerMap);
        }


        std::string DataLoggerManager::loggerServerId(const std::string& deviceId, bool addIfNotYetInMap) {
            std::string serverId;

            const std::string & deviceIdInMap(DATALOGGER_PREFIX + deviceId); // DATALOGGER_PREFIX for xml files from < 2.6.0
            boost::mutex::scoped_lock lock(m_loggerMapMutex);
            if (m_loggerMap.has(deviceIdInMap)) {
                serverId = m_loggerMap.get<string>(deviceIdInMap);
            } else if (addIfNotYetInMap) {
                if (m_serverList.empty()) {
                    // Cannot happen but for better diagnostics in case it does:
                    throw KARABO_PARAMETER_EXCEPTION("List of servers for data logging is empty."
                                                     " You have to define one data logger server, at least!");
                }
                m_serverIndex %= m_serverList.size();
                serverId = m_serverList[m_serverIndex++];
                m_loggerMap.set(deviceIdInMap, serverId);

                // Logger map changed, so publish - online and as backup
                emit<Hash>("signalLoggerMap", m_loggerMap);
                karabo::io::saveToFile(m_loggerMap, m_loggerMapFile);
            }
            return serverId;
        }


        void DataLoggerManager::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
            m_strand->post(bind_weak(&DataLoggerManager::instanceNewOnStrand, this, topologyEntry));
        }


        void DataLoggerManager::instanceNewOnStrand(const karabo::util::Hash& topologyEntry) {
            const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
            // const ref is fine even for temporary std::string
            const std::string& instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                                             topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));
            KARABO_LOG_FRAMEWORK_INFO << "instanceNew --> instanceId: '" << instanceId
                        << "', type: '" << type << "'";

            if (type == "device") {
                const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                if (entry.hasAttribute(instanceId, "archive") && entry.getAttribute<bool>(instanceId, "archive")) {
                    // A device that should be archived
                    newDeviceToLog(instanceId);
                }
                if (entry.hasAttribute(instanceId, "classId") &&
                        entry.getAttribute<std::string>(instanceId, "classId") == m_logger) {
                    // A new logger has started - check whether there is more work for it to do
                    newLogger(instanceId);
                }
            } else if (type == "server") {
                if (m_loggerData.has(instanceId)) {
                    // One of our servers!
                    newLoggerServer(instanceId);
                }
            }
        }


        void DataLoggerManager::newDeviceToLog(const std::string& deviceId) {

            // Figure out which server and thus which logger this runs:
            const std::string serverId(loggerServerId(deviceId, true));

            // Put deviceId to backlog - independent of state:
            Hash& data = m_loggerData.get<Hash>(serverId);
            data.get<std::unordered_set<std::string> >("backlog").insert(deviceId);

            // If logger is already running, transfer the (likely new and size-1-) backlog to it
            if (data.get<LoggerState>("state") == LoggerState::RUNNING) {
                addDevicesToBeLogged(serverIdToLoggerId(serverId), data);
            } else {
                KARABO_LOG_FRAMEWORK_INFO << "New device '" << deviceId << "' to be logged, but logger not yet running";
            }
        }


        void DataLoggerManager::newLogger(const std::string& loggerId) {
            const std::string serverId(loggerIdToServerId(loggerId));
            // Get data for this server to access backlog and state
            Hash& data = m_loggerData.get<Hash>(serverId);
            data.set("state", LoggerState::RUNNING);
            // Any "devices" or "beingAdded" left do not need to be added to "backlog" here for cases where logger is
            // killed and the loss of heart beats was not yet discovered:
            // The instanceNew that triggers this newLogger here has triggered an instanceGone before anyway.
            addDevicesToBeLogged(loggerId, data);
        }


        void DataLoggerManager::addDevicesToBeLogged(const std::string& loggerId, Hash& serverData) {

            std::unordered_set<std::string>& backlog = serverData.get<std::unordered_set<std::string> >("backlog");
            if (!backlog.empty()) {
                // Keep track of what is being added
                std::unordered_set<std::string>& beingAdded = serverData.get<std::unordered_set<std::string> >("beingAdded");
                beingAdded.insert(backlog.begin(), backlog.end());
                KARABO_LOG_FRAMEWORK_INFO << "For '" << loggerId << "', adding devices: '" << toString(backlog);
                auto successHandler = bind_weak(&DataLoggerManager::addDevicesDone, this, true, loggerId, backlog, _1);
                auto failureHandler = bind_weak(&DataLoggerManager::addDevicesDone, this, false, loggerId, backlog,
                                                std::vector<std::string>());
                const unsigned int timeout = get<unsigned int>("timeout");
                request(loggerId, "slotAddDevicesToBeLogged", std::vector<std::string>(backlog.begin(), backlog.end()))
                        .timeout(timeout).receiveAsync<std::vector<std::string> >(successHandler, failureHandler);

                backlog.clear();
            }
        }


        void DataLoggerManager::addDevicesDone(bool ok, const std::string& loggerId,
                                               const std::unordered_set<std::string>& calledDevices,
                                               const std::vector<std::string>& alreadyLoggedDevices) {
            // Put on strand to be sequential, but take care that the re-throw trick for exception details
            // will not work anymore.
            std::string errorTxt;
            if (!ok) {
                try {
                    throw;
                } catch (const std::exception& e) {
                    errorTxt = e.what();
                }
            }
            m_strand->post(bind_weak(&DataLoggerManager::addDevicesDoneOnStrand, this, errorTxt, loggerId,
                                     calledDevices, alreadyLoggedDevices));
        }


        void DataLoggerManager::addDevicesDoneOnStrand(const std::string& errorTxt, const std::string& loggerId,
                                                       const std::unordered_set<std::string>& calledDevices,
                                                       const std::vector<std::string>& alreadyLoggedDevices) {
            const std::string serverId(loggerIdToServerId(loggerId));
            Hash& data = m_loggerData.get<Hash>(serverId);

            if (errorTxt.empty()) {
                if (alreadyLoggedDevices.empty()) {
                    KARABO_LOG_FRAMEWORK_INFO << "For '" << loggerId << "', added devices to be logged: '"
                            << toString(calledDevices) << "'";
                } else {
                    // Can happen when, during initialising, a logger is discovered that was running since before
                    // DataLoggerManager was instantiated.
                    KARABO_LOG_FRAMEWORK_WARN << "Added '" << toString(calledDevices) << "' to be logged by '"
                            << loggerId << "', but '" << toString(alreadyLoggedDevices) << "' were already logged.";
                }
                // Remove from "beingAdded" and add to "devices" since done, even those that were already logged:
                // We just did not yet know about it (see above).
                std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string> >("beingAdded");
                for (const std::string& calledDevice : calledDevices) {
                    beingAdded.erase(calledDevice);
                }
                data.get<std::unordered_set<std::string> >("devices").insert(calledDevices.begin(), calledDevices.end());
            } else {
                // Can happen as timeout when logger just shutdown
                KARABO_LOG_FRAMEWORK_ERROR << "For '" << loggerId << "', failed to add '" << toString(calledDevices)
                        << "' to be logged since: " << errorTxt;

                // Put devices to log back to backlog,
                // but only those "beingAdded" (others could have shutdown meanwhile)
                std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string> >("beingAdded");
                std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string> >("backlog");
                for (const std::string& calledDevice : calledDevices) {
                    auto it = beingAdded.find(calledDevice);
                    if (it != beingAdded.end()) {
                        backlog.insert(calledDevice);
                        beingAdded.erase(it);
                    }
                }
                if (data.get<LoggerState>("state") == LoggerState::RUNNING) {
                    // Try again, logger likely just came up:
                    addDevicesToBeLogged(loggerId, data);
                }
            }
        }

        void DataLoggerManager::newLoggerServer(const std::string& serverId) {

            instantiateLogger(serverId);
            instantiateReaders(serverId);
        }


        void DataLoggerManager::instantiateLogger(const std::string& serverId) {
            // Get data for this server to access backlog and state
            Hash& data = m_loggerData.get<Hash>(serverId);

            if (data.get<LoggerState>("state") == LoggerState::OFFLINE) {
                // If state is RUNNING, likely since logger discovered before server when manager starts into a running
                // system. Our try to instantiate below will then fail - but no problem!
                data.set("state", LoggerState::INSTANTIATING);
            }

            // Instantiate logger, but do not yet specify "devicesToBeLogged":
            // Having one channel only to transport this info (slotAddDevicesToBeLogged) simplifies logic.
            Hash config;
            if (m_logger == "FileDataLogger") {
                config.set("directory", get<std::string>("logger.FileDataLogger.directory"));
                config.set("maximumFileSize", get<int>("logger.FileDataLogger.maximumFileSize"));
            } else if (m_logger == "InfluxDataLogger") {
                config.set("urlWrite", get<std::string>("logger.InfluxDataLogger.urlWrite"));
                config.set("urlQuery", get<std::string>("logger.InfluxDataLogger.urlRead"));
                config.set("dbname", get<std::string>("logger.InfluxDataLogger.dbname"));
                config.set("maxBatchPoints", get<std::uint32_t>("logger.InfluxDataLogger.maxBatchPoints"));
            }
            config.set("flushInterval", get<int>("flushInterval"));
            config.set("performanceStatistics.enable", get<bool>("enablePerformanceStats"));
            config.set("useP2p", get<bool>("useP2p"));

            const std::string loggerId(serverIdToLoggerId(serverId));
            const Hash hash("classId", m_logger,
                            "deviceId", loggerId,
                            "configuration", config);

            KARABO_LOG_FRAMEWORK_INFO << "Trying to instantiate '" << loggerId << "' on server '" << serverId << "'";
            remote().instantiateNoWait(serverId, hash);
        }


        void DataLoggerManager::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            m_strand->post(bind_weak(&DataLoggerManager::instanceGoneOnStrand, this, instanceId, instanceInfo));
        }


        void DataLoggerManager::instanceGoneOnStrand(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {

            // const ref is fine even for temporary std::string
            const std::string& type = (instanceInfo.has("type") && instanceInfo.is<std::string>("type") ?
                                       instanceInfo.get<std::string>("type") : std::string("unknown"));
            const std::string& serverId = (instanceInfo.has("serverId") && instanceInfo.is<std::string>("serverId") ?
                                           instanceInfo.get<string>("serverId") : std::string("?"));

            KARABO_LOG_FRAMEWORK_INFO << "instanceGoneHandler -->  instanceId : '"
                    << instanceId << "', type : " << type << " on server '" << serverId << "'";

            if (type == "device") {
                // Figure out who logs and tell to stop
                goneDeviceToLog(instanceId);
                if (instanceInfo.has("classId") &&
                        instanceInfo.get<std::string>("classId") == m_logger) {
                    goneLogger(instanceId);
                }
            } else if (type == "server") {
                if (m_loggerData.has(instanceId)) {
                    // It is one of our logger servers
                    goneLoggerServer(instanceId);
                }
            }
        }


        void DataLoggerManager::goneDeviceToLog(const std::string& deviceId) {
            const std::string serverId(loggerServerId(deviceId, false));
            if (!serverId.empty()) { // else device not in map and thus neither logged
                // Remove from any tracking:
                Hash& data = m_loggerData.get<Hash>(serverId);
                std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string> >("backlog");
                std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string> >("beingAdded");
                std::unordered_set<std::string>& loggedIds = data.get<std::unordered_set<std::string> >("devices");
                backlog.erase(deviceId);
                beingAdded.erase(deviceId);
                loggedIds.erase(deviceId);

                const LoggerState state = data.get<LoggerState>("state");
                switch (state) {
                    case LoggerState::RUNNING:
                        // Likely a normal device shutdown - inform the logger:
                        call(serverIdToLoggerId(serverId), "slotTagDeviceToBeDiscontinued", "D", deviceId);
                        // Add a consistency check:
                        if (!backlog.empty()) {
                            KARABO_LOG_FRAMEWORK_WARN << "Backlog for running server '" << serverId
                                    << "' not empty, but contains '" << toString(backlog) << "'";
                        }
                        break;
                    case LoggerState::OFFLINE:
                    case LoggerState::INSTANTIATING:
                        // Add a consistency check:
                        if (!loggedIds.empty()) {
                            KARABO_LOG_FRAMEWORK_WARN << "Logged devices for "
                                    << (state == LoggerState::OFFLINE ? "offline" : "instantiating")
                                    << "  server '" << serverId << "' not empty, but contains " << toString(loggedIds);
                        }
                        break;
                }
            }
        }


        void DataLoggerManager::goneLogger(const std::string& loggerId) {

            const std::string serverId(loggerId.substr(strlen(DATALOGGER_PREFIX)));

            Hash& data = m_loggerData.get<Hash>(serverId);
            std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string> >("backlog");
            std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string> >("beingAdded");
            std::unordered_set<std::string>& loggedIds = data.get<std::unordered_set<std::string> >("devices");

            switch (data.get<LoggerState>("state")) {
                case LoggerState::OFFLINE:
                    KARABO_LOG_FRAMEWORK_WARN << "Logger '" << loggerId << "' gone, but its server gone before.";
                    // But nothing more to do, backlog, beingAdded and loggedIds treated in goneLoggerServer(..)
                    break;
                case LoggerState::INSTANTIATING:
                    KARABO_LOG_FRAMEWORK_WARN << "Logger '" << loggerId << "' gone again while instantiating.";
                    // no 'break;'!
                case LoggerState::RUNNING:
                    // Append logged devices as well as those being added to backlog.
                    // Note: Relying on treatment of those "beingAdded" in failure handling of addDevicesDoneOnStrand
                    //       could be too late if the below instantiateLogger succeeds
                    backlog.insert(loggedIds.begin(), loggedIds.end());
                    loggedIds.clear();
                    backlog.insert(beingAdded.begin(), beingAdded.end());
                    beingAdded.clear();
                    // Instantiate again -- will set "state" appropriately
                    instantiateLogger(serverId);
            }
        }


        void DataLoggerManager::goneLoggerServer(const std::string& serverId) {
            Hash& data = m_loggerData.get<Hash>(serverId);

            switch (data.get<LoggerState>("state")) {
                case LoggerState::OFFLINE:
                    KARABO_LOG_FRAMEWORK_ERROR << "Server '" << serverId << "' gone, but it was already gone before: " << data;
                    // Weird situation - move "devices"/"beingAdded" to "backlog" as in other cases...
                    break;
                case LoggerState::INSTANTIATING:
                    // Expected nice behaviour: Already took note that logger is gone and so tried to start again.
                    // Nothing to do.
                    KARABO_LOG_FRAMEWORK_INFO << "Server '" << serverId
                            << "' gone while instantiating " << m_logger << ".";
                    break;
                case LoggerState::RUNNING:
                    // Looks like a non-graceful shutdown of the server that is detected by lack of heartbeats where
                    // the DeviceClient currently (Karabo 2.6.0) often sends the "gone" signal for the server before
                    // the one of the DataLogger.
                    KARABO_LOG_FRAMEWORK_WARN << "Server '" << serverId
                            << "' gone while " << m_logger << " still alive.";
                    // Also then we have to move "devices"/"beingAdded" to "backlog".
                    break;
            }

            // Append logged and "being added" devices to backlog - better do for all situations...
            std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string> >("backlog");
            std::unordered_set<std::string>& beingAdded = data.get<std::unordered_set<std::string> >("beingAdded");
            std::unordered_set<std::string>& loggedIds = data.get<std::unordered_set<std::string> >("devices");
            backlog.insert(loggedIds.begin(), loggedIds.end());
            loggedIds.clear();
            backlog.insert(beingAdded.begin(), beingAdded.end());
            beingAdded.clear();

            data.set("state", LoggerState::OFFLINE);
        }
    }
}

