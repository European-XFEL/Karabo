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
 * * One problem remains: if the loggers and the manager work fine, but then the manager is shutdwon and a logged
 *   device stops afterwards, a restart of the manager will not notice that logging this device should be stopped .
 */

#include <vector>
#include <unordered_set>
#include <string>
#include <algorithm>    // std::find
#include <string.h>     // strlen

#include <boost/asio/deadline_timer.hpp>

#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/StringTools.hh"

#include "DataLoggerManager.hh"


namespace karabo {
    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLoggerManager)


        void DataLoggerManager::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::NORMAL)
                    .setNewDefaultValue(State::INIT)
                    .commit();

            INT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(40).minInc(1)
                    .reconfigurable()
                    .commit();

            INT32_ELEMENT(expected).key("maximumFileSize")
                    .displayedName("Maximum file size")
                    .description("After any archived file has reached this size it will be time-stamped and not appended anymore")
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .reconfigurable()
                    .assignmentOptional().defaultValue(100)
                    .commit();

            BOOL_ELEMENT(expected).key("enablePerformanceStats")
                    .displayedName("Performance stats on/off")
                    .description("Value of 'performanceStatistics.enable' used when instantiating loggers")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(true) // true will cause alarms when loggers are too slow
                    .commit();

            BOOL_ELEMENT(expected).key("useP2p")
                    .displayedName("Use p2p shortcut")
                    .description("Whether to instruct loggers to use point-to-point instead of broker")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(false)
                    .commit();

            PATH_ELEMENT(expected).key("directory")
                    .displayedName("Directory")
                    .description("The directory where the log files should be placed")
                    .assignmentOptional().defaultValue("karaboHistory")
                    .commit();

            VECTOR_STRING_ELEMENT(expected).key("serverList")
                    .displayedName("Server list")
                    .description("List of device server IDs where the DataLogger instance run. "
                                 "The load balancing is round-robin. Must not be empty")
                    .init()
                    .minSize(1)
                    .assignmentMandatory()
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue<int>(Schema::AccessLevel::ADMIN)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("archive")
                    .setNewDefaultValue(false)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_DataLoggerManager_0")
                    .commit();
        }

        DataLoggerManager::DataLoggerManager(const Hash& input)
        : karabo::core::Device<>(input)
            , m_serverList(input.get<vector<string> >("serverList"))
            , m_serverIndex(0), m_loggerMapFile("loggermap.xml")
            , m_strand(boost::make_shared<karabo::net::Strand>(karabo::net::EventLoop::getIOService())) {

            m_loggerMap.clear();
            if (boost::filesystem::exists(m_loggerMapFile)) {
                karabo::io::loadFromFile(m_loggerMap, m_loggerMapFile);
            }

            KARABO_SYSTEM_SIGNAL("signalLoggerMap", Hash /*loggerMap*/);
            KARABO_SLOT(slotGetLoggerMap);

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

            updateState(State::NORMAL);
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


        void DataLoggerManager::instantiateReaders(const std::string& serverId) {
            for (unsigned int i = 0; i < DATALOGREADERS_PER_SERVER; ++i) {
                const std::string readerId = DATALOGREADER_PREFIX + toString(i) + "-" + serverId;
                if (!remote().exists(readerId).first) {
                    const Hash hash("classId", "DataLogReader", "deviceId", readerId,
                            "configuration.directory", get<string>("directory"));
                    KARABO_LOG_FRAMEWORK_INFO << "Trying to instantiate '" << readerId << "' on server '" << serverId << "'";

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
                if (entry.hasAttribute(instanceId, "classId")
                    && entry.getAttribute<std::string>(instanceId, "classId") == "DataLogger") {
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

            addDevicesToBeLogged(loggerId, data);
        }


        void DataLoggerManager::addDevicesToBeLogged(const std::string& loggerId, Hash& serverData) {

            std::unordered_set<std::string>& backlog = serverData.get<std::unordered_set<std::string> >("backlog");
            if (!backlog.empty()) {
                // Keep track of what is being added
                std::unordered_set<std::string>& beingAdded = serverData.get<std::unordered_set<std::string> >("beingAdded");
                beingAdded.insert(backlog.begin(), backlog.end());

                KARABO_LOG_FRAMEWORK_INFO << "Adding devices '" << toString(backlog) << "' for logging by " << loggerId;
                auto successHandler = bind_weak(&DataLoggerManager::addDevicesDone, this, true, loggerId, backlog, _1);
                auto failureHandler = bind_weak(&DataLoggerManager::addDevicesDone, this, false, loggerId, backlog,
                                                std::vector<std::string>());
                request(loggerId, "slotAddDevicesToBeLogged", std::vector<std::string>(backlog.begin(), backlog.end()))
                        .timeout(m_timeout).receiveAsync<std::vector<std::string> >(successHandler, failureHandler);

                backlog.clear();
            }
        }


        void DataLoggerManager::addDevicesDone(bool ok, const std::string& loggerId,
                                               const std::unordered_set<std::string>& calledDevices,
                                               const std::vector<std::string>& alreadyLoggedDevices) {
            // Put on strand to be sequential
            m_strand->post(bind_weak(&DataLoggerManager::addDevicesDoneOnStrand, this, ok, loggerId,
                                     calledDevices, alreadyLoggedDevices));
        }


        void DataLoggerManager::addDevicesDoneOnStrand(bool ok, const std::string& loggerId,
                                                       const std::unordered_set<std::string>& calledDevices,
                                                       const std::vector<std::string>& alreadyLoggedDevices) {
            const std::string serverId(loggerIdToServerId(loggerId));
            Hash& data = m_loggerData.get<Hash>(serverId);

            if (ok) {
                if (alreadyLoggedDevices.empty()) {
                    KARABO_LOG_FRAMEWORK_INFO << "Added '" << toString(calledDevices) << "' to be logged by '"
                            << loggerId << "'";
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
                // It is a failure handler where 'throw' gives us back the exception
                try {
                    throw;
                } catch (const std::exception& e) {
                    // Can happen as timeout when logger just shutdown
                    KARABO_LOG_FRAMEWORK_ERROR << "Failed to add '" << toString(calledDevices) << "' to be logged by '"
                            << loggerId << "' since: " << e.what();
                }
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

            data.set("state", LoggerState::INSTANTIATING);

            // Instantiate logger, but do not yet specify "devicesToBeLogged":
            // Having one channel only to transport this info (slotAddDevicesToBeLogged) simplifies logic.
            const Hash config("directory", get<string>("directory"),
                              "maximumFileSize", get<int>("maximumFileSize"),
                              "flushInterval", get<int>("flushInterval"),
                              "performanceStatistics.enable", get<bool>("enablePerformanceStats"),
                              "useP2p", get<bool>("useP2p"));
            const std::string loggerId(serverIdToLoggerId(serverId));
            const Hash hash("classId", "DataLogger",
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
                if (instanceInfo.has("classId") && instanceInfo.get<std::string>("classId") == "DataLogger") {
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
                    KARABO_LOG_FRAMEWORK_INFO << "Server '" << serverId << "' gone while instantiating DataLogger."; 
                    break;
                case LoggerState::RUNNING:
                    // Looks like a non-graceful shutdown of the server that is detected by lack of heartbeats where
                    // the DeviceClient currently (Karabo 2.6.0) often sends the "gone" signal for the server before
                    // the one of the DataLogger.
                    KARABO_LOG_FRAMEWORK_WARN << "Server '" << serverId << "' gone while DataLogger still alive.";
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

