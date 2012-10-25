/*
 * $Id$
 *
 * Author: burkhard.heisen@xfel.eu>
 *
 * Created on February 9, 2011, 2:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <cstdlib>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include <karabo/io/Reader.hh>
#include <karabo/io/Writer.hh>
#include <karabo/log/Logger.hh>
#include <iosfwd>
#include <bits/basic_string.h>
#include <algorithm>

#include "Device.hh"
#include "DeviceServer.hh"


namespace karabo {

    namespace core {

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::log;
        using namespace karabo::net;
        using namespace karabo::xms;
        using namespace log4cpp;

        KARABO_REGISTER_ONLY_ME_CC(DeviceServer)

        DeviceServer::DeviceServer() : m_log(0) {
            Hash config("Xsd.indentation", -1);
            m_format = Format<Schema>::create(config);
        }

        DeviceServer::~DeviceServer() {
        }

        void DeviceServer::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected).key("devSrvInstId")
                    .displayedName("Device-Server Instance Id")
                    .description("The device-server instance id uniquely identifies a device-server instance in the distributed system")
                    .assignmentOptional().noDefaultValue()
                    .commit();

            CHOICE_ELEMENT<BrokerConnection > (expected).key("connection")
                    .displayedName("Connection")
                    .description("The connection to the communication layer of the distributed system")
                    .assignmentOptional().defaultValue("Jms")
                    .advanced()
                    .init()
                    .commit();

            // isMaster
            BOOL_ELEMENT(expected).key("isMaster")
                    .displayedName("Is Master Server?")
                    .description("Decides whether this device-server runs as a master or gets dynamically configured "
                    " through an existing master (DHCP-like)")
                    .assignmentOptional().defaultValue(false)
                    .commit();

            // nameRequestTimeout
            UINT32_ELEMENT(expected).key("nameRequestTimeout")
                    .displayedName("Name Request Timeout")
                    .description("Time to wait for name resolution (via name-server) until timeout [ms]")
                    .advanced()
                    .assignmentOptional().defaultValue(5000)
                    .commit();

            // Logger
            SINGLE_ELEMENT<Logger > (expected).key("Logger")
                    .description("Logging settings")
                    .displayedName("Logger")
                    .assignmentOptional().defaultValue("Logger")
                    .commit();

            // PluginLoader
            SINGLE_ELEMENT<PluginLoader > (expected).key("PluginLoader")
                    .displayedName("PluginLoader")
                    .description("Plugin Loader sub-configuration")
                    .assignmentOptional().defaultValue("PluginLoader")
                    .commit();

//            // autoStartDevice
//            LIST_ELEMENT<Device > (expected).key("autoStart")
//                    .displayedName("AutoStartDevice(s)")
//                    .description("Full configuration of the device(s) which should automatically be started upon availability")
//                    .assignmentOptional().noDefaultValue()
//                    .commit();
        }

        void DeviceServer::configure(const karabo::util::Hash& input) {

            input.get("isMaster", m_isMaster);

            // Set device server instance 
            if (input.has("devSrvInstId")) {
                input.get("devSrvInstId", m_devSrvInstId);
            } else {
                if (m_isMaster) {
                    m_devSrvInstId = "Master/DeviceServer/1";
                } else {
                    m_devSrvInstId = "";
                }
            }

            m_connectionConfig = input.get<Hash>("connection");
            m_connection = BrokerConnection::createChoice("connection", input);
            loadLogger(input);
            loadPluginLoader(input);
            input.get("nameRequestTimeout", m_nameRequestTimeout);
            if (input.has("autoStart")) {
                m_autoStart = input.get < vector < Hash > >("autoStart");
            }
        }

        void DeviceServer::loadLogger(const Hash& input) {
            Hash config = input.get<Hash>("Logger");
            vector<Hash>& appenders = config.get<vector<Hash> >("appenders");
            Hash appenderConfig;
            appenderConfig.setFromPath("Network.layout.Pattern.pattern", "%d{%F %H:%M:%S} | %p | %c | %m");
            appenderConfig.setFromPath("Network.connection", m_connectionConfig);
            appenders.push_back(appenderConfig);
            //config.set("priority", "DEBUG");
            Logger::Pointer log = Logger::create("Logger", config);
            log->initialize();
        }

        void DeviceServer::loadPluginLoader(const Hash& input) {
            m_pluginLoader = PluginLoader::createSingle("PluginLoader", "PluginLoader", input);
        }

        void DeviceServer::run() {
            bool hasHearbeat = true;

            if (m_isMaster) {
                hasHearbeat = false;

            } else {
                m_connection->start(); // This is needed to activate JMS, not so nice -> maybe cleaned
                if (m_devSrvInstId.empty()) {
                    // Request name from global slot "slotProvideName"
                    Requestor(m_connection->createChannel(), m_instanceId).call("*", "slotDeviceServerProvideName", boost::asio::ip::host_name()).timeout(m_nameRequestTimeout).receive(m_devSrvInstId);
                }
            }
            
            // Initialize category
            m_log = &(karabo::log::Logger::logger(m_devSrvInstId));
            
            log() << Priority::INFO << "Starting European XFEL DeviceServer on host: " << boost::asio::ip::host_name();

            // Initialize SignalSlotable instance
            init(m_connection, m_devSrvInstId);

            registerAndConnectSignalsAndSlots();

            startStateMachine();
            runEventLoop(hasHearbeat); // Block
            m_pluginThread.join();
        }

        void DeviceServer::registerAndConnectSignalsAndSlots() {
            SIGNAL3("signalNewDeviceClassAvailable", string, string, string) /* DeviceServerInstanceId, classId, xsd */
            SIGNAL2("signalNewDeviceInstanceAvailable", string, Hash) /* DeviceServerInstanceId, currentConfig */
            //SIGNAL1("signalDeviceServerInstanceGone", string) /* DeviceServerInstanceId */
            SLOT1(slotStartDevice, Hash)
            SLOT1(slotRegistrationOk, string)
            SLOT1(slotRegistrationFailed, string)
            SLOT0(slotKillDeviceServerInstance)
            SLOT1(slotKillDeviceInstance, string)

            // Connect to global slot
            connectN("", "signalNewDeviceClassAvailable", "*", "slotNewDeviceClassAvailable");
            connectN("", "signalNewDeviceInstanceAvailable", "*", "slotNewDeviceInstanceAvailable");
        }

        log4cpp::Category & DeviceServer::log() {
            return (*m_log);
        }

        void DeviceServer::updateCurrentState(const std::string & currentState) {
            reply(currentState);
        }

        void DeviceServer::registrationStateOnEntry() {
            if (m_isMaster) {
                // Go on, we can not ask for a name
                slotRegistrationOk("I am master!");
            } else {
                call("*", "slotNewDeviceServerAvailable", boost::asio::ip::host_name(), m_devSrvInstId);
            }
        }

        void DeviceServer::registrationFailed(const string& errorMessage) {
            log() << Priority::ERROR << errorMessage;
        }

        void DeviceServer::registrationOk(const std::string& message) {
            log() << Priority::INFO << "Master says: " << message;
        }

        void DeviceServer::idleStateOnEntry() {
            // Write name to file
            Writer<Hash>::create("TextFile", Hash("filename", "autoload.xml"))->write(Hash("DeviceServer.devSrvInstId", m_devSrvInstId));
            log() << Priority::INFO << "DeviceServer starts up with id: " << m_devSrvInstId;

            if (m_isMaster) {
                slotStartDevice(Hash("MasterDevice.devInstId", "Master/MasterDevice/1", "MasterDevice.connection", m_connectionConfig));
                slotStartDevice(Hash("GuiServerDevice.devInstId", "Master/GuiServerDevice/1", "GuiServerDevice.connection", m_connectionConfig, "GuiServerDevice.loggerConnection", m_connectionConfig));
                
            } else {
                // Check whether we have installed devices available
                updateAvailableDevices();
                if (!m_availableDevices.empty()) {
                    if (!m_autoStart.empty()) {
                        for (size_t i = 0; i < m_autoStart.size(); ++i) {
                            if (!m_autoStart[i].empty()) {
                                slotStartDevice(m_autoStart[i]);
                            }
                        }
                    }
                    inbuildDevicesAvailable();
                }
                log() << Priority::INFO << "Keep watching directory: " << m_pluginLoader->getPluginDirectory() << " for Device plugins";
                m_pluginThread = boost::thread(boost::bind(&karabo::core::DeviceServer::scanPlugins, this));
            }
        }

        void DeviceServer::updateAvailableDevices() {
            vector<string> devices = Factory<Device>::getRegisteredKeys();
            log() << Priority::DEBUG << "Devices available: " << String::sequenceToString(devices);

            BOOST_FOREACH(string device, devices) {
                if (device == "MasterDevice" || device == "GuiServerDevice") continue;
                if (!m_availableDevices.has(device)) {
                    std::stringstream stream;
                    m_format->convert(Device::expectedParameters(device, karabo::util::READ | karabo::util::WRITE | karabo::util::INIT), stream);
                    m_availableDevices.set(device, Hash("mustNotify", true, "xsd", stream.str()));
                }
            }
        }

        void DeviceServer::scanPlugins() {
            bool inError = false;
            m_deviceServerStopped = true;
            while (m_deviceServerStopped) {
                try {
                    bool hasNewPlugins = m_pluginLoader->update();
                    if (hasNewPlugins) {
                        // Update the list of available devices
                        updateAvailableDevices();
                        newPluginAvailable();
                    }
                    boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
                    if (inError) {
                        endError();
                        inError = false;
                    }
                } catch (const Exception& e) {
                    errorFound(e.userFriendlyMsg(), e.detailedMsg());
                    inError = true;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(10000));
                }
            }
            stopEventLoop();
        }
        
        void DeviceServer::stopDeviceServer() {
            m_deviceServerStopped = false;
        }

        void DeviceServer::errorFoundAction(const std::string& user, const std::string & detail) {
            log() << Priority::ERROR << "[short] " << user;
            log() << Priority::ERROR << "[detailed] " << detail;
        }

        void DeviceServer::endErrorAction() {

        }

        void DeviceServer::startDeviceAction(const karabo::util::Hash& config) {
            try {
                log() << Priority::INFO << "Trying to start device with the following configuration:\n" << config;
                
                // Inject device-server information
                Hash modifiedConfig(config);
                Hash& tmp = modifiedConfig.get<Hash>(modifiedConfig.begin());
                tmp.set("devSrvInstId", m_devSrvInstId);
                // Apply sensible default in case no device instance id is supplied
                if (!tmp.has("devInstId")) {
                    std::string classId = modifiedConfig.begin()->first;    
                    std::string devInstId = this->generateDefaultDeviceInstanceId(classId);
                    tmp.set("devInstId", devInstId);
                }
                Device::Pointer device = Device::create(modifiedConfig);
                boost::thread* t = m_deviceThreads.create_thread(boost::bind(&karabo::core::Device::run, device));
                
                // Associate deviceInstance with its thread
                string deviceInstanceId = device->getInstanceId();
                m_deviceInstanceMap[deviceInstanceId] = t;
                
                emit("signalNewDeviceInstanceAvailable", getInstanceId(), device->getCurrentConfiguration());
            } catch (const Exception& e) {
                log() << Priority::ERROR << "Device could not be started because: " << e.userFriendlyMsg();
                return;
            }
        }

        void DeviceServer::notifyNewDeviceAction() {
            for (Hash::iterator it = m_availableDevices.begin(); it != m_availableDevices.end(); ++it) {
                Hash& tmp = m_availableDevices.get<Hash > (it);
                if (tmp.get<bool>("mustNotify") == true) {
                    tmp.set("mustNotify", false);
                    log() << Priority::DEBUG << "Notifying about " << it->first;
                    emit("signalNewDeviceClassAvailable", getInstanceId(), it->first, tmp.get<string > ("xsd"));
                }
            }
        }

        void DeviceServer::noStateTransition(const std::string& typeId, int state) {
            string eventName(typeId);
            boost::regex re(".*\\d+(.+Event).*");
            boost::smatch what;
            bool result = boost::regex_search(typeId, what, re);
            if (result && what.size() == 2) {
                eventName = what.str(1);
            }
            ostringstream msg;
            msg << "DeviceServer \"" << getInstanceId() << "\" does not allow a transition for event \"" << eventName << "\"";
            log() << Priority::DEBUG << msg.str();
        }
        
        void DeviceServer::slotKillDeviceServerInstance() {
            
             log() << Priority::INFO << "Received kill signal";
            
            // Notify all devices
            for (DeviceInstanceMap::const_iterator it = m_deviceInstanceMap.begin(); it != m_deviceInstanceMap.end(); ++it) {
                call(it->first, "slotKillDeviceInstance");
            }
             
            // Join all device threads
             m_deviceThreads.join_all();
            
             // Signal about future death
            call("*", "slotDeviceServerInstanceGone", m_devSrvInstId);
            
            // Stop device server
            stopDeviceServer();
        }
        
        void DeviceServer::slotKillDeviceInstance(const std::string& instanceId) {
            
            log() << Priority::WARN << "Received kill signal for device " << instanceId;
             
            DeviceInstanceMap::iterator it = m_deviceInstanceMap.find(instanceId);
            if (it != m_deviceInstanceMap.end()) {
                call(it->first, "slotKillDeviceInstance");
                boost::thread* t = it->second;
                t->join();
                m_deviceThreads.remove_thread(t);
                m_deviceInstanceMap.erase(it);
                log() << Priority::INFO << "Device: " << instanceId << " finally died";
            }
        }
        
        std::string DeviceServer::generateDefaultDeviceInstanceId(const std::string& classId) {
            string index = String::toString(m_deviceInstanceMap.size()+1);
            // Prepare shortened Device-Server name
            vector<string> tokens;
            boost::split(tokens, m_devSrvInstId, boost::is_any_of("/"));
            string domain(tokens.front() + "-" + tokens.back());
            return domain + "/" + classId + "/" + index;
        }
    } 
} 
