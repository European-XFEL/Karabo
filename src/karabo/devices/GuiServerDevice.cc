/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "GuiServerDevice.hh"

#include "karabo/core/InstanceChangeThrottler.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/TcpChannel.hh"
#include "karabo/net/UserAuthClient.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/State.hh"
#include "karabo/util/VectorElement.hh"
#include "karabo/util/Version.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::io;
using namespace karabo::xms;
using namespace boost::placeholders;


KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, karabo::devices::GuiServerDevice)

namespace karabo {
    namespace devices {

        const std::unordered_set<std::string> GuiServerDevice::m_writeCommands(
              {"projectSaveItems", "initDevice", "killDevice", "execute", "killServer", "acknowledgeAlarm",
               "projectUpdateAttribute", "reconfigure", "updateAttributes"});


        // configure here restrictions to the command type against client versions
        const std::unordered_map<std::string, Version> GuiServerDevice::m_minVersionRestrictions{
              {"projectSaveItems", Version("2.10.0")},
              {"projectUpdateAttribute", Version("2.10.0")},
        };

        const std::string GuiServerDevice::m_errorDetailsDelim("\nDetails:\n");

        void GuiServerDevice::expectedParameters(Schema& expected) {
            OVERWRITE_ELEMENT(expected)
                  .key("state")
                  .setNewOptions(State::INIT, State::ON, State::ERROR)
                  .setNewDefaultValue(State::INIT)
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("port")
                  .displayedName("Hostport")
                  .description("Local port for this server")
                  .assignmentOptional()
                  .defaultValue(44444)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("authServer")
                  .displayedName("Auth Server")
                  .description("URL for the Authentication Server")
                  .assignmentOptional()
                  .defaultValue("")
                  .init()
                  .commit();

            OVERWRITE_ELEMENT(expected).key("deviceId").setNewDefaultValue("Karabo_GuiServer_0").commit();

            OVERWRITE_ELEMENT(expected).key("visibility").setNewDefaultValue<int>(Schema::AccessLevel::ADMIN).commit();

            // Monitor performance of this system relevant device
            OVERWRITE_ELEMENT(expected).key("performanceStatistics.enable").setNewDefaultValue(true).commit();

            INT32_ELEMENT(expected)
                  .key("delayOnInput")
                  .displayedName("Delay on Input channel")
                  .description(
                        "Extra Delay on the InputChannel in this device to inform the output channel "
                        "about its readiness to receive new data. Lowering this delay adds load to the output channel "
                        "the GUI server connects to.")
                  .assignmentOptional()
                  .defaultValue(500)
                  .reconfigurable()
                  .minInc(0)
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .commit();

            INT32_ELEMENT(expected)
                  .key("lossyDataQueueCapacity")
                  .displayedName("Lossy Data forwarding queue size")
                  .description(
                        "The number of lossy data messages to store in the forwarding ring buffer. NOTE: Will be "
                        "applied to newly connected clients only")
                  .assignmentOptional()
                  .defaultValue(100)
                  .reconfigurable()
                  .minExc(0)
                  .maxInc(1000)
                  .commit();

            INT32_ELEMENT(expected)
                  .key("propertyUpdateInterval")
                  .displayedName("Property update interval")
                  .description("Minimum interval between subsequent property updates forwarded to clients.")
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(500)
                  .reconfigurable()
                  .minInc(0)
                  .maxInc(10000) // 0.1 Hz minimum
                  .commit();

            INT32_ELEMENT(expected)
                  .key("waitInitDevice")
                  .displayedName("Instantiate wait time")
                  .description("Time interval between the instantiation of devices.")
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .assignmentOptional()
                  .defaultValue(100)
                  .reconfigurable()
                  .minInc(100)
                  .maxInc(5000) // NOTE: Not _too_ fast. The device instantiation timer is always running!
                  .commit();

            INT32_ELEMENT(expected)
                  .key("checkConnectionsInterval")
                  .displayedName("Check Connections Interval")
                  .description(
                        "Time interval between checking client connections. Clients with an increasing backlog "
                        "of more than 1000 pending messages will be disconnected after two consecutive checks.")
                  .unit(Unit::SECOND)
                  .assignmentOptional()
                  .defaultValue(300)
                  .reconfigurable()
                  .minInc(1)
                  .maxInc(24 * 3600) // at least once per day
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("connectedClientCount")
                  .displayedName("Connected clients count")
                  .description("The number of clients currently connected to the server.")
                  .readOnly()
                  .initialValue(0)
                  .commit();

            NODE_ELEMENT(expected)
                  .key("networkPerformance")
                  .displayedName("Network performance monitoring")
                  .description("Contains information about how much data is being read/written from/to the network")
                  .commit();

            INT32_ELEMENT(expected)
                  .key("networkPerformance.sampleInterval")
                  .displayedName("Sample interval")
                  .description("Minimum interval between subsequent network performance recordings.")
                  .unit(Unit::SECOND)
                  .assignmentOptional()
                  .defaultValue(5)
                  .reconfigurable()
                  .minInc(1)
                  .maxInc(3600) // Once per second to once per hour
                  .commit();

            UINT64_ELEMENT(expected)
                  .key("networkPerformance.clientBytesRead")
                  .displayedName("Bytes read from clients")
                  .description("The number of bytes read from the network in the last `sampleInterval` seconds")
                  .readOnly()
                  .initialValue(0)
                  .commit();

            UINT64_ELEMENT(expected)
                  .key("networkPerformance.clientBytesWritten")
                  .displayedName("Bytes written to clients")
                  .description("The number of bytes written to the network in the last `sampleInterval` seconds")
                  .readOnly()
                  .initialValue(0)
                  .commit();

            UINT64_ELEMENT(expected)
                  .key("networkPerformance.pipelineBytesRead")
                  .displayedName("Bytes read from pipeline connections")
                  .description("The number of bytes read from the network in the last `sampleInterval` seconds")
                  .readOnly()
                  .initialValue(0)
                  .commit();

            UINT64_ELEMENT(expected)
                  .key("networkPerformance.pipelineBytesWritten")
                  .displayedName("Bytes written to pipeline connections")
                  .description("The number of bytes written to the network in the last `sampleInterval` seconds")
                  .readOnly()
                  .initialValue(0)
                  .commit();

            // Server <-> Client protocol changes that impose minimal client version requirements:
            //
            // Minimal client version 2.5.0 -> instanceNew|Update|Gone protocol changed; those three events began to be
            //                                 sent to the clients in a single instancesChanged event.
            //
            // Minimal client version 2.7.0 -> 'deviceConfiguration' message type replaced by 'deviceConfigurations'.
            //                                 While 'deviceConfiguration' (singular) carried the properties that have
            //                                 changed for a single device in a given interval, 'deviceConfigurations'
            //                                 (plural) carries the properties that have changed for all the devices of
            //                                 interest for a specific client in a given interval.
            STRING_ELEMENT(expected)
                  .key("minClientVersion")
                  .displayedName("Minimum Client Version")
                  .description(
                        "If this variable does not respect the N.N.N(.N) convention,"
                        " the Server will not enforce a version check")
                  .assignmentOptional()
                  .defaultValue("2.11.3")
                  .reconfigurable()
                  .adminAccess()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("isReadOnly")
                  .displayedName("isReadOnly")
                  .description(
                        "Define if this GUI Server is in readOnly "
                        "mode for clients")
                  .assignmentOptional()
                  .defaultValue(false)
                  .init()
                  .adminAccess()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("dataLogManagerId")
                  .displayedName("Data Log Manager Id")
                  .description("The DataLoggerManager device to query for log readers.")
                  .assignmentOptional()
                  .defaultValue(karabo::util::DATALOGMANAGER_ID)
                  .reconfigurable()
                  .adminAccess()
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("ignoreTimeoutClasses")
                  .displayedName("Ignore Timeout ClassIds")
                  .description(
                        "ClassIds that are treated like macros: The GUI server will ignore "
                        "timeouts of slots of devices of these classes.")
                  .assignmentOptional()
                  .defaultValue(std::vector<std::string>())
                  .reconfigurable()
                  .adminAccess()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("timeout")
                  .displayedName("Request Timeout")
                  .description(
                        "If client requests to 'reconfigure', 'execute' or 'requestGeneric' have a 'timeout' "
                        "specified, take in fact the maximum of that value and this one.")
                  .assignmentOptional()
                  .defaultValue(10) // in 2.10.0, client has 5
                  .reconfigurable()
                  .adminAccess()
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("bannerData")
                  .displayedName("Banner Data")
                  .description(
                        "Banner message for connecting clients, provided by slotNotify. "
                        "Three elements are expected: Message, background color, foreground color.")
                  .readOnly()
                  .initialValue({})
                  .expertAccess()
                  .commit();

            SLOT_ELEMENT(expected)
                  .key("slotDumpToLog")
                  .displayedName("Dump Debug to Log")
                  .description("Dumps info about connections to log file (care - can be huge)")
                  .expertAccess()
                  .commit();
        }


        GuiServerDevice::GuiServerDevice(const Hash& config)
            : Device<>(config),
              m_deviceInitTimer(EventLoop::getIOService()),
              m_networkStatsTimer(EventLoop::getIOService()),
              m_checkConnectionTimer(EventLoop::getIOService()),
              m_timeout(config.get<int>("timeout")),
              m_authClient(config.get<std::string>("authServer")) {
            KARABO_INITIAL_FUNCTION(initialize)

            KARABO_SLOT(slotLoggerMap, Hash /*loggerMap*/)
            KARABO_SLOT(slotAlarmSignalsUpdate, std::string, std::string, karabo::util::Hash);
            KARABO_SLOT(slotProjectUpdate, karabo::util::Hash, std::string);
            KARABO_SLOT(slotDumpToLog);
            KARABO_SLOT(slotDumpDebugInfo, karabo::util::Hash);
            KARABO_SLOT(slotDisconnectClient, std::string);
            KARABO_SLOT(slotNotify, karabo::util::Hash);
            KARABO_SLOT(slotBroadcast, karabo::util::Hash);

            Hash h;
            h.set("port", config.get<unsigned int>("port"));
            h.set("type", "server");
            h.set("serializationType", "binary"); // Will lead to binary header hashes
            m_dataConnection = Connection::create("Tcp", h);
            m_serializer = BinarySerializer<Hash>::create("Bin"); // for reading

            m_isReadOnly = config.get<bool>("isReadOnly");
        }


        GuiServerDevice::~GuiServerDevice() {
            if (m_dataConnection) m_dataConnection->stop();
        }


        void GuiServerDevice::initialize() {
            try {
                // Protect clients from too frequent updates of a single property:
                remote().setDeviceMonitorInterval(get<int>("propertyUpdateInterval"));

                // Register handlers
                // NOTE: boost::bind() is OK for these handlers because SignalSlotable calls them directly instead
                // of dispatching them via the event loop.
                remote().registerInstanceNewMonitor(
                      boost::bind(&karabo::devices::GuiServerDevice::instanceNewHandler, this, _1));
                remote().registerInstanceGoneMonitor(
                      boost::bind(&karabo::devices::GuiServerDevice::instanceGoneHandler, this, _1, _2));
                remote().registerSchemaUpdatedMonitor(
                      boost::bind(&karabo::devices::GuiServerDevice::schemaUpdatedHandler, this, _1, _2));
                remote().registerClassSchemaMonitor(
                      boost::bind(&karabo::devices::GuiServerDevice::classSchemaHandler, this, _1, _2, _3));

                remote().registerInstanceChangeMonitor(
                      bind_weak(&karabo::devices::GuiServerDevice::instanceChangeHandler, this, _1));

                remote().registerDevicesMonitor(
                      bind_weak(&karabo::devices::GuiServerDevice::devicesChangedHandler, this, _1));

                // If someone manages to bind_weak(&karabo::devices::GuiServerDevice::requestNoWait<>, this, ...),
                // we would not need loggerMapConnectedHandler...
                asyncConnect(get<std::string>("dataLogManagerId"), "signalLoggerMap", "", "slotLoggerMap",
                             bind_weak(&karabo::devices::GuiServerDevice::loggerMapConnectedHandler, this));

                // Switch on instance tracking - which is blocking a while.
                // Note that instanceNew(..) will be called for all instances already in the game.
                remote().enableInstanceTracking();

                m_dataConnection->startAsync(bind_weak(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));

                m_guiDebugProducer = getConnection();

                startDeviceInstantiation();
                startNetworkMonitor();
                startMonitorConnectionQueues(karabo::util::Hash());

                // TODO: remove this once "fast slot reply policy" is enforced
                const std::vector<std::string>& timingOutClasses =
                      get<std::vector<std::string>>("ignoreTimeoutClasses");
                recalculateTimingOutDevices(remote().getSystemTopology(), timingOutClasses, false);

                updateState(State::ON);

                // Produce some information
                KARABO_LOG_INFO << "GUI Server is up and listening on port: " << get<unsigned int>("port");
                if (!get<std::string>("authServer").empty()) {
                    KARABO_LOG_INFO << "Using the Karabo Authentication Server at '" << get<std::string>("authServer")
                                    << "'";
                }

            } catch (const std::exception& e) {
                updateState(State::ERROR);

                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initialize(): " << e.what();
            }
        }


        void GuiServerDevice::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
            if (incomingReconfiguration.has("ignoreTimeoutClasses")) {
                const std::vector<std::string>& timingOutClasses =
                      incomingReconfiguration.get<std::vector<std::string>>("ignoreTimeoutClasses");
                recalculateTimingOutDevices(remote().getSystemTopology(), timingOutClasses, true);
            }
            if (incomingReconfiguration.has("timeout")) {
                m_timeout = incomingReconfiguration.get<int>("timeout");
            }
        }


        bool GuiServerDevice::skipExecutionTimeout(const std::string& deviceId) {
            boost::mutex::scoped_lock lock(m_timingOutDevicesMutex);
            return m_timingOutDevices.find(deviceId) != m_timingOutDevices.end();
        }


        void GuiServerDevice::recalculateTimingOutDevices(const karabo::util::Hash& topologyEntry,
                                                          const std::vector<std::string>& timingOutClasses,
                                                          bool clearSet) {
            boost::mutex::scoped_lock lock(m_timingOutDevicesMutex);
            if (clearSet) m_timingOutDevices.clear();
            if (topologyEntry.has("device")) {
                const karabo::util::Hash& devices = topologyEntry.get<Hash>("device");
                for (karabo::util::Hash::const_iterator it = devices.begin(); it != devices.end(); ++it) {
                    if (std::find(timingOutClasses.begin(), timingOutClasses.end(),
                                  it->getAttribute<std::string>("classId")) != timingOutClasses.end()) {
                        m_timingOutDevices.insert(it->getKey());
                    }
                }
            }
        }


        void GuiServerDevice::loggerMapConnectedHandler() {
            requestNoWait(get<std::string>("dataLogManagerId"), "slotGetLoggerMap", "", "slotLoggerMap");
        }


        void GuiServerDevice::postReconfigure() {
            remote().setDeviceMonitorInterval(get<int>("propertyUpdateInterval"));

            // One might also want to react on possible changes of "delayOnInput",
            // i.e. change delay value for existing input channels.
            // For now, changing "delayOnInput" will only affect new InputChannels, i.e. _all_ GUI clients requesting
            // data of a specific output channel have to dis- and then reconnect to see the new delay.
        }

        void GuiServerDevice::startDeviceInstantiation() {
            // NOTE: This timer is a rate limiter for device instantiations
            m_deviceInitTimer.expires_from_now(boost::posix_time::milliseconds(get<int>("waitInitDevice")));
            m_deviceInitTimer.async_wait(bind_weak(&karabo::devices::GuiServerDevice::initSingleDevice, this,
                                                   boost::asio::placeholders::error));
        }

        void GuiServerDevice::startNetworkMonitor() {
            m_networkStatsTimer.expires_from_now(
                  boost::posix_time::seconds(get<int>("networkPerformance.sampleInterval")));
            m_networkStatsTimer.async_wait(bind_weak(&karabo::devices::GuiServerDevice::collectNetworkStats, this,
                                                     boost::asio::placeholders::error));
        }

        void GuiServerDevice::startMonitorConnectionQueues(const Hash& currentSuspects) {
            const int interval = get<int>("checkConnectionsInterval");
            m_checkConnectionTimer.expires_from_now(boost::posix_time::seconds(interval));
            m_checkConnectionTimer.async_wait(bind_weak(&GuiServerDevice::monitorConnectionQueues, this,
                                                        boost::asio::placeholders::error, currentSuspects));
        }

        void GuiServerDevice::collectNetworkStats(const boost::system::error_code& error) {
            if (error) {
                KARABO_LOG_FRAMEWORK_ERROR << "Network monitor timer was cancelled!";
                return;
            }

            size_t clientBytesRead = 0, clientBytesWritten = 0;
            {
                boost::mutex::scoped_lock lock(m_channelMutex);
                for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
                    clientBytesRead += it->first->dataQuantityRead();
                    clientBytesWritten += it->first->dataQuantityWritten();
                }
            }

            size_t pipeBytesRead = 0, pipeBytesWritten = 0;
            {
                boost::mutex::scoped_lock lock(m_networkMutex);
                for (const auto& nameAndChannelSet : m_networkConnections) {
                    const std::string& channelName = nameAndChannelSet.first;
                    const InputChannel::Pointer inputChannel = getInputChannelNoThrow(channelName);
                    if (inputChannel) {
                        pipeBytesRead += inputChannel->dataQuantityRead();
                        pipeBytesWritten += inputChannel->dataQuantityWritten();
                    }
                }
            }

            set(Hash("networkPerformance.clientBytesRead", static_cast<unsigned long long>(clientBytesRead),
                     "networkPerformance.clientBytesWritten", static_cast<unsigned long long>(clientBytesWritten),
                     "networkPerformance.pipelineBytesRead", static_cast<unsigned long long>(pipeBytesRead),
                     "networkPerformance.pipelineBytesWritten", static_cast<unsigned long long>(pipeBytesWritten)));

            startNetworkMonitor();
        }

        void GuiServerDevice::onConnect(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel) {
            if (e) return;

            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Incoming connection";

                // Set 3 different queues for publishing (writeAsync) to the GUI client...
                // priority 2 bound to FAST_DATA traffic: This queue is filled only when GUI client reports readiness
                // for a pipeline channel, so we can afford a LOSSLESS policy. In fact we have to:
                // If something would be dropped, the client will never report readiness again for that pipeline. And
                // we do not have to fear that the queue grows very big - it is limited to the number of pipelines that
                // the client monitors.
                // We do not use the same queue as for priority 4 (although both are lossless) since sending FAST_DATA
                // still has lower priority than other data.
                channel->setAsyncChannelPolicy(FAST_DATA, "LOSSLESS");
                // priority 3 bound to REMOVE_OLDEST dropping policy
                channel->setAsyncChannelPolicy(REMOVE_OLDEST, "REMOVE_OLDEST", get<int>("lossyDataQueueCapacity"));
                // priority 4 should be LOSSLESS
                channel->setAsyncChannelPolicy(LOSSLESS, "LOSSLESS");

                channel->readAsyncHash(
                      bind_weak(&karabo::devices::GuiServerDevice::onLoginMessage, this, _1, channel, _2));

                string const version = karabo::util::Version::getVersion();
                Hash systemInfo("type", "brokerInformation");
                systemInfo.set("topic", m_topic);
                systemInfo.set("hostname", get<std::string>("hostName"));
                systemInfo.set("hostport", get<unsigned int>("port"));
                systemInfo.set("deviceId", getInstanceId());
                systemInfo.set("readOnly", m_isReadOnly);
                systemInfo.set("version", version);
                systemInfo.set("authServer", get<std::string>("authServer"));

                channel->writeAsync(systemInfo);

                // Forward banner info if some:
                const std::vector<std::string> banner_data(get<std::vector<std::string>>("bannerData"));
                if (banner_data.size() == 3ul) {
                    Hash banner("type", "notification", "contentType", "banner", "message", banner_data[0]);
                    if (!banner_data[1].empty()) {
                        banner.set("background", banner_data[1]);
                    }
                    if (!banner_data[2].empty()) {
                        banner.set("foreground", banner_data[2]);
                    }
                    channel->writeAsync(banner);
                }

                // Re-register acceptor socket (allows handling multiple clients)
                m_dataConnection->startAsync(bind_weak(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));


            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onConnect(): " << e.what();
                m_dataConnection->startAsync(bind_weak(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));
            }
        }

        void GuiServerDevice::registerConnect(const karabo::util::Version& version,
                                              const karabo::net::Channel::Pointer& channel, const std::string& userId,
                                              const std::string& oneTimeToken) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = ChannelData(version, userId, oneTimeToken); // keeps channel information
            // Update the number of clients connected
            set("connectedClientCount", static_cast<unsigned int>(m_channels.size()));
        }


        void GuiServerDevice::onLoginMessage(const karabo::net::ErrorCode& e,
                                             const karabo::net::Channel::Pointer& channel, karabo::util::Hash& info) {
            if (e) {
                channel->close();
                return;
            }
            try {
                if (!info.has("type")) {
                    KARABO_LOG_FRAMEWORK_WARN << "Ignoring request that lacks type specification: " << info;
                    return;
                }
                const string& type = info.get<string>("type");
                if (type == "login") {
                    // onLogin will re-register the Hash reader.
                    onLogin(channel, info);
                    return;
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "Ignoring request from client not yet logged in: " << info;
                    const std::string message("Action '" + type + "' refused before log in");
                    const Hash h("type", "notification", "message", message);
                    safeClientWrite(WeakChannelPointer(channel), h);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onLoginMessage(): " << e.what();
            }

            // Read the next Hash from the client
            channel->readAsyncHash(bind_weak(&karabo::devices::GuiServerDevice::onLoginMessage, this, _1, channel, _2));
        }

        void GuiServerDevice::sendLoginErrorAndDisconnect(const Channel::Pointer& channel, const string& userId,
                                                          const string& cliVersion, const string& errorMsg) {
            auto weakChannel = WeakChannelPointer(channel);
            const Hash h("type", "notification", "message", errorMsg);
            safeClientWrite(weakChannel, h);
            KARABO_LOG_FRAMEWORK_WARN << "Refused login request of user '" << userId << "' using GUI client version "
                                      << cliVersion << " (from " << getChannelAddress(channel) << "): " + errorMsg;
            auto timer(boost::make_shared<boost::asio::deadline_timer>(karabo::net::EventLoop::getIOService()));
            timer->expires_from_now(boost::posix_time::milliseconds(500));
            timer->async_wait(bind_weak(&GuiServerDevice::deferredDisconnect, this, boost::asio::placeholders::error,
                                        weakChannel, timer));
        }

        void GuiServerDevice::onTokenAuthorizeResult(const WeakChannelPointer& weakChannel, const std::string& clientId,
                                                     const karabo::util::Version& clientVersion,
                                                     const std::string& oneTimeToken,
                                                     const karabo::net::OneTimeTokenAuthorizeResult& authResult) {
            karabo::net::Channel::Pointer channel = weakChannel.lock();
            if (channel) {
                KARABO_LOG_FRAMEWORK_DEBUG << "One-time token validation results:\nSuccess: " << authResult.success
                                           << "\nUserId: " << authResult.userId
                                           << "\nAccess Level: " << authResult.accessLevel
                                           << "\nErrMsg: " << authResult.errMsg;
                if (!authResult.success) {
                    const string errorMsg = "Error validating token: " + authResult.errMsg;
                    sendLoginErrorAndDisconnect(channel, clientId, clientVersion.getString(), errorMsg);
                    return;
                } else {
                    registerConnect(clientVersion, channel, authResult.userId, oneTimeToken);

                    // For read-only servers, the access level is always OBSERVER.
                    Hash h("type", "loginInformation");
                    h.set("accessLevel", m_isReadOnly ? static_cast<int>(Schema::AccessLevel::OBSERVER)
                                                      : static_cast<int>(authResult.accessLevel));
                    safeClientWrite(channel, h);

                    sendSystemTopology(weakChannel);
                }
            }
        }

        void GuiServerDevice::onLogin(const karabo::net::Channel::Pointer& channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onLogin";

                // Check valid login.
                const Version clientVersion(hash.get<string>("version"));
                const bool userAuthActive = !get<string>("authServer").empty();
                // Before version 2.16 of the Framework, the  GUI client sends the clientId (clientHostname-clientPID)
                // under the "username" key. Since version 2.16, that key name is being deprecated in favor or the
                // "clientId" key. For backward compatibility, both keys will be kept during the deprecation period.
                const string clientId =
                      hash.has("clientId") ? hash.get<string>("clientId") : hash.get<string>("username");
                const string cliVersion = clientVersion.getString();

                if (clientVersion < Version(get<std::string>("minClientVersion"))) {
                    const string errorMsg = "Your GUI client has version '" + cliVersion +
                                            "', but the minimum required is: " + get<std::string>("minClientVersion");
                    sendLoginErrorAndDisconnect(channel, clientId, cliVersion, errorMsg);
                    return;
                }
                if (userAuthActive && !hash.has("oneTimeToken")) {
                    const string errorMsg = "Refused non-user-authenticated login.\n\nGUI server at '" +
                                            get<string>("hostName") + ":" + toString(get<unsigned int>("port")) +
                                            "' only accepts authenticated logins.\nPlease update your GUI client.";
                    sendLoginErrorAndDisconnect(channel, clientId, cliVersion, errorMsg);
                    return;
                }

                auto weakChannel = WeakChannelPointer(channel);
                // Handles token validation, if needed.
                if (userAuthActive) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "One-time token to be validated/authorized: "
                                               << hash.get<string>("oneTimeToken");

                    const std::string oneTimeToken = hash.get<string>("oneTimeToken");
                    m_authClient.authorizeOneTimeToken(
                          oneTimeToken, m_topic,
                          bind_weak(&karabo::devices::GuiServerDevice::onTokenAuthorizeResult, this, weakChannel,
                                    clientId, clientVersion, oneTimeToken, _1));
                } else {
                    // No authentication involved
                    // Use the value of the key "clientUserId" (introduced in 2.16) for logging and auditing purposes.
                    if (hash.has("clientUserId")) {
                        registerConnect(clientVersion, channel, hash.get<string>("clientUserId"));
                    } else {
                        registerConnect(clientVersion, channel);
                    }

                    sendSystemTopology(weakChannel);
                }

                std::stringstream extraInfo;
                if (hash.has("info")) {
                    extraInfo << "\nDetails: " << hash.get<Hash>("info");
                }
                KARABO_LOG_FRAMEWORK_INFO << "Login request of client_id: " << clientId << " (version " << cliVersion
                                          << ")." << extraInfo.str();

                channel->readAsyncHash(bind_weak(&karabo::devices::GuiServerDevice::onRead, this, _1, weakChannel, _2));

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onLogin(): " << e.what();
            }
        }


        void GuiServerDevice::onRead(const karabo::net::ErrorCode& e, WeakChannelPointer channel,
                                     karabo::util::Hash& info) {
            if (e) {
                onError(e, channel);
                return;
            }

            try {
                // GUI communication scenarios
                if (info.has("type")) {
                    const string& type = info.get<string>("type");
                    if (m_isReadOnly && violatesReadOnly(type, info)) {
                        // not allowed, bail out and inform client
                        const std::string message("Action '" + type +
                                                  "' is not allowed on GUI servers in readOnly mode!");
                        const Hash h("type", "notification", "message", message);
                        safeClientWrite(channel, h);
                    } else if (violatesClientConfiguration(type, channel)) {
                        // not allowed, bail out and inform client
                        const std::string message(
                              "Action '" + type +
                              "' is not allowed on this GUI client version. Please upgrade your GUI client");
                        const Hash h("type", "notification", "message", message);
                        safeClientWrite(channel, h);
                    } else if (type == "reconfigure") {
                        onReconfigure(channel, info);
                    } else if (type == "execute") {
                        onExecute(channel, info);
                    } else if (type == "getDeviceConfiguration") {
                        onGetDeviceConfiguration(channel, info);
                    } else if (type == "getDeviceSchema") {
                        onGetDeviceSchema(channel, info);
                    } else if (type == "getClassSchema") {
                        onGetClassSchema(channel, info);
                    } else if (type == "initDevice") {
                        onInitDevice(channel, info);
                    } else if (type == "killServer") {
                        onKillServer(info);
                    } else if (type == "killDevice") {
                        onKillDevice(info);
                    } else if (type == "startMonitoringDevice") {
                        onStartMonitoringDevice(channel, info);
                    } else if (type == "stopMonitoringDevice") {
                        onStopMonitoringDevice(channel, info);
                    } else if (type == "getPropertyHistory") {
                        onGetPropertyHistory(channel, info);
                    } else if (type == "getConfigurationFromPast") {
                        onGetConfigurationFromPast(channel, info);
                    } else if (type == "subscribeNetwork") {
                        onSubscribeNetwork(channel, info);
                    } else if (type == "requestNetwork") {
                        onRequestNetwork(channel, info);
                    } else if (type == "error") {
                        onGuiError(info);
                    } else if (type == "acknowledgeAlarm") {
                        onAcknowledgeAlarm(channel, info);
                    } else if (type == "requestAlarms") {
                        onRequestAlarms(channel, info);
                    } else if (type == "updateAttributes") {
                        onUpdateAttributes(channel, info);
                    } else if (type == "projectBeginUserSession") {
                        onProjectBeginUserSession(channel, info);
                    } else if (type == "projectEndUserSession") {
                        onProjectEndUserSession(channel, info);
                    } else if (type == "projectSaveItems") {
                        onProjectSaveItems(channel, info);
                    } else if (type == "projectLoadItems") {
                        onProjectLoadItems(channel, info);
                    } else if (type == "projectListProjectManagers") {
                        onProjectListProjectManagers(channel, info);
                    } else if (type == "projectListItems") {
                        onProjectListItems(channel, info);
                    } else if (type == "projectListDomains") {
                        onProjectListDomains(channel, info);
                    } else if (type == "projectUpdateAttribute") {
                        onProjectUpdateAttribute(channel, info);
                    } else if (type == "requestGeneric") {
                        onRequestGeneric(channel, info);
                    } else if (type == "subscribeLogs") {
                        onSubscribeLogs(channel, info);
                    } else if (type == "setLogPriority") {
                        onSetLogPriority(channel, info);
                    } else {
                        // Inform the client that he is using a non compatible protocol
                        const std::string message("The gui server with version " + get<string>("classVersion") +
                                                  " does not support the client application request of " + type);
                        const Hash h("type", "notification", "message", message);
                        safeClientWrite(channel, h);
                        KARABO_LOG_FRAMEWORK_WARN << "Ignoring request of unknown type '" << type << "'";
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "Ignoring request that lacks type specification: " << info;
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRead(): " << e.what();
            }

            // Read the next Hash from the client
            karabo::net::Channel::Pointer chan = channel.lock();
            if (chan) {
                chan->readAsyncHash(bind_weak(&karabo::devices::GuiServerDevice::onRead, this, _1, channel, _2));
            }
        }

        bool GuiServerDevice::isProjectLoadingReplyType(const std::string& replyType) {
            return replyType == "projectListDomains" || replyType == "projectListItems" ||
                   replyType == "projectLoadItems" || replyType == "projectBeginUserSession" ||
                   replyType == "projectEndUserSession";
        }

        bool GuiServerDevice::violatesReadOnly(const std::string& type, const karabo::util::Hash& info) {
            KARABO_LOG_FRAMEWORK_DEBUG << "violatesReadOnly " << info;
            if (m_writeCommands.find(type) != m_writeCommands.end()) {
                return true;
            } else if (type == "requestGeneric" && info.has("replyType") &&
                       isProjectLoadingReplyType(info.get<string>("replyType"))) {
                // Request involved in the loading of projects are allowed in read-only mode.
                return false;
            } else if (type == "requestGeneric" && info.has("slot") && info.get<string>("slot") != "requestScene" &&
                       info.get<string>("slot") != "slotGetScene") {
                // Requesting scenes are allowed in read-only mode. Configuration Management is not
                return true;
            } else {
                return false;
            }
        }


        bool GuiServerDevice::violatesClientConfiguration(const std::string& type, WeakChannelPointer channel) {
            auto itTypeMinVersion = m_minVersionRestrictions.find(type);
            if (itTypeMinVersion == m_minVersionRestrictions.end()) {
                // `type` not in the restrictions map, so unrestricted.
                return false;
            } else {
                auto chan = channel.lock();
                if (chan) {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    ConstChannelIterator itChannelData = m_channels.find(chan);
                    if (itChannelData != m_channels.end()) {
                        return (itChannelData->second.clientVersion < itTypeMinVersion->second);
                    } else {
                        KARABO_LOG_FRAMEWORK_WARN << "Channel missing its ChannelData. It should never happen.";
                        return true;
                    }
                }
                // channel is null
                return true;
            }
        }

        void GuiServerDevice::onGuiError(const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGuiError";
                Hash::Pointer hdr = boost::make_shared<Hash>();
                Hash::Pointer body = boost::make_shared<Hash>(hash);
                m_guiDebugProducer->write("karaboGuiDebug", hdr, body, 0, 0);

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGuiError(): " << e.what();
            }
        }


        void GuiServerDevice::deferredDisconnect(const boost::system::error_code& err, WeakChannelPointer channel,
                                                 boost::shared_ptr<boost::asio::deadline_timer> timer) {
            KARABO_LOG_FRAMEWORK_DEBUG << "deferredDisconnect";

            auto chan = channel.lock();
            if (chan) {
                // Trigger a call to onError that cleans up
                chan->close();
            }
        }


        void GuiServerDevice::setTimeout(karabo::xms::SignalSlotable::Requestor& requestor,
                                         const karabo::util::Hash& input, const std::string& instanceKey) {
            if (input.has("timeout")) {
                // TODO: remove `skipExecutionTimeout` once "fast slot reply policy" is enforced
                if (!(input.has(instanceKey) && skipExecutionTimeout(input.get<std::string>(instanceKey)))) {
                    // Take the max of what was requested by client and configured on GUI server
                    const int timeoutSec =
                          std::max(input.get<int>("timeout"), m_timeout.load()); // load() for template resolution
                    requestor.timeout(timeoutSec * 1000);                        // convert to ms
                }
            }
        }


        void GuiServerDevice::onReconfigure(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onReconfigure";
                const string& deviceId = hash.get<string>("deviceId");
                const Hash& config = hash.get<Hash>("configuration");
                // TODO Supply user specific context
                if (hash.has("reply") && hash.get<bool>("reply")) {
                    auto requestor = request(deviceId, "slotReconfigure", config);
                    setTimeout(requestor, hash, "deviceId");
                    auto successHandler =
                          bind_weak(&GuiServerDevice::forwardReconfigureReply, this, true, channel, hash);
                    auto failureHandler =
                          bind_weak(&GuiServerDevice::forwardReconfigureReply, this, false, channel, hash);
                    requestor.receiveAsync(successHandler, failureHandler);
                } else {
                    call(deviceId, "slotReconfigure", config);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onReconfigure(): " << e.what();
            }
        }


        void GuiServerDevice::forwardReconfigureReply(bool success, WeakChannelPointer channel, const Hash& input) {
            Hash h("type", "reconfigureReply", "success", success, "input", input);
            if (!success) {
                // Failure, so can get access to exception causing it:
                std::set<std::string> paths;
                input.get<Hash>("configuration").getPaths(paths);
                std::string failTxt;
                std::string details;
                try {
                    throw;
                } catch (const karabo::util::TimeoutException& te) {
                    // TODO: currently ignoring also naughty classes. Remove this once this is enforced.
                    const bool ignoreTimeout =
                          !input.has("timeout") || skipExecutionTimeout(input.get<std::string>("deviceId"));
                    // if the input hash has no timeout key or comes from a "naughty" class, declare success
                    if (ignoreTimeout) {
                        h.set("success", true);
                    }
                    failTxt = "Request not answered within ";
                    if (ignoreTimeout) {
                        // default timeout is in ms. Convert to minutes
                        (failTxt += toString(karabo::xms::SignalSlotable::Requestor::m_defaultAsyncTimeout /
                                             60000.f)) += " minutes.";
                    } else {
                        // Not 100% precise if "timeout" got reconfigured after request was sent...
                        const int timeout = std::max(input.get<int>("timeout"), m_timeout.load());
                        (failTxt += toString(timeout)) += " seconds.";
                    }
                    karabo::util::Exception::clearTrace();
                } catch (const RemoteException& e) {
                    failTxt = e.userFriendlyMsg(true);
                    details = e.details();
                } catch (const Exception& e) {
                    failTxt = e.userFriendlyMsg(false);
                    details = e.detailedMsg();
                } catch (const std::exception& e) {
                    failTxt = e.what();
                }
                KARABO_LOG_FRAMEWORK_WARN << "Failure on request to reconfigure '" << toString(paths) << "' of device '"
                                          << input.get<std::string>("deviceId") << "': " << failTxt
                                          << (details.empty() ? std::string() : ".\nFailure details:\n" + details);
                if (!details.empty()) {
                    (failTxt += m_errorDetailsDelim) += details;
                }
                h.set("reason", std::move(failTxt));
            }
            safeClientWrite(channel, h);
        }

        void GuiServerDevice::onExecute(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onExecute " << hash;
                const string& deviceId = hash.get<string>("deviceId");
                const string& command = hash.get<string>("command");
                // TODO Supply user specific context
                if (hash.has("reply") && hash.get<bool>("reply")) {
                    auto requestor = request(deviceId, command);
                    setTimeout(requestor, hash, "deviceId");
                    // Any reply values are ignored (we do not know their types):
                    auto successHandler = bind_weak(&GuiServerDevice::forwardExecuteReply, this, true, channel, hash);
                    auto failureHandler = bind_weak(&GuiServerDevice::forwardExecuteReply, this, false, channel, hash);
                    requestor.receiveAsync(successHandler, failureHandler);
                } else {
                    call(deviceId, command);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onExecute(): " << e.what();
            }
        }


        void GuiServerDevice::forwardExecuteReply(bool success, WeakChannelPointer channel, const Hash& input) {
            Hash h("type", "executeReply", "success", success, "input", input);
            if (!success) {
                // Failure, so can get access to exception causing it:
                std::string failTxt;
                std::string details;
                try {
                    throw;
                } catch (const karabo::util::TimeoutException&) {
                    // TODO: currently ignoring also naughty classes. Remove this once this is enforced.
                    const bool ignoreTimeout =
                          !input.has("timeout") || skipExecutionTimeout(input.get<std::string>("deviceId"));
                    // if the input hash has no timeout key or comes from a "naughty" class, declare success
                    if (ignoreTimeout) {
                        h.set("success", true);
                    }
                    failTxt = "Request not answered within ";
                    if (ignoreTimeout) {
                        // default timeout is in ms. Convert to minutes
                        (failTxt += toString(karabo::xms::SignalSlotable::Requestor::m_defaultAsyncTimeout /
                                             60000.f)) += " minutes.";
                    } else {
                        // Not 100% precise if "timeout" got reconfigured after request was sent...
                        const int timeout = std::max(input.get<int>("timeout"), m_timeout.load());
                        (failTxt += toString(timeout)) += " seconds.";
                    }
                    karabo::util::Exception::clearTrace();
                } catch (const RemoteException& e) {
                    failTxt = e.userFriendlyMsg(true);
                    details = e.details();
                } catch (const Exception& e) {
                    failTxt = e.userFriendlyMsg(false);
                    details = e.detailedMsg();
                } catch (const std::exception& e) {
                    failTxt = e.what();
                }
                KARABO_LOG_FRAMEWORK_WARN << "Failure on request to execute '" << input.get<std::string>("command")
                                          << "' on device '" << input.get<std::string>("deviceId") << "':" << failTxt
                                          << (details.empty() ? std::string() : ".\n Failure details:\n" + details)
                                          << ".";
                if (!details.empty()) {
                    (failTxt += m_errorDetailsDelim) += details;
                }
                h.set("reason", std::move(failTxt));
            }
            safeClientWrite(channel, h);
        }


        void GuiServerDevice::onInitDevice(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            try {
                const string& serverId = hash.get<string>("serverId");
                const string& deviceId = hash.get<string>("deviceId");
                KARABO_LOG_FRAMEWORK_DEBUG << "onInitDevice: Queuing request to start device instance \"" << deviceId
                                           << "\" on server \"" << serverId << "\"";

                if (!deviceId.empty() && hash.has("schemaUpdates")) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Schema updates were provided for device " << deviceId;

                    AttributeUpdates attrUpdates;
                    attrUpdates.eventMask = 0;
                    attrUpdates.updates = hash.get<std::vector<Hash>>("schemaUpdates");

                    boost::mutex::scoped_lock lock(m_pendingAttributesMutex);
                    m_pendingAttributeUpdates[deviceId] = attrUpdates;
                }

                DeviceInstantiation inst;
                inst.channel = channel;
                inst.hash = hash;
                {
                    boost::mutex::scoped_lock lock(m_pendingInstantiationsMutex);
                    m_pendingDeviceInstantiations.push(inst);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onInitDevice(): " << e.what();
            }
        }


        void GuiServerDevice::initSingleDevice(const boost::system::error_code& err) {
            if (err) {
                KARABO_LOG_FRAMEWORK_ERROR << "Device instantiation timer was cancelled!";
                return;
            }

            try {
                boost::mutex::scoped_lock lock(m_pendingInstantiationsMutex);
                if (!m_pendingDeviceInstantiations.empty()) {
                    const DeviceInstantiation& inst = m_pendingDeviceInstantiations.front();
                    const string& serverId = inst.hash.get<string>("serverId");
                    const string& deviceId = inst.hash.get<string>("deviceId");

                    KARABO_LOG_FRAMEWORK_DEBUG << "initSingleDevice: Requesting to start device instance \"" << deviceId
                                               << "\" on server \"" << serverId << "\"";
                    // initReply both as success and failure handler, identified by boolean flag as last argument
                    request(serverId, "slotStartDevice", inst.hash)
                          .timeout(15000) // 15 seconds
                          .receiveAsync<bool, string>(bind_weak(&karabo::devices::GuiServerDevice::initReply, this,
                                                                inst.channel, deviceId, inst.hash, _1, _2, false),
                                                      bind_weak(&karabo::devices::GuiServerDevice::initReply, this,
                                                                inst.channel, deviceId, inst.hash, false, "", true));

                    m_pendingDeviceInstantiations.pop();
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initSingleDevice(): " << e.what();
            }

            // Always restart the timer!
            startDeviceInstantiation();
        }


        void GuiServerDevice::initReply(WeakChannelPointer channel, const string& givenDeviceId,
                                        const karabo::util::Hash& givenConfig, bool success, const string& message,
                                        bool isFailureHandler) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting init reply - " << (isFailureHandler ? "" : "not ")
                                           << "as failureHandler";

                Hash h("type", "initReply", "deviceId", givenDeviceId, "success", success, "message", message);
                if (isFailureHandler) {
                    std::string& msg = h.get<std::string>("message");
                    if (!msg.empty()) { // as failure handler, initReply is called with empty 'message'
                        msg += ": ";
                    }
                    std::string details;
                    // Called as a failure handler, so can re-throw
                    try {
                        throw;
                    } catch (const RemoteException& e) {
                        msg += e.userFriendlyMsg(true);
                        details = e.details();
                    } catch (const Exception& e) { // includes/usually should be TimeoutException
                        msg += e.userFriendlyMsg(false);
                        details = e.detailedMsg();
                    } catch (const std::exception& e) {
                        msg += e.what();
                    }
                    if (!details.empty()) {
                        (msg += m_errorDetailsDelim) += details;
                    }
                }
                if (isFailureHandler || !success) {
                    KARABO_LOG_FRAMEWORK_WARN << "Instantiating device '" << givenDeviceId
                                              << "' failed: " << h.get<std::string>("message");
                }
                safeClientWrite(channel, h);

                const NewInstanceAttributeUpdateEvents event =
                      (isFailureHandler || !success ? INSTANCE_GONE_EVENT : DEVICE_SERVER_REPLY_EVENT);
                tryToUpdateNewInstanceAttributes(givenDeviceId, event);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initReply " << e.what();
            }
        }


        void GuiServerDevice::safeClientWrite(const WeakChannelPointer channel, const karabo::util::Hash& message,
                                              int prio) {
            karabo::net::Channel::Pointer chan = channel.lock();
            if (chan && chan->isOpen()) {
                // Using false for copyAllData parameter in the call below is safe: NDArrays appear only in pipeline
                // data forwarded from an InputChannel. That forwarding happens from a single method in InputChannel;
                // that method makes no use of the data after forwarding it.
                chan->writeAsync(message, prio, false);
            }
        }


        void GuiServerDevice::safeAllClientsWrite(const karabo::util::Hash& message, int prio) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            for (ConstChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                if (it->first && it->first->isOpen()) it->first->writeAsync(message, prio);
            }
        }


        void GuiServerDevice::onGetDeviceConfiguration(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            try {
                const string& deviceId = hash.get<string>("deviceId");

                Hash config = remote().getConfigurationNoWait(deviceId);

                if (!config.empty()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceConfiguration for '" << deviceId << "': direct answer";
                    // Can't we just use 'config' instead of 'remote().get(deviceId)'?
                    Hash h("type", "deviceConfigurations", "configurations", Hash(deviceId, remote().get(deviceId)));
                    safeClientWrite(channel, h);
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceConfiguration for '" << deviceId
                                               << "': expect later answer";
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetDeviceConfiguration(): " << e.what();
            }
        }


        void GuiServerDevice::onKillServer(const karabo::util::Hash& info) {
            try {
                string serverId = info.get<string>("serverId");
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillServer : \"" << serverId << "\"";
                // TODO Supply user specific context
                call(serverId, "slotKillServer");
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onKillServer(): " << e.what();
            }
        }


        void GuiServerDevice::onKillDevice(const karabo::util::Hash& info) {
            try {
                string deviceId = info.get<string>("deviceId");
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillDevice : \"" << deviceId << "\"";
                call(deviceId, "slotKillDevice");
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onKillDevice(): " << e.what();
            }
        }


        void GuiServerDevice::onStartMonitoringDevice(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                const string& deviceId = info.get<string>("deviceId");

                {
                    bool isKnown = false; // Assume it is yet unknown - if any channel knows it, change this flag
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    karabo::net::Channel::Pointer chan = channel.lock();
                    for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                        ChannelData& channelData = it->second;
                        if (it->first == chan) {
                            const bool inserted = channelData.visibleInstances.insert(deviceId).second;
                            if (!inserted) {
                                KARABO_LOG_FRAMEWORK_INFO << " A client registers a second time to monitor device '"
                                                          << deviceId << "'";
                                isKnown = true;
                            }
                        } else {
                            if (channelData.visibleInstances.find(deviceId) != channelData.visibleInstances.end()) {
                                isKnown = true;
                            }
                        }
                    }
                    if (!isKnown) {
                        remote().registerDeviceForMonitoring(deviceId);
                    }
                    KARABO_LOG_FRAMEWORK_DEBUG << "onStartMonitoringDevice " << deviceId << " ("
                                               << (isKnown ? "known" : "new") << ")";
                }

                // Send back fresh information about device
                // TODO This could check a dirty-flag whether the device changed since last time seen
                onGetDeviceConfiguration(channel, info);

            } catch (const exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onStartMonitoringDevice(): " << e.what();
            }
        }


        void GuiServerDevice::onStopMonitoringDevice(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                const string& deviceId = info.get<string>("deviceId");

                boost::mutex::scoped_lock lock(m_channelMutex);
                karabo::net::Channel::Pointer chan = channel.lock();
                size_t newCount = 0;
                for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    ChannelData& channelData = it->second;
                    if (it->first == chan) {
                        const size_t numErased = channelData.visibleInstances.erase(deviceId);
                        if (numErased < 1u) {
                            KARABO_LOG_FRAMEWORK_INFO << " A client is not monitoring device '" << deviceId
                                                      << "', but wants to stop monitoring it.";
                        }
                    } else {
                        if (channelData.visibleInstances.find(deviceId) != channelData.visibleInstances.end()) {
                            ++newCount;
                        }
                    }
                }

                KARABO_LOG_FRAMEWORK_DEBUG << "onStopMonitoringDevice " << deviceId << " (" << newCount
                                           << " keep monitoring)";
                if (newCount == 0) { // no client has interest anymore
                    remote().unregisterDeviceFromMonitoring(deviceId);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onStopMonitoringDevice(): " << e.what();
            }
        }


        void GuiServerDevice::onGetClassSchema(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                karabo::net::Channel::Pointer chan = channel.lock();
                if (!chan) return;

                const string serverId = info.get<string>("serverId");
                const string classId = info.get<string>("classId");
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    ChannelIterator itChannelData = m_channels.find(chan);
                    if (itChannelData != m_channels.end()) {
                        itChannelData->second.requestedClassSchemas[serverId].insert(classId);
                    }
                }
                Schema schema = remote().getClassSchemaNoWait(serverId, classId);
                if (!schema.empty()) {
                    Hash h("type", "classSchema", "serverId", serverId, "classId", classId, "schema",
                           std::move(schema));
                    safeClientWrite(channel, h);
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetClassSchema : serverId=\"" << serverId << "\", classId=\""
                                               << classId << "\": provided direct answer";
                    // Remove registration again - but we had to register before we trigger the schema request via
                    // getClassSchemaNoWait since otherwise registration may come too late.
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    ChannelIterator itChannelData = m_channels.find(chan);
                    if (itChannelData != m_channels.end()) {
                        ChannelData& chData = itChannelData->second;
                        auto itServToClassMap = chData.requestedClassSchemas.find(serverId);
                        if (itServToClassMap != chData.requestedClassSchemas.end()) {
                            itServToClassMap->second.erase(classId);
                            if (itServToClassMap->second.empty()) chData.requestedClassSchemas.erase(itServToClassMap);
                        }
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetClassSchema : serverId=\"" << serverId << "\", classId=\""
                                               << classId << "\": expect later answer";
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetClassSchema(): " << e.what();
            }
        }


        void GuiServerDevice::onGetDeviceSchema(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                karabo::net::Channel::Pointer chan = channel.lock();
                const string& deviceId = info.get<string>("deviceId");
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    if (chan) {
                        ChannelIterator itChannelData = m_channels.find(chan);
                        if (itChannelData != m_channels.end()) {
                            itChannelData->second.requestedDeviceSchemas.insert(deviceId);
                        }
                    }
                }

                Schema schema = remote().getDeviceSchemaNoWait(deviceId);
                if (schema.empty()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceSchema for '" << deviceId << "': expect later answer";
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceSchema for '" << deviceId << "': direct answer";
                    Hash h("type", "deviceSchema", "deviceId", deviceId, "schema", std::move(schema));
                    safeClientWrite(channel, h);

                    // Clean-up again, registration not needed. But it had to be registered before calling
                    // getDeviceSchemaNoWait since with weird threading, schemaUpdatedHandler could have been called
                    // before we register here.
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    if (chan) {
                        ChannelIterator itChannelData = m_channels.find(chan);
                        if (itChannelData != m_channels.end()) {
                            itChannelData->second.requestedDeviceSchemas.erase(deviceId);
                        }
                    }
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetDeviceSchema(): " << e.what();
            }
        }


        void GuiServerDevice::onGetPropertyHistory(WeakChannelPointer channel, const karabo::util::Hash& info) {
            // Before even thinking about changing this method, don't forget
            // that all changes must also be reflected in Python API2's
            // device_client.getHistory.
            try {
                const string& deviceId = info.get<string>("deviceId");
                const string& property = info.get<string>("property");
                const string& t0 = info.get<string>("t0");
                const string& t1 = info.get<string>("t1");
                int maxNumData = 0;
                if (info.has("maxNumData")) maxNumData = info.getAs<int>("maxNumData");
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetPropertyHistory: " << deviceId << "." << property << ", " << t0
                                           << " - " << t1 << " (" << maxNumData << " points)";

                Hash args("from", t0, "to", t1, "maxNumData", maxNumData);

                const std::string& readerId(getDataReaderId(deviceId));
                auto okHandler =
                      bind_weak(&karabo::devices::GuiServerDevice::propertyHistory, this, channel, true, _1, _2, _3);
                auto failHandler = bind_weak(&karabo::devices::GuiServerDevice::propertyHistory, this, channel, false,
                                             deviceId, property, vector<Hash>());
                request(readerId, "slotGetPropertyHistory", deviceId, property, args)
                      .receiveAsync<string, string, vector<Hash>>(okHandler, failHandler);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetPropertyHistory(): " << e.what();
            }
        }


        void GuiServerDevice::propertyHistory(WeakChannelPointer channel, bool success, const std::string& deviceId,
                                              const std::string& property,
                                              const std::vector<karabo::util::Hash>& data) {
            try {
                Hash h("type", "propertyHistory", "deviceId", deviceId, "property", property, "data", data, "success",
                       success);
                std::string& reason = h.bindReference<std::string>("reason");

                if (success) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting property history: " << deviceId << "." << property << " "
                                               << data.size();
                } else {
                    std::string details;
                    // TODO: Failure handler - figure out what went wrong:
                    // In principle, 'reason' should be properly filled using m_errorDetailsDelim, RemoteException etc.
                    // But currently (2.14.0), GUI ignores 'reason' anyway.
                    try {
                        throw;
                    } catch (const RemoteException& e) {
                        reason = e.userFriendlyMsg(true);
                        details = e.details();
                    } catch (const Exception& e) { // includes/usually should be TimeoutException
                        reason = e.userFriendlyMsg(false);
                        details = e.detailedMsg();
                    } catch (const std::exception& e) {
                        reason = e.what();
                    }
                    KARABO_LOG_FRAMEWORK_INFO << "Property history request to " << deviceId << "." << property
                                              << " failed: " << reason
                                              << (details.empty() ? "" : "\nFailure details:\n") << details;
                    if (!details.empty()) {
                        (reason += m_errorDetailsDelim) += details;
                    }
                }

                safeClientWrite(channel, h, REMOVE_OLDEST);

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in propertyHistory: " << e.what();
            }
        }


        void GuiServerDevice::onGetConfigurationFromPast(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                const string& deviceId = info.get<string>("deviceId");
                const string& time = info.get<string>("time");
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetConfigurationFromPast: " << deviceId << " @ " << time;

                const std::string& readerId(getDataReaderId(deviceId));
                const bool& preview(info.has("preview") ? info.get<bool>("preview") : false);

                auto handler = bind_weak(&karabo::devices::GuiServerDevice::configurationFromPast, this, channel,
                                         deviceId, time, preview, _1, _2, _3, _4);
                auto failureHandler = bind_weak(&karabo::devices::GuiServerDevice::configurationFromPastError, this,
                                                channel, deviceId, time);
                // Two minutes timeout due to current implementation of slotGetConfigurationFromPast in FileLogReader:
                // The amount of data it has to read depends on the time when the device (more precisely: its
                // datalogger) was started the last time before the point in time that you requested and all the
                // parameter updates in between these two time points.
                request(readerId, "slotGetConfigurationFromPast", deviceId, time)
                      .timeout(120000) // 2 minutes - if we do not specify it will be 2*KARABO_SYS_TTL, i.e. 4 minutes
                      .receiveAsync<Hash, Schema, bool, std::string>(handler, failureHandler);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetConfigurationFromPast(): " << e.what();
                // Be a bit cautious: exception might come from an ill-formed info
                const boost::optional<const Hash::Node&> idNode = info.find("deviceId");
                const boost::optional<const Hash::Node&> tNode = info.find("time");
                const std::string id(idNode && idNode->is<std::string>() ? idNode->getValue<std::string>() : "unknown");
                const std::string time(tNode && tNode->is<std::string>() ? tNode->getValue<std::string>() : "unknown");

                configurationFromPastError(channel, id, time);
            }
        }


        void GuiServerDevice::configurationFromPast(WeakChannelPointer channel, const std::string& deviceId,
                                                    const std::string& time, const bool& preview,
                                                    const karabo::util::Hash& config,
                                                    const karabo::util::Schema& /*schema*/,
                                                    const bool configAtTimepoint, const std::string& configTimepoint) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting configuration from past: " << deviceId << " @ " << time;

                Hash h("type", "configurationFromPast", "deviceId", deviceId, "time", time, "preview", preview);
                if (config.empty()) {
                    // Currently (Oct 2018) DataLogReader::getConfigurationFromPast does not reply errors, but empty
                    // configuration if it could not fulfill the request, e.g. because the device was not online at the
                    // requested time.
                    h.set("success", false);
                    h.set("reason", "Received empty configuration:\nLikely '" + deviceId +
                                          "' has not been online (or not logging) until the requested time '" + time +
                                          "'.");
                } else {
                    h.set("success", true);
                    h.set("config", config);
                    h.set("configAtTimepoint", configAtTimepoint);
                    h.set("configTimepoint", configTimepoint);
                }

                safeClientWrite(channel, h, REMOVE_OLDEST);

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in configurationFromPast: " << e.what();
            }
        }


        void GuiServerDevice::configurationFromPastError(WeakChannelPointer channel, const std::string& deviceId,
                                                         const std::string& time) {
            // Log failure reason
            std::string failureReason;
            std::string details;
            try {
                throw; // Error handlers are called within a try block, so we can rethrow the caught exception
            } catch (const karabo::util::TimeoutException&) {
                failureReason = "Request timed out:\nProbably the data logging infrastructure is not available.";
            } catch (const RemoteException& e) {
                failureReason = e.userFriendlyMsg(true);
                details = e.details();
            } catch (const Exception& e) {
                failureReason = e.userFriendlyMsg(false);
                details = e.detailedMsg();
            } catch (const std::exception& e) {
                failureReason = e.what();
            }
            KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting configuration from past failed: " << deviceId << " @ " << time
                                       << " : " << failureReason << "\nFailure details:\n"
                                       << details;
            if (!details.empty()) {
                (failureReason += m_errorDetailsDelim) += details;
            }

            try {
                const Hash h("type", "configurationFromPast", "deviceId", deviceId, "time", time, "success", false,
                             "reason", failureReason);
                safeClientWrite(channel, h, REMOVE_OLDEST);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in configurationFromPastError: " << e.what();
            }
        }


        std::string GuiServerDevice::getDataReaderId(const std::string& deviceId) const {
            const string loggerId = DATALOGGER_PREFIX + deviceId;
            boost::mutex::scoped_lock lock(m_loggerMapMutex);
            if (m_loggerMap.has(loggerId)) {
                return DATALOGREADER_PREFIX + ("0-" + m_loggerMap.get<string>(loggerId));
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Cannot determine DataLogReaderId: No '" << loggerId << "' in map for '"
                                           << deviceId << "'";                      // Full details in log file, ...
                throw KARABO_PARAMETER_EXCEPTION("Cannot determine DataLogReader"); // ...less for exception.
                return std::string();                                               // please the compiler
            }
        }


        void GuiServerDevice::onSubscribeNetwork(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                const string& channelName = info.get<string>("channelName");
                const bool subscribe = info.get<bool>("subscribe");
                KARABO_LOG_FRAMEWORK_DEBUG << "onSubscribeNetwork : channelName = '" << channelName << "' "
                                           << (subscribe ? "+" : "-");

                boost::mutex::scoped_lock lock(m_networkMutex);
                std::set<WeakChannelPointer>& channelSet = m_networkConnections[channelName]; // might create empty set
                if (subscribe) {
                    const bool notYetRegistered = channelSet.empty();
                    const bool inserted = channelSet.insert(channel).second;
                    if (!inserted) {
                        // This happens when a GUI client has a scene open while the device is down and then restarts:
                        // Client [at least until 2.14.X] will call this (but does not have to or maybe should not).
                        KARABO_LOG_FRAMEWORK_INFO << "A GUI client wants to subscribe a second time to output channel: "
                                                  << channelName;
                    }
                    // Mark as ready - no matter whether ready already before...
                    m_readyNetworkConnections[channelName][channel] = true;
                    if (notYetRegistered) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Register to monitor '" << channelName << "'";

                        auto dataHandler = bind_weak(&GuiServerDevice::onNetworkData, this, channelName, _1, _2);
                        // Channel configuration - we rely on defaults as: "dataDistribution" == copy, "onSlowness" ==
                        // drop
                        const Hash cfg("delayOnInput", get<int>("delayOnInput"));
                        if (!remote().registerChannelMonitor(channelName, dataHandler, cfg)) {
                            KARABO_LOG_FRAMEWORK_WARN << "Already monitoring '" << channelName << "'!";
                            // Should we remote().unregisterChannelMonitor' and try again? But problem never seen...
                        }
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Do not register to monitor '" << channelName << "' "
                                                   << "since "
                                                   << channelSet.size() - (1u * inserted) // -1 except if not new
                                                   << " client(s) already registered.";
                    }
                } else { // i.e. un-subscribe
                    if (0 == channelSet.erase(channel)) {
                        // Would happen if 'instanceGoneHandler' would clear m_readyNetworkConnections (as done
                        // before 2.15.X) when a scene is closed that shows channel data, but the device is not alive
                        // (anymore).
                        KARABO_LOG_FRAMEWORK_WARN << "A GUI client wants to un-subscribe from an output channel that it"
                                                  << " is not subscribed: " << channelName;
                    }
                    auto itReadyByChannel = m_readyNetworkConnections.find(channelName);
                    if (itReadyByChannel != m_readyNetworkConnections.end()) {
                        // No interest, no readiness:
                        itReadyByChannel->second.erase(channel);
                        if (itReadyByChannel->second.empty()) {
                            m_readyNetworkConnections.erase(itReadyByChannel);
                        }
                    }
                    if (channelSet.empty()) {
                        if (!remote().unregisterChannelMonitor(channelName)) {
                            // See comment above about channelSet.erase(..)
                            KARABO_LOG_FRAMEWORK_WARN << "Failed to unregister '" << channelName << "'";
                        }
                        m_networkConnections.erase(channelName); // Caveat: Makes 'channelSet' a dangling reference...
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Do not unregister to monitor '" << channelName << "' "
                                                   << "since " << channelSet.size() << " client(s) still interested";
                    }
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onSubscribeNetwork(): " << e.what();
            }
        }


        void GuiServerDevice::onSubscribeLogs(WeakChannelPointer channel, const karabo::util::Hash& info) {
            Hash h("type", "subscribeLogsReply", "success", true, // Put to false in 2.18.X
                   "reason", "Log subscription not supported anymore since 2.17.0");
            safeClientWrite(channel, h);
        }


        void GuiServerDevice::onSetLogPriority(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                const std::string& priority = info.get<std::string>("priority");
                const std::string& instanceId = info.get<std::string>("instanceId");
                KARABO_LOG_FRAMEWORK_DEBUG << "onSetLogPriority : '" << instanceId << "' to '" << priority << "'";

                auto requestor = request(instanceId, "slotLoggerPriority", priority);
                auto successHandler = bind_weak(&GuiServerDevice::forwardSetLogReply, this, true, channel, info);
                auto failureHandler = bind_weak(&GuiServerDevice::forwardSetLogReply, this, false, channel, info);
                requestor.receiveAsync(successHandler, failureHandler);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onSubscribeLogs(): " << e.what();
            }
        }


        void GuiServerDevice::forwardSetLogReply(bool success, WeakChannelPointer channel, const Hash& input) {
            Hash h("type", "setLogPriorityReply", "success", success, "input", input);
            if (!success) {
                // Failure, so can get access to exception causing it:
                std::string reason;
                std::string details;
                try {
                    throw;
                } catch (const RemoteException& e) {
                    reason = e.userFriendlyMsg(true);
                    details = e.details();
                } catch (const Exception& e) { // includes/usually should be TimeoutException
                    reason = e.userFriendlyMsg(false);
                    details = e.detailedMsg();
                } catch (const std::exception& e) {
                    reason = e.what();
                }
                KARABO_LOG_FRAMEWORK_WARN << "Failure on setLogPriority on server '"
                                          << input.get<std::string>("instanceId") << "': " << reason
                                          << (details.empty() ? std::string() : ".\nFailure details:\n" + details)
                                          << ".";
                if (!details.empty()) {
                    (reason += m_errorDetailsDelim) += details;
                }
                h.set("reason", std::move(reason));
            }
            safeClientWrite(channel, h);
        }


        void GuiServerDevice::onRequestNetwork(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                const string& channelName = info.get<string>("channelName");
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestNetwork for " << channelName;
                boost::mutex::scoped_lock lock(m_networkMutex);
                m_readyNetworkConnections[channelName][channel] = true;
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestNetwork: " << e.what();
            }
        }


        void GuiServerDevice::onNetworkData(const std::string& channelName, const karabo::util::Hash& data,
                                            const karabo::xms::InputChannel::MetaData& meta) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onNetworkData ....";

                Hash h("type", "networkData", "name", channelName);
                // Assign timestamp and aggressively try to avoid any copies by move-assign the 'data' to the payload
                // that we send to the clients. For that we cast const away from 'data' and in fact modify it!
                // That is safe, see comment in InputChannel::triggerIOEvent() which calls this method.
                Hash::Node& dataNode = h.set("data", Hash());
                dataNode.getValue<Hash>() = std::move(const_cast<Hash&>(data));
                Hash::Node& metaNode = h.set("meta.timestamp", true);
                meta.getTimestamp().toHashAttributes(metaNode.getAttributes());
                boost::mutex::scoped_lock lock(m_networkMutex);
                NetworkMap::const_iterator iter = m_networkConnections.find(channelName);
                if (iter != m_networkConnections.cend()) {
                    for (const WeakChannelPointer& channel : iter->second) { // iter->second is set<WeakChannelPointer>
                        bool& ready = m_readyNetworkConnections[channelName][channel];
                        if (ready) {
                            // Ready for data, so send and set non-ready.
                            safeClientWrite(channel, h, FAST_DATA);
                            ready = false; // it's a reference
                        }
                    }
                } // else: all clients lost interest, but still some data arrives
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onNetworkData: " << e.what();
            }
        }


        void GuiServerDevice::sendSystemTopology(WeakChannelPointer channel) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "sendSystemTopology";
                KARABO_LOG_FRAMEWORK_DEBUG << remote().getSystemTopology();
                Hash h("type", "systemTopology", "systemTopology", remote().getSystemTopology());
                safeClientWrite(channel, h);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in sendSystemTopology(): " << e.what();
            }
        }


        void GuiServerDevice::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
            // topologyEntry is an empty Hash at path <type>.<instanceId> with all the instanceInfo as attributes
            try {
                const std::string& type = topologyEntry.begin()->getKey();
                if (type == "device") {
                    const std::vector<std::string>& timingOutClasses =
                          get<std::vector<std::string>>("ignoreTimeoutClasses");
                    recalculateTimingOutDevices(topologyEntry, timingOutClasses, false);
                    const Hash& deviceHash = topologyEntry.get<Hash>(type);
                    const std::string& instanceId = deviceHash.begin()->getKey();
                    {
                        boost::mutex::scoped_lock lock(m_channelMutex);
                        for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                            if (it->second.visibleInstances.find(instanceId) != it->second.visibleInstances.end()) {
                                KARABO_LOG_FRAMEWORK_INFO << "instanceNewHandler registers " << instanceId;
                                remote().registerDeviceForMonitoring(instanceId);
                                break; // no need to check whether any further channel is interested
                            }
                        }
                    }

                    if (instanceId == get<std::string>("dataLogManagerId")) {
                        // The corresponding 'connect' is done by SignalSlotable's automatic reconnect feature.
                        // Even this request might not be needed since the logger manager emits the corresponding
                        // signal. But we cannot be 100% sure that our 'connect' has been registered in time.
                        requestNoWait(get<std::string>("dataLogManagerId"), "slotGetLoggerMap", "", "slotLoggerMap");
                    }

                    tryToUpdateNewInstanceAttributes(instanceId, INSTANCE_NEW_EVENT);

                    connectPotentialAlarmService(topologyEntry);
                    registerPotentialProjectManager(topologyEntry);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in instanceNewHandler(): " << e.what();
            }
        }


        void GuiServerDevice::instanceChangeHandler(const karabo::util::Hash& instChangeData) {
            try {
                // Sends the instance changes to all the connected GUI clients.
                Hash h("type", "topologyUpdate", "changes", instChangeData);
                safeAllClientsWrite(h);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in instanceChangeHandler(): " << e.what();
            }
        }


        void GuiServerDevice::instanceGoneHandler(const std::string& instanceId,
                                                  const karabo::util::Hash& /*instInfo*/) {
            try {
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);

                    size_t numClientsUnregister = 0ul;
                    for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                        it->second.requestedDeviceSchemas.erase(instanceId);
                        it->second.requestedClassSchemas.erase(instanceId); // instanceId might be a server

                        // Count clients that had interest in instanceId and keep their interests or not
                        if (it->second.visibleInstances.erase(instanceId) > 0) ++numClientsUnregister;
                    }
                    if (numClientsUnregister > 0ul) {
                        KARABO_LOG_FRAMEWORK_INFO << "Unregister from " << instanceId << " since gone, "
                                                  << numClientsUnregister << " clients monitored it";
                        remote().unregisterDeviceFromMonitoring(instanceId);
                    }
                }

                {
                    // Erase instance from the attribute update map (maybe)
                    boost::mutex::scoped_lock lock(m_pendingAttributesMutex);
                    m_pendingAttributeUpdates.erase(instanceId);
                }

                // Older versions cleaned m_networkConnections from input channels of the dead 'instanceId' here.
                // That works since the GUI client (as of 2.14.X) gives an onSubscribeNetwork request again if it
                // gets notified that the device is back again.
                // But that is not needed: DeviceClient and SignalSlotable take care to reconnect for any registered
                // channels. In fact, that will lead to a faster reconnection than waiting for the client's request.

                {
                    boost::upgrade_lock<boost::shared_mutex> lk(m_projectManagerMutex);
                    auto manager = m_projectManagers.find(instanceId);
                    if (manager != m_projectManagers.end()) {
                        boost::upgrade_to_unique_lock<boost::shared_mutex> ulk(lk);
                        m_projectManagers.erase(manager);
                    }
                }
                {
                    // clean up the device from the list of slow devices
                    boost::mutex::scoped_lock lock(m_timingOutDevicesMutex);
                    auto it = m_timingOutDevices.find(instanceId);
                    if (it != m_timingOutDevices.end()) {
                        m_timingOutDevices.erase(it);
                    }
                }
                tryToUpdateNewInstanceAttributes(instanceId, INSTANCE_GONE_EVENT);

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in instanceGoneHandler(): " << e.what();
            }
        }


        void GuiServerDevice::devicesChangedHandler(const karabo::util::Hash& deviceUpdates) {
            // The keys of 'deviceUpdates' are the deciveIds with updates and the values behind the keys are
            // Hashes with the updated properties.
            try {
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Loop on all clients
                for (ConstChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    if (!it->first || !it->first->isOpen()) continue;

                    Hash configs;
                    for (auto mapIter = deviceUpdates.mbegin(); mapIter != deviceUpdates.mend(); ++mapIter) {
                        const std::string& deviceId = mapIter->first;
                        // Optimization: send only updates for devices the client is interested in.
                        if (it->second.visibleInstances.find(deviceId) != it->second.visibleInstances.end()) {
                            const Hash& updates = mapIter->second.getValue<Hash>();
                            configs.set(deviceId, updates);
                        }
                    }
                    if (!configs.empty()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Sending " << configs.size()
                                                   << " configuration updates to GUI client";
                        Hash h("type", "deviceConfigurations", "configurations", std::move(configs));
                        it->first->writeAsync(h);
                    }
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in devicesChangesHandler(): " << e.what();
            }
        }


        void GuiServerDevice::classSchemaHandler(const std::string& serverId, const std::string& classId,
                                                 const karabo::util::Schema& classSchema) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "classSchemaHandler: serverId: \"" << serverId << "\" - classId :\""
                                           << classId << "\"";

                Hash h("type", "classSchema", "serverId", serverId, "classId", classId, "schema", classSchema);

                boost::mutex::scoped_lock lock(m_channelMutex);
                for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    auto itReq = it->second.requestedClassSchemas.find(serverId);
                    if (itReq != it->second.requestedClassSchemas.end()) {
                        if (itReq->second.find(classId) != itReq->second.end()) {
                            const Channel::Pointer& channel = it->first;
                            // If e.g. a schema of a non-existing plugin was requested, the schema could well be empty.
                            // Forward to client anyway since otherwise it will not ask again later.
                            if (classSchema.empty()) {
                                // No harm if logged for more than one client
                                KARABO_LOG_FRAMEWORK_WARN << "Received empty schema for class '" << classId
                                                          << "' on server '" << serverId << "'.";
                            }
                            if (channel && channel->isOpen()) {
                                channel->writeAsync(h);
                            }
                            itReq->second.erase(classId);
                            // remove from the server key if all "classSchema" requests are fulfilled
                            if (itReq->second.empty()) it->second.requestedClassSchemas.erase(itReq);
                        }
                    }
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in classSchemaHandler(): " << e.what();
            }
        }


        void GuiServerDevice::schemaUpdatedHandler(const std::string& deviceId, const karabo::util::Schema& schema) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Sending schema updated for '" << deviceId << "'";

                if (schema.empty()) {
                    KARABO_LOG_FRAMEWORK_WARN << "Going to send an empty schema for deviceId \"" << deviceId << "\".";
                }

                Hash h("type", "deviceSchema", "deviceId", deviceId, "schema", schema);

                boost::mutex::scoped_lock lock(m_channelMutex);
                // Loop on all clients
                for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    // Optimization: write only to clients subscribed to deviceId
                    if ((it->second.visibleInstances.find(deviceId) !=
                         it->second.visibleInstances.end()) // if instance is visible
                        || (it->second.requestedDeviceSchemas.find(deviceId) !=
                            it->second.requestedDeviceSchemas.end())) { // if instance is requested
                        if (it->first && it->first->isOpen()) {
                            it->first->writeAsync(h);
                        }
                        it->second.requestedDeviceSchemas.erase(deviceId);
                    }
                }


            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in schemaUpdatedHandler(): " << e.what();
            }
        }


        void GuiServerDevice::onError(const karabo::net::ErrorCode& errorCode, WeakChannelPointer channel) {
            KARABO_LOG_FRAMEWORK_INFO << "onError : TCP socket got error : " << errorCode.value() << " -- \""
                                      << errorCode.message() << "\",  Close connection to a client";

            try {
                karabo::net::Channel::Pointer chan = channel.lock();
                {
                    std::set<std::string> devIdsToUnregister;
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    ChannelIterator it = m_channels.find(chan);
                    if (it != m_channels.end()) {
                        it->first->close(); // This closes socket and unregisters channel from connection
                        devIdsToUnregister.swap(it->second.visibleInstances); // copy to the empty set
                        m_channels.erase(it);                                 // Remove channel as such
                        // Now iterate on all remaining clients to see which devices monitored by the removed channel
                        // are also monitored by any of them.
                        for (it = m_channels.begin(); it != m_channels.end(); ++it) {
                            auto& visibles = it->second.visibleInstances;
                            for (auto itId = devIdsToUnregister.begin(); itId != devIdsToUnregister.end();) {
                                if (visibles.find(*itId) != visibles.end()) {
                                    // A deviceId that also another client has interest in: do not unregister
                                    itId = devIdsToUnregister.erase(itId);
                                } else {
                                    ++itId;
                                }
                            }
                        }
                        // Any device that no-one is still monitoring has to get unregistered
                        KARABO_LOG_FRAMEWORK_INFO << "Unregister from '" << toString(devIdsToUnregister)
                                                  << "' since only client monitoring disconnected";
                        for (const std::string& devId : devIdsToUnregister) {
                            remote().unregisterDeviceFromMonitoring(devId);
                        }
                    } else {
                        KARABO_LOG_FRAMEWORK_WARN << "Trying to disconnect non-existing client channel at "
                                                  << chan.get() << " (address " << getChannelAddress(chan) << ").";
                    }
                    KARABO_LOG_FRAMEWORK_INFO << m_channels.size() << " client(s) left.";

                    // Update the number of clients connected
                    set("connectedClientCount", static_cast<unsigned int>(m_channels.size()));
                }

                {
                    boost::mutex::scoped_lock lock(m_networkMutex);
                    NetworkMap::iterator iter = m_networkConnections.begin();
                    while (iter != m_networkConnections.end()) {
                        std::set<WeakChannelPointer>& channelSet = iter->second;
                        channelSet.erase(channel); // no matter whether in or not...
                        // Remove from readiness structures
                        for (auto itPair = m_readyNetworkConnections.begin();
                             itPair != m_readyNetworkConnections.end();) {
                            itPair->second.erase(channel); // itPair->second is map<WeakChannelPointer, bool>
                            if (itPair->second.empty()) {
                                // channel was the last with interest in this pipeline
                                itPair = m_readyNetworkConnections.erase(itPair);
                            } else {
                                ++itPair;
                            }
                        }
                        if (channelSet.empty()) {
                            // First use 'iter', then remove it:
                            remote().unregisterChannelMonitor(iter->first);
                            iter = m_networkConnections.erase(iter);
                        } else {
                            ++iter;
                        }
                    }
                    KARABO_LOG_FRAMEWORK_INFO << m_networkConnections.size() << " pipeline channel(s) left.";
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onError(): " << e.what();
            }
        }


        void GuiServerDevice::slotLoggerMap(const karabo::util::Hash& loggerMap) {
            boost::mutex::scoped_lock lock(m_loggerMapMutex);
            m_loggerMap = loggerMap;
        }


        void GuiServerDevice::slotDumpToLog() {
            // Empty Hash as argument ==> complete info.
            // Can be HUGE: full topology and complete cache of monitored devices...
            // Note: This will leave no trace if logging level is WARN or above.
            KARABO_LOG_FRAMEWORK_INFO << "Debug info requested by slotDumpToLog:\n"
                                      << getDebugInfo(karabo::util::Hash());
        }


        void GuiServerDevice::slotDumpDebugInfo(const karabo::util::Hash& info) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotDumpDebugInfo : info ...\n" << info;
            reply(getDebugInfo(info));
        }


        karabo::util::Hash GuiServerDevice::getDebugInfo(const karabo::util::Hash& info) {
            Hash data;

            if (info.empty() || info.has("clients")) {
                // connected clients

                // Start with the client TCP connections
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);

                    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
                        const std::string clientAddr = getChannelAddress(it->first);
                        const std::vector<std::string> monitoredDevices(it->second.visibleInstances.begin(),
                                                                        it->second.visibleInstances.end());
                        TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(it->first);

                        data.set(
                              clientAddr,
                              Hash("queueInfo", tcpChannel->queueInfo(), "monitoredDevices", monitoredDevices,
                                   // Leave string and bool vectors for the pipeline connections to be filled in below
                                   "pipelineConnections", std::vector<std::string>(), "pipelineConnectionsReadiness",
                                   std::vector<bool>(), "clientVersion", it->second.clientVersion.getString()));
                    }
                }

                // Then add pipeline information to the client connection infos
                {
                    boost::mutex::scoped_lock lock(m_networkMutex);
                    for (auto mapIter = m_networkConnections.begin(); mapIter != m_networkConnections.end();
                         ++mapIter) {
                        const std::string& channelName = mapIter->first;
                        const std::set<WeakChannelPointer>& channelSet = mapIter->second;
                        for (const WeakChannelPointer& channel : channelSet) {
                            Channel::Pointer channelPtr = channel.lock(); // promote to shared pointer
                            if (channelPtr) {
                                const std::string clientAddr = getChannelAddress(channelPtr);
                                if (data.has(clientAddr)) {
                                    std::vector<std::string>& pipelineConnections =
                                          data.get<std::vector<std::string>>(clientAddr + ".pipelineConnections");
                                    pipelineConnections.push_back(channelName);
                                    std::vector<bool>& pipelinesReady =
                                          data.get<std::vector<bool>>(clientAddr + ".pipelineConnectionsReadiness");
                                    pipelinesReady.push_back(m_readyNetworkConnections[channelName][channel]);
                                } else {
                                    // Veeery unlikely, but can happen in case a new client has connected AND subscribed
                                    // to a pipeline between creation of 'clientAddr + ".pipelineConnections"' structure
                                    // above and this call here.
                                    KARABO_LOG_FRAMEWORK_INFO << "Client '" << clientAddr << "' among network "
                                                              << "connections, but was not (yet) among channels.";
                                }
                            } // else - client might have gone meanwhile...
                        }
                    }
                }
            }
            if (info.empty() || info.has("pipelines")) {
                // The input channels created via remote().registerChannelMonitor(..):
                const SignalSlotable::InputChannels inputs(getInputChannels());
                Hash& channelsInfo = data.bindReference<Hash>("inputChannels");
                for (const auto& keyAndChannel : inputs) {
                    Hash& oneChannelInfo = channelsInfo.bindReference<Hash>(keyAndChannel.first); // key is 'local' id
                    const InputChannel::Pointer& inputChannel = keyAndChannel.second;
                    for (const std::pair<const std::string, net::ConnectionStatus>& oneConnection :
                         inputChannel->getConnectionStatus()) {
                        oneChannelInfo.set("id", inputChannel->getInstanceId()); // instanceId is unique in system
                        std::string& status = oneChannelInfo.bindReference<std::string>("status");
                        switch (oneConnection.second) {
                            case ConnectionStatus::CONNECTED:
                                status = "CONNECTED";
                                break;
                            case ConnectionStatus::DISCONNECTED:
                                status = "DISCONNECTED";
                                break;
                            case ConnectionStatus::CONNECTING:
                                status = "CONNECTING";
                                break;
                            case ConnectionStatus::DISCONNECTING:
                                status = "DICONNECTING";
                                // no default: - compiler complains about missing ConnectionStatus values
                        }
                    }
                }
            }
            if (info.empty() || info.has("devices")) {
                // monitored devices
                Hash& monitoredDevices = data.bindReference<Hash>("monitoredDeviceConfigs");
                // Create a superset of all devices seen by any of the clients
                std::set<std::string> visibleDevices; // ordered set => monitoredDevices will have ids sorted
                boost::mutex::scoped_lock lock(m_channelMutex);
                for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
                    visibleDevices.insert(it->second.visibleInstances.begin(), it->second.visibleInstances.end());
                }
                // Report configs for these devices
                for (const std::string& devId : visibleDevices) {
                    Hash config = remote().getConfigurationNoWait(devId);
                    if (config.empty()) {
                        // It's important to know if `getConfigurationNoWait` returned an empty config!
                        monitoredDevices.set(devId, Hash("configMissing", true));
                    } else {
                        monitoredDevices.set(devId, std::move(config));
                    }
                }
            }

            if (info.empty() || info.has("topology")) {
                // system topology
                data.set("systemTopology", remote().getSystemTopology());
            }

            return data;
        }


        void GuiServerDevice::monitorConnectionQueues(const boost::system::error_code& err,
                                                      const Hash& lastCheckSuspects) {
            (lastCheckSuspects.empty() ? KARABO_LOG_FRAMEWORK_DEBUG : KARABO_LOG_FRAMEWORK_INFO)
                  << "monitorConnectionQueues - last suspects: " << lastCheckSuspects;

            // Get queue infos from mutex protected list of channels
            Hash queueInfos;
            {
                boost::mutex::scoped_lock lock(m_channelMutex);
                for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
                    const std::string clientAddr = getChannelAddress(it->first);
                    TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(it->first);
                    queueInfos.set(clientAddr, tcpChannel->queueInfo());
                }
            }

            // Loop, check pending messages per client, and trigger disconnection if
            // - client is 'bad',
            // - was already 'bad' last round,
            // - and "badness" got worse.
            Hash currentSuspects;
            for (const Hash::Node& infoNode : queueInfos) {
                const std::string& clientAddr = infoNode.getKey();
                unsigned long long sumPending = 0ull;
                for (const Hash::Node& queueInfoNode : infoNode.getValue<Hash>()) {
                    sumPending += queueInfoNode.getValue<Hash>().get<unsigned long long>("pendingCount");
                }
                if (sumPending > 1000ull) {
                    if (lastCheckSuspects.has(clientAddr) // Already suspicious last time...
                        && sumPending > lastCheckSuspects.get<unsigned long long>(clientAddr)) { // ...and worse now!
                        KARABO_LOG_FRAMEWORK_ERROR << "Client '" << clientAddr << "' has " << sumPending
                                                   << " messages queued, were "
                                                   << lastCheckSuspects.get<unsigned long long>(clientAddr)
                                                   << " during last check. Trigger disconnection!";
                        // Self message (fire and forget) to disconnect (note it will be a delayed disconnect anyway).
                        // This should save us from memory problems as in redmine ticket
                        // https://in.xfel.eu/redmine/issues/107136
                        call("", "slotDisconnectClient", clientAddr);
                    } else {
                        // Add to suspects
                        KARABO_LOG_FRAMEWORK_WARN << "Client '" << clientAddr << "' has " << sumPending
                                                  << " messages queued!";
                        currentSuspects.set(clientAddr, sumPending);
                    }
                }
            }

            // Trigger next check with info from current one
            startMonitorConnectionQueues(currentSuspects);
        }


        void GuiServerDevice::slotDisconnectClient(const std::string& client) {
            WeakChannelPointer channel;
            bool found = false;
            {
                boost::mutex::scoped_lock lock(m_channelMutex);

                for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
                    const karabo::net::Channel::Pointer& chan = it->first;
                    if (client == getChannelAddress(chan)) {
                        found = true;
                        channel = chan;
                        break;
                    }
                }
            }

            if (found) {
                const karabo::xms::SignalSlotable::SlotInstancePointer& senderInfo =
                      getSenderInfo("slotDisconnectClient");
                const std::string& user = senderInfo->getUserIdOfSender();
                const std::string& senderId = senderInfo->getInstanceIdOfSender();
                std::ostringstream ostr;
                ostr << "Instance '" << senderId << "' ";
                if (!user.empty()) { // Once we send this information it might be useful to log...
                    ostr << " (user '" << user << "') ";
                }
                ostr << "enforced GUI server to disconnect.";
                KARABO_LOG_FRAMEWORK_INFO << client << ": " << ostr.str();
                safeClientWrite(channel, Hash("type", "notification", "message", ostr.str()));

                // Give client a bit of time to receive the message...
                auto timer(boost::make_shared<boost::asio::deadline_timer>(karabo::net::EventLoop::getIOService()));
                timer->expires_from_now(boost::posix_time::milliseconds(1000));
                timer->async_wait(bind_weak(&GuiServerDevice::deferredDisconnect, this,
                                            boost::asio::placeholders::error, channel, timer));
            }

            reply(found);
        }

        void GuiServerDevice::slotNotify(const karabo::util::Hash& info) {
            const std::string& message = info.get<std::string>("message");
            const std::string contentTypeStr("contentType");
            const std::string& type = info.get<std::string>(contentTypeStr);
            if (type == "banner") {
                Hash banner("type", "notification", "message", message, contentTypeStr, type);
                std::vector<std::string> bannerData;
                if (!message.empty()) {
                    bannerData.push_back(message);
                    const std::string bgColorKey("background");
                    if (!info.has(bgColorKey)) {
                        bannerData.push_back(std::string());
                    } else {
                        const std::string bgColor(info.get<std::string>(bgColorKey));
                        banner.set(bgColorKey, bgColor);
                        bannerData.push_back(bgColor);
                    }

                    const std::string fgColorKey("foreground");
                    if (!info.has(fgColorKey)) {
                        bannerData.push_back(std::string());
                    } else {
                        const std::string fgColor(info.get<std::string>(fgColorKey));
                        banner.set(fgColorKey, fgColor);
                        bannerData.push_back(fgColor);
                    }
                }
                set("bannerData", bannerData);
                safeAllClientsWrite(banner);
            } else {
                Hash announcement(info);
                announcement.set("type", "notification");
                KARABO_LOG_FRAMEWORK_INFO << "Sending custom notification message to all clients: " << announcement;
                safeAllClientsWrite(announcement);
            }

            reply(Hash()); // Hash to comply with generic slot call protocol, i.e. Hash-in, Hash-out.
        }

        void GuiServerDevice::slotBroadcast(const karabo::util::Hash& info) {
            Hash result("success", false);
            const std::string clientAddress(info.get<std::string>("clientAddress"));
            const karabo::xms::SignalSlotable::SlotInstancePointer& senderInfo = getSenderInfo("slotBroadcast");
            const std::string& user = senderInfo->getUserIdOfSender();
            const std::string& senderId = senderInfo->getInstanceIdOfSender();
            // This slot is potentially dangerous. For traceability, we log here the requestor.
            KARABO_LOG_FRAMEWORK_INFO << "Received broadcast request from : '" << senderId << "', user: " << user
                                      << ", content :" << info;
            if (clientAddress.empty()) {
                safeAllClientsWrite(info.get<Hash>("message"));
                result.set("success", true);
            } else {
                boost::mutex::scoped_lock lock(m_channelMutex);
                for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
                    const std::string channelAddress = getChannelAddress(it->first);
                    if (clientAddress == channelAddress) {
                        it->first->writeAsync(info.get<Hash>("message"), LOSSLESS);
                        result.set("success", true);
                        break;
                    }
                }
            }
            reply(result);
        }

        void GuiServerDevice::tryToUpdateNewInstanceAttributes(const std::string& deviceId, const int callerMask) {
            try {
                boost::mutex::scoped_lock lock(m_pendingAttributesMutex);
                const auto it = m_pendingAttributeUpdates.find(deviceId);

                if (it != m_pendingAttributeUpdates.end()) {
                    if (callerMask == INSTANCE_GONE_EVENT) {
                        m_pendingAttributeUpdates.erase(it);
                        return;
                    }
                    // Set the caller's bit in the event mask
                    it->second.eventMask |= callerMask;
                    if ((it->second.eventMask & FULL_MASK_EVENT) != FULL_MASK_EVENT) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Updating schema attributes of device: " << deviceId
                                                   << " still pending until all events received...";
                        return;
                    }

                    KARABO_LOG_FRAMEWORK_DEBUG << "Updating schema attributes of device: " << deviceId;
                    request(deviceId, "slotUpdateSchemaAttributes", it->second.updates)
                          .receiveAsync<Hash>(
                                bind_weak(&GuiServerDevice::onUpdateNewInstanceAttributesHandler, this, deviceId, _1));
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in sending attribute update " << e.what();
            }
        }

        void GuiServerDevice::onUpdateNewInstanceAttributesHandler(const std::string& deviceId, const Hash& response) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Handling attribute update response from " << deviceId;
                if (!response.get<bool>("success")) {
                    KARABO_LOG_ERROR << "Schema attribute update failed for device: " << deviceId;
                }

                boost::mutex::scoped_lock lock(m_pendingAttributesMutex);
                if (m_pendingAttributeUpdates.erase(deviceId) == 0) {
                    KARABO_LOG_ERROR << "Received non-requested attribute update response from: " << deviceId;
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in receiving attribute update response: " << e.what();
            }
        }


        void GuiServerDevice::slotAlarmSignalsUpdate(const std::string& alarmServiceId, const std::string& type,
                                                     const karabo::util::Hash& updateRows) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting alarm update";
                // Flushes all the instance changes that are waiting for the next throttler cycle to be dispatched.
                // This is done to guarantee that the clients will receive those instance changes before the alarm
                // updates. An alarm info, for instance, may refer to a device whose instanceNew event was being
                // held by the Throttler.
                remote().flushThrottledInstanceChanges();
                Hash h("type", type, "instanceId", alarmServiceId, "rows", updateRows);
                // Broadcast to all GUIs
                safeAllClientsWrite(h, LOSSLESS);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in broad casting alarms(): " << e.what();
            }
        }


        void GuiServerDevice::onAcknowledgeAlarm(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onAcknowledgeAlarm : info ...\n" << info;
                const std::string& alarmServiceId = info.get<std::string>("alarmInstanceId");
                call(alarmServiceId, "slotAcknowledgeAlarm", info.get<Hash>("acknowledgedRows"));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onAcknowledgeAlarm(): " << e.what();
            }
        };


        void GuiServerDevice::onRequestAlarms(WeakChannelPointer channel, const karabo::util::Hash& info,
                                              const bool replyToAllClients) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestAlarms : info ...\n" << info;
                // TODO: Add error handling for receiveAsync!
                //       (But protocol does anyway not foresee to forwared failure to GUI client as of 2.14.0.)
                const std::string& requestedInstance = info.get<std::string>("alarmInstanceId");
                request(requestedInstance, "slotRequestAlarmDump")
                      .receiveAsync<karabo::util::Hash>(
                            bind_weak(&GuiServerDevice::onRequestedAlarmsReply, this, channel, _1, replyToAllClients));

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestAlarms(): " << e.what();
            }
        };


        void GuiServerDevice::onRequestedAlarmsReply(WeakChannelPointer channel, const karabo::util::Hash& reply,
                                                     const bool replyToAllClients) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestedAlarmsReply : info ...\n" << reply;
                // Flushes all the instance changes that are waiting for the next throttler cycle to be dispatched.
                // This is done to guarantee that the clients will receive those instance changes before the alarm
                // updates. An alarm info, for instance, may refer to a device whose instanceNew event was being
                // held by the Throttler.
                remote().flushThrottledInstanceChanges();
                Hash h("type", "alarmInit", "instanceId", reply.get<std::string>("instanceId"), "rows",
                       reply.get<Hash>("alarms"));
                if (replyToAllClients) {
                    safeAllClientsWrite(h, LOSSLESS);
                } else {
                    safeClientWrite(channel, h, LOSSLESS);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestedAlarmsReply(): " << e.what();
            }
        }


        void GuiServerDevice::onUpdateAttributes(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onUpdateAttributes : info ...\n" << info;
                const std::string& instanceId = info.get<std::string>("instanceId");
                const std::vector<Hash>& updates = info.get<std::vector<Hash>>("updates");
                // TODO: Add error handling for receiveAsync!
                //       (But protocol does anyway not foresee to forwared failure to GUI client as of 2.14.0.)
                request(instanceId, "slotUpdateSchemaAttributes", updates)
                      .receiveAsync<Hash>(bind_weak(&GuiServerDevice::onRequestedAttributeUpdate, this, channel, _1));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onUpdateAttributes(): " << e.what();
            }
        }


        void GuiServerDevice::onRequestedAttributeUpdate(WeakChannelPointer channel, const karabo::util::Hash& reply) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestedAttributeUpdate : success ...\n"
                                           << reply.get<bool>("success");
                Hash h("type", "attributesUpdated", "reply", reply);
                safeClientWrite(channel, h, LOSSLESS);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestedAttributeUpdate(): " << e.what();
            }
        }


        void GuiServerDevice::connectPotentialAlarmService(const karabo::util::Hash& topologyEntry) {
            std::string type, instanceId;
            typeAndInstanceFromTopology(topologyEntry, type, instanceId);
            if (topologyEntry.get<Hash>(type).begin()->hasAttribute("classId") &&
                topologyEntry.get<Hash>(type).begin()->getAttribute<std::string>("classId") == "AlarmService") {
                // Connect to signal and then
                // actively ask this previously unknown device to submit its alarms as init messages on all channels
                asyncConnect(instanceId, "signalAlarmServiceUpdate", "", "slotAlarmSignalsUpdate",
                             bind_weak(&GuiServerDevice::onRequestAlarms, this, WeakChannelPointer(),
                                       Hash("alarmInstanceId", instanceId), true));
            }
        }


        void GuiServerDevice::registerPotentialProjectManager(const karabo::util::Hash& topologyEntry) {
            std::string type, instanceId;
            typeAndInstanceFromTopology(topologyEntry, type, instanceId);
            if (topologyEntry.get<Hash>(type).begin()->hasAttribute("classId") &&
                topologyEntry.get<Hash>(type).begin()->getAttribute<std::string>("classId") == "ProjectManager") {
                boost::unique_lock<boost::shared_mutex> lk(m_projectManagerMutex);
                asyncConnect(instanceId, "signalProjectUpdate", "", "slotProjectUpdate");
                m_projectManagers.insert(instanceId);
            }
        }


        void GuiServerDevice::slotProjectUpdate(const Hash& info, const std::string& instanceId) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "slotProjectUpdate : info ...\n" << info;
                Hash h("type", "projectUpdate", "info", info);
                safeAllClientsWrite(h);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in slotProjectUpdate: " << e.what();
            }
        }


        void GuiServerDevice::typeAndInstanceFromTopology(const karabo::util::Hash& topologyEntry, std::string& type,
                                                          std::string& instanceId) {
            if (topologyEntry.empty()) return;

            type = topologyEntry.begin()->getKey(); // fails if empty...
            // TODO let device client return also instanceId as first argument
            // const ref is fine even for temporary std::string
            instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type)
                                ? topologyEntry.get<Hash>(type).begin()->getKey()
                                : std::string("?"));
        }


        std::vector<std::string> GuiServerDevice::getKnownProjectManagers() const {
            boost::shared_lock<boost::shared_mutex> lk(m_projectManagerMutex);
            return std::vector<std::string>(m_projectManagers.begin(), m_projectManagers.end());
        }


        void GuiServerDevice::onRequestGeneric(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Generic request called with:  " << info;
                const std::string& instanceId = info.get<std::string>("instanceId");
                const std::string& slot = info.get<std::string>("slot");
                const Hash& args = info.get<Hash>("args");
                auto requestor = request(instanceId, slot, args);
                setTimeout(requestor, info, "instanceId");
                auto successHandler = bind_weak(&GuiServerDevice::forwardHashReply, this, true, channel, info, _1);
                auto failureHandler = bind_weak(&GuiServerDevice::forwardHashReply, this, false, channel, info, Hash());
                requestor.receiveAsync<Hash>(successHandler, failureHandler);
            } catch (const std::exception&) {
                // Make client aware of failure - we are in `catch` block as forwardHashReply(false, ...) expects:
                forwardHashReply(false, channel, info, Hash());
                // No need to LOG, is done in forwardHashReply
            }
        }


        void GuiServerDevice::forwardHashReply(bool success, WeakChannelPointer channel, const Hash& info,
                                               const Hash& reply) {
            const std::string replyType(info.has("replyType") ? info.get<std::string>("replyType") : "requestGeneric");
            Hash request;
            if (info.has("empty") && info.get<bool>("empty") == true) {
                if (info.has("token")) {
                    // if the request has a token return it.
                    const std::string& token = info.get<std::string>("token");
                    request.set("token", token);
                }
            } else {
                request = info;
            }

            Hash h("type", replyType, "success", success, "request", request, "reply", reply, "reason", "");

            if (!success) {
                std::string failTxt;
                std::string details;
                try {
                    throw;
                } catch (const karabo::util::TimeoutException&) {
                    failTxt = "Request not answered within ";
                    if (info.has("timeout")) {
                        // Not 100% precise if "timeout" got reconfigured after request was sent...
                        const int timeout =
                              std::max(info.get<int>("timeout"), m_timeout.load()); // load() for template resolution
                        failTxt += toString(timeout);
                    } else {
                        failTxt += toString(karabo::xms::SignalSlotable::Requestor::m_defaultAsyncTimeout / 1000.f);
                    }
                    failTxt += " seconds.";
                    karabo::util::Exception::clearTrace();
                } catch (const karabo::util::RemoteException& e) {
                    failTxt = e.userFriendlyMsg(true);
                    details = e.details();
                } catch (const karabo::util::Exception& e) {
                    failTxt = e.userFriendlyMsg(false);
                    details = e.detailedMsg();
                } catch (const std::exception& e) {
                    failTxt = e.what();
                }
                const std::string& slot =
                      (info.has("slot") ? info.get<std::string>("slot") : std::string("<missing slot definition>"));
                KARABO_LOG_FRAMEWORK_WARN << "Failure on request to " << info.get<std::string>("instanceId") << "."
                                          << slot << " via info: " << info << failTxt
                                          << (details.empty() ? std::string() : ".\nFailure details:\n" + details)
                                          << ".";
                if (!details.empty()) {
                    (failTxt += m_errorDetailsDelim) += details;
                }
                h.set("reason", std::move(failTxt));
            }
            safeClientWrite(channel, h);
        }


        void GuiServerDevice::onProjectBeginUserSession(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectBeginUserSession : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectBeginUserSession",
                                           "Project manager does not exist: Begin User Session failed."))
                    return;
                const std::string& token = info.get<std::string>("token");
                // TODO: Add failure handling for receiveAsync?
                //       (But protocol does anyway not foresee to forwared failure to GUI client as of 2.14.0.)
                request(projectManager, "slotBeginUserSession", token)
                      .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel,
                                                          "projectBeginUserSession", _1));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectBeginUserSession(): " << e.what();
            }
        }


        void GuiServerDevice::onProjectEndUserSession(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectEndUserSession : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectEndUserSession",
                                           "Project manager does not exist: End User Session failed."))
                    return;
                const std::string& token = info.get<std::string>("token");
                // TODO: Add failure handling for receiveAsync?
                //       (But protocol does anyway not foresee to forwared failure to GUI client as of 2.14.0.)
                request(projectManager, "slotEndUserSession", token)
                      .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel,
                                                          "projectEndUserSession", _1));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectEndUserSession(): " << e.what();
            }
        }


        void GuiServerDevice::onProjectSaveItems(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectSaveItems : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectSaveItems",
                                           "Project manager does not exist: Project items cannot be saved."))
                    return;
                const std::string& token = info.get<std::string>("token");
                const std::vector<Hash>& items = info.get<std::vector<Hash>>("items");
                const std::string& client = (info.has("client") ? info.get<std::string>("client") : std::string());
                // TODO: Add failure handling for receiveAsync?
                //       (But protocol does anyway not foresee to forwared failure to GUI client as of 2.14.0.)
                request(projectManager, "slotSaveItems", token, items, client)
                      .receiveAsync<Hash>(
                            util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectSaveItems", _1));

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectSaveItems(): " << e.what();
            }
        }


        void GuiServerDevice::onProjectLoadItems(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectLoadItems : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectLoadItems",
                                           "Project manager does not exist: Project items cannot be loaded."))
                    return;
                const std::string& token = info.get<std::string>("token");
                const std::vector<Hash>& items = info.get<std::vector<Hash>>("items");
                // TODO: Add failure handling for receiveAsync?
                //       (But protocol does anyway not foresee to forwared failure to GUI client as of 2.14.0.)
                request(projectManager, "slotLoadItems", token, items)
                      .receiveAsync<Hash>(
                            util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectLoadItems", _1));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectLoadItems(): " << e.what();
            }
        }


        void GuiServerDevice::onProjectListProjectManagers(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                Hash h("type", "projectListProjectManagers", "reply", getKnownProjectManagers());
                safeClientWrite(channel, h, LOSSLESS);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectListProjectManagers(): " << e.what();
            }
        }


        void GuiServerDevice::onProjectListItems(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectListItems : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectListItems",
                                           "Project manager does not exist: Project list cannot be retrieved."))
                    return;
                const std::string& token = info.get<std::string>("token");
                const std::string& domain = info.get<std::string>("domain");
                const std::vector<std::string>& item_types = info.get<std::vector<std::string>>("item_types");
                // TODO: Add failure handling for receiveAsync?
                //       (But protocol does anyway not foresee to forwared failure to GUI client as of 2.14.0.)
                request(projectManager, "slotListItems", token, domain, item_types)
                      .receiveAsync<Hash>(
                            util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectListItems", _1));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectListItems(): " << e.what();
            }
        }


        void GuiServerDevice::onProjectListDomains(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectListDomains : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectListDomains",
                                           "Project manager does not exist: Domain list cannot be retrieved."))
                    return;
                const std::string& token = info.get<std::string>("token");
                // TODO: Add failure handling for receiveAsync?
                //       (But protocol does anyway not foresee to forwared failure to GUI client as of 2.14.0.)
                request(projectManager, "slotListDomains", token)
                      .receiveAsync<Hash>(
                            util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectListDomains", _1));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectListDomains(): " << e.what();
            }
        }


        void GuiServerDevice::onProjectUpdateAttribute(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectUpdateAttribute : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectUpdateAttribute",
                                           "Project manager does not exist: Cannot update project attribute (trash)."))
                    return;
                const std::string& token = info.get<std::string>("token");
                const std::vector<Hash>& items = info.get<std::vector<Hash>>("items");
                // TODO: Add failure handling for receiveAsync?
                //       (But protocol does anyway not foresee to forwared failure to GUI client as of 2.14.0.)
                request(projectManager, "slotUpdateAttribute", token, items)
                      .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel,
                                                          "projectUpdateAttribute", _1));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectUpdateAttribute(): " << e.what();
            }
        }


        void GuiServerDevice::forwardReply(WeakChannelPointer channel, const std::string& replyType,
                                           /*const karabo::net::ErrorCode& e,*/ const karabo::util::Hash& reply) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "forwardReply : " << replyType;
                Hash h("type", replyType, "reply", reply);
                safeClientWrite(channel, h, LOSSLESS);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in forwarding reply of type '" << replyType << "': " << e.what();
            }
        }

        bool GuiServerDevice::checkProjectManagerId(WeakChannelPointer channel, const std::string& deviceId,
                                                    const std::string& type, const std::string& reason) {
            boost::shared_lock<boost::shared_mutex> lk(m_projectManagerMutex);
            if (m_projectManagers.find(deviceId) != m_projectManagers.end()) return true;
            Hash h("type", type, "reply", Hash("success", false, "reason", reason));
            safeClientWrite(channel, h, LOSSLESS);
            return false;
        }

        std::string GuiServerDevice::getChannelAddress(const karabo::net::Channel::Pointer& channel) const {
            TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(channel);
            std::string addr = tcpChannel->remoteAddress();

            // convert periods to underscores, so that this can be used as a Hash key...
            std::transform(addr.begin(), addr.end(), addr.begin(), [](char c) { return c == '.' ? '_' : c; });

            return addr;
        }

    } // namespace devices
} // namespace karabo
