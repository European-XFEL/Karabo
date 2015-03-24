/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <map>
#include <boost/algorithm/string.hpp>
#include <karabo/io/Input.hh>
#include "DataLoggerManager.hh"
#include "karabo/io/FileTools.hh"

#define DATALOGGER_PREFIX "DataLogger-"
#define DATALOGREADER_PREFIX "DataLogReader-"

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

            std::vector<std::string> emptyVec;

            VECTOR_STRING_ELEMENT(expected).key("serverList")
                    .displayedName("Server list")
                    .description("List of device server IDs where the DataLogger instance run. The load balancing is round-robin.")
                    .init()
                    .assignmentOptional().defaultValue(emptyVec)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_DataLoggerManager_0")
                    .commit();

            // Slow beats
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();
        }

        DataLoggerManager::DataLoggerManager(const Hash& input) : karabo::core::Device<karabo::core::OkErrorFsm>(input) {
            string filename = "loggermap.xml";
            //set<int>("nThreads", 10);
            m_serverList = input.get<vector<string> >("serverList");
            m_serverIndex = 0;
            m_saved = false;
            m_loggerMap.clear();
            if (boost::filesystem::exists(filename)) {
                karabo::io::loadFromFile(m_loggerMap, filename);
            }
        }

        DataLoggerManager::~DataLoggerManager() {
            KARABO_LOG_INFO << "dead.";
        }

        void DataLoggerManager::okStateOnEntry() {
            // Register handlers
            remote().registerInstanceNewMonitor(boost::bind(&DataLoggerManager::instanceNewHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&DataLoggerManager::instanceGoneHandler, this, _1, _2));

            KARABO_SIGNAL("signalLoggerMap", Hash /*loggerMap*/);
            KARABO_SLOT(slotGetLoggerMap);

            // Check if the local server where DataLoggerManager is running is in the server list
            {
                bool takenIntoAccount = false;
                for (vector<string>::iterator ii = m_serverList.begin(); ii != m_serverList.end(); ii++) {
                    if (*ii == getServerId()) {
                        takenIntoAccount = true;
                        break;
                    }
                }
                if (!takenIntoAccount) m_serverList.push_back(getServerId());
            }

            // Start DataLogReaders on all DataLogger device servers
            for (vector<string>::iterator ii = m_serverList.begin(); ii != m_serverList.end(); ii++) {
                string serverId = *ii;
                Hash config;
                config.set("DataLogReader.deviceId", DATALOGREADER_PREFIX + serverId);
                config.set("DataLogReader.directory", "karaboHistory");
                remote().instantiateNoWait(serverId, config);
            }
            
            
            // Get all current instances in the system
            const Hash& systemTopology = remote().getSystemTopology();
            {
                boost::optional<const Hash::Node&> node = systemTopology.find("device");
                if (node) {
                    const Hash& devices = node->getValue<Hash>();
                    for (Hash::const_iterator it = devices.begin(); it != devices.end(); ++it) { // Loop all devices ...
                        // ... but consider only those to be archived
                        if (it->hasAttribute("archive") && (it->getAttribute<bool>("archive") == true)) {
                            const std::string& deviceId = it->getKey();
                            if (deviceId == m_instanceId) continue; // Skip myself

                            // Check if deviceId is known in the world
                            string loggerId = DATALOGGER_PREFIX + deviceId;
                            {
                                string serverId;
                                if (m_loggerMap.has(loggerId)) {
                                    serverId = m_loggerMap.get<string>(loggerId);
                                } else {
                                    m_serverIndex %= m_serverList.size();
                                    serverId = m_serverList[m_serverIndex++];
                                    m_loggerMap.set(loggerId, serverId);
                                    m_saved = false;
                                }
                                Hash config;
                                config.set("DataLogger.deviceId", loggerId);
                                config.set("DataLogger.deviceToBeLogged", deviceId);
                                config.set("DataLogger.directory", "karaboHistory");
                                config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                                config.set("DataLogger.flushInterval", get<int>("flushInterval"));
                                remote().instantiateNoWait(serverId, config);
                            }
                        }
                    }
                }
            }
            
            emit("signalLoggerMap", m_loggerMap);
        }

        void DataLoggerManager::slotGetLoggerMap() {
            reply(m_loggerMap);
        }
        
        void DataLoggerManager::instanceNewHandler(const karabo::util::Hash& topologyEntry) {

            try {
                const std::string& type = topologyEntry.begin()->getKey();
                KARABO_LOG_FRAMEWORK_DEBUG << "instanceNewHandler --> " << type;

                if (type == "device") { // Take out only devices for the time being
                    const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                    const string& deviceId = entry.begin()->getKey();

                    // Consider the devices that should be archived 
                    if (entry.hasAttribute(deviceId, "archive") && (entry.getAttribute<bool>(deviceId, "archive") == true)) {
                        // Check whether according logger device exists (it should not) and instantiate
                        string loggerId = DATALOGGER_PREFIX + deviceId;
                        {
                            string serverId;
                            if (m_loggerMap.has(loggerId)) {
                                serverId = m_loggerMap.get<string>(loggerId);
                            } else {
                                m_serverIndex %= m_serverList.size();
                                serverId = m_serverList[m_serverIndex++];
                                m_loggerMap.set(loggerId, serverId);
                                emit<Hash>("signalLoggerMap", m_loggerMap);
                                m_saved = false;
                            }

                            Hash config;
                            config.set("DataLogger.deviceId", loggerId);
                            config.set("DataLogger.deviceToBeLogged", deviceId);
                            config.set("DataLogger.directory", "karaboHistory");
                            config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                            config.set("DataLogger.flushInterval", get<int>("flushInterval"));
                            remote().instantiateNoWait(serverId, config);
                        }
                    }
                }

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }

        void DataLoggerManager::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            try {
                string loggerId = DATALOGGER_PREFIX + instanceId;
                this->call(loggerId, "slotTagDeviceToBeDiscontinued", true, 'D');
                remote().killDeviceNoWait(loggerId);
                if (!m_saved) {
                    string filename = "loggermap.xml";
                    karabo::io::saveToFile(m_loggerMap, filename);
                    m_saved = true;
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }
    }
}
