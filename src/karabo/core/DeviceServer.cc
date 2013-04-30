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

#include <karabo/util/SimpleElement.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/io/Input.hh>
#include <karabo/io/Output.hh>
#include <karabo/log/Logger.hh>

#include "Device.hh"
#include "DeviceServer.hh"
#include "karabo/io/FileTools.hh"

namespace karabo {

    namespace core {
        
       //template class Runner<DeviceServer>;

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::log;
        using namespace karabo::net;
        using namespace karabo::xms;
        using namespace log4cpp;

        KARABO_REGISTER_FOR_CONFIGURATION(DeviceServer)

        DeviceServer::DeviceServer() : m_log(0), m_deviceInstanceCount(0) {
            Hash config("Xsd.indentation", -1);
            m_schemaSerializer = TextSerializer<Schema>::create(config);
        }

        DeviceServer::~DeviceServer() {
        }

        void DeviceServer::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected).key("serverId")
                    .displayedName("Server ID")
                    .description("The device-server instance id uniquely identifies a device-server instance in the distributed system")
                    .assignmentOptional().noDefaultValue()
                    .commit();

            CHOICE_ELEMENT(expected).key("connection")
                    .displayedName("Connection")
                    .description("The connection to the communication layer of the distributed system")
                    .appendNodesOfConfigurationBase<BrokerConnection>()
                    .assignmentOptional().defaultValue("Jms")
                    .advanced()
                    .init()
                    .commit();

            
            BOOL_ELEMENT(expected).key("isMaster")
                    .displayedName("Is Master Server?")
                    .description("Decides whether this device-server runs as a master or gets dynamically configured "
                    " through an existing master (DHCP-like)")
                    .assignmentOptional().defaultValue(false)
                    .commit();

          
            UINT32_ELEMENT(expected).key("nameRequestTimeout")
                    .displayedName("Name Request Timeout")
                    .description("Time to wait for name resolution (via name-server) until timeout [ms]")
                    .advanced()
                    .assignmentOptional().defaultValue(5000)
                    .commit();

          
            NODE_ELEMENT(expected).key("Logger")
                    .description("Logging settings")
                    .displayedName("Logger")
                    .appendParametersOfConfigurableClass<Logger>("Logger")
                    .commit();

            
            NODE_ELEMENT(expected).key("PluginLoader")
                    .displayedName("Plugin Loader")
                    .description("Plugin Loader sub-configuration")
                    .appendParametersOfConfigurableClass<PluginLoader>("PluginLoader")
                    .commit();
        }

        DeviceServer::DeviceServer(const karabo::util::Hash& input) {

            input.get("isMaster", m_isMaster);

            // Set device server instance 
            if (input.has("serverId")) {
                input.get("serverId", m_serverId);
            } else {
                if (m_isMaster) {
                    m_serverId = "Master_DeviceServer_1";
                } else {
                    m_serverId = "";
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
            appenderConfig.set("Network.layout.Pattern.format", "%d{%F %H:%M:%S} | %p | %c | %m");
            appenderConfig.set("Network.connection", m_connectionConfig);
            appenders.push_back(appenderConfig);
            Logger::configure(config);
        }

        void DeviceServer::loadPluginLoader(const Hash& input) {
            m_pluginLoader = PluginLoader::createNode("PluginLoader", "PluginLoader", input);
        }

        void DeviceServer::run() {
            bool hasHearbeat = true;

            if (m_isMaster) {
                hasHearbeat = false;

            } else {
                m_connection->start(); // This is needed to activate JMS, not so nice -> maybe cleaned
                if (m_serverId.empty()) {
                    // Request name from global slot "slotProvideName"
                    Requestor(m_connection->createChannel(), m_instanceId).call("*", "slotDeviceServerProvideName", boost::asio::ip::host_name()).timeout(m_nameRequestTimeout).receive(m_serverId);
                }
            }
            
            // Initialize category
            m_log = &(karabo::log::Logger::getLogger(m_serverId));
            
            log() << Priority::INFO << "Starting Karabo DeviceServer on host: " << boost::asio::ip::host_name();

            // Initialize SignalSlotable instance
            init(m_connection, m_serverId);

            registerAndConnectSignalsAndSlots();

            boost::thread t(boost::bind(&karabo::core::DeviceServer::runEventLoop, this, hasHearbeat, Hash()));
            this->startFsm();
            t.join();
            
            //startFsm();
            //runEventLoop(hasHearbeat); // Block
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

        void DeviceServer::onStateUpdate(const std::string& currentState) {
            reply(currentState);
        }

        void DeviceServer::registrationStateOnEntry() {
            if (m_isMaster) {
                // Go on, we can not ask for a name
                slotRegistrationOk("I am master!");
            } else {
                call("*", "slotNewDeviceServerAvailable", boost::asio::ip::host_name(), m_serverId);
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
            karabo::io::saveToFile(Hash("DeviceServer.serverId", m_serverId), "autoload.xml");
            log() << Priority::INFO << "DeviceServer starts up with id: " << m_serverId;

            if (m_isMaster) {
                slotStartDevice(Hash("MasterDevice.deviceId", "Master_MasterDevice_1", "MasterDevice.connection", m_connectionConfig));
                slotStartDevice(Hash("GuiServerDevice.deviceId", "Master_GuiServerDevice_1", "GuiServerDevice.connection", m_connectionConfig, "GuiServerDevice.loggerConnection", m_connectionConfig));
                
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
            vector<string> devices = Configurator<BaseDevice>::getRegisteredClasses();
            log() << Priority::DEBUG << "Devices available: " << karabo::util::toString(devices);

            BOOST_FOREACH(string device, devices) {
                if (device == "MasterDevice" || device == "GuiServerDevice") continue;
                if (!m_availableDevices.has(device)) {
                    std::string archive;
                    m_schemaSerializer->save(BaseDevice::getSchema(device, Schema::AssemblyRules(karabo::util::READ | karabo::util::WRITE | karabo::util::INIT)), archive);
                    m_availableDevices.set(device, Hash("mustNotify", true, "xsd", archive));
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
                std::string classId = config.begin()->getKey();
                 
                log() << Priority::INFO << "Trying to start " << classId << "...";
                log() << Priority::DEBUG << "with the following configuration:\n" << config;
                
                // Inject device-server information
                Hash modifiedConfig(config);
                Hash& tmp = modifiedConfig.begin()->getValue<Hash>();
                tmp.set("serverId", m_serverId);
                // Apply sensible default in case no device instance id is supplied
                if (!tmp.has("deviceId")) {
                    std::string deviceId = this->generateDefaultDeviceInstanceId(classId);
                    tmp.set("deviceId", deviceId);
                }
                BaseDevice::Pointer device = BaseDevice::create(modifiedConfig);
                boost::thread* t = m_deviceThreads.create_thread(boost::bind(&karabo::core::BaseDevice::run, device));
                
                // Associate deviceInstance with its thread
                string deviceInstanceId = device->getInstanceId();
                m_deviceInstanceMap[deviceInstanceId] = t;
                
                emit("signalNewDeviceInstanceAvailable", getInstanceId(), Hash(classId, device->getCurrentConfiguration()));
            } catch (const Exception& e) {
                log() << Priority::ERROR << "Device could not be started because: " << e.userFriendlyMsg();
                return;
            }
        }

        void DeviceServer::notifyNewDeviceAction() {
            for (Hash::iterator it = m_availableDevices.begin(); it != m_availableDevices.end(); ++it) {
                Hash& tmp = it->getValue<Hash>();
                if (tmp.get<bool>("mustNotify") == true) {
                    tmp.set("mustNotify", false);
                    log() << Priority::DEBUG << "Notifying about " << it->getKey();
                    emit("signalNewDeviceClassAvailable", getInstanceId(), it->getKey(), tmp.get<string > ("xsd"));
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
            call("*", "slotDeviceServerInstanceGone", m_serverId);
            
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
            string index = karabo::util::toString(++m_deviceInstanceCount);
            // Prepare shortened Device-Server name
            vector<string> tokens;
            boost::split(tokens, m_serverId, boost::is_any_of("/"));
            string domain(tokens.front() + "-" + tokens.back());
            return domain + "/" + classId + "/" + index;
        }
    } 
} 
