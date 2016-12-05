/*
 * $Id$
 *
 * Author: burkhard.heisen@xfel.eu>
 *
 * Created on February 9, 2011, 2:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "DeviceServer.hh"
#include "Device.hh"

#include "karabo/util/SimpleElement.hh"
#include "karabo/util/NodeElement.hh"
#include "karabo/util/ChoiceElement.hh"
#include "karabo/util/Version.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/log/Logger.hh"
#include "karabo/io/Input.hh"
#include "karabo/io/Output.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/net/JmsConnection.hh"
#include "karabo/net/utils.hh"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <cstdlib>
#include <csignal>

namespace karabo {

    namespace core {

        //template class Runner<DeviceServer>;

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::log;
        using namespace karabo::net;
        using namespace karabo::xms;
        using namespace krb_log4cpp;


        KARABO_REGISTER_FOR_CONFIGURATION(DeviceServer)

        void DeviceServer::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected).key("serverId")
                    .displayedName("Server ID")
                    .description("The device-server instance id uniquely identifies a device-server instance in the distributed system")
                    .assignmentOptional().noDefaultValue()
                    .commit();

            INT32_ELEMENT(expected).key("visibility")
                    .displayedName("Visibility")
                    .description("Configures who is allowed to see this server at all")
                    .assignmentOptional().defaultValue(karabo::util::Schema::OBSERVER)
                    .options("0 1 2 3 4")
                    .adminAccess()
                    .reconfigurable()
                    .commit();

            NODE_ELEMENT(expected).key("connection")
                    .displayedName("Connection")
                    .description("The connection to the communication layer of the distributed system")
                    .appendParametersOf<JmsConnection>()
                    .expertAccess()
                    .commit();

            INT32_ELEMENT(expected).key("heartbeatInterval")
                    .displayedName("Heartbeat interval")
                    .description("The heartbeat interval")
                    .assignmentOptional().defaultValue(10)
                    .adminAccess()
                    .commit();

            VECTOR_STRING_ELEMENT(expected).key("devices")
                    .displayedName("Devices")
                    .description("The devices classes the server will manage")
                    .assignmentOptional().defaultValue(BaseDevice::getRegisteredClasses())
                    .expertAccess()
                    .commit();

            LIST_ELEMENT(expected).key("autoStart")
                    .displayedName("Auto start")
                    .description("Auto starts selected devices")
                    .appendNodesOfConfigurationBase<karabo::core::BaseDevice>()
                    .assignmentOptional().noDefaultValue()
                    .commit();

            BOOL_ELEMENT(expected).key("scanPlugins")
                    .displayedName("Scan plug-ins?")
                    .description("Decides whether the server will scan the content of the plug-in folder and dynamically load found devices")
                    .expertAccess()
                    .assignmentOptional().defaultValue(true)
                    .commit();

            const std::string defaultPluginPath = Version::getPathToKaraboInstallation() + "/plugins";
            PATH_ELEMENT(expected)
                    .key("pluginDirectory")
                    .displayedName("Plugin Directory")
                    .description("Directory to search for plugins")
                    .assignmentOptional().defaultValue(defaultPluginPath)
                    .isDirectory()
                    .expertAccess()
                    .commit();

            NODE_ELEMENT(expected).key("Logger")
                    .description("Logging settings")
                    .displayedName("Logger")
                    .appendParametersOf<Logger>()
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.file.filename")
                    .setNewDefaultValue("device-server.log")
                    .commit();
        }


        DeviceServer::DeviceServer(const karabo::util::Hash& config) : m_log(0), m_scanPluginsTimer(EventLoop::getIOService()) {

            if (config.has("serverId")) {
                config.get("serverId", m_serverId);
            } else {
                m_serverId = generateDefaultServerId();
            }

            config.get("devices", m_devices);

            // Device configurations for those to automatically start
            if (config.has("autoStart")) config.get("autoStart", m_autoStart);

            // Whether to scan for additional plug-ins at runtime
            config.get("scanPlugins", m_scanPlugins);

            // What visibility this server should have
            config.get("visibility", m_visibility);

            m_connection = Configurator<JmsConnection>::createNode("connection", config);
            m_connection->connect();

            m_pluginLoader = PluginLoader::create("PluginLoader", Hash("pluginDirectory", config.get<string>("pluginDirectory"),
                                                                       "pluginsToLoad", "*"));
            loadLogger(config);

            karabo::util::Hash instanceInfo;
            instanceInfo.set("type", "server");
            instanceInfo.set("serverId", m_serverId);
            instanceInfo.set("version", karabo::util::Version::getVersion());
            instanceInfo.set("host", net::bareHostName());
            instanceInfo.set("visibility", m_visibility);

            // Initialize SignalSlotable instance
            init(m_serverId, m_connection, config.get<int>("heartbeatInterval"), instanceInfo);

            registerSlots();
        }


        std::string DeviceServer::generateDefaultServerId() const {
            return net::bareHostName() += "/" + util::toString(getpid());
        }


        DeviceServer::~DeviceServer() {
            stopDeviceServer();
            KARABO_LOG_FRAMEWORK_TRACE << "DeviceServer::~DeviceServer() dtor : m_logger.use_count()=" << m_logger.use_count();
            m_logger.reset();
        }


        void DeviceServer::loadLogger(const Hash& input) {

            Hash config = input.get<Hash>("Logger");

            boost::filesystem::path path(Version::getPathToKaraboInstallation() + "/var/log/" + m_serverId);
            boost::filesystem::create_directories(path);
            path += "/device-server.log";

            config.set("file.filename", path.generic_string());
            if (!config.has("network.topic")) {
                // If not specified, use the local topic for log messages
                config.set("network.topic", m_topic);
            }

            Logger::configure(config);

            // By default all categories use all three appenders
            Logger::useOstream();
            Logger::useFile();
            Logger::useNetwork();

            // All class logs will have karabo as parent category (outermost namespace)
            // Those messages should not go via the broker
            Logger::useOstream("karabo", false); // The false means, that we do not inherit any parent appenders
            Logger::useFile("karabo", false);

            // Initialize category
            m_log = &(karabo::log::Logger::getCategory(m_serverId));

            KARABO_LOG_FRAMEWORK_INFO << "Logfiles are written to: " << path;
        }


        void DeviceServer::loadPluginLoader(const Hash& input) {
            m_pluginLoader = PluginLoader::createNode("PluginLoader", "PluginLoader", input);
        }


        void DeviceServer::finalizeInternalInitialization() {

            // This starts SignalSlotable
            SignalSlotable::start();

            startFsm();

            KARABO_LOG_INFO << "Starting Karabo DeviceServer on host: " << net::bareHostName()
                    << ", serverId: " << m_serverId
                    << ", Broker: " << m_connection->getBrokerUrl();

            m_serverIsRunning = true;
        }


        bool DeviceServer::isRunning() const {

            return m_serverIsRunning;

        }


        void DeviceServer::registerSlots() {
            KARABO_SLOT(slotStartDevice, Hash /*configuration*/)
            KARABO_SLOT(slotKillServer)
            KARABO_SLOT(slotDeviceGone, string /*deviceId*/)
            KARABO_SLOT(slotGetClassSchema, string /*classId*/)
            KARABO_SLOT(slotLoggerPriority, string /*priority*/)
        }


        krb_log4cpp::Category & DeviceServer::log() {
            return (*m_log);
        }


        void DeviceServer::onStateUpdate(const State& currentState) {
        }


        void DeviceServer::okStateOnEntry() {

            KARABO_LOG_INFO << "DeviceServer starts up with id: " << m_serverId;

            // Check whether we have installed devices available
            updateAvailableDevices();
            if (!m_availableDevices.empty()) {
                newPluginAvailable();
            }


            BOOST_FOREACH(const Hash& device, m_autoStart) {
                slotStartDevice(device);
            }

            // Whether to scan for additional plug-ins at runtime
            if (m_scanPlugins) {
                KARABO_LOG_INFO << "Keep watching directory: " << m_pluginLoader->getPluginDirectory() << " for Device plugins";
                EventLoop::getIOService().post(util::bind_weak(&DeviceServer::scanPlugins, this, boost::system::error_code()));
            }
        }


        void DeviceServer::updateAvailableDevices() {
            const vector<string>& devices = Configurator<BaseDevice>::getRegisteredClasses();
            KARABO_LOG_INFO << "Updated list of devices available: " << karabo::util::toString(devices);


            BOOST_FOREACH(const string& device, devices) {
                if (!m_availableDevices.has(device)) {
                    Schema schema;
                    KARABO_LOG_FRAMEWORK_DEBUG << "Plugin contains device class \"" << device << "\".  Try to get schema ...";
                    try {
                        schema = BaseDevice::getSchema(device, Schema::AssemblyRules(karabo::util::READ | karabo::util::WRITE | karabo::util::INIT));
                    } catch (const std::exception& e) {
                        KARABO_LOG_ERROR << "Device \"" << device << "\" is ignored because of Schema building failure : " << e.what();
                        continue;
                    }
                    m_availableDevices.set(device, Hash("mustNotify", true, "xsd", schema));
                }
            }
        }


        void DeviceServer::scanPlugins(const boost::system::error_code& e) {

            if (e) return;

            int delay = 10; // If there is a problem, do not try too soon...
            try {
                const bool hasNewPlugins = m_pluginLoader->update();
                if (hasNewPlugins) {
                    // Update the list of available devices
                    updateAvailableDevices();
                    newPluginAvailable();
                }
                delay = 3; // usual delay
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Exception raised in scanPlugins: " << e;
            } catch (const std::exception& se) {
                KARABO_LOG_ERROR << "Standard exception raised in scanPlugins: " << se.what();
            } catch (...) {
                KARABO_LOG_ERROR << "Unknown exception raised in scanPlugins: ";
            }

            // reload timer
            m_scanPluginsTimer.expires_from_now(boost::posix_time::seconds(delay));
            m_scanPluginsTimer.async_wait(bind_weak(&DeviceServer::scanPlugins, this, boost::asio::placeholders::error));
        }


        void DeviceServer::stopDeviceServer() {
            {
                boost::mutex::scoped_lock lock(m_deviceInstanceMutex);

                // Notify all devices
                KARABO_LOG_FRAMEWORK_DEBUG << "stopServer() device map size: " << m_deviceInstanceMap.size();
                for (auto it = m_deviceInstanceMap.begin(); it != m_deviceInstanceMap.end(); ++it) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "stopServer() call slotKillDevice for " << it->first;
                    call(it->first, "slotKillDevice");
                }

                m_deviceInstanceMap.clear();
                KARABO_LOG_FRAMEWORK_DEBUG << "stopServer() device map cleared";
            }

            m_scanPluginsTimer.cancel();
            // TODO Remove from here and use the one from run() method
            m_serverIsRunning = false;
        }


        void DeviceServer::errorFoundAction(const std::string& user, const std::string& detail) {
            KARABO_LOG_ERROR << "[short] " << user;
            KARABO_LOG_ERROR << "[detailed] " << detail;
        }


        void DeviceServer::slotStartDevice(const karabo::util::Hash& configuration) {

            const boost::tuple<std::string, std::string, util::Hash>& idClassIdConfig
                    = this->prepareInstantiate(configuration);

            const std::string& deviceId = idClassIdConfig.get<0>();
            const std::string& classId = idClassIdConfig.get<1>();
            KARABO_LOG_FRAMEWORK_INFO << "Trying to start a '" << classId
                    << "' with deviceId '" << deviceId << "'...";
            KARABO_LOG_FRAMEWORK_DEBUG << "...with the following configuration:\n" << configuration;

            this->instantiate(deviceId, classId, idClassIdConfig.get<2>());
        }


        boost::tuple<std::string, std::string, util::Hash>
        DeviceServer::prepareInstantiate(const karabo::util::Hash& configuration) {

            if (configuration.has("classId")) {
                // New style
                const std::string& classId = configuration.get<string>("classId");

                // Get configuration
                Hash config(configuration.get<Hash>("configuration"));

                // Inject serverId
                config.set("_serverId_", m_serverId);

                // Inject deviceId use - sensible default in case no device instance id is supplied
                if (!configuration.has("deviceId")) {
                    config.set("_deviceId_", this->generateDefaultDeviceId(classId));
                } else if (configuration.get<string>("deviceId").empty()) {
                    config.set("_deviceId_", this->generateDefaultDeviceId(classId));
                } else {
                    config.set("_deviceId_", configuration.get<string>("deviceId"));
                }

                // Inject connection
                config.set("_connection_", m_connection);

                return boost::make_tuple(config.get<std::string>("_deviceId_"), classId, config);
            } else {
                // Old style, e.g. used for auto started devices
                const std::string& classId = configuration.begin()->getKey();

                // Get configuration
                Hash modifiedConfig(configuration);
                Hash& tmp = modifiedConfig.begin()->getValue<Hash>();
                // Inject serverId
                tmp.set("_serverId_", m_serverId);
                // Inject deviceId use - sensible default in case no device instance id is supplied
                if (!tmp.has("deviceId")) {
                    tmp.set("_deviceId_", this->generateDefaultDeviceId(classId));
                } else if (tmp.get<string>("deviceId").empty()) {
                    tmp.set("_deviceId_", this->generateDefaultDeviceId(classId));
                } else {
                    tmp.set("_deviceId_", tmp.get<string>("deviceId"));
                }

                // Inject connection
                tmp.set("_connection_", m_connection);

                const std::pair<std::string, util::Hash>& idCfg
                        = util::confTools::splitIntoClassIdAndConfiguration(modifiedConfig);
                return boost::make_tuple(tmp.get<std::string>("_deviceId_"), idCfg.first, idCfg.second);
            }
        }


        void DeviceServer::instantiate(const std::string& deviceId, const std::string& classId, const util::Hash& config) {
            try {

                BaseDevice::Pointer device = BaseDevice::create(classId, config);

                // This will throw an exception if it can't be started (because of duplicated name for example)
                device->finalizeInternalInitialization();

                {
                    // Keep the device instance
                    boost::mutex::scoped_lock lock(m_deviceInstanceMutex);
                    m_deviceInstanceMap[deviceId] = device;
                }

                // Answer initiation of device (KARABO_LOG_* is done by device)
                reply(true, deviceId); // TODO think about

            } catch (const Exception& e) {
                const std::string message("Device of class " + classId +
                                          " could not be started because: " + e.userFriendlyMsg());
                KARABO_LOG_ERROR << message;
                reply(false, message);
            } catch (const std::exception& se) {
                const std::string message("Device of class " + classId +
                                          " could not be started because of standard exception: ");
                KARABO_LOG_ERROR << message << se.what();
                reply(false, message + se.what());
            } catch (...) {
                const std::string message("Device of class " + classId +
                                          " could not be started because of unknown exception");
                KARABO_LOG_ERROR << message;
                reply(false, message);
            }
        }


        void DeviceServer::newPluginAvailable() {
            vector<string> deviceClasses;
            vector<int> visibilities;
            deviceClasses.reserve(m_availableDevices.size());

            for (Hash::iterator it = m_availableDevices.begin(); it != m_availableDevices.end(); ++it) {
                const std::string& deviceClass = it->getKey();
                if (std::find(m_devices.begin(), m_devices.end(), deviceClass) != m_devices.end()) {
                    deviceClasses.push_back(it->getKey());

                    Hash& tmp = it->getValue<Hash>();
                    if (tmp.get<bool>("mustNotify") == true) {
                        tmp.set("mustNotify", false);
                    }
                    visibilities.push_back(tmp.get<Schema>("xsd").getDefaultValue<int>("visibility"));
                }
            }
            KARABO_LOG_DEBUG << "Sending instance update as new device plugins are available: "
                    << karabo::util::toString(deviceClasses);
            this->updateInstanceInfo(Hash("deviceClasses", deviceClasses, "visibilities", visibilities));
        }


        void DeviceServer::noStateTransition(const std::string& typeId, int state) {
            string eventName(typeId);
            boost::regex re(".*\\d+(.+Event).*");
            boost::smatch what;
            bool result = boost::regex_search(typeId, what, re);
            if (result && what.size() == 2) {
                eventName = what.str(1);
            }
            KARABO_LOG_WARN << "Current state of server \"" << getInstanceId() << "\" does not allow a transition for event \"" << eventName << "\"";
        }


        void DeviceServer::slotKillServer() {

            KARABO_LOG_INFO << "Received kill signal";

            reply(m_serverId);

            // Terminate the process which will call our destructor through the signal handling
            // implemented in main() in deviceServer.cc.
            std::raise(SIGTERM);
            KARABO_LOG_FRAMEWORK_DEBUG << "slotKillServer DONE";
        }


        void DeviceServer::slotDeviceGone(const std::string & instanceId) {

            KARABO_LOG_FRAMEWORK_INFO << "Device '" << instanceId << "' notifies '" << this->getInstanceId()
                    << "' about its future death.";

            boost::mutex::scoped_lock lock(m_deviceInstanceMutex);
            if (m_deviceInstanceMap.erase(instanceId) > 0) {
                KARABO_LOG_INFO << "Device '" << instanceId << "' removed from server.";
            }
        }


        void DeviceServer::slotGetClassSchema(const std::string& classId) {
            Schema schema = BaseDevice::getSchema(classId);
            reply(schema, classId, this->getInstanceId());
        }


        std::string DeviceServer::generateDefaultDeviceId(const std::string & classId) {
            const string index = karabo::util::toString(++m_deviceInstanceCount[classId]);
            // Prepare shortened Device-Server name
            vector<string> tokens;
            string domain = m_serverId;
            boost::split(tokens, m_serverId, boost::is_any_of("_"));
            if (tokens.back() == karabo::util::toString(getpid())) {
                domain = tokens.front() + "-" + tokens.back();
            }
            return domain + "_" + classId + "_" + index;
        }


        void DeviceServer::slotLoggerPriority(const std::string& newprio) {
            using namespace krb_log4cpp;
            string oldprio = Logger::getPriority();
            Logger::setPriority(newprio);
            KARABO_LOG_INFO << "Logger Priority changed : " << oldprio << " ==> " << newprio;
        }
    }
}
