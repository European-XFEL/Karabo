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

#include "karabo/io/Input.hh"
#include "karabo/io/FileTools.hh"

#include "DataLoggerManager.hh"

#define DATALOGGER_PREFIX "DataLogger-"
#define DATALOGREADER_PREFIX "DataLogReader"
#define DATALOGREADERS_PER_SERVER 2

namespace karabo {
    namespace devices {

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

            VECTOR_STRING_ELEMENT(expected).key("serverList")
                    .displayedName("Server list")
                    .description("List of device server IDs where the DataLogger instance run. "
                                 "The load balancing is round-robin. If empty, try to get from logger map "
                                 "or, as last source, just use the server of this device.")
                    .init()
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
        }


        DataLoggerManager::DataLoggerManager(const Hash& input)
            : karabo::core::Device<karabo::core::OkErrorFsm>(input),
            m_serverList(input.get<vector<string> >("serverList")),
            m_serverIndex(0), m_loggerMapFile("loggermap.xml") {
            m_loggerMap.clear();
            if (boost::filesystem::exists(m_loggerMapFile)) {
                karabo::io::loadFromFile(m_loggerMap, m_loggerMapFile);
            }

            KARABO_SYSTEM_SIGNAL("signalLoggerMap", Hash /*loggerMap*/);
            KARABO_SLOT(slotGetLoggerMap);
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

            remote().registerInstanceNewMonitor(boost::bind(&DataLoggerManager::ensureLoggerRunning, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&DataLoggerManager::instanceGoneHandler, this, _1, _2));

            // try to restart readers and loggers if needed - it works reliably on "stopped" system
            restartReadersAndLoggers();

            // Publish logger map read from disc. Do that as late as possible in the initialisation procedure
            // to give those interested the chance to register their slots after we sent signalInstanceNew.
            emit<Hash>("signalLoggerMap", m_loggerMap);
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
            for (int i = 0; i < DATALOGREADERS_PER_SERVER; i++) {
                const std::string readerId = DATALOGREADER_PREFIX + toString(i) + "-" + serverId;
                if (!remote().exists(readerId).first) {
                    const Hash hash("classId", "DataLogReader", "deviceId", readerId,
                                    "configuration.directory", get<string>("directory"));
                    remote().instantiateNoWait(serverId, hash);
                    KARABO_LOG_FRAMEWORK_INFO << "instantiateReaders: reader '" << readerId << "' started on server '" << serverId << "'";
                }
            }
        }


        void DataLoggerManager::slotGetLoggerMap() {
            boost::mutex::scoped_lock lock(m_loggerMapMutex); // m_loggerMap must not be changed while we process it
            reply(m_loggerMap);
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
                                    // Cannot happen (see okStateOnEntry), but for better diagnostics in case it does:
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
                            remote().instantiateNoWait(serverId, hash);
                            KARABO_LOG_FRAMEWORK_INFO << "ensureLoggerRunning [device] : logger '" << loggerId << "' STARTED";
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
                        // Now, without mutex lock, treat the collected deviceIds ('exists' can take long...)
                        const Hash config("directory", get<string>("directory"),
                                          "maximumFileSize", get<int>("maximumFileSize"),
                                          "flushInterval", get<int>("flushInterval"));
                        Hash hash("classId", "DataLogger", "configuration", config);


                        BOOST_FOREACH(const std::string& deviceId, devicesToLog) {
                            const string loggerId = DATALOGGER_PREFIX + deviceId;
                            // Check if loggerId already started based on real-time
                            if (remote().exists(deviceId).first && !remote().exists(loggerId).first) {
                                hash.set("deviceId", loggerId);
                                hash.set("configuration.deviceToBeLogged", deviceId);
                                remote().instantiateNoWait(serverId, hash);
                                KARABO_LOG_FRAMEWORK_INFO << "ensureLoggerRunning [server] : logger '" << loggerId << "' STARTED";
                            }
                        }
                    }
                }

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "In ensureLoggerRunning:\n" << e;
            } catch (const std::exception& e) {
                KARABO_LOG_ERROR << "In ensureLoggerRunning: " << e.what() << ".";
            } catch (...) {
                KARABO_LOG_ERROR << "Unknown exception in ensureLoggerRunning.";
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

            if (type == "device") {
                const string loggerId = DATALOGGER_PREFIX + instanceId;
                const vector<string> onlineDevices = remote().getDevices();

                bool deviceExists = std::find(onlineDevices.begin(), onlineDevices.end(), instanceId) != onlineDevices.end(); //remote().exists(instanceId).first;
                bool loggerExists = std::find(onlineDevices.begin(), onlineDevices.end(), loggerId) != onlineDevices.end(); //remote().exists(loggerId).first;

                // Safety check
                if (!deviceExists) {
                    try {
                        if (loggerExists) {
                            this->call(loggerId, "slotTagDeviceToBeDiscontinued", true, 'D');
                            remote().killDeviceNoWait(loggerId);
                        }
                    } catch (const Exception& e) {
                        KARABO_LOG_ERROR << e;
                    }
                }
            }
        }
    }
}
