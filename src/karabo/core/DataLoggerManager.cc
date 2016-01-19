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
#include <boost/algorithm/string.hpp>
#include <karabo/io/Input.hh>
#include "DataLoggerManager.hh"
#include "karabo/io/FileTools.hh"

#define DATALOGGER_PREFIX "DataLogger-"
#define DATALOGREADER_PREFIX "DataLogReader"
#define DATALOGREADERS_PER_SERVER 2

namespace karabo {
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<karabo::core::OkErrorFsm>, DataLoggerManager)


        void DataLoggerManager::expectedParameters(Schema& expected) {

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

            std::vector<std::string> emptyVec;

            VECTOR_STRING_ELEMENT(expected).key("serverList")
                    .displayedName("Server list")
                    .description("List of device server IDs where the DataLogger instance run. "
                    "The load balancing is round-robin. If empty, try to get from logger map "
                    "or, as last source, just use the server of this device.")
                    .init()
                    .assignmentOptional().defaultValue(emptyVec)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(4)
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
        }

        DataLoggerManager::DataLoggerManager(const Hash& input)
        : karabo::core::Device<karabo::core::OkErrorFsm>(input),
                m_serverList(input.get<vector<string> >("serverList")),
                m_serverIndex(0), m_saved(false)
        {
            const string filename("loggermap.xml");
            //set<int>("nThreads", 10);
            m_loggerMap.clear();
            if (boost::filesystem::exists(filename)) {
                karabo::io::loadFromFile(m_loggerMap, filename);
            }

        }

        DataLoggerManager::~DataLoggerManager() {
            KARABO_LOG_INFO << "dead.";
        }

        void DataLoggerManager::okStateOnEntry() {

            // Switch on the heartbeat tracking 
            trackAllInstances();
            // First call : trigger the process of gathering the info about network presence
            remote().getSystemInformation();

            // Register handlers here: it will switch on multi-threading!

            remote().registerInstanceNewMonitor(boost::bind(&DataLoggerManager::instanceNewHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&DataLoggerManager::instanceGoneHandler, this, _1, _2));

            KARABO_SYSTEM_SIGNAL("signalLoggerMap", Hash /*loggerMap*/);
            KARABO_SLOT(slotGetLoggerMap);

            // Server list must not be empty. If it is configured to be empty,
            // try other sources: our logger map or, if that is empty as well,
            // just use our server.
            if (m_serverList.empty()) {
                m_loggerMap.getPaths(m_serverList); // or just getKeys(..)?
                if (m_serverList.empty()) {
                    m_serverList.push_back(this->getServerId());
                }
                // Keep device property in sync:
                this->set("serverList", m_serverList);
            }

            // try to restart readers and loggers if needed - it works reliably on "stopped" system
            restartReadersAndLoggers();
        }

        void DataLoggerManager::restartReadersAndLoggers() {
            Hash runtimeInfo = remote().getSystemInformation();

            KARABO_LOG_FRAMEWORK_DEBUG << "restartReadersAndLoggers: runtime system information ...\n" << runtimeInfo;

            boost::mutex::scoped_lock lock(m_handlerMutex);

            if (runtimeInfo.has("server")) {
                // Start DataLogReaders on all DataLogger device servers
                for (vector<string>::iterator ii = m_serverList.begin(); ii != m_serverList.end(); ii++) {
                    string serverId = *ii;
                    const Hash& onlineServers = runtimeInfo.get<Hash>("server");
                    if (!onlineServers.has(serverId)) continue;
                    instantiateReaders(serverId);

                    if (!runtimeInfo.has("device")) continue;
                    const Hash& onlineDevices = runtimeInfo.get<Hash>("device");
                    for (Hash::const_map_iterator i = onlineDevices.mbegin(); i != onlineDevices.mend(); ++i) {
                        string deviceId = i->first;

                        // check if deviceId should be archived ...
                        const Hash::Node& node = i->second;
                        if (!node.hasAttribute("archive") || (node.getAttribute<bool>("archive") == false)) continue;

                        string loggerId = DATALOGGER_PREFIX + deviceId;

                        // Check if loggerId is valid ID
                        if (!m_loggerMap.has(loggerId)) continue;
                        string srv = m_loggerMap.get<string>(loggerId);
                        // Check if loggerId belongs to this DataLoggerServer
                        if (srv != serverId) continue;
                        // Check if loggerId already started
                        if (remote().exists(deviceId).first && !remote().exists(loggerId).first) {
                            Hash config;
                            config.set("DataLogger.deviceId", loggerId);
                            config.set("DataLogger.deviceToBeLogged", deviceId);
                            config.set("DataLogger.directory", get<string>("directory"));
                            config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                            config.set("DataLogger.flushInterval", get<int>("flushInterval"));
                            remote().instantiateNoWait(serverId, config);
                            KARABO_LOG_FRAMEWORK_INFO << "restartReadersAndLoggers : logger \"" << loggerId << "\" STARTED";
                        }
                    }
                }
            }
        }

        void DataLoggerManager::instantiateReaders(const std::string& serverId) {
            std::vector<std::string> devices = remote().getDevices(serverId);
            for (int i = 0; i < DATALOGREADERS_PER_SERVER; i++) {
                std::string readerId = DATALOGREADER_PREFIX + toString(i) + "-" + serverId;
                if (!remote().exists(readerId).first) {
                    Hash config;
                    config.set("DataLogReader.deviceId", readerId);
                    config.set("DataLogReader.directory", get<string>("directory"));
                    remote().instantiateNoWait(serverId, config);
                    KARABO_LOG_FRAMEWORK_INFO << "instantiateReaders: reader \"" << readerId << "\" started on server \"" << serverId << "\"";
                }
            }
        }

        void DataLoggerManager::slotGetLoggerMap() {
            reply(m_loggerMap);
        }

        void DataLoggerManager::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
            try {
                const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                // const ref is fine even for temporary std::string
                const std::string& instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                                                 topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));
                KARABO_LOG_FRAMEWORK_INFO << "instanceNewHandler --> instanceId: '" << instanceId
                        << "', type: '" << type << "'";

                if (type == "device") { // Take out only devices for the time being
                    const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                    const string& deviceId = instanceId;

                    // Check if the device should be archived 
                    if (entry.hasAttribute(deviceId, "archive") && (entry.getAttribute<bool>(deviceId, "archive") == true)) {
                        string loggerId = DATALOGGER_PREFIX + deviceId;

                        vector<string> onlineDevices = remote().getDevices();

                        bool deviceExists = std::find(onlineDevices.begin(), onlineDevices.end(), deviceId) != onlineDevices.end();
                        bool loggerExists = std::find(onlineDevices.begin(), onlineDevices.end(), loggerId) != onlineDevices.end();

                        boost::mutex::scoped_lock lock(m_handlerMutex);
                        if (deviceExists) {
                            if (loggerExists) {
                                // Device was dead and came back so quickly that we did not notice
                                // => just re-establish the connections.
                                connect(deviceId, "signalChanged", loggerId, "slotChanged");
                                connect(deviceId, "signalStateChanged", loggerId, "slotChanged");
                            } else {
                                string serverId;
                                if (m_loggerMap.has(loggerId)) {
                                    serverId = m_loggerMap.get<string>(loggerId);
                                } else {
                                    if (m_serverList.empty()) {
                                        // Cannot happen (see okStateOnEntry), but for better diagnostics in case it does:
                                        throw KARABO_LOGIC_EXCEPTION("List of servers for data logging is empty.");
                                    }
                                    m_serverIndex %= m_serverList.size();
                                    serverId = m_serverList[m_serverIndex++];
                                    m_loggerMap.set(loggerId, serverId);
                                    emit<Hash>("signalLoggerMap", m_loggerMap);
                                    m_saved = false;
                                }

                                Hash config;
                                config.set("DataLogger.deviceId", loggerId);
                                config.set("DataLogger.deviceToBeLogged", deviceId);
                                config.set("DataLogger.directory", get<string>("directory"));
                                config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                                config.set("DataLogger.flushInterval", get<int>("flushInterval"));
                                remote().instantiateNoWait(serverId, config);
                                KARABO_LOG_FRAMEWORK_INFO << "instanceNewHandler [device] : logger \"" << loggerId << "\" STARTED";
                            }
                        }
                    }
                } else if (type == "server") {
                    const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                    const string& serverId = instanceId;
                    if (find(m_serverList.begin(), m_serverList.end(), serverId) != m_serverList.end()) {
                        instantiateReaders(serverId);

                        Hash runtimeInfo = remote().getSystemInformation();
                        boost::mutex::scoped_lock lock(m_handlerMutex);
                        if (!runtimeInfo.has("device")) return;
                        const Hash& onlineDevices = runtimeInfo.get<Hash>("device");
                        for (Hash::const_map_iterator i = onlineDevices.mbegin(); i != onlineDevices.mend(); ++i) {
                            string deviceId = i->first;

                            // check if deviceId should be archived ...
                            const Hash::Node& node = i->second;
                            if (!node.hasAttribute("archive") || (node.getAttribute<bool>("archive") == false)) continue;

                            string loggerId = DATALOGGER_PREFIX + deviceId;

                            // Check if loggerId is valid ID
                            if (!m_loggerMap.has(loggerId)) continue;
                            string srv = m_loggerMap.get<string>(loggerId);
                            // Check if loggerId belongs to this DataLoggerServer
                            if (srv != serverId) continue;
                            // Check if loggerId already started based on real-time
                            if (remote().exists(deviceId).first && !remote().exists(loggerId).first) {
                                Hash config;
                                config.set("DataLogger.deviceId", loggerId);
                                config.set("DataLogger.deviceToBeLogged", deviceId);
                                config.set("DataLogger.directory", get<string>("directory"));
                                config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                                config.set("DataLogger.flushInterval", get<int>("flushInterval"));
                                remote().instantiateNoWait(serverId, config);
                                KARABO_LOG_FRAMEWORK_INFO << "instanceNewHandler [server] : logger \"" << loggerId << "\" STARTED";
                            }
                        }
                    }
                }

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "In instanceNewHandler:\n" << e;
            } catch (const std::exception& e) {
                KARABO_LOG_ERROR << "In instanceNewHandler: " << e.what() << ".";
            } catch (...) {
                KARABO_LOG_ERROR << "Unknown exception in instanceNewHandler.";
            }
        }

        void DataLoggerManager::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            // const ref is fine even for temporary std::string
            const std::string& type = (instanceInfo.has("type") && instanceInfo.is<std::string>("type") ?
                                       instanceInfo.get<std::string>("type") : std::string("unknown"));
            const std::string& serverId = (instanceInfo.has("serverId") && instanceInfo.is<std::string>("serverId") ?
                                           instanceInfo.get<string>("serverId") : std::string("?"));

            KARABO_LOG_FRAMEWORK_INFO << "instanceGoneHandler -->  instanceId : \""
                    << instanceId << "\", type : " << type << " on server \"" << serverId << "\"";

            if (type == "device") {
                string loggerId = DATALOGGER_PREFIX + instanceId;
                vector<string> onlineDevices = remote().getDevices();

                bool deviceExists = std::find(onlineDevices.begin(), onlineDevices.end(), instanceId) != onlineDevices.end(); //remote().exists(instanceId).first;
                bool loggerExists = std::find(onlineDevices.begin(), onlineDevices.end(), loggerId) != onlineDevices.end(); //remote().exists(loggerId).first;

                // Safety check
                if (!deviceExists) {
                    try {
                        string loggerId = DATALOGGER_PREFIX + instanceId;
                        if (loggerExists) {
                            this->call(loggerId, "slotTagDeviceToBeDiscontinued", true, 'D');
                            remote().killDeviceNoWait(loggerId);
                            if (!m_saved) {
                                string filename = "loggermap.xml";
                                karabo::io::saveToFile(m_loggerMap, filename);
                                m_saved = true;
                            }
                        }
                    } catch (const Exception& e) {
                        KARABO_LOG_ERROR << e;
                    }
                }
            }
        }
    }
}
