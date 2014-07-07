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
#include <karabo/util/Version.hh>
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
                    .advanced()
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
                    .advanced()
                    .commit();

            NODE_ELEMENT(expected).key("Logger.ostream")
                    .description("Log Appender settings for terminal")
                    .displayedName("Ostream Appender")
                    .appendParametersOfConfigurableClass<AppenderConfigurator>("Ostream")
                    .advanced()
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
                    .advanced()
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


        DeviceServer::DeviceServer(const karabo::util::Hash& input) : m_log(0) {

            string serverIdFileName("serverId.xml");

            // Set serverId
            if (boost::filesystem::exists(serverIdFileName)) {
                Hash hash;
                karabo::io::loadFromFile(hash, serverIdFileName);
                if (input.has("serverId")) {
                    input.get("serverId", m_serverId);
                    // Update file for next startup
                    karabo::io::saveToFile(Hash("DeviceServer.serverId", m_serverId), serverIdFileName);
                } else if (hash.has("DeviceServer.serverId")) hash.get("DeviceServer.serverId", m_serverId);
                else {
                    KARABO_LOG_FRAMEWORK_WARN << "Found serverId.xml without serverId contained";
                    m_serverId = generateDefaultServerId();
                    karabo::io::saveToFile(Hash("DeviceServer.serverId", m_serverId), serverIdFileName);
                }
            } else { // No file
                if (input.has("serverId")) {
                    input.get("serverId", m_serverId);
                } else {
                    m_serverId = generateDefaultServerId();
                }
                // Generate file for next startup
                karabo::io::saveToFile(Hash("DeviceServer.serverId", m_serverId), serverIdFileName);
            }

            // Device configurations for those to automatically start
            if (input.has("autoStart")) input.get("autoStart", m_autoStart);

            // Whether to scan for additional plug-ins at runtime
            input.get("scanPlugins", m_scanPlugins);

            // What visibility this server should have
            input.get("visibility", m_visibility);

            // Deprecate the isMaster in future
            input.get("isMaster", m_isMaster);
            if (m_isMaster) {

                //                cerr << "\n#### WARNING ####\nThe \"isMaster\" option will be deprecated!\n"
                //                        << "If you were using the startMasterDeviceServer script,\n"
                //                        << "please delete the whole masterDeviceServer folder and use \"make test\" (in base folder) instead."
                //                        << "\n##################\n\n";

                m_visibility = 5;
                m_scanPlugins = false;
                m_autoStart.resize(2);
                m_autoStart[0] = Hash("GuiServerDevice.deviceId", "Karabo_GuiServer_0");
                m_autoStart[1] = Hash("FileDataLogger.deviceId", "Karabo_FileDataLogger_0");
            }

            m_connectionConfiguration = input.get<Hash>("connection");
            m_connection = BrokerConnection::createChoice("connection", input);
            m_pluginLoader = PluginLoader::create("PluginLoader", Hash("pluginDirectory", input.get<string>("pluginDirectory")));
            loadLogger(input);
        }


        std::string DeviceServer::generateDefaultServerId() const {
            return string(boost::asio::ip::host_name() + "_Server_" + karabo::util::toString(getpid()));
        }


        DeviceServer::~DeviceServer() {
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

            config.set("appenders[2].Network.layout", Hash());
            config.set("appenders[2].Network.layout.Pattern.format", "%d{%F %H:%M:%S} | %p | %c | %m");
            config.set("appenders[2].Network.connection", m_connectionConfiguration);

            Hash category = config.get<Hash>("karabo");
            category.set("name", "karabo");
            config.set("categories[0].Category", category);
            config.set("categories[0].Category.appenders[1].Ostream.layout.Pattern.format", "%p  %c  : %m%n");
            config.erase("karabo");
            // cerr << "loadLogger final:" << endl << config << endl;
            Logger::configure(config);
        }


        void DeviceServer::loadPluginLoader(const Hash& input) {
            m_pluginLoader = PluginLoader::createNode("PluginLoader", "PluginLoader", input);
        }


        void DeviceServer::run() {

            // Initialize category
            m_log = &(karabo::log::Logger::getLogger(m_serverId));

            KARABO_LOG_INFO << "Starting Karabo DeviceServer on host: " << boost::asio::ip::host_name();

            // Initialize SignalSlotable instance
            init(m_serverId, m_connection);

            registerAndConnectSignalsAndSlots();

            karabo::util::Hash instanceInfo;
            instanceInfo.set("type", "server");
            instanceInfo.set("serverId", m_serverId);
            instanceInfo.set("version", karabo::util::Version::getVersion());
            instanceInfo.set("host", boost::asio::ip::host_name());
            instanceInfo.set("visibility", m_visibility);
            boost::thread t(boost::bind(&karabo::core::DeviceServer::runEventLoop, this, 20, instanceInfo));
            this->startFsm();
            t.join();

        }


        void DeviceServer::registerAndConnectSignalsAndSlots() {
            SIGNAL3("signalNewDeviceClassAvailable", string /*serverId*/, string /*classId*/, Schema /*classSchema*/)
            SIGNAL3("signalClassSchema", karabo::util::Schema /*classSchema*/, string /*classId*/, string /*deviceId*/);
            SLOT1(slotStartDevice, Hash /*configuration*/)
            SLOT0(slotKillServer)
            SLOT1(slotDeviceGone, string /*deviceId*/)
            SLOT1(slotGetClassSchema, string /*classId*/)
            
            // Connect to global slot(s))
            connectN("", "signalClassSchema", "*", "slotClassSchema");
            connectN("", "signalNewDeviceClassAvailable", "*", "slotNewDeviceClassAvailable");
        }


        log4cpp::Category & DeviceServer::log() {
            return (*m_log);
        }


        void DeviceServer::onStateUpdate(const std::string& currentState) {
        }


        void DeviceServer::idleStateOnEntry() {

            KARABO_LOG_INFO << "DeviceServer starts up with id: " << m_serverId;

            // Check whether we have installed devices available
            updateAvailableDevices();
            if (!m_availableDevices.empty()) {
                inbuildDevicesAvailable();
            }


            BOOST_FOREACH(Hash device, m_autoStart) {
                slotStartDevice(device);
            }

            if (m_scanPlugins) {
                KARABO_LOG_INFO << "Keep watching directory: " << m_pluginLoader->getPluginDirectory() << " for Device plugins";
                m_pluginThread = boost::thread(boost::bind(&karabo::core::DeviceServer::scanPlugins, this));
            }
        }


        void DeviceServer::updateAvailableDevices() {
            vector<string> devices = Configurator<BaseDevice>::getRegisteredClasses();
            KARABO_LOG_INFO << "Updated list of devices available: " << karabo::util::toString(devices);


            BOOST_FOREACH(string device, devices) {
                if (!m_availableDevices.has(device)) {
                    Schema schema = BaseDevice::getSchema(device, Schema::AssemblyRules(karabo::util::READ | karabo::util::WRITE | karabo::util::INIT));
                    m_availableDevices.set(device, Hash("mustNotify", true, "xsd", schema));
                }
            }
        }


        void DeviceServer::scanPlugins() {
            try {
                bool inError = false;
                m_serverIsRunning = true;
                while (m_serverIsRunning) {
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
                //stopEventLoop();

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Exception raised in scanPlugins: " << e;
            } catch (...) {
                KARABO_LOG_ERROR << "Unknown exception raised in scanPlugins: ";
            }
        }


        void DeviceServer::stopDeviceServer() {
            m_serverIsRunning = false;
            if (m_pluginThread.joinable()) m_pluginThread.join();
            stopEventLoop();
        }


        void DeviceServer::errorFoundAction(const std::string& user, const std::string& detail) {
            KARABO_LOG_ERROR << "[short] " << user;
            KARABO_LOG_ERROR << "[detailed] " << detail;
        }


        void DeviceServer::endErrorAction() {

        }


        void DeviceServer::startDeviceAction(const karabo::util::Hash& hash) {

            if (hash.has("classId")) {
                instantiateNew(hash);
            } else {
                instantiateOld(hash);
            }
        }


        void DeviceServer::instantiateNew(const karabo::util::Hash& hash) {
            try {

                std::string classId = hash.get<string>("classId");

                KARABO_LOG_INFO << "Trying to start " << classId << "...";
                KARABO_LOG_DEBUG << "with the following configuration:\n" << hash;

                // Get configuration
                Hash config = hash.get<Hash>("configuration");

                // Inject serverId
                config.set("_serverId_", m_serverId);

                // Inject deviceId
                if (!hash.has("deviceId")) {
                    config.set("_deviceId_", this->generateDefaultDeviceId(classId));
                } else if (hash.get<string>("deviceId").empty()) {
                    config.set("_deviceId_", this->generateDefaultDeviceId(classId));
                } else {
                    config.set("_deviceId_", hash.get<string>("deviceId"));
                }

                BaseDevice::Pointer device = BaseDevice::create(classId, config); // TODO If constructor blocks, we are lost here!!
                boost::thread* t = m_deviceThreads.create_thread(boost::bind(&karabo::core::BaseDevice::run, device));

                // Associate deviceInstance with its thread
                string deviceInstanceId = device->getInstanceId();
                m_deviceInstanceMap[deviceInstanceId] = t;

                // Answer initiation of device
                reply(true, deviceInstanceId); // TODO think about

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Device could not be started because: " << e.userFriendlyMsg();
                reply(false, "Device could not be started because: " + e.userFriendlyMsg());
                return;
            }
        }


        void DeviceServer::instantiateOld(const karabo::util::Hash& hash) {
            try {
                std::string classId = hash.begin()->getKey();

                KARABO_LOG_INFO << "Trying to start " << classId << "...";
                KARABO_LOG_DEBUG << "with the following configuration:\n" << hash;

                // Inject device-server information
                Hash modifiedConfig(hash);
                Hash& tmp = modifiedConfig.begin()->getValue<Hash>();
                tmp.set("_serverId_", m_serverId);
                // Apply sensible default in case no device instance id is supplied
                if (!tmp.has("deviceId")) {
                    tmp.set("_deviceId_", this->generateDefaultDeviceId(classId));
                } else if (tmp.get<string>("deviceId").empty()) {
                    tmp.set("_deviceId_", this->generateDefaultDeviceId(classId));
                } else {
                    tmp.set("_deviceId_", tmp.get<string>("deviceId"));
                }
                BaseDevice::Pointer device = BaseDevice::create(modifiedConfig); // TODO If constructor blocks, we are lost here!!
                boost::thread* t = m_deviceThreads.create_thread(boost::bind(&karabo::core::BaseDevice::run, device));

                // Associate deviceInstance with its thread
                string deviceInstanceId = device->getInstanceId();
                m_deviceInstanceMap[deviceInstanceId] = t;

                // Answer initiation of device
                reply(true, deviceInstanceId); // TODO think about

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Device could not be started because: " << e.userFriendlyMsg();
                reply(false, "Device could not be started because: " + e.userFriendlyMsg());
                return;
            }

        }


        void DeviceServer::notifyNewDeviceAction() {
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

            // Reply the same
            reply(m_serverId);

            // Stop device server
            stopDeviceServer();
        }


        void DeviceServer::slotDeviceGone(const std::string & instanceId) {

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


        void DeviceServer::slotGetClassSchema(const std::string & classId) {
            Schema schema = BaseDevice::getSchema(classId);
            emit("signalClassSchema", schema, classId, this->getInstanceId());
            reply(schema);
        }


        std::string DeviceServer::generateDefaultDeviceId(const std::string & classId) {
            string index = karabo::util::toString(++m_deviceInstanceCount[classId]);
            // Prepare shortened Device-Server name
            vector<string> tokens;
            string domain = m_serverId;
            boost::split(tokens, m_serverId, boost::is_any_of("_"));
            if (tokens.back() == karabo::util::toString(getpid())) {
                domain = tokens.front() + "-" + tokens.back();
            }
            return domain + "_" + classId + "_" + index;
        }
    }
}
