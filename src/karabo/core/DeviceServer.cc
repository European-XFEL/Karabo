/*
 * $Id$
 *
 * Author: burkhard.heisen@xfel.eu>
 *
 * Created on February 9, 2011, 2:24 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "DeviceServer.hh"

#include <unistd.h>

// temp change to trigger CI
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <regex>
#include <tuple>

#include "Device.hh"
#include "karabo/data/schema/ChoiceElement.hh"
#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/log/Logger.hh"
#include "karabo/log/utils.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/Strand.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/JsonToHashParser.hh"
#include "karabo/util/Version.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::DeviceServer)

namespace karabo {

    namespace core {

        enum class ServerFlags {

            DEVELOPMENT = (1u << 0),
            // add future flags as bitmask:
            // SOME_OTHER_SERVER_FLAG = (1u << 1),
        };

        // template class Runner<DeviceServer>;

        namespace nl = nlohmann;
        using namespace std;
        using namespace karabo::data;
        using namespace karabo::util;
        using namespace karabo::log;
        using namespace karabo::net;
        using namespace karabo::xms;
        using namespace std::placeholders;

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

            STRING_ELEMENT(expected)
                  .key("init")
                  .displayedName("Auto start")
                  .description("Auto starts selected devices")
                  .assignmentOptional()
                  .defaultValue("")
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("scanPlugins")
                  .displayedName("Unused")
                  .description("Unused since Karabo 2.19.0, plugins are only scanned once when starting.")
                  .expertAccess()
                  .assignmentOptional()
                  .defaultValue(true)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("pluginDirectory")
                  .displayedName("Plugin Directory")
                  .description("Directory to search for plugins")
                  .assignmentOptional()
                  .defaultValue(PluginLoader::defaultPluginPath())
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

            VECTOR_STRING_ELEMENT(expected)
                  .key("serverFlags")
                  .displayedName("Server Flags")
                  .description(
                        "ServerFlags describing the device server, "
                        "the values must correspond to the enum ServerFlags")
                  .assignmentOptional()
                  .defaultValue({})
                  .init()
                  .commit();
        }

        DeviceServer::DeviceServer(const karabo::data::Hash& config)
            : m_timeId(0ull),
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
            // Runner establishes 'autoStart' property as (json) string (default: "")
            if (config.has("autoStart")) {
                throw KARABO_PARAMETER_EXCEPTION("'autoStart' syntax not supported anymore, use 'init'");
            }
            {
                std::string json = config.get<std::string>("init");
                if (!json.empty()) {
                    Hash cfg = generateAutoStartHash(jsonToHash(json));
                    // cfg contains 'autoStart' property as VECTOR_HASH type
                    cfg.get("autoStart", m_autoStart);
                }
            }

            // What visibility this server should have
            m_visibility = Schema::AccessLevel::OBSERVER;

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

            // For a choice element, there is exactly one sub-Hash where the key is the chosen (here: Broker)
            // sub-class. We have to transfer the instance id and thus copy the relevant part of the (const) config.
            // (Otherwise we could just pass 'input' to createChoice(..).)
            Hash brokerConfig("connection", config.get<Hash>("connection"));
            Hash& connectionCfg = brokerConfig.get<Hash>("connection").begin()->getValue<Hash>();
            connectionCfg.set("instanceId", m_serverId);
            m_connection = Configurator<Broker>::createChoice("connection", brokerConfig);
            m_connection->connect();

            karabo::data::Hash instanceInfo;
            instanceInfo.set("type", "server");
            instanceInfo.set("serverId", m_serverId);
            instanceInfo.set("version", karabo::util::Version::getVersion());
            instanceInfo.set("host", m_hostname);
            // getlogin() is a linux function only
            auto user = getlogin();
            instanceInfo.set("user", (user ? user : "none"));
            instanceInfo.set("lang", "cpp");
            instanceInfo.set("visibility", m_visibility);
            instanceInfo.set("log", config.get<std::string>("Logger.priority"));

            int flags = 0;
            const std::vector<std::string>& serverFlags = config.get<std::vector<std::string>>("serverFlags");
            for (const std::string& flag : serverFlags)
                if (flag == "Development") {
                    flags |= static_cast<int>(ServerFlags::DEVELOPMENT);
                } else {
                    throw KARABO_LOGIC_EXCEPTION("Provided serverFlag is not supported: " + flag);
                }
            instanceInfo.set("serverFlags", flags);
            instanceInfo.merge(availablePlugins());

            // Initialize SignalSlotable instance
            init(m_serverId, m_connection, config.get<int>("heartbeatInterval"), instanceInfo);

            registerSlots();
        }


        std::string DeviceServer::generateDefaultServerId() const {
            return m_hostname + "/" + data::toString(getpid());
        }


        DeviceServer::~DeviceServer() {
            stopDeviceServer();
            Logger::reset();
        }


        void DeviceServer::loadLogger(const Hash& input) {
            Hash config = input.get<Hash>("Logger");

            std::filesystem::path path(Version::getPathToKaraboInstallation() + "/var/log/" + m_serverId);
            std::filesystem::create_directories(path);
            path += "/device-server.log";

            config.set("file.filename", path.generic_string());

            Logger::configure(config);

            // By default all categories use all three appenders
            // Note: If logging via broker shall be established, take care that its Logger::useXxx()
            //       is called after broker communication is established.
            Logger::useConsole();
            Logger::useFile();
            Logger::useCache();

            KARABO_LOG_INFO << "Logfiles are written to: " << path;
        }


        void DeviceServer::finalizeInternalInitialization() {
            // Do before calling start() since not thread safe,
            // std::bind is safe since handler is only called directly from SignalSlotable code of 'this'.
            registerBroadcastHandler(std::bind(&DeviceServer::onBroadcastMessage, this, _1, _2));
            // This starts SignalSlotable
            SignalSlotable::start();

            autostartDevices();

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


        void DeviceServer::onBroadcastMessage(const karabo::data::Hash::Pointer& header,
                                              const karabo::data::Hash::Pointer& body) {
            // const_cast to have ids refer to a const Node...
            boost::optional<const Hash::Node&> ids = const_cast<const Hash*>(header.get())->find("slotInstanceIds");
            if (!ids || !ids->is<std::string>()) {
                return;
            }
            // Message header is properly formed, so forward to all devices
            const std::string& slotInstanceIds = ids->getValue<std::string>();
            std::lock_guard<std::mutex> lock(m_deviceInstanceMutex);
            for (const auto& deviceId_ptr : m_deviceInstanceMap) {
                const std::string& devId = deviceId_ptr.first;
                // Check whether besides to '*', message was also addressed to device directly (theoretically...)
                if (slotInstanceIds.find(("|" + devId) += "|") == std::string::npos) {
                    if (!tryToCallDirectly(devId, header, body)) {
                        // Can happen if devId just tries to come up, but has not yet registered for shortcut
                        // messaging. But this registration happens before the device broadcasts its existence and
                        // before that the device is not really part of the game, so no harm.
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
                karabo::data::Epochstamp epochNow; // before mutex lock since that could add a delay
                std::lock_guard<std::mutex> lock(m_timeChangeMutex);
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
                std::lock_guard<std::mutex> lock(m_deviceInstanceMutex);
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
            data::Epochstamp stamp(0ull, 0ull);
            {
                std::lock_guard<std::mutex> lock(m_timeChangeMutex);
                id = m_timeId;
                stamp = data::Epochstamp(m_timeSec, m_timeFrac);
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
            const data::TimeDuration periodDuration(
                  period / 1000000ull,                       // '/ 10^6': any full seconds part
                  (period % 1000000ull) * 1000000000000ull); // '* 10^12': micro- to attoseconds
            const data::TimeDuration sinceId(periodDuration * delta);
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
                std::lock_guard<std::mutex> lock(m_deviceInstanceMutex);
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


        void DeviceServer::slotLoggerContent(const karabo::data::Hash& input) {
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


        void DeviceServer::autostartDevices() {
            for (const Hash& device : m_autoStart) {
                slotStartDevice(device);
            }

            KARABO_LOG_INFO << "DeviceServer starts up with id: " << m_serverId;
        }

        void DeviceServer::stopDeviceServer() {
            // First stop background work
            m_timeTickerTimer.cancel();

            // Then stop devices
            {
                std::lock_guard<std::mutex> lock(m_deviceInstanceMutex);

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


        void DeviceServer::slotStartDevice(const karabo::data::Hash& configuration) {
            // Just register an asynchronous reply and put on the "stack".
            const SignalSlotable::AsyncReply reply(this);

            EventLoop::getIOService().post(util::bind_weak(&DeviceServer::startDevice, this, configuration, reply));
        }


        void DeviceServer::startDevice(const karabo::data::Hash& configuration,
                                       const SignalSlotable::AsyncReply& reply) {
            const std::tuple<std::string, std::string, data::Hash>& idClassIdConfig =
                  this->prepareInstantiate(configuration);

            const std::string& deviceId = std::get<0>(idClassIdConfig);
            const std::string& classId = std::get<1>(idClassIdConfig);
            KARABO_LOG_FRAMEWORK_INFO << "Trying to start a '" << classId << "' with deviceId '" << deviceId << "'...";
            KARABO_LOG_FRAMEWORK_DEBUG << "...with the following configuration:\n" << configuration;
            instantiate(deviceId, classId, std::get<2>(idClassIdConfig), reply);
        }


        std::tuple<std::string, std::string, data::Hash> DeviceServer::prepareInstantiate(
              const karabo::data::Hash& configuration) {
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

                return std::make_tuple(config.get<std::string>("_deviceId_"), classId, config);
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

                const std::pair<std::string, data::Hash>& idCfg =
                      data::confTools::splitIntoClassIdAndConfiguration(modifiedConfig);
                return std::make_tuple(tmp.get<std::string>("_deviceId_"), idCfg.first, idCfg.second);
            }
        }


        void DeviceServer::instantiate(const std::string& deviceId, const std::string& classId,
                                       const data::Hash& config, const xms::SignalSlotable::AsyncReply& asyncReply) {
            // Each device adds one thread already. But since
            // device->finalizeInternalInitialization() blocks for > 1 s, we temporarily add another thread.
            EventLoop::addThread();
            bool putInMap = false;
            std::string errorMsg;
            std::string errorDetails;
            try {
                BaseDevice::Pointer device = BaseDevice::create(classId, config);

                {
                    std::lock_guard<std::mutex> lock(m_deviceInstanceMutex);
                    if (m_deviceInstanceMap.find(deviceId) != m_deviceInstanceMap.end()) {
                        // Note: If you alter this string, adjust also
                        // DataLoggerManager::loggerInstantiationHandler(..)
                        throw KARABO_LOGIC_EXCEPTION("Device '" + deviceId +
                                                     "' already running/starting on this server.");
                    }
                    // Keep the device instance - doing this before finalizeInternalInitialization to enable the
                    // device to kill itself during instantiation (see slotDeviceGone).
                    m_deviceInstanceMap[deviceId] = std::make_pair(device, Strand::Pointer());
                    putInMap = true;
                }

                // This will throw an exception if it can't be started (because of duplicated name for example)
                device->finalizeInternalInitialization(
                      getConnection()->clone(deviceId), // use clone to potentially share
                      false,                            // DeviceServer will forward broadcasts!
                      m_timeServerId);

                {
                    std::lock_guard<std::mutex> lock(m_deviceInstanceMutex);
                    // After finalizeInternalInitialization, the device participates in time information
                    // distribution
                    m_deviceInstanceMap[deviceId].second = std::make_shared<Strand>(EventLoop::getIOService());
                }

            } catch (const karabo::data::Exception& e) {
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
                    // To be precise, the following unlikely case is not excluded: The device was put in map above,
                    // but killed itself during its initialization phase and slotStartDevice has been called once
                    // more for the same deviceId and placed it into the map again before we get here to remove the
                    // one that killed itself.
                    std::lock_guard<std::mutex> lock(m_deviceInstanceMutex);
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

        Hash DeviceServer::availablePlugins() {
            vector<string> deviceClasses;
            vector<int> visibilities;

            const std::vector<std::string>& base_devices = Configurator<BaseDevice>::getRegisteredClasses();
            for (const std::string& baseDevice : base_devices) {
                if (std::find(m_deviceClasses.begin(), m_deviceClasses.end(), baseDevice) != m_deviceClasses.end()) {
                    deviceClasses.push_back(baseDevice);
                    try {
                        auto schema = BaseDevice::getSchema(
                              baseDevice,
                              Schema::AssemblyRules(karabo::data::READ | karabo::data::WRITE | karabo::data::INIT));

                        // Hash conf{"mustNotify", false, "xsd", schema};
                        // "visibility" is not a parameter and hard-coded default is ...
                        visibilities.push_back(karabo::data::Schema::OBSERVER);

                    } catch (const std::exception& e) {
                        KARABO_LOG_ERROR << "Device \"" << baseDevice
                                         << "\" is ignored because of Schema building failure : " << e.what();
                        // Remove the last added element, since adding its visibility failed
                        deviceClasses.pop_back();
                    }
                }
            }
            return Hash{"deviceClasses", deviceClasses, "visibilities", visibilities};
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

            std::lock_guard<std::mutex> lock(m_deviceInstanceMutex);
            if (m_deviceInstanceMap.erase(instanceId) > 0) {
                KARABO_LOG_INFO << "Device '" << instanceId << "' removed from server.";
            }
        }


        void DeviceServer::slotGetClassSchema(const std::string& classId) {
            Schema schema = BaseDevice::getSchema(classId);
            reply(schema, classId, this->getInstanceId());
        }


        std::string DeviceServer::generateDefaultDeviceId(const std::string& classId) {
            const string index = karabo::data::toString(++m_deviceInstanceCount[classId]);
            // Prepare shortened Device-Server name
            vector<string> tokens;
            string domain = m_serverId;
            boost::split(tokens, m_serverId, boost::is_any_of("_"));
            if (tokens.back() == karabo::data::toString(getpid())) {
                domain = tokens.front() + "-" + tokens.back();
            }
            return domain + "_" + classId + "_" + index;
        }


        void DeviceServer::slotLoggerPriority(const std::string& newprio) {
            std::string oldprio = Logger::getPriority();
            Logger::setPriority(newprio);
            KARABO_LOG_INFO << "Logger Priority changed : " << oldprio << " ==> " << newprio;
            this->updateInstanceInfo(Hash("log", newprio));
        }
    } // namespace core
} // namespace karabo
