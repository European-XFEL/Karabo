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

#include <unistd.h>

// temp change to trigger CI

#include <boost/algorithm/string.hpp>
#include <boost/bind/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/tuple/tuple.hpp>
#include <csignal>
#include <cstdlib>

#include "Device.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/io/Input.hh"
#include "karabo/io/Output.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/Strand.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/ChoiceElement.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/ListElement.hh"
#include "karabo/util/NodeElement.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/VectorElement.hh"
#include "karabo/util/Version.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::DeviceServer)

namespace karabo {

    namespace core {

        // template class Runner<DeviceServer>;

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::log;
        using namespace karabo::net;
        using namespace karabo::xms;
        using namespace krb_log4cpp;
        using namespace boost::placeholders;

        void DeviceServer::expectedParameters(Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("serverId")
                  .displayedName("Server ID")
                  .description(
                        "The device-server instance id uniquely identifies a device-server instance in the distributed "
                        "system")
                  .assignmentOptional()
                  .noDefaultValue()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("hostName")
                  .displayedName("Forced Hostname")
                  .description(
                        "The hostname can be optionally forced to a specific "
                        "string. The host's definition will be used if not specified.")
                  .assignmentOptional()
                  .noDefaultValue()
                  .expertAccess()
                  .init()
                  .commit();

            // clang-format off
            std::vector<int> visibilityOptions = {
                Schema::AccessLevel::OBSERVER,
                Schema::AccessLevel::USER,
                Schema::AccessLevel::OPERATOR,
                Schema::AccessLevel::EXPERT,
                Schema::AccessLevel::ADMIN};
            // clang-format on

            INT32_ELEMENT(expected)
                  .key("visibility")
                  .displayedName("Visibility")
                  .description("Configures who is allowed to see this server at all")
                  .assignmentOptional()
                  .defaultValue(Schema::AccessLevel::OBSERVER)
                  .options(visibilityOptions)
                  .adminAccess()
                  .init()
                  .commit();

            CHOICE_ELEMENT(expected)
                  .key("connection")
                  .displayedName("Connection")
                  .description("The connection to the communication layer of the distributed system")
                  .appendNodesOfConfigurationBase<karabo::net::Broker>()
                  .assignmentOptional()
                  .defaultValue(karabo::net::Broker::brokerTypeFromEnv())
                  .expertAccess()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("heartbeatInterval")
                  .displayedName("Heartbeat interval")
                  .description("The heartbeat interval")
                  .assignmentOptional()
                  .defaultValue(10)
                  .minInc(10) // avoid too much traffic
                  .adminAccess()
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("deviceClasses")
                  .displayedName("Device Classes")
                  .description("The devices classes the server will manage")
                  .assignmentOptional()
                  .defaultValue(BaseDevice::getRegisteredClasses())
                  .expertAccess()
                  .commit();

            LIST_ELEMENT(expected)
                  .key("autoStart")
                  .displayedName("Auto start")
                  .description("Auto starts selected devices")
                  .appendNodesOfConfigurationBase<karabo::core::BaseDevice>()
                  .assignmentOptional()
                  .noDefaultValue()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("scanPlugins")
                  .displayedName("Scan plug-ins?")
                  .description(
                        "Decides whether the server will scan the content of the plug-in folder and dynamically load "
                        "found devices")
                  .expertAccess()
                  .assignmentOptional()
                  .defaultValue(true)
                  .commit();

            PATH_ELEMENT(expected)
                  .key("pluginDirectory")
                  .displayedName("Plugin Directory")
                  .description("Directory to search for plugins")
                  .assignmentOptional()
                  .defaultValue(PluginLoader::defaultPluginPath())
                  .isDirectory()
                  .expertAccess()
                  .commit();

            NODE_ELEMENT(expected)
                  .key("Logger")
                  .description("Logging settings")
                  .displayedName("Logger")
                  .appendParametersOf<Logger>()
                  .commit();

            OVERWRITE_ELEMENT(expected).key("Logger.file.filename").setNewDefaultValue("device-server.log").commit();

            STRING_ELEMENT(expected)
                  .key("timeServerId")
                  .displayedName("TimeServer ID")
                  .description("The instance id uniquely identifies a TimeServer instance in the distributed system")
                  .assignmentOptional()
                  .defaultValue("")
                  .commit();
        }

        DeviceServer::DeviceServer(const karabo::util::Hash& config)
            : m_log(0),
              m_scanPluginsTimer(EventLoop::getIOService()),
              m_timeId(0ull),
              m_timeSec(0ull),
              m_timeFrac(0ull),
              m_timePeriod(1ull) // non-zero as double protection against division by zero in DeviceServer::timeTick
              ,
              m_noTimeTickYet(true),
              m_timeIdLastTick(0ull),
              m_timeTickerTimer(EventLoop::getIOService()) {
            if (config.has("serverId")) {
                config.get("serverId", m_serverId);
            } else {
                m_serverId = generateDefaultServerId();
            }

            config.get("deviceClasses", m_deviceClasses);

            // Device configurations for those to automatically start
            if (config.has("autoStart")) config.get("autoStart", m_autoStart);

            // Whether to scan for additional plug-ins at runtime
            config.get("scanPlugins", m_scanPlugins);

            // What visibility this server should have
            config.get("visibility", m_visibility);

            // What is the TimeServer ID
            config.get("timeServerId", m_timeServerId);

            // if a hostname ought to be forced
            if (config.has("hostName")) {
                config.get("hostName", m_hostname);
            } else {
                m_hostname = net::bareHostName();
            }

            // Load logger before creating broker connection to log that being done.
            // Requires that there is no logging to the broker as we had before 2.17.0.
            loadLogger(config);

            // For a choice element, there is exactly one sub-Hash where the key is the chosen (here: Broker) sub-class.
            // We have to transfer the instance id and thus copy the relevant part of the (const) config.
            // (Otherwise we could just pass 'input' to createChoice(..).)
            Hash brokerConfig("connection", config.get<Hash>("connection"));
            Hash& connectionCfg = brokerConfig.get<Hash>("connection").begin()->getValue<Hash>();
            connectionCfg.set("instanceId", m_serverId);
            m_connection = Configurator<Broker>::createChoice("connection", brokerConfig);
            m_connection->connect();

            m_pluginLoader = PluginLoader::create(
                  "PluginLoader", Hash("pluginDirectory", config.get<string>("pluginDirectory"), "pluginsToLoad", "*"));

            karabo::util::Hash instanceInfo;
            instanceInfo.set("type", "server");
            instanceInfo.set("serverId", m_serverId);
            instanceInfo.set("version", karabo::util::Version::getVersion());
            instanceInfo.set("host", m_hostname);
            instanceInfo.set("lang", "cpp");
            instanceInfo.set("visibility", m_visibility);
            instanceInfo.set("log", config.get<std::string>("Logger.priority"));

            // Initialize SignalSlotable instance
            init(m_serverId, m_connection, config.get<int>("heartbeatInterval"), instanceInfo);

            registerSlots();
        }


        std::string DeviceServer::generateDefaultServerId() const {
            return m_hostname + "/" + util::toString(getpid());
        }


        DeviceServer::~DeviceServer() {
            stopDeviceServer();
            KARABO_LOG_FRAMEWORK_TRACE << "DeviceServer::~DeviceServer() dtor : m_logger.use_count()="
                                       << m_logger.use_count();
            m_logger.reset();
        }


        void DeviceServer::loadLogger(const Hash& input) {
            Hash config = input.get<Hash>("Logger");

            boost::filesystem::path path(Version::getPathToKaraboInstallation() + "/var/log/" + m_serverId);
            boost::filesystem::create_directories(path);
            path += "/device-server.log";

            config.set("file.filename", path.generic_string());

            Logger::configure(config);

            // By default all categories use all three appenders
            // Note: If logging via broker shall be established, take care that its Logger::useXxx()
            //       is called after broker communication is established.
            Logger::useOstream();
            Logger::useFile();
            Logger::useCache();

            // Initialize category
            m_log = &(karabo::log::Logger::getCategory(m_serverId));

            KARABO_LOG_FRAMEWORK_INFO << "Logfiles are written to: " << path;
        }


        void DeviceServer::loadPluginLoader(const Hash& input) {
            m_pluginLoader = PluginLoader::createNode("PluginLoader", "PluginLoader", input);
        }


        void DeviceServer::finalizeInternalInitialization() {
            // Do before calling start() since not thread safe,
            // boost::bind is safe since handler is only called directly from SignalSlotable code of 'this'.
            registerBroadcastHandler(boost::bind(&DeviceServer::onBroadcastMessage, this, _1, _2));
            // This starts SignalSlotable
            SignalSlotable::start();

            startFsm();

            KARABO_LOG_INFO << "Starting Karabo DeviceServer (pid: " << ::getpid() << ") on host: " << m_hostname
                            << ", serverId: " << m_serverId << ", Broker: " << m_connection->getBrokerUrl();

            m_serverIsRunning = true;

            if (!m_timeServerId.empty()) {
                KARABO_LOG_FRAMEWORK_DEBUG << m_serverId << ": Connecting to time server \"" << m_timeServerId << "\"";
                const std::string timeServerId(m_timeServerId); // copy to avoid catching this
                asyncConnect(m_timeServerId, "signalTimeTick", "", "slotTimeTick", [timeServerId]() {
                    KARABO_LOG_FRAMEWORK_INFO << "Successfully connected to time server '" << timeServerId << "'";
                });
            }
        }


        void DeviceServer::onBroadcastMessage(const karabo::util::Hash::Pointer& header,
                                              const karabo::util::Hash::Pointer& body) {
            // const_cast to have ids refer to a const Node...
            boost::optional<const Hash::Node&> ids = const_cast<const Hash*>(header.get())->find("slotInstanceIds");
            if (!ids || !ids->is<std::string>()) {
                return;
            }
            // Message header is properly formed, so forward to all devices
            const std::string& slotInstanceIds = ids->getValue<std::string>();
            boost::mutex::scoped_lock lock(m_deviceInstanceMutex);
            for (const auto& deviceId_ptr : m_deviceInstanceMap) {
                const std::string& devId = deviceId_ptr.first;
                // Check whether besides to '*', message was also addressed to device directly (theoretically...)
                if (slotInstanceIds.find(("|" + devId) += "|") == std::string::npos) {
                    if (!tryToCallDirectly(devId, header, body)) {
                        // Can happen if devId just tries to come up, but has not yet registered for shortcut messaging.
                        // But this registration happens before the device broadcasts its existence and before that the
                        // device is not really part of the game, so no harm.
                        KARABO_LOG_FRAMEWORK_DEBUG
                              << "Failed to forward broadcast message to local device " << devId
                              << " which likely is just coming up and thus not fully part of the system yet.";
                    }
                }
            }
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
            KARABO_SLOT(slotTimeTick, unsigned long long /*id */, unsigned long long /* sec */,
                        unsigned long long /* frac */, unsigned long long /* period */);
            KARABO_SLOT(slotLoggerContent, Hash);
        }


        void DeviceServer::slotTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                        unsigned long long period) {
            if (period == 0ull) {
                KARABO_LOG_ERROR << "Ignore invalid input in slotTimeTick: period=0, id=" << id << ", sec=" << sec
                                 << ", frac=" << frac;
                return;
            }
            bool firstCall = false;
            {
                karabo::util::Epochstamp epochNow; // before mutex lock since that could add a delay
                boost::mutex::scoped_lock lock(m_timeChangeMutex);
                m_timeId = id;
                m_timeSec = sec;
                m_timeFrac = frac;
                // Fallback to the local timing ...
                if (sec == 0) {
                    m_timeSec = epochNow.getSeconds();
                    m_timeFrac = epochNow.getFractionalSeconds();
                }
                m_timePeriod = period;
                firstCall = m_noTimeTickYet;
                m_noTimeTickYet = false;
            }

            {
                // Just forward to devices this external update
                boost::mutex::scoped_lock lock(m_deviceInstanceMutex);
                for (auto& kv : m_deviceInstanceMap) {
                    // We could post via Strand kv.second.second: That would still guarantee ordering and a long
                    // blocking Device::onTimeTick would not delay the call of slotTimeTick of the following
                    // devices. On the other hand, posting always adds some delay and the risk is low since
                    //  Device::onTimeTick is barely used (if at all).
                    if (kv.second.second) { // otherwise not yet fully initialized
                        kv.second.first->slotTimeTick(id, sec, frac, period);
                    }
                }
            }

            // Now synchronize the machinery that takes care that devices' onTimeUpdate gets called every period.

            // Cancel pending timer if we had an update from the time server...
            if (m_timeTickerTimer.cancel() > 0 || firstCall) { // order matters if timer was already running
                // ...but start again (or the first time), freshly synchronized.
                timeTick(boost::system::error_code(), id);
            }
        }


        void DeviceServer::timeTick(const boost::system::error_code ec, unsigned long long newId) {
            if (ec) return;
            // Get values of last 'external' update via slotTimeTick.
            unsigned long long id = 0, period = 0;
            util::Epochstamp stamp(0ull, 0ull);
            {
                boost::mutex::scoped_lock lock(m_timeChangeMutex);
                id = m_timeId;
                stamp = util::Epochstamp(m_timeSec, m_timeFrac);
                period = m_timePeriod;
            }

            // Internal ticking might have been too slow while external update could not cancel the timer (because
            // timeTick was already posted to the event loop, but did not yet reach the timer reload).
            // So change input as if the cancel was successful:
            if (newId < id) {
                newId = id;
            }

            // Calculate how many ids we are away from last external update and adjust stamp
            const unsigned long long delta = newId - id; // newId >= id is fulfilled
            const util::TimeDuration periodDuration(
                  period / 1000000ull,                       // '/ 10^6': any full seconds part
                  (period % 1000000ull) * 1000000000000ull); // '* 10^12': micro- to attoseconds
            const util::TimeDuration sinceId(periodDuration * delta);
            stamp += sinceId;

            // Call hook that indicates next id. In case the internal ticker was too slow, call it for
            // each otherwise missed id (with same time...). If it was too fast, do not call again.
            //
            // But first some safeguards for first tick at all or if a very big jump happened.
            if (m_timeIdLastTick == 0ull) m_timeIdLastTick = newId - 1; // first time tick
            // It is safe to divide by period: non-zero value taken care of when setting m_timePeriod:
            const unsigned long long largestOnTimeUpdateBacklog = 600000000ull / period; // 6*10^8: 10 min in microsec
            if (newId > m_timeIdLastTick + largestOnTimeUpdateBacklog) {
                // Don't treat an 'id' older than 10 min - for a period of 100 millisec that is 6000 ids in the past
                KARABO_LOG_WARN << "Big gap between trainIds: from " << m_timeIdLastTick << " to " << newId
                                << ". Call hook for time updates only for last " << largestOnTimeUpdateBacklog
                                << " ids.";
                m_timeIdLastTick = newId - largestOnTimeUpdateBacklog;
            }
            while (m_timeIdLastTick < newId) {
                ++m_timeIdLastTick;
                boost::mutex::scoped_lock lock(m_deviceInstanceMutex);
                for (auto& kv : m_deviceInstanceMap) {
                    if (kv.second.second) { // otherwise not yet fully initialized
                        kv.second.second->post(bind_weak(&BaseDevice::onTimeUpdate, kv.second.first.get(),
                                                         m_timeIdLastTick, stamp.getSeconds(),
                                                         stamp.getFractionalSeconds(), period));
                    }
                }
            }

            // reload timer for next id:
            m_timeTickerTimer.expires_at((stamp += periodDuration).getPtime());
            m_timeTickerTimer.async_wait(
                  util::bind_weak(&DeviceServer::timeTick, this, boost::asio::placeholders::error, ++newId));
        }


        krb_log4cpp::Category& DeviceServer::log() {
            return (*m_log);
        }


        void DeviceServer::slotLoggerContent(const karabo::util::Hash& input) {
            unsigned int numberOfLogs = 10u;
            if (input.has("logs")) {
                auto& element = input.getNode("logs");
                // extract the value of the number of lines in a type permissive way
                numberOfLogs = element.getValue<decltype(numberOfLogs), long long, unsigned long long, int, short,
                                                unsigned short>();
            }
            Hash reply_("serverId", getInstanceId());
            reply_.set("content", Logger::getCachedContent(numberOfLogs));
            reply(reply_);
        }


        void DeviceServer::onStateUpdate(const State& currentState) {}


        void DeviceServer::okStateOnEntry() {
            KARABO_LOG_INFO << "DeviceServer starts up with id: " << m_serverId;

            // Check whether we have installed devices available
            updateAvailableDevices();
            if (!m_availableDevices.empty()) {
                newPluginAvailable();
            }


            for (const Hash& device : m_autoStart) {
                slotStartDevice(device);
            }

            // Whether to scan for additional plug-ins at runtime
            if (m_scanPlugins) {
                KARABO_LOG_INFO << "Keep watching directory: " << m_pluginLoader->getPluginDirectory()
                                << " for Device plugins";
                EventLoop::getIOService().post(
                      util::bind_weak(&DeviceServer::scanPlugins, this, boost::system::error_code()));
            }
        }


        void DeviceServer::updateAvailableDevices() {
            const vector<string>& devices = Configurator<BaseDevice>::getRegisteredClasses();
            KARABO_LOG_INFO << "Updated list of devices available: " << karabo::util::toString(devices);


            for (const string& device : devices) {
                if (!m_availableDevices.has(device)) {
                    Schema schema;
                    KARABO_LOG_FRAMEWORK_DEBUG << "Plugin contains device class \"" << device
                                               << "\".  Try to get schema ...";
                    try {
                        schema = BaseDevice::getSchema(
                              device,
                              Schema::AssemblyRules(karabo::util::READ | karabo::util::WRITE | karabo::util::INIT));
                    } catch (const std::exception& e) {
                        KARABO_LOG_ERROR << "Device \"" << device
                                         << "\" is ignored because of Schema building failure : " << e.what();
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
            m_scanPluginsTimer.async_wait(
                  bind_weak(&DeviceServer::scanPlugins, this, boost::asio::placeholders::error));
        }


        void DeviceServer::stopDeviceServer() {
            // First stop background work
            m_timeTickerTimer.cancel();
            m_scanPluginsTimer.cancel();

            // Then stop devices
            {
                boost::mutex::scoped_lock lock(m_deviceInstanceMutex);

                // Notify all devices
                KARABO_LOG_FRAMEWORK_DEBUG << "stopServer() device map size: " << m_deviceInstanceMap.size();
                for (auto it = m_deviceInstanceMap.begin(); it != m_deviceInstanceMap.end(); ++it) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "stopServer() call slotKillDevice for " << it->first;
                    call(it->first, "slotKillDevice");
                }

                m_deviceInstanceMap.clear();
                KARABO_LOG_FRAMEWORK_DEBUG << "stopServer() device maps cleared";
            }

            // TODO Remove from here and use the one from run() method
            m_serverIsRunning = false;
        }


        void DeviceServer::errorFoundAction(const std::string& user, const std::string& detail) {
            KARABO_LOG_ERROR << "[short] " << user;
            KARABO_LOG_ERROR << "[detailed] " << detail;
        }


        void DeviceServer::slotStartDevice(const karabo::util::Hash& configuration) {
            // Just register an asynchronous reply and put on the "stack".
            const SignalSlotable::AsyncReply reply(this);

            EventLoop::getIOService().post(util::bind_weak(&DeviceServer::startDevice, this, configuration, reply));
        }


        void DeviceServer::startDevice(const karabo::util::Hash& configuration,
                                       const SignalSlotable::AsyncReply& reply) {
            const boost::tuple<std::string, std::string, util::Hash>& idClassIdConfig =
                  this->prepareInstantiate(configuration);

            const std::string& deviceId = idClassIdConfig.get<0>();
            const std::string& classId = idClassIdConfig.get<1>();
            KARABO_LOG_FRAMEWORK_INFO << "Trying to start a '" << classId << "' with deviceId '" << deviceId << "'...";
            KARABO_LOG_FRAMEWORK_DEBUG << "...with the following configuration:\n" << configuration;
            instantiate(deviceId, classId, idClassIdConfig.get<2>(), reply);
        }


        boost::tuple<std::string, std::string, util::Hash> DeviceServer::prepareInstantiate(
              const karabo::util::Hash& configuration) {
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

                // Inject Hostname
                config.set("hostName", m_hostname);

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

                // Inject Hostname
                tmp.set("hostName", m_hostname);

                const std::pair<std::string, util::Hash>& idCfg =
                      util::confTools::splitIntoClassIdAndConfiguration(modifiedConfig);
                return boost::make_tuple(tmp.get<std::string>("_deviceId_"), idCfg.first, idCfg.second);
            }
        }


        void DeviceServer::instantiate(const std::string& deviceId, const std::string& classId,
                                       const util::Hash& config, const xms::SignalSlotable::AsyncReply& asyncReply) {
            // Each device adds one thread already. But since
            // device->finalizeInternalInitialization() blocks for > 1 s, we temporarily add another thread.
            EventLoop::addThread();
            bool putInMap = false;
            std::string errorMsg;
            std::string errorDetails;
            try {
                BaseDevice::Pointer device = BaseDevice::create(classId, config);

                {
                    boost::mutex::scoped_lock lock(m_deviceInstanceMutex);
                    if (m_deviceInstanceMap.find(deviceId) != m_deviceInstanceMap.end()) {
                        throw KARABO_LOGIC_EXCEPTION("Device '" + deviceId +
                                                     "' already running/starting on this server.");
                    }
                    // Keep the device instance - doing this before finalizeInternalInitialization to enable the device
                    // to kill itself during instantiation (see slotDeviceGone).
                    m_deviceInstanceMap[deviceId] = std::make_pair(device, Strand::Pointer());
                    putInMap = true;
                }

                // This will throw an exception if it can't be started (because of duplicated name for example)
                device->finalizeInternalInitialization(
                      getConnection()->clone(deviceId), // use clone to potentially share
                      false,                            // DeviceServer will forward broadcasts!
                      m_timeServerId);

                {
                    boost::mutex::scoped_lock lock(m_deviceInstanceMutex);
                    // After finalizeInternalInitialization, the device participates in time information distribution
                    m_deviceInstanceMap[deviceId].second = boost::make_shared<Strand>(EventLoop::getIOService());
                }

            } catch (const karabo::util::Exception& e) {
                errorMsg = e.userFriendlyMsg(false);
                if (errorMsg.empty()) errorMsg = "Unknown failure"; // Should not happen, but better protect
                errorDetails = e.detailedMsg();
            } catch (const std::exception& e) {
                errorMsg = e.what();
            }
            if (errorMsg.empty()) {
                // Answer initiation of device (KARABO_LOG_* is done by device)
                asyncReply(true, deviceId);
            } else {
                // Instantiation failed
                if (putInMap) { // Otherwise the device is not from this request.
                    // To be precise, the following unlikely case is not excluded: The device was put in map above, but
                    // killed itself during its initialization phase and slotStartDevice has been called once more for
                    // the same deviceId and placed it into the map again before we get here to remove the one that
                    // killed itself.
                    boost::mutex::scoped_lock lock(m_deviceInstanceMutex);
                    m_deviceInstanceMap.erase(deviceId);
                }
                const std::string message("Device '" + deviceId + "' of class '" + classId +
                                          "' could not be started: " + errorMsg);
                KARABO_LOG_ERROR << message
                                 << (errorDetails.empty() ? std::string() : "\nFailure details:\n" + errorDetails);
                asyncReply.error(message, errorDetails);
            }
            EventLoop::removeThread();
        }

        void DeviceServer::newPluginAvailable() {
            vector<string> deviceClasses;
            vector<int> visibilities;
            deviceClasses.reserve(m_availableDevices.size());

            for (Hash::iterator it = m_availableDevices.begin(); it != m_availableDevices.end(); ++it) {
                const std::string& deviceClass = it->getKey();
                if (std::find(m_deviceClasses.begin(), m_deviceClasses.end(), deviceClass) != m_deviceClasses.end()) {
                    deviceClasses.push_back(it->getKey());

                    Hash& tmp = it->getValue<Hash>();
                    if (tmp.get<bool>("mustNotify") == true) {
                        tmp.set("mustNotify", false);
                    }
                    visibilities.push_back(tmp.get<Schema>("xsd").getDefaultValue<int>("visibility"));
                }
            }
            KARABO_LOG_FRAMEWORK_INFO << "Sending instance update as new device plugins are available: "
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
            KARABO_LOG_WARN << "Current state of server \"" << getInstanceId()
                            << "\" does not allow a transition for event \"" << eventName << "\"";
        }


        void DeviceServer::slotKillServer() {
            KARABO_LOG_INFO << "Received kill signal";

            reply(m_serverId);

            // Terminate the process which will call our destructor through the signal handling
            // implemented in main() in deviceServer.cc.
            std::raise(SIGTERM);
            KARABO_LOG_FRAMEWORK_DEBUG << "slotKillServer DONE";
        }


        void DeviceServer::slotDeviceGone(const std::string& instanceId) {
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


        std::string DeviceServer::generateDefaultDeviceId(const std::string& classId) {
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
            this->updateInstanceInfo(Hash("log", newprio));
        }
    } // namespace core
} // namespace karabo
