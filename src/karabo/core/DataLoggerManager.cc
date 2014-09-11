/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <map>
#include <karabo/io/Input.hh>
#include "DataLoggerManager.hh"
#include "karabo/io/FileTools.hh"

namespace karabo {
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, DataLoggerManager)

        void DataLoggerManager::expectedParameters(Schema& expected) {

            INT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(40)
                    .reconfigurable()
                    .commit();

            STRING_ELEMENT(expected).key("fileFormat")
                    .displayedName("File format")
                    .description("The file format to use for logging")
                    .options("xml, bin, hdf5")
                    .assignmentOptional().defaultValue("bin")
                    .commit();

            INT32_ELEMENT(expected).key("maximumFileSize")
                    .displayedName("Maximum file size")
                    .description("After any archived file has reached this size it will be time-stamped and not appended anymore")
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .reconfigurable()
                    .assignmentOptional().defaultValue(100)
                    .commit();

            FLOAT_ELEMENT(expected).key("lastFlushDuration")
                    .displayedName("Last flush duration")
                    .description("Time needed for the last flush")
                    .unit(Unit::SECOND)
                    .readOnly()
                    .warnHigh(20)
                    .alarmHigh(40)
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


        DataLoggerManager::DataLoggerManager(const Hash& input) : Device<OkErrorFsm>(input) {
        }


        DataLoggerManager::~DataLoggerManager() {
        }

        std::string DataLoggerManager::generateNewDataLoggerInstanceId(const std::string& managerId) {
            static unsigned long long seq = 0;
            stringstream ss;
            ss << managerId << "-DataLogger_" << ++seq;
            return ss.str();
        }
        
        void DataLoggerManager::okStateOnEntry() {
            // Register handlers
            remote().registerInstanceNewMonitor(boost::bind(&karabo::core::DataLoggerManager::instanceNewHandler, this, _1));
            remote().registerInstanceGoneMonitor(boost::bind(&karabo::core::DataLoggerManager::instanceGoneHandler, this, _1, _2));
            
            // Prepare backend to persist data (later we should use brokerhost/brokerport/brokertopic)
            if (!boost::filesystem::exists("karaboHistory")) {
                boost::filesystem::create_directory("karaboHistory");
            }

            // Get all current instances in the system
            const Hash& systemTopology = remote().getSystemTopology();
            boost::optional<const Hash::Node&> node = systemTopology.find("device");
            if (node) {
                const Hash& devices = node->getValue<Hash>();
                for (Hash::const_iterator it = devices.begin(); it != devices.end(); ++it) { // Loop all devices
                    // TODO: May be to archive ONLY those that HAVE "archive" attribute and it IS true
                    if (it->hasAttribute("archive") && (it->getAttribute<bool>("archive") == false)) continue;
                    const std::string& deviceId = it->getKey();
                    if (deviceId == m_instanceId) continue; // Skip myself
                    
                    // Check whether according logger devices exists and if not instantiate
                    // The key is device to be logged, the value is corresponding logger device
                    
                    // Check if deviceId registered
                    boost::mutex::scoped_lock lock(m_loggedDevicesMutex);
                    map<string,string>::iterator devit = m_loggedDevices.find(deviceId);
                    if (devit == m_loggedDevices.end()) {
                        // instantiate the logger associated with "deviceId"
                        string loggerId = generateNewDataLoggerInstanceId(m_instanceId);
                        Hash config;
                        config.set("DataLogger.deviceId", loggerId);
                        config.set("DataLogger.deviceToBeLogged", deviceId);
                        config.set("DataLogger.directory", "karaboHistory");
                        config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                        remote().instantiateNoWait(getServerId(), config);
                        m_loggedDevices[deviceId] = loggerId;
                        m_dataLoggers[loggerId] = deviceId;
                    }
                }
            }
        }


        void DataLoggerManager::instanceNewHandler(const karabo::util::Hash& topologyEntry) {

            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "instanceNewHandler";
                const std::string& type = topologyEntry.begin()->getKey();

                if (type == "device") { // Take out only devices for the time being
                    const Hash& entry = topologyEntry.begin()->getValue<Hash>();
                    const string& deviceId = entry.begin()->getKey();

                    // Skip devices that are marked to globally prevent archiving
                    if (entry.hasAttribute(deviceId, "archive") && (entry.getAttribute<bool>(deviceId, "archive") == false)) return;

                    // Check whether according logger device exists (it should not) and instantiate
                    boost::mutex::scoped_lock lock(m_loggedDevicesMutex);
                    map<string,string>::iterator devit = m_loggedDevices.find(deviceId);
                    if (devit == m_loggedDevices.end()) {
                        string loggerId = generateNewDataLoggerInstanceId(m_instanceId);
                        Hash config;
                        config.set("DataLogger.deviceId", loggerId);
                        config.set("DataLogger.deviceToBeLogged", deviceId);
                        config.set("DataLogger.directory", "karaboHistory");
                        config.set("DataLogger.maximumFileSize", get<int>("maximumFileSize"));
                        remote().instantiateNoWait(m_instanceId, config);
                        m_loggedDevices[deviceId] = loggerId;
                        m_dataLoggers[loggerId] = deviceId;
                    }
                }

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }

        void DataLoggerManager::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            
            // Call slotTagDeviceToBeDiscontinued and than kill the logger device
            try {
                map<string,string>::iterator devit = m_loggedDevices.find(instanceId);
                if (devit != m_loggedDevices.end()) {
                    boost::mutex::scoped_lock lock(m_loggedDevicesMutex);
                    string loggerId = devit->second;
                    remote().execute<bool, char>(loggerId, "slotTagDeviceToBeDiscontinued", true, 'D');
                    remote().killDeviceNoWait(loggerId);
                    m_loggedDevices.erase(devit);
                    m_dataLoggers.erase(loggerId);
                }    
            } catch(const Exception& e) {
                KARABO_LOG_ERROR << e;
            }
        }        
    }
}
