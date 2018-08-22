/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <map>
#include <vector>
#include <string>
#include <algorithm>    // std::find
#include <string.h>     // strlen

#include <boost/algorithm/string.hpp>
#include <boost/thread/pthread/mutex.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "karabo/io/Input.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/util/DataLogUtils.hh"

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
                    .assignmentOptional().defaultValue(40)
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

            // Slow beats
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();

            UINT32_ELEMENT(expected).key("instantiationDelay")
                    .displayedName("Instantiation Delay")
                    .description("Time to wait between dataloggers instantiation."
                                 "If it is 0 no wait is done. "
                                 "WARNING: if a value greater than 0 but lower than 10 ms is spevcified, 10 ms will be used"
                                 "NOTE: on a single datalogger server delay time will be instantiationDelay * n_of_servers")
                    .unit(karabo::util::Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                    .adminAccess()
                    .reconfigurable()
                    .minInc(0).maxInc(1000)
                    .assignmentOptional().defaultValue(200)
                    .commit();
        }

        DataLoggerManager::DataLoggerManager(const Hash& input)
        : karabo::core::Device<>(input),
        m_serverList(input.get<vector<string> >("serverList")),
            m_serverIndex(0), m_loggerMapFile("loggermap.xml"),
            m_instantiateDelayTimer(boost::asio::deadline_timer(karabo::net::EventLoop::getIOService())) {
            m_loggerMap.clear();
            if (boost::filesystem::exists(m_loggerMapFile)) {
                karabo::io::loadFromFile(m_loggerMap, m_loggerMapFile);
            }

            KARABO_SYSTEM_SIGNAL("signalLoggerMap", Hash /*loggerMap*/);
            KARABO_SLOT(slotGetLoggerMap);

            KARABO_INITIAL_FUNCTION(initialize);
        }

        DataLoggerManager::~DataLoggerManager() {
            KARABO_LOG_INFO << "dead.";
        }

        void DataLoggerManager::initialize() {

            checkLoggerMap(); // throws if loggerMap and serverList are inconsistent

            for (const std::string& serverId : m_serverList) {
                m_instantiationQueues.emplace(serverId, std::deque<karabo::util::Hash>());
            }

            // Switch on instance tracking
            remote().enableInstanceTracking();

            // Register handlers here: it will switch on multi-threading!

            remote().registerInstanceNewMonitor(boost::bind(&DataLoggerManager::ensureLoggerRunning, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&DataLoggerManager::instanceGoneHandler, this, _1, _2));

            // try to restart readers and loggers if needed - it works reliably on "stopped" system
            restartReadersAndLoggers();

            // Publish logger map read from disc. Do that as late as possible in the initialization procedure
            // to give those interested the chance to register their slots after we sent signalInstanceNew.
            {
                boost::mutex::scoped_lock lock(m_loggerMapMutex); // m_loggerMap must not be changed while we process it
                emit<Hash>("signalLoggerMap", m_loggerMap);
            }

            m_instantiateDelayTimer.expires_from_now(boost::posix_time::milliseconds(calcInstantiationTimerDelay()));
            m_instantiateDelayTimer.async_wait(karabo::util::bind_weak(&DataLoggerManager::doInstantiateHandler, this, boost::asio::placeholders::error, m_instantiationQueues.begin()));

            updateState(State::NORMAL);
        }

        void DataLoggerManager::preDestruction() {
            // cancel all timers
            m_instantiateDelayTimer.cancel();
        }

        void DataLoggerManager::checkLoggerMap() {
            // Check that all servers that are supposed to host DataLoggers are in server list.

            // First get server ids - the values of the logger map.
            std::set<std::string> serversInMap;
            {
                boost::mutex::scoped_lock lock(m_loggerMapMutex); // m_loggerMap must not be changed while we process it
                for (Hash::const_iterator it = m_loggerMap.begin(), itEnd = m_loggerMap.end(); it != itEnd; ++it) {
                    serversInMap.insert(it->getValue<std::string>());
                }
            }
            // Now loop and check
            for (const std::string& serverInMap : serversInMap) {
                if (find(m_serverList.begin(), m_serverList.end(), serverInMap) == m_serverList.end()) {
                    throw KARABO_LOGIC_EXCEPTION("Inconsistent '" + m_loggerMapFile + "' and \"serverList\" configuration: '"
                            + serverInMap + "' is in map, but not in list.");
                }
            }
        }

        void DataLoggerManager::restartReadersAndLoggers() {
            const Hash runtimeInfo = remote().getSystemInformation();

            KARABO_LOG_FRAMEWORK_DEBUG << "restartReadersAndLoggers: runtime system information ...\n" << runtimeInfo;

            if (runtimeInfo.has("server")) {
                const Hash& onlineServers = runtimeInfo.get<Hash>("server");
                // Start DataLogReaders on all DataLogger device servers
                for (vector<string>::iterator ii = m_serverList.begin(); ii != m_serverList.end(); ii++) {
                    const string& serverId = *ii;
                    if (!onlineServers.has(serverId)) continue;
                    instantiateReaders(serverId);
                }

                // Now start loggers for online devices - ensureLoggerRunning checks whether they exist already
                if (!runtimeInfo.has("device")) return;
                const Hash& onlineDevices = runtimeInfo.get<Hash>("device");
                for (Hash::const_iterator i = onlineDevices.begin(); i != onlineDevices.end(); ++i) {
                    const Hash::Node& deviceNode = *i;
                    // Topology entry as understood by ensureLoggerRunning: Hash with path "device.<deviceId>"
                    Hash topologyEntry("device", Hash());
                    // Copy node with key "<deviceId>" and attributes into the single Hash in topologyEntry:
                    topologyEntry.begin()->getValue<Hash>().setNode(deviceNode);
                    ensureLoggerRunning(topologyEntry);
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

        void DataLoggerManager::delayedInstantiation(const std::string serverId, const karabo::util::Hash& hash) {
            if (get<unsigned int>("instantiationDelay") <= 0) {
                remote().instantiateNoWait(serverId, hash);
            } else {

                {
                    boost::mutex::scoped_lock lock(m_instantiateMutex);
                    m_instantiationQueues[serverId].push_back(hash);
                }
            }
        }


        void DataLoggerManager::doInstantiateHandler(const boost::system::error_code& error, std::unordered_map<std::string, std::deque<karabo::util::Hash>>::iterator queueMapIter) {
            m_instantiateDelayTimer.expires_from_now(boost::posix_time::milliseconds(calcInstantiationTimerDelay())); // re-arm timer

            const std::string& serverId = queueMapIter->first;
            std::deque < karabo::util::Hash>& queue = queueMapIter->second;

            {
                boost::mutex::scoped_lock lock(m_instantiateMutex);
                if (!queue.empty()) {
                    karabo::util::Hash cfg = queue.front();

                    queue.pop_front();
                    remote().instantiateNoWait(serverId, cfg);
                }
            }

            if (++queueMapIter == m_instantiationQueues.end()) {
                queueMapIter = m_instantiationQueues.begin();
            }

            m_instantiateDelayTimer.async_wait(karabo::util::bind_weak(&DataLoggerManager::doInstantiateHandler, this, boost::asio::placeholders::error, queueMapIter));
            // if timer is already expired when async_wait() is called, doInstantateHandler is called immediately
        }


        unsigned int DataLoggerManager::calcInstantiationTimerDelay() {
            //if delay is less than 10 ms, set it at 10
            unsigned int delay = get<unsigned int>("instantiationDelay");
            delay = (delay < 10) ? 10 : delay;
            return delay;
        }

        void DataLoggerManager::ensureLoggerRunning(const karabo::util::Hash& topologyEntry) {
            try {
                const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                // const ref is fine even for temporary std::string
                const std::string& instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                        topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));
                KARABO_LOG_FRAMEWORK_INFO << "ensureLoggerRunning --> instanceId: '" << instanceId
                        << "', type: '" << type << "'";

                if (type == "device") { // Take out only devices for the time being
                    const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                    const string& deviceId = instanceId;

                    // Check if the device should be archived 
                    if (entry.hasAttribute(deviceId, "archive") && (entry.getAttribute<bool>(deviceId, "archive") == true)) {
                        const string loggerId = DATALOGGER_PREFIX + deviceId;

                        vector<string> onlineDevices = remote().getDevices();

                        bool deviceExists = std::find(onlineDevices.begin(), onlineDevices.end(), deviceId) != onlineDevices.end();
                        bool loggerExists = std::find(onlineDevices.begin(), onlineDevices.end(), loggerId) != onlineDevices.end();

                        boost::mutex::scoped_lock lock(m_loggerMapMutex);
                        if (deviceExists && !loggerExists) {
                            string serverId;
                            bool newMap = false;
                            if (m_loggerMap.has(loggerId)) {
                                serverId = m_loggerMap.get<string>(loggerId);
                            } else {
                                if (m_serverList.empty()) {
                                    // Cannot happen but for better diagnostics in case it does:
                                    throw KARABO_PARAMETER_EXCEPTION("List of servers for data logging is empty."
                                            " You have to define one data logger server, at least!");
                                }
                                m_serverIndex %= m_serverList.size();
                                serverId = m_serverList[m_serverIndex++];
                                m_loggerMap.set(loggerId, serverId);
                                emit<Hash>("signalLoggerMap", m_loggerMap);
                                newMap = true;
                            }
                            const Hash config("deviceToBeLogged", deviceId,
                                    "directory", get<string>("directory"),
                                    "maximumFileSize", get<int>("maximumFileSize"),
                                    "flushInterval", get<int>("flushInterval"));
                            const Hash hash("classId", "DataLogger", "deviceId", loggerId, "configuration", config);
                            KARABO_LOG_FRAMEWORK_INFO << "Trying to instantiate '" << loggerId << "' on server '"
                                    << serverId << "' since device '" << deviceId << "' appeared (or its logger died)";

                            delayedInstantiation(serverId, hash);

                            // First instantiate the new logger - now we have time to update the logger map file.
                            if (newMap) {
                                karabo::io::saveToFile(m_loggerMap, m_loggerMapFile);
                            }
                        }
                    }
                } else if (type == "server") {
                    const string& serverId = instanceId;
                    if (find(m_serverList.begin(), m_serverList.end(), serverId) != m_serverList.end()) {
                        instantiateReaders(serverId);

                        const Hash runtimeInfo = remote().getSystemInformation();
                        if (!runtimeInfo.has("device")) return;
                        const Hash& onlineDevices = runtimeInfo.get<Hash>("device");

                        // Collect (under mutex lock) deviceIds for which we have to start a logger on serverId
                        std::vector<std::string> devicesToLog;
                        {
                            boost::mutex::scoped_lock lock(m_loggerMapMutex);
                            for (Hash::const_map_iterator i = onlineDevices.mbegin(); i != onlineDevices.mend(); ++i) {

                                // check if deviceId should be archived ...
                                const Hash::Node& node = i->second;
                                if (!node.hasAttribute("archive") || (node.getAttribute<bool>("archive") == false)) continue;

                                const string& deviceId = i->first;
                                const string loggerId = DATALOGGER_PREFIX + deviceId;

                                // Check if loggerId is valid ID
                                if (!m_loggerMap.has(loggerId)) continue;
                                const string& srv = m_loggerMap.get<string>(loggerId);
                                // Check if loggerId belongs to this DataLoggerServer
                                if (srv != serverId) continue;
                                devicesToLog.push_back(deviceId);
                            }
                        }
                        // Now, without mutex lock, treat the collected deviceIds (to keep mutex lock short)
                        const Hash config("directory", get<string>("directory"),
                                "maximumFileSize", get<int>("maximumFileSize"),
                                "flushInterval", get<int>("flushInterval"));
                        Hash hash("classId", "DataLogger", "configuration", config);

                        BOOST_FOREACH(const std::string& deviceId, devicesToLog) {
                            const string loggerId = DATALOGGER_PREFIX + deviceId;
                            // No need to check whether loggerId already exists: if yes, this instantiation will fail
                            hash.set("deviceId", loggerId);
                            hash.set("configuration.deviceToBeLogged", deviceId);
                            KARABO_LOG_FRAMEWORK_INFO << "Trying to instantiate '" << loggerId << "' on server '" << serverId
                                    << "' which just appeared";
                            delayedInstantiation(serverId, hash);
                        }
                    }
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "In ensureLoggerRunning: " << e.what();
            }
        }

        void DataLoggerManager::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            // const ref is fine even for temporary std::string
            const std::string& type = (instanceInfo.has("type") && instanceInfo.is<std::string>("type") ?
                    instanceInfo.get<std::string>("type") : std::string("unknown"));
            const std::string& serverId = (instanceInfo.has("serverId") && instanceInfo.is<std::string>("serverId") ?
                    instanceInfo.get<string>("serverId") : std::string("?"));

            KARABO_LOG_FRAMEWORK_INFO << "instanceGoneHandler -->  instanceId : '"
                    << instanceId << "', type : " << type << " on server '" << serverId << "'";
            try {
                if (type == "device") {
                    const vector<string> onlineDevices = remote().getDevices();
                    boost::optional<const Hash::Node&> classIdNode = instanceInfo.find("classId");
                    if (classIdNode && classIdNode->is<std::string>() && classIdNode->getValue<std::string>() == "DataLogger"
                            && instanceId.find(DATALOGGER_PREFIX) == 0) {
                        // A DataLogger with the expected prefix - check whether its logged device is still running:
                        const std::string loggedId(instanceId.substr(strlen(DATALOGGER_PREFIX))); // cut prefix
                        const bool loggedExists = std::find(onlineDevices.begin(), onlineDevices.end(), loggedId) != onlineDevices.end();

                        if (loggedExists) {
                            // Logged device still online - restart logger:
                            const Hash runtimeInfo = remote().getSystemInformation();
                            const Hash::Node& deviceNode = runtimeInfo.getNode("device." + loggedId);
                            // Topology entry as understood by ensureLoggerRunning (see also restartReadersAndLoggers):
                            // Hash with path "device.<deviceId>"
                            Hash topologyEntry("device", Hash());
                            // Copy node with key "<deviceId>" and attributes into the single Hash in topologyEntry:
                            topologyEntry.begin()->getValue<Hash>().setNode(deviceNode);
                            // NOTE: This will trigger (the try of) instantiation of the logger, even if it was gone
                            //       due to a clean shutdown of the logger server. But that does not harm, the
                            //       instantiation is not run on the server being shut down: Thanks to util::bind_weak,
                            //       the server does not listen anymore when its destructor runs.
                            // NOTE 2: If the automatic restart of the logger is not the desired behaviour, one can
                            //         set the 'archive' flag of the logged device to 'false'.
                            ensureLoggerRunning(topologyEntry);
                        }
                    } else {
                        const string loggerId = DATALOGGER_PREFIX + instanceId;

                        const bool deviceExists = std::find(onlineDevices.begin(), onlineDevices.end(), instanceId) != onlineDevices.end();
                        const bool loggerExists = std::find(onlineDevices.begin(), onlineDevices.end(), loggerId) != onlineDevices.end();

                        // Safety check
                        if (!deviceExists) {
                            if (loggerExists) {
                                this->call(loggerId, "slotTagDeviceToBeDiscontinued", true, 'D');
                                remote().killDeviceNoWait(loggerId);
                            } else {
                                //check if logger is still in some queue to be instantiated
                                boost::mutex::scoped_lock lock(m_instantiateMutex);
                                for (auto &q : m_instantiationQueues) {
                                    for (auto it = q.second.begin(); it != q.second.end();) {
                                        const std::string& loggedId = it->get<std::string>("configuration.deviceToBeLogged");

                                        //erase item from queue if loggedId matches instanceId
                                        if (loggedId == instanceId) {
                                            it = q.second.erase(it); // erase return iterator pointing to next element
                                        } else {
                                            ++it;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "In instanceGoneHandler: " << e.what();
            }
        }
    }
}
