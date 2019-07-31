/*
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
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

            prepareForLoggerMap(); // throws if loggerMap and serverList are inconsistent

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


        void DataLoggerManager::prepareForLoggerMap() {
            // Check that all servers that are supposed to host DataLoggers are in server list.

            // First get server ids - the values of the logger map.
            std::set<std::string> serversInMap;
            {
                boost::mutex::scoped_lock lock(m_loggerMapMutex); // m_loggerMap must not be changed while we process it
                for (Hash::const_iterator it = m_loggerMap.begin(), itEnd = m_loggerMap.end(); it != itEnd; ++it) {
                    serversInMap.insert(it->getValue<std::string>());
                }
            }
            // Now loop and check - and setup m_loggerData
            const Hash data("state", LoggerState::OFFLINE,
                            "backlog", std::unordered_set<std::string>(),
                            "devices", std::unordered_set<std::string>());
            for (const std::string& serverInMap : serversInMap) {
                if (find(m_serverList.begin(), m_serverList.end(), serverInMap) == m_serverList.end()) {
                    throw KARABO_LOGIC_EXCEPTION("Inconsistent '" + m_loggerMapFile + "' and \"serverList\" configuration: '"
                            + serverInMap + "' is in map, but not in list.");
                }
                m_loggerData.set(serverInMap, data);
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
                    instantiateLogger(instanceId);
                    instantiateReaders(instanceId);
                }
            }
        }


        void DataLoggerManager::newDeviceToLog(const std::string& deviceId) {

            // Figure out which server and thus which logger this runs:
            const std::string serverId(loggerServerId(deviceId, true));

            // See whether the logger is running or not: either put to backlog or tell to log
            Hash& data = m_loggerData.get<Hash>(serverId);
            switch (data.get<LoggerState>("state")) {
                case LoggerState::OFFLINE:
                case LoggerState::INSTANTIATING:
                    data.get<std::unordered_set<std::string> >("backlog").insert(deviceId);
                    break;
                case LoggerState::RUNNING:
                    data.get<std::unordered_set<std::string> >("devices").insert(deviceId);
                    call(serverIdToLoggerId(serverId), "slotAddDevicesToBeLogged", std::vector<std::string>(1, deviceId));
            }
        }


        void DataLoggerManager::newLogger(const std::string& loggerId) {
            const std::string serverId(loggerIdToServerId(loggerId));
            // Get data for this server to access backlog and state
            Hash& data = m_loggerData.get<Hash>(serverId);
            data.set("state", LoggerState::RUNNING);

            std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string> >("backlog");
            if (!backlog.empty()) {
                // After we instantiated, some more devices to log did appear - so tell the logger
                std::unordered_set<std::string>& devices = data.get<std::unordered_set<std::string> >("devices");
                devices.insert(backlog.begin(), backlog.end());

                call(loggerId, "slotAddDevicesToBeLogged", std::vector<std::string>(backlog.begin(), backlog.end()));
                backlog.clear();
            }
        }


        void DataLoggerManager::instantiateLogger(const std::string& serverId) {
            // Get data for this server to access backlog and state
            Hash& data = m_loggerData.get<Hash>(serverId);
            std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string> >("backlog");
            data.set("devices", backlog);

            // Instantiate logger and clear its backlog
            data.set("state", LoggerState::INSTANTIATING);
            const Hash config("devicesToBeLogged", std::vector<std::string>(backlog.begin(), backlog.end()),
                              "directory", get<string>("directory"),
                              "maximumFileSize", get<int>("maximumFileSize"),
                              "flushInterval", get<int>("flushInterval"),
                              "performanceStatistics.enable", get<bool>("enablePerformanceStats"));
            const std::string loggerId(serverIdToLoggerId(serverId));
            const Hash hash("classId", "DataLogger",
                            "deviceId", loggerId,
                            "configuration", config);
            KARABO_LOG_FRAMEWORK_INFO << "Trying to instantiate '" << loggerId << "' on server '" << serverId << "'";
            remote().instantiateNoWait(serverId, hash);
            backlog.clear();
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
                    goneServer(instanceId);
                }
            }
        }


        void DataLoggerManager::goneDeviceToLog(const std::string& deviceId) {
            const std::string serverId(loggerServerId(deviceId, false));
            if (!serverId.empty()) { // else device not in map and thus neither logged
                Hash& data = m_loggerData.get<Hash>(serverId);

                std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string> >("backlog");
                std::unordered_set<std::string>& loggedIds = data.get<std::unordered_set<std::string> >("devices");
                switch (data.get<LoggerState>("state")) {
                    case LoggerState::OFFLINE:
                        backlog.erase(deviceId);
                        if (!loggedIds.empty()) {
                            KARABO_LOG_FRAMEWORK_WARN << "Logged devices for offline server '" << serverId
                                    << "' not empty, but contains " << toString(loggedIds);
                        }
                        break;
                    case LoggerState::INSTANTIATING:
                        loggedIds.erase(deviceId); // filled when switching to INSTANTIATING
                        backlog.erase(deviceId); // could be added while INSTANTIATING
                        break;
                    case LoggerState::RUNNING:
                        loggedIds.erase(deviceId); // backlog should be empty
                        if (!backlog.empty()) {
                            KARABO_LOG_FRAMEWORK_WARN << "Backlog for running server '" << serverId
                                    << "' not empty, but contains " << toString(backlog);
                        }
                        call(serverIdToLoggerId(serverId), "slotTagDeviceToBeDiscontinued", "D", deviceId);
                }
            }
        }


        void DataLoggerManager::goneLogger(const std::string& loggerId) {

            const std::string serverId(loggerId.substr(strlen(DATALOGGER_PREFIX)));

            Hash& data = m_loggerData.get<Hash>(serverId);
            std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string> >("backlog");
            std::unordered_set<std::string>& loggedIds = data.get<std::unordered_set<std::string> >("devices");

            switch (data.get<LoggerState>("state")) {
                case LoggerState::OFFLINE:
                    KARABO_LOG_FRAMEWORK_WARN << "Logger '" << loggerId << "' gone, but its server gone before.";
                    // But nothing more to do, backlog and loggedIds treated in goneServer(..)
                    break;
                case LoggerState::INSTANTIATING:
                    KARABO_LOG_FRAMEWORK_WARN << "Server of '" << loggerId << "' gone while instantiating DataLogger.";
                    // no 'break;'!
                case LoggerState::RUNNING:
                    // Append logged devices to backlog
                    backlog.insert(loggedIds.begin(), loggedIds.end());
                    loggedIds.clear();
                    // Instantiate again with backlog -- will set "state" appropriately
                    instantiateLogger(serverId);
            }
        }


        void DataLoggerManager::goneServer(const std::string & serverId) {
            Hash& data = m_loggerData.get<Hash>(serverId);

            switch (data.get<LoggerState>("state")) {
                case LoggerState::OFFLINE:
                    KARABO_LOG_FRAMEWORK_ERROR << "Server '" << serverId << "' gone, but it was already gone before: "
                            << data;
                    // Weird situation - move "devices" to "backlog" as in other cases...
                    break;
                case LoggerState::INSTANTIATING:
                    // Expected nice behaviour: Already took note that logger is gone and so tried to start again.
                    // So we have to move "devices" to "backlog" again.
                    KARABO_LOG_FRAMEWORK_INFO << "Server '" << serverId << "' gone while instantiating DataLogger.";
                    break;
                case LoggerState::RUNNING:
                    // Looks like a non-graceful shutdown of the server that is detected by lack of heartbeats where
                    // the DeviceClient currently (Karabo 2.5.1) often sends the "gone" signal for the server before
                    // the one of the DataLogger.
                    KARABO_LOG_FRAMEWORK_WARN << "Server '" << serverId << "' gone while DataLogger still alive.";
                    // Also then we have to move "devices" to "backlog".
                    break;
            }

            // Append logged devices to backlog
            std::unordered_set<std::string>& backlog = data.get<std::unordered_set<std::string> >("backlog");
            std::unordered_set<std::string>& loggedIds = data.get<std::unordered_set<std::string> >("devices");
            backlog.insert(loggedIds.begin(), loggedIds.end());
            loggedIds.clear();

            data.set("state", LoggerState::OFFLINE);
        }
    }
}

