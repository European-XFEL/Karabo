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

#include <karabo/net/utils.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/Version.hh>
#include <karabo/util/Configurator.hh>
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

            CHOICE_ELEMENT(expected).key("connection")
                    .displayedName("Connection")
                    .description("The connection to the communication layer of the distributed system")
                    .appendNodesOfConfigurationBase<BrokerConnection>()
                    .assignmentOptional().defaultValue("Jms")
                    .expertAccess()
                    .init()
                    .commit();

            BOOL_ELEMENT(expected).key("isMaster")
                    .displayedName("Is Master Server?")
                    .description("Decides whether this device-server runs as a master")
                    .assignmentOptional().defaultValue(false)
                    .commit();

            BOOL_ELEMENT(expected).key("debugMode")
                    .displayedName("Is Debug Mode?")
                    .description("Decides whether this device-server runs in debug or production mode")
                    .adminAccess()
                    .assignmentOptional().defaultValue(false)
                    .commit();

            INT32_ELEMENT(expected).key("heartbeatInterval")
                    .displayedName("Heartbeat interval")
                    .description("The heartbeat interval")
                    .assignmentOptional().defaultValue(10)
                    .adminAccess()
                    .commit();

            INT32_ELEMENT(expected).key("nThreads")
                    .displayedName("Number of threads")
                    .description("Defines the number of threads that can be used to work on incoming events")
                    .assignmentOptional().defaultValue(2)
                    .minInc(1)
                    .adminAccess()
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

            PATH_ELEMENT(expected)
                    .key("pluginDirectory")
                    .displayedName("Plugin Directory")
                    .description("Directory to search for plugins")
                    .assignmentOptional().defaultValue("plugins")
                    .isDirectory()
                    .expertAccess()
                    .commit();

            NODE_ELEMENT(expected).key("Logger")
                    .description("Logging settings")
                    .displayedName("Logger")
                    .appendParametersOfConfigurableClass<Logger>("Logger")
                    .commit();

            NODE_ELEMENT(expected).key("Logger.rollingFile")
                    .description("Log Appender settings for file")
                    .displayedName("Rolling File Appender")
                    .appendParametersOfConfigurableClass<AppenderConfigurator>("RollingFile")
                    .expertAccess()
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.rollingFile.layout")
                    .setNewDefaultValue("Pattern")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.rollingFile.layout.Pattern.format")
                    .setNewDefaultValue("%d{%F %H:%M:%S} %p  %c  : %m%n")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.rollingFile.filename")
                    .setNewDefaultValue("device-server.log")
                    .commit();

            NODE_ELEMENT(expected).key("Logger.network")
                    .description("Log Appender settings for Network")
                    .displayedName("Network Appender")
                    .appendParametersOfConfigurableClass<AppenderConfigurator>("Network")
                    .expertAccess()
                    .commit();

            NODE_ELEMENT(expected).key("Logger.ostream")
                    .description("Log Appender settings for terminal")
                    .displayedName("Ostream Appender")
                    .appendParametersOfConfigurableClass<AppenderConfigurator>("Ostream")
                    .expertAccess()
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.ostream.layout")
                    .setNewDefaultValue("Pattern")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.ostream.layout.Pattern.format")
                    .setNewDefaultValue("%p  %c  : %m%n")
                    .commit();

            NODE_ELEMENT(expected).key("Logger.karabo")
                    .description("Logger category for karabo framework")
                    .displayedName("Karabo framework logger")
                    .appendParametersOfConfigurableClass<CategoryConfigurator>("Category")
                    .expertAccess()
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.karabo.name")
                    .setNewAssignmentOptional()
                    .setNewDefaultValue("karabo")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.karabo.additivity")
                    .setNewDefaultValue(false)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.karabo.appenders")
                    .setNewDefaultValue("RollingFile")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.karabo.appenders.RollingFile.layout")
                    .setNewDefaultValue("Pattern")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.karabo.appenders.RollingFile.layout.Pattern.format")
                    .setNewDefaultValue("%d{%F %H:%M:%S} %p  %c  : %m%n")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.karabo.appenders.RollingFile.filename")
                    .setNewDefaultValue("device-server.log")
                    .commit();
        }


        DeviceServer::DeviceServer(const karabo::util::Hash& config) : m_log(0) {

            string serverIdFileName("serverId.xml");

            // Set serverId
            if (boost::filesystem::exists(serverIdFileName)) {
                Hash hash;
                karabo::io::loadFromFile(hash, serverIdFileName);
                if (config.has("serverId")) {
                    config.get("serverId", m_serverId);
                    // Update file for next startup
                    karabo::io::saveToFile(Hash("DeviceServer.serverId", m_serverId), serverIdFileName);
                } else if (hash.has("DeviceServer.serverId")) hash.get("DeviceServer.serverId", m_serverId);
                else {
                    KARABO_LOG_FRAMEWORK_WARN << "Found serverId.xml without serverId contained";
                    m_serverId = generateDefaultServerId();
                    karabo::io::saveToFile(Hash("DeviceServer.serverId", m_serverId), serverIdFileName);
                }
            } else { // No file
                if (config.has("serverId")) {
                    config.get("serverId", m_serverId);
                } else {
                    m_serverId = generateDefaultServerId();
                }
                // Generate file for next startup
                karabo::io::saveToFile(Hash("DeviceServer.serverId", m_serverId), serverIdFileName);
            }

            // Device configurations for those to automatically start
            if (config.has("autoStart")) config.get("autoStart", m_autoStart);

            // Whether to scan for additional plug-ins at runtime
            config.get("scanPlugins", m_scanPlugins);

            // What visibility this server should have
            config.get("visibility", m_visibility);

            // Deprecate the isMaster in future
            config.get("debugMode", m_debugMode);
            config.get("isMaster", m_isMaster);
            if (m_isMaster) {

                //                cerr << "\n#### WARNING ####\nThe \"isMaster\" option will be deprecated!\n"
                //                        << "If you were using the startMasterDeviceServer script,\n"
                //                        << "please delete the whole masterDeviceServer folder and use \"make test\" (in base folder) instead."
                //                        << "\n##################\n\n";

                m_visibility = 5;
                m_scanPlugins = false;
                m_autoStart.resize(3);
                m_autoStart[0] = Hash("GuiServerDevice.deviceId", "Karabo_GuiServer_0");
                m_autoStart[1] = Hash("DataLoggerManager.deviceId", "Karabo_DataLoggerManager_0");
                m_autoStart[2] = Hash("ProjectManager.deviceId", "Karabo_ProjectManager");
            }

            m_connectionConfiguration = config.get<Hash>("connection");
            m_connection = BrokerConnection::createChoice("connection", config);
            // Real connection parameters were chosen in above call.  Use the same parameters for logger
            m_connectionConfiguration.set("Jms.hostname", m_connection->getBrokerHostname() + ":" + toString(m_connection->getBrokerPort()));
            m_connectionConfiguration.set("Jms.port", m_connection->getBrokerPort());
            m_pluginLoader = PluginLoader::create("PluginLoader", Hash("pluginDirectory", config.get<string>("pluginDirectory")));
            loadLogger(config);

            m_heartbeatIntervall = config.get<int>("heartbeatInterval");

            setNumberOfThreads(config.get<int>("nThreads"));
        }


        std::string DeviceServer::generateDefaultServerId() const {
            return net::bareHostName() += "_Server_" + util::toString(getpid());
        }


        bool DeviceServer::isDebugMode() {
            return m_debugMode;
        };


        DeviceServer::~DeviceServer() {
            KARABO_LOG_FRAMEWORK_TRACE << "DeviceServer::~DeviceServer() dtor : m_logger.use_count()=" << m_logger.use_count();
            m_logger.reset();
        }


        void DeviceServer::loadLogger(const Hash& input) {

            Hash config = input.get<Hash>("Logger");


            // make a copy of additional appenders defined by user
            vector<Hash> appenders = config.get < vector<Hash> >("appenders");

            // handle predefined DeviceServer appenders
            vector<Hash> newAppenders(3, Hash());
            newAppenders[0].set("Ostream", config.get<Hash>("ostream"));
            newAppenders[1].set("RollingFile", config.get<Hash>("rollingFile"));
            newAppenders[2].set("Network", config.get<Hash>("network"));



            config.erase("ostream");
            config.erase("rollingFile");
            config.erase("network");

            for (size_t i = 0; i < appenders.size(); ++i) {
                if (appenders[i].has("Ostream")) {
                    if (appenders[i].get<string>("Ostream.name") == "default")
                        continue;
                }
                newAppenders.push_back(appenders[i]);
            }


            config.set("appenders", newAppenders);

            // network appender has fixed format (the one expected by GUI)
            config.set("appenders[2].Network.connection", m_connectionConfiguration);

            Hash category = config.get<Hash>("karabo");
            category.set("name", "karabo");
            config.set("categories[0].Category", category);
            config.set("categories[0].Category.appenders[1].Ostream.layout.Pattern.format", "%p  %c  : %m%n");
            config.erase("karabo");
            //cerr << "loadLogger final:" << endl << config << endl;
            m_logger = Logger::configure(config);
        }


        void DeviceServer::loadPluginLoader(const Hash& input) {
            m_pluginLoader = PluginLoader::createNode("PluginLoader", "PluginLoader", input);
        }


        void DeviceServer::run() {

            m_serverIsRunning = true;

            // Initialize category
            m_log = &(karabo::log::Logger::getLogger(m_serverId));

            const std::string hostName = net::bareHostName();
            KARABO_LOG_INFO << "Starting Karabo DeviceServer on host: " << hostName
                    << ", serverId: " << m_serverId
                    << ", Broker (host:port:topic): " << m_connectionConfiguration.get<string>("Jms.hostname") << ":"
                    << m_connectionConfiguration.get<string>("Jms.destinationName");

            // Initialize SignalSlotable instance
            init(m_serverId, m_connection);

            registerAndConnectSignalsAndSlots();

            karabo::util::Hash instanceInfo;
            instanceInfo.set("type", "server");
            instanceInfo.set("serverId", m_serverId);
            instanceInfo.set("version", karabo::util::Version::getVersion());
            instanceInfo.set("host", hostName);
            instanceInfo.set("visibility", m_visibility);
            boost::thread t(boost::bind(&karabo::core::DeviceServer::runEventLoop, this, m_heartbeatIntervall, instanceInfo));

            boost::this_thread::sleep(boost::posix_time::milliseconds(100));

            bool ok = ensureOwnInstanceIdUnique();
            if (!ok) {
                // There is already an ERROR from ensureOwnInstanceIdUnique, but that is not broadcasted.
                KARABO_LOG_ERROR << "DeviceServer '" << m_serverId << " could not start since its id already exists.";
                t.join(); // Blocks
                return;
            }

            this->startFsm();
            t.join();

            // TODO That should be the right solution, but we never get here
            m_serverIsRunning = false;
        }


        bool DeviceServer::isRunning() const {

            return m_serverIsRunning;

        }


        void DeviceServer::registerAndConnectSignalsAndSlots() {
            KARABO_SLOT1(slotStartDevice, Hash /*configuration*/)
            KARABO_SLOT0(slotKillServer)
            KARABO_SLOT1(slotDeviceGone, string /*deviceId*/)
            KARABO_SLOT1(slotGetClassSchema, string /*classId*/)
            KARABO_SLOT1(slotLoggerPriority, string /*priority*/)
        }


        krb_log4cpp::Category & DeviceServer::log() {
            return (*m_log);
        }


        void DeviceServer::onStateUpdate(const std::string& currentState) {
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

            if (m_scanPlugins) {
                KARABO_LOG_INFO << "Keep watching directory: " << m_pluginLoader->getPluginDirectory() << " for Device plugins";
                m_pluginThread = boost::thread(boost::bind(&karabo::core::DeviceServer::scanPlugins, this));
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


        void DeviceServer::scanPlugins() {
            m_doScanPlugins = true;
            while (m_doScanPlugins) {
                int delay = 3000;
                try {
                    bool hasNewPlugins = m_pluginLoader->update();
                    if (hasNewPlugins) {
                        // Update the list of available devices
                        updateAvailableDevices();
                        newPluginAvailable();
                    }
                } catch (const Exception& e) {
                    KARABO_LOG_ERROR << "Exception raised in scanPlugins: " << e;
                    delay = 10000;
                } catch (const std::exception& se) {
                    KARABO_LOG_ERROR << "Standard exception raised in scanPlugins: " << se.what();
                    delay = 10000;
                } catch (...) {
                    KARABO_LOG_ERROR << "Unknown exception raised in scanPlugins: ";
                    delay = 10000;
                }
                boost::this_thread::sleep(boost::posix_time::milliseconds(delay));
            }
        }


        void DeviceServer::stopDeviceServer() {
            m_doScanPlugins = false;
            if (m_pluginThread.joinable()) m_pluginThread.join();
            stopEventLoop();
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
                config.set("_connection_", m_connectionConfiguration);

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
                tmp.set("_connection_", m_connectionConfiguration);

                const std::pair<std::string, util::Hash>& idCfg
                        = util::confTools::splitIntoClassIdAndConfiguration(modifiedConfig);
                return boost::make_tuple(tmp.get<std::string>("_deviceId_"), idCfg.first, idCfg.second);
            }
        }


        void DeviceServer::instantiate(const std::string& deviceId, const std::string& classId, const util::Hash& config) {
            try {

                boost::mutex::scoped_lock lock(m_deviceInstanceMutex);

                // Add a place (pointer by ref!) for the device thread in the instance map:
                boost::thread*& deviceThread = m_deviceInstanceMap[deviceId];
                if (deviceThread) {
                    // There is already such a device. If joining works, it's just a "zombie".
                    if (deviceThread->try_join_for(boost::chrono::milliseconds(100))) {
                        m_deviceThreads.remove_thread(deviceThread);
                        delete deviceThread;
                    } else {
                        std::string message("Device of class " + classId + " could not be started: ");
                        reply(false, ((message += "deviceId '") += deviceId) += "' already exists on server.");
                        KARABO_LOG_ERROR << message;
                        return;
                    }
                }
                BaseDevice::Pointer device = BaseDevice::create(classId, config); // TODO If constructor blocks, we are lost here!!
                if (!device) {
                    m_deviceInstanceMap.erase(deviceId);
                    throw KARABO_PARAMETER_EXCEPTION("Failed to create device of class " + classId + " with configuration...");
                }

                device->injectConnection(deviceId, m_connection);

                // Create the thread for the device and place it (indirectly) into the device instance map):
                deviceThread = m_deviceThreads.create_thread(boost::bind(&karabo::core::BaseDevice::run, device));

                // Answer initiation of device (KARABO_LOG_* is done by device)
                reply(true, deviceId); // TODO think about

            } catch (const Exception& e) {
                const std::string message("Device of class " + classId + " could not be started because: " + e.userFriendlyMsg());
                KARABO_LOG_ERROR << message;
                reply(false, message);
            } catch (const std::exception& se) {
                const std::string message("Device of class " + classId + " could not be started because of standard exception: ");
                KARABO_LOG_ERROR << message << se.what();
                reply(false, message + se.what());
            } catch (...) {
                const std::string message("Device of class " + classId + " could not be started because of unknown exception");
                KARABO_LOG_ERROR << message;
                reply(false, message);
            }
        }


        void DeviceServer::newPluginAvailable() {
            vector<string> deviceClasses;
            vector<int> visibilities;
            deviceClasses.reserve(m_availableDevices.size());

            for (Hash::iterator it = m_availableDevices.begin(); it != m_availableDevices.end(); ++it) {
                deviceClasses.push_back(it->getKey());

                Hash& tmp = it->getValue<Hash>();
                if (tmp.get<bool>("mustNotify") == true) {
                    tmp.set("mustNotify", false);
                }
                visibilities.push_back(tmp.get<Schema>("xsd").getDefaultValue<int>("visibility"));
            }
            KARABO_LOG_DEBUG << "Sending instance update as new device plugins are available: " << karabo::util::toString(deviceClasses);
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

            {
                boost::mutex::scoped_lock lock(m_deviceInstanceMutex);


                // Notify all devices
                for (DeviceInstanceMap::const_iterator it = m_deviceInstanceMap.begin(); it != m_deviceInstanceMap.end(); ++it) {
                    call(it->first, "slotKillDevice");
                }

                for (DeviceInstanceMap::iterator it = m_deviceInstanceMap.begin(); it != m_deviceInstanceMap.end(); ++it) {
                    it->second->join();
                    m_deviceThreads.remove_thread(it->second);
                    delete it->second;
                }

                m_deviceInstanceMap.clear();

            }

            // Reply the same
            reply(m_serverId);

            // Stop device server
            stopDeviceServer();
            KARABO_LOG_FRAMEWORK_DEBUG << "slotKillServer DONE";
        }


        void DeviceServer::slotDeviceGone(const std::string & instanceId) {

            KARABO_LOG_FRAMEWORK_INFO << "Device '" << instanceId << "' notifies '" << this->getInstanceId()
                    << "' about its future death.";

            boost::mutex::scoped_lock lock(m_deviceInstanceMutex);

            DeviceInstanceMap::iterator it = m_deviceInstanceMap.find(instanceId);
            if (it != m_deviceInstanceMap.end()) {
                it->second->join();
                m_deviceThreads.remove_thread(it->second);
                delete it->second;
                m_deviceInstanceMap.erase(it);
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
            string oldprio = Priority::getPriorityName(m_logger->getLogger<Self>().getRootPriority());
            m_logger->getLogger<Self>().setRootPriority(Priority::getPriorityValue(newprio));
            KARABO_LOG_INFO << "Logger Priority changed : " << oldprio << " ==> " << newprio;
        }
    }
}
