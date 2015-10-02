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

            std::vector<std::string> emptyVec;

            VECTOR_STRING_ELEMENT(expected).key("serverList")
                    .displayedName("Server list")
                    .description("List of device server IDs where the DataLogger instance run. The load balancing is round-robin.")
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
            m_maintainedDevices.clear();
            m_instantiated.clear();
        }

        DataLoggerManager::~DataLoggerManager() {
            KARABO_LOG_INFO << "dead.";
        }

        void DataLoggerManager::okStateOnEntry() {
            m_maintainedDevices.clear();
            boost::filesystem::path filename("maintained_devices.xml");
            if (boost::filesystem::exists(filename)) {
                Hash h;
                karabo::io::loadFromFile(h, filename.string());
                KARABO_LOG_FRAMEWORK_INFO << "hash read from \"" << filename << "\"...\n" << h;
                if (h.has("maintained_devices")) {
                    h.get("maintained_devices", m_maintainedDevices); 
                }
            }

            // Get all current instances in the system and trigger 'instanceNewHandler' calls
            remote().getSystemTopology();
            
            // Register handlers
            
            trackAllInstances();
            
            remote().registerInstanceNewMonitor(boost::bind(&DataLoggerManager::instanceNewHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&DataLoggerManager::instanceGoneHandler, this, _1, _2));

            KARABO_SYSTEM_SIGNAL("signalLoggerMap", Hash /*loggerMap*/);
            KARABO_SLOT(slotGetLoggerMap);

        }

        
        void DataLoggerManager::instantiateReaders(const std::string& serverId) {
            std::vector<std::string> devices = remote().getDevices(serverId);
            for (int i = 0; i < DATALOGREADERS_PER_SERVER; i++) {
                std::string readerId = DATALOGREADER_PREFIX + toString(i) + "-" + serverId;
                if (!remote().exists(readerId).first) {
                    Hash config;
                    config.set("DataLogReader.deviceId", readerId);
                    config.set("DataLogReader.directory", "karaboHistory");
                    remote().instantiateNoWait(serverId, config);
                    KARABO_LOG_FRAMEWORK_DEBUG << "instantiateReaders: reader \"" << readerId << "\" started on server \"" << serverId << "\"";
                }
            }
        }
        
        
        void DataLoggerManager::slotGetLoggerMap() {
            reply(m_loggerMap);
        }
        
        
        bool DataLoggerManager::Registry::has(const std::string& id) const {
            for (std::map<std::string, std::vector<std::string> >::const_iterator i = m_registered.begin(); i != m_registered.end(); ++i) {
                if (id == i->first) return true;
                for (std::vector<std::string>::const_iterator j = i->second.begin(); j!=i->second.end(); ++j) {
                    if (id == *j) return true;
                }
            }
            return false;
        }
        
        
        bool DataLoggerManager::Registry::has(const std::string& serverId, const std::string& deviceId) const {
            map<std::string, std::vector<std::string> >::const_iterator i = m_registered.find(serverId);
            if (i == m_registered.end()) return false;
            return find(i->second.begin(), i->second.end(), deviceId) != i->second.end();
        }
        
        
        void DataLoggerManager::Registry::insert(const std::string& serverId, const std::string& deviceId) {
            std::map<std::string, std::vector<std::string> >::iterator i = m_registered.find(serverId);
            if (i == m_registered.end()) m_registered[serverId] = vector<string>();
            std::vector<std::string>& v = m_registered[serverId];
            if (find(v.begin(), v.end(), deviceId) == v.end())
                v.push_back(deviceId);
        }
        
        
        void DataLoggerManager::Registry::erase(const std::string& serverId) {
            std::map<std::string, std::vector<std::string> >::iterator i = m_registered.find(serverId);
            if (i == m_registered.end()) return;
            m_registered.erase(i);
        }
        
        
        void DataLoggerManager::Registry::clear() {
            m_registered.clear();
        }
        
        
        void DataLoggerManager::Registry::printContent() {
            KARABO_LOG_FRAMEWORK_INFO << "--------------------------------------------------------------------";
            for (std::map<std::string, std::vector<std::string> >::const_iterator i = m_registered.begin(); i != m_registered.end(); ++i) {
                KARABO_LOG_FRAMEWORK_INFO << i->first;
                for (std::vector<std::string>::const_iterator j = i->second.begin(); j!=i->second.end(); ++j) {
                    KARABO_LOG_FRAMEWORK_INFO << "\t" << *j;
                }
            }
            KARABO_LOG_FRAMEWORK_INFO << "--------------------------------------------------------------------";
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
                        string loggerId = DATALOGGER_PREFIX + deviceId;
                        bool deviceExists = remote().exists(deviceId).first;
                        bool loggerExists = remote().exists(loggerId).first;
                        
                        boost::mutex::scoped_lock lock(m_handlerMutex);
                        if (find(m_maintainedDevices.begin(), m_maintainedDevices.end(), deviceId) == m_maintainedDevices.end()) {
                            m_maintainedDevices.push_back(deviceId);
                            Hash h("maintained_devices", m_maintainedDevices);
                            string filename = "maintained_devices.xml";
                            karabo::io::saveToFile(h, filename);
                        }
                        
                        // Check whether corresponding logger device exists (it should not) and instantiate
                        //vector<string> allDevices = remote().getDevices();
                        //if (find(allDevices.begin(), allDevices.end(), loggerId) == allDevices.end())
                        if (deviceExists && !loggerExists)
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
                            
                            // check if instantiated already
                            if (m_instantiated.has(serverId, loggerId)) return;
                            
                            Hash config;
                            config.set("DataLogger.deviceId", loggerId);
                            config.set("DataLogger.deviceToBeLogged", deviceId);
                            config.set("DataLogger.directory", "karaboHistory");
                            config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                            config.set("DataLogger.flushInterval", get<int>("flushInterval"));
                            remote().instantiateNoWait(serverId, config);
                            KARABO_LOG_FRAMEWORK_INFO << "instanceNewHandler [device] : logger \"" << loggerId << "\" STARTED";
                            // Register logger as instantiated
                            m_instantiated.insert(serverId, loggerId);
                        }
                    }
                } else if (type == "server") {
                    const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                    const string& serverId = entry.begin()->getKey();
                    if (find(m_serverList.begin(), m_serverList.end(), serverId) != m_serverList.end()) {
                        instantiateReaders(serverId);
                        KARABO_LOG_FRAMEWORK_INFO << "instanceNewHandler: DataLogReaders are started for server \"" << serverId << "\"";
                        
                        boost::mutex::scoped_lock lock(m_handlerMutex);
                        
                        m_instantiated.printContent();
                        
                        for (vector<string>::iterator i = m_maintainedDevices.begin(); i != m_maintainedDevices.end(); ++i) {
                            string deviceId = *i;
                            string loggerId = DATALOGGER_PREFIX + deviceId;
                
                            if (!m_loggerMap.has(loggerId)) continue;
                            string srv = m_loggerMap.get<string>(loggerId);
                            if (srv != serverId) continue;
                            
                            // check if logger has been instantiated earlier ...
                            if (m_instantiated.has(serverId, loggerId)) continue;
                            
                            if (remote().exists(deviceId).first && !remote().exists(loggerId).first) {                                
                                Hash config;
                                config.set("DataLogger.deviceId", loggerId);
                                config.set("DataLogger.deviceToBeLogged", deviceId);
                                config.set("DataLogger.directory", "karaboHistory");
                                config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                                config.set("DataLogger.flushInterval", get<int>("flushInterval"));
                                remote().instantiateNoWait(serverId, config);
                                KARABO_LOG_FRAMEWORK_INFO << "instanceNewHandler [server] : logger \"" << loggerId << "\" STARTED";
                                m_instantiated.insert(serverId, loggerId);
                            }
                        }
                    }
                }

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }

        void DataLoggerManager::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            const std::string& type = instanceInfo.get<string>("type");
            KARABO_LOG_FRAMEWORK_INFO << "instanceGoneHandler -->  instanceId : \"" << instanceId << "\", type : " << type << ", instanceInfo ...\n" << instanceInfo;
            if (type == "device") {
                // skip if it is not maintained device
                if (std::find(m_maintainedDevices.begin(), m_maintainedDevices.end(), instanceId) == m_maintainedDevices.end()) return;
                string loggerId = DATALOGGER_PREFIX + instanceId;
                bool deviceExists = remote().exists(instanceId).first;
                bool loggerExists = remote().exists(loggerId).first;
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
            } else if (type == "server") {
                boost::mutex::scoped_lock lock(m_handlerMutex);
                m_instantiated.erase(instanceId);
            }
        }
    }
}
