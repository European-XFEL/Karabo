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
            
            NODE_ELEMENT(expected).key("PluginLoader")
                    .displayedName("Plugin Loader")
                    .description("Plugin Loader sub-configuration")
                    .appendParametersOfConfigurableClass<PluginLoader>("PluginLoader")
                    .commit();

            NODE_ELEMENT(expected).key("Logger")
                    .description("Logging settings")
                    .displayedName("Logger")
                    .appendParametersOfConfigurableClass<Logger>("Logger")
                    .commit();
            
            OVERWRITE_ELEMENT(expected).key("Logger.appenders")
                    .setNewDefaultValue("Ostream")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.appenders.Ostream.layout")
                    .setNewDefaultValue("Pattern")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.appenders.Ostream.layout.Pattern.format")
                    //.setNewDefaultValue("%d{%F %H:%M:%S} | %p | %c | %m")
                    .setNewDefaultValue("%p  %c  : %m%n")
                    .commit();                     
        }


        DeviceServer::DeviceServer(const karabo::util::Hash& input) : m_log(0), m_deviceInstanceCount(0) {

            input.get("isMaster", m_isMaster);

            // Set device server instance 
            if (input.has("serverId")) {
                input.get("serverId", m_serverId);
                // Automatically load this configuration on next startup
                karabo::io::saveToFile(Hash("DeviceServer.serverId", m_serverId), "autoload.xml");
            } else {
                m_serverId = generateDefaultServerId();
            }

            m_connectionConfig = input.get<Hash>("connection");
            m_connection = BrokerConnection::createChoice("connection", input);
            loadLogger(input);
            loadPluginLoader(input);
            input.get("nameRequestTimeout", m_nameRequestTimeout);
        }


        std::string DeviceServer::generateDefaultServerId() const {
            return string(boost::asio::ip::host_name() + "_Server_" + karabo::util::toString(getpid()));
        }


        DeviceServer::~DeviceServer() {
        }


        void DeviceServer::loadLogger(const Hash& input) {
            Hash config = input.get<Hash>("Logger");
            config.set("categories[0].Category.name", "karabo");
            config.set("categories[0].Category.appenders[0].Ostream.layout.Pattern.format", "%p  %c  : %m%n");
            config.set("categories[0].Category.additivity", false);
            config.set("appenders[1].Network.layout.Pattern.format", "%d{%F %H:%M:%S} | %p | %c | %m");
            config.set("appenders[1].Network.connection", m_connectionConfig);
            Logger::configure(config);
        }


        void DeviceServer::loadPluginLoader(const Hash& input) {
            m_pluginLoader = PluginLoader::createNode("PluginLoader", "PluginLoader", input);
        }


        void DeviceServer::run() {
            bool hasHearbeat = true;

            if (m_isMaster) {
                hasHearbeat = false;
            }

            // Initialize category
            m_log = &(karabo::log::Logger::getLogger(m_serverId));

            KARABO_LOG_INFO << "Starting Karabo DeviceServer on host: " << boost::asio::ip::host_name();

            // Initialize SignalSlotable instance
            init(m_connection, m_serverId);

            registerAndConnectSignalsAndSlots();

            karabo::util::Hash info("type", "server", "serverId", m_serverId, "version", DeviceServer::classInfo().getVersion(), "host", boost::asio::ip::host_name());
            boost::thread t(boost::bind(&karabo::core::DeviceServer::runEventLoop, this, hasHearbeat, info));
            this->startFsm();
            t.join();
            m_pluginThread.join();
        }


        void DeviceServer::registerAndConnectSignalsAndSlots() {
            SIGNAL3("signalNewDeviceClassAvailable", string /*serverId*/, string /*classId*/, Schema /*classSchema*/)
            SLOT1(slotStartDevice, Hash /*configuration*/)
            SLOT0(slotKillServer)
            SLOT1(slotDeviceGone, string /*deviceId*/)
            SLOT1(slotGetClassSchema, string /*classId*/)

            // Connect to global slot
            connectN("", "signalNewDeviceClassAvailable", "*", "slotNewDeviceClassAvailable");
        }


        log4cpp::Category & DeviceServer::log() {
            return (*m_log);
        }


        void DeviceServer::onStateUpdate(const std::string& currentState) {
            reply(currentState);
        }


        void DeviceServer::idleStateOnEntry() {

            KARABO_LOG_INFO << "DeviceServer starts up with id: " << m_serverId;

            if (m_isMaster) {
                slotStartDevice(Hash("MasterDevice.deviceId", "Karabo_Master_0", "MasterDevice.connection", m_connectionConfig));
                //slotStartDevice(Hash("GuiServerDevice.deviceId", "Karabo_GuiServer_0", "GuiServerDevice.connection", m_connectionConfig, "GuiServerDevice.loggerConnection", m_connectionConfig));
            } else {
                // Check whether we have installed devices available
                updateAvailableDevices();
                if (!m_availableDevices.empty()) {
                    inbuildDevicesAvailable();
                }
                KARABO_LOG_INFO << "Keep watching directory: " << m_pluginLoader->getPluginDirectory() << " for Device plugins";
                m_pluginThread = boost::thread(boost::bind(&karabo::core::DeviceServer::scanPlugins, this));
            }
        }


        void DeviceServer::updateAvailableDevices() {
            vector<string> devices = Configurator<BaseDevice>::getRegisteredClasses();


            BOOST_FOREACH(string device, devices) {
                if (device == "MasterDevice" || device == "GuiServerDevice") continue;
                if (!m_availableDevices.has(device)) {
                    KARABO_LOG_INFO << "Updated list of devices available: " << karabo::util::toString(devices);
                    Schema schema = BaseDevice::getSchema(device, Schema::AssemblyRules(karabo::util::READ | karabo::util::WRITE | karabo::util::INIT));
                    m_availableDevices.set(device, Hash("mustNotify", true, "xsd", schema));
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
            KARABO_LOG_ERROR << "[short] " << user;
            KARABO_LOG_ERROR << "[detailed] " << detail;
        }


        void DeviceServer::endErrorAction() {

        }


        void DeviceServer::startDeviceAction(const karabo::util::Hash& config) {
            try {
                std::string classId = config.begin()->getKey();

                KARABO_LOG_INFO << "Trying to start " << classId << "...";
                KARABO_LOG_DEBUG << "with the following configuration:\n" << config;

                // Inject device-server information
                Hash modifiedConfig(config);
                Hash& tmp = modifiedConfig.begin()->getValue<Hash>();
                tmp.set("serverId", m_serverId);
                // Apply sensible default in case no device instance id is supplied
                if (!tmp.has("deviceId")) {
                    std::string deviceId = this->generateDefaultDeviceId(classId);
                    tmp.set("deviceId", deviceId);
                }
                BaseDevice::Pointer device = BaseDevice::create(modifiedConfig); // TODO If constructor blocks, we are lost here!!
                boost::thread* t = m_deviceThreads.create_thread(boost::bind(&karabo::core::BaseDevice::run, device));

                // Associate deviceInstance with its thread
                string deviceInstanceId = device->getInstanceId();
                m_deviceInstanceMap[deviceInstanceId] = t;

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Device could not be started because: " << e.userFriendlyMsg();
                return;
            }
        }


        void DeviceServer::notifyNewDeviceAction() {
            vector<string> deviceClasses;
            deviceClasses.reserve(m_availableDevices.size());
            for (Hash::iterator it = m_availableDevices.begin(); it != m_availableDevices.end(); ++it) {
                deviceClasses.push_back(it->getKey());
                Hash& tmp = it->getValue<Hash>();
                if (tmp.get<bool>("mustNotify") == true) {
                    tmp.set("mustNotify", false);
                }
            }
            KARABO_LOG_DEBUG << "Sending instance update as new device plugins are available: " << karabo::util::toString(deviceClasses);
            this->updateInstanceInfo(Hash("deviceClasses", deviceClasses));
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
            KARABO_LOG_DEBUG << msg.str();
        }


        void DeviceServer::slotKillServer() {

            KARABO_LOG_INFO << "Received kill signal";

            // Notify all devices
            for (DeviceInstanceMap::const_iterator it = m_deviceInstanceMap.begin(); it != m_deviceInstanceMap.end(); ++it) {
                call(it->first, "slotKillDevice");
            }

            // Join all device threads
            m_deviceThreads.join_all();

            // Signal about future death
            call("*", "slotDeviceServerInstanceGone", m_serverId);

            // Stop device server
            stopDeviceServer();
        }


        void DeviceServer::slotDeviceGone(const std::string& instanceId) {

            KARABO_LOG_WARN << "Device \"" << instanceId << "\" notifies future death." << instanceId;

            DeviceInstanceMap::iterator it = m_deviceInstanceMap.find(instanceId);
            if (it != m_deviceInstanceMap.end()) {
                boost::thread* t = it->second;
                t->join();
                m_deviceThreads.remove_thread(t);
                m_deviceInstanceMap.erase(it);
                KARABO_LOG_INFO << "Device: \"" << instanceId << "\" removed from server.";
            }
        }


        void DeviceServer::slotGetClassSchema(const std::string& classId) {
            Schema schema = BaseDevice::getSchema(classId);
            reply(schema);
        }


        std::string DeviceServer::generateDefaultDeviceId(const std::string& classId) {
            string index = karabo::util::toString(++m_deviceInstanceCount);
            // Prepare shortened Device-Server name
            vector<string> tokens;
            boost::split(tokens, m_serverId, boost::is_any_of("_"));
            string domain(tokens.front() + "-" + tokens.back());
            return domain + "_" + classId + "_" + index;
        }
    }
}
