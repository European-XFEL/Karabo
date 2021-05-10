/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "GuiServerDevice.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/State.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/TcpChannel.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/VectorElement.hh"
#include "karabo/util/Version.hh"

#include "karabo/core/InstanceChangeThrottler.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::io;
using namespace karabo::xms;


KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, karabo::devices::GuiServerDevice)

namespace karabo {
    namespace devices {

        // requestFromSlot is a fine grained writeable command and will be handled differently!
        const std::unordered_set <std::string> GuiServerDevice::m_writeCommands ({
            "projectSaveItems", "initDevice", "killDevice", "execute", "killServer",
            "acknowledgeAlarm", "projectUpdateAttribute", "reconfigure", "updateAttributes"}
            );


        // configure here restrictions to the command type against client versions
        const std::unordered_map <std::string, Version> GuiServerDevice::m_minVersionRestrictions {
            {"projectSaveItems", Version("2.10.0")},
            {"projectUpdateAttribute", Version("2.10.0")},
        };

        void GuiServerDevice::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::ON, State::ERROR)
                    .setNewDefaultValue(State::INIT)
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("port")
                    .displayedName("Hostport")
                    .description("Local port for this server")
                    .assignmentOptional().defaultValue(44444)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_GuiServer_0")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue<int>(Schema::AccessLevel::ADMIN)
                    .commit();

            // Slow beats on GuiServer
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();

            // Monitor performance of this system relevant device
            OVERWRITE_ELEMENT(expected).key("performanceStatistics.enable")
                    .setNewDefaultValue(true)
                    .commit();

            INT32_ELEMENT(expected).key("delayOnInput")
                    .displayedName("Delay on Input channel")
                    .description("Extra Delay on the InputChannel in this device to inform the output channel "
                    "about its readiness to receive new data. Lowering this delay adds load to the output channel the GUI server connects to.")
                    .assignmentOptional().defaultValue(500)
                    .reconfigurable()
                    .minInc(200) // Max 5 Hz
                    .unit(Unit::SECOND)
                    .metricPrefix(MetricPrefix::MILLI)
                    .commit();

            INT32_ELEMENT(expected).key("lossyDataQueueCapacity")
                    .displayedName("Lossy Data forwarding queue size")
                    .description("The number of lossy data messages to store in the forwarding ring buffer. NOTE: Will be applied to newly connected clients only")
                    .assignmentOptional().defaultValue(100)
                    .reconfigurable()
                    .minExc(0).maxInc(1000)
                    .commit();

            INT32_ELEMENT(expected).key("propertyUpdateInterval")
                    .displayedName("Property update interval")
                    .description("Minimum interval between subsequent property updates forwarded to clients.")
                    .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                    .assignmentOptional().defaultValue(500)
                    .reconfigurable()
                    .minInc(0).maxInc(10000) // 0.1 Hz minimum
                    .commit();

            INT32_ELEMENT(expected).key("waitInitDevice")
                    .displayedName("Instantiate wait time")
                    .description("Time interval between the instantiation of devices.")
                    .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                    .assignmentOptional().defaultValue(100)
                    .reconfigurable()
                    .minInc(100).maxInc(5000) // NOTE: Not _too_ fast. The device instantiation timer is always running!
                    .commit();

            INT32_ELEMENT(expected).key("forwardLogInterval")
                    .displayedName("Log Interval")
                    .description("Time interval between the forwarding of logs.")
                    .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                    .assignmentOptional().defaultValue(1000)
                    .reconfigurable()
                    .minInc(500).maxInc(5000)
                    .commit();

            UINT32_ELEMENT(expected).key("connectedClientCount")
                    .displayedName("Connected clients count")
                    .description("The number of clients currently connected to the server.")
                    .readOnly().initialValue(0)
                    .commit();

            STRING_ELEMENT(expected).key("logForwardingLevel")
                    .displayedName("Log Forwarding Level")
                    .description("The lowest log message level which will be forwarded to GUI clients.")
                    .options("ERROR,WARN,INFO,DEBUG")
                    .assignmentOptional().defaultValue("INFO")
                    .reconfigurable()
                    .commit();

            NODE_ELEMENT(expected).key("networkPerformance")
                    .displayedName("Network performance monitoring")
                    .description("Contains information about how much data is being read/written from/to the network")
                    .commit();

            INT32_ELEMENT(expected).key("networkPerformance.sampleInterval")
                    .displayedName("Sample interval")
                    .description("Minimum interval between subsequent network performance recordings.")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(5)
                    .reconfigurable()
                    .minInc(1).maxInc(3600)  // Once per second to once per hour
                    .commit();

            UINT64_ELEMENT(expected).key("networkPerformance.clientBytesRead")
                    .displayedName("Bytes read from clients")
                    .description("The number of bytes read from the network in the last `sampleInterval` seconds")
                    .readOnly().initialValue(0)
                    .commit();

            UINT64_ELEMENT(expected).key("networkPerformance.clientBytesWritten")
                    .displayedName("Bytes written to clients")
                    .description("The number of bytes written to the network in the last `sampleInterval` seconds")
                    .readOnly().initialValue(0)
                    .commit();

            UINT64_ELEMENT(expected).key("networkPerformance.pipelineBytesRead")
                    .displayedName("Bytes read from pipeline connections")
                    .description("The number of bytes read from the network in the last `sampleInterval` seconds")
                    .readOnly().initialValue(0)
                    .commit();

            UINT64_ELEMENT(expected).key("networkPerformance.pipelineBytesWritten")
                    .displayedName("Bytes written to pipeline connections")
                    .description("The number of bytes written to the network in the last `sampleInterval` seconds")
                    .readOnly().initialValue(0)
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
            STRING_ELEMENT(expected).key("minClientVersion")
                    .displayedName("Minimum Client Version")
                    .description("If this variable does not respect the N.N.N(.N) convention,"
                                 " the Server will not enforce a version check")
                    .assignmentOptional().defaultValue("2.10.4")
                    .reconfigurable()
                    .adminAccess()
                    .commit();

            BOOL_ELEMENT(expected).key("isReadOnly")
                    .displayedName("isReadOnly")
                    .description("Define if this GUI Server is in readOnly "
                                 "mode for clients")
                    .assignmentOptional().defaultValue(false)
                    .init()
                    .adminAccess()
                    .commit();

            STRING_ELEMENT(expected).key("dataLogManagerId")
                    .displayedName("Data Log Manager Id")
                    .description("The DataLoggerManager device to query for log readers.")
                    .assignmentOptional().defaultValue(karabo::util::DATALOGMANAGER_ID)
                    .reconfigurable()
                    .adminAccess()
                    .commit();

            VECTOR_STRING_ELEMENT(expected).key("ignoreTimeoutClasses")
                    .displayedName("Ignore Timeout ClassIds")
                    .description("ClassIds that are treated like macros: The GUI server will ignore "\
                                 "timeouts of slots of devices of these classes.")
                    .assignmentOptional().defaultValue(std::vector<std::string>())
                    .reconfigurable()
                    .adminAccess()
                    .commit();

            INT32_ELEMENT(expected).key("timeout")
                    .displayedName("Request Timeout")
                    .description("If client requests to 'reconfigure', 'execute' or 'requestGeneric' have a 'timeout' "
                                 "specified, take in fact the maximum of that value and this one.")
                    .assignmentOptional().defaultValue(10) // in 2.10.0, client has 5
                    .reconfigurable()
                    .adminAccess()
                    .commit();

            SLOT_ELEMENT(expected).key("slotDumpToLog")
                    .displayedName("Dump Debug to Log")
                    .description("Dumps info about connections to log file (care - can be huge)")
                    .expertAccess()
                    .commit();
        }


        GuiServerDevice::GuiServerDevice(const Hash& config)
        : Device<>(config)
        , m_deviceInitTimer(EventLoop::getIOService())
        , m_networkStatsTimer(EventLoop::getIOService())
        , m_forwardLogsTimer(EventLoop::getIOService())
        , m_timeout(config.get<int>("timeout")) {

            KARABO_INITIAL_FUNCTION(initialize)

            KARABO_SLOT(slotLoggerMap, Hash /*loggerMap*/)
            KARABO_SLOT(slotAlarmSignalsUpdate, std::string, std::string, karabo::util::Hash);
            KARABO_SLOT(slotProjectUpdate, karabo::util::Hash, std::string);
            KARABO_SLOT(slotDumpToLog);
            KARABO_SLOT(slotDumpDebugInfo, karabo::util::Hash);
            KARABO_SLOT(slotDisconnectClient, std::string);

            Hash h;
            h.set("port", config.get<unsigned int>("port"));
            h.set("type", "server");
            h.set("serializationType", "binary"); // Will lead to binary header hashes
            m_dataConnection = Connection::create("Tcp", h);
            m_serializer = BinarySerializer<Hash>::create("Bin"); // for reading

            m_isReadOnly = config.get<bool>("isReadOnly");
            m_loggerInput = config;
            m_loggerMinForwardingPriority = krb_log4cpp::Priority::getPriorityValue(config.get<std::string>("logForwardingLevel"));
        }


        GuiServerDevice::~GuiServerDevice() {
            if (m_dataConnection) m_dataConnection->stop();
        }


        void GuiServerDevice::initialize() {

            try {

                // Switch on instance tracking
                remote().enableInstanceTracking();

                // Protect clients from too frequent updates of a single property:
                remote().setDeviceMonitorInterval(get<int>("propertyUpdateInterval"));

                // Register handlers
                // NOTE: boost::bind() is OK for these handlers because SignalSlotable calls them directly instead
                // of dispatching them via the event loop.
                remote().registerInstanceNewMonitor(boost::bind(&karabo::devices::GuiServerDevice::instanceNewHandler, this, _1));
                remote().registerInstanceGoneMonitor(boost::bind(&karabo::devices::GuiServerDevice::instanceGoneHandler, this, _1, _2));
                remote().registerSchemaUpdatedMonitor(boost::bind(&karabo::devices::GuiServerDevice::schemaUpdatedHandler, this, _1, _2));
                remote().registerClassSchemaMonitor(boost::bind(&karabo::devices::GuiServerDevice::classSchemaHandler, this, _1, _2, _3));

                remote().registerInstanceChangeMonitor(bind_weak(&karabo::devices::GuiServerDevice::instanceChangeHandler, this, _1));

                remote().registerDevicesMonitor(bind_weak(&karabo::devices::GuiServerDevice::devicesChangedHandler, this, _1));

                // If someone manages to bind_weak(&karabo::devices::GuiServerDevice::requestNoWait<>, this, ...),
                // we would not need loggerMapConnectedHandler...
                asyncConnect(get<std::string>("dataLogManagerId"), "signalLoggerMap", "", "slotLoggerMap",
                             bind_weak(&karabo::devices::GuiServerDevice::loggerMapConnectedHandler, this));

                // scan topology to treat alarm services, project managers, ...
                // NOTE: If instanceNewHandler would be registered before enableInstanceTracking(),
                //       this code would probably not be needed here, but just in instanceNewHandler!
                const Hash& topology = remote().getSystemTopology();
                const boost::optional<const Hash::Node&> devices = topology.find("device");
                if (devices) {
                    const Hash& deviceEntry = devices->getValue<Hash>();
                    for (Hash::const_iterator it = deviceEntry.begin(); it != deviceEntry.end(); ++it) {
                        const std::string& deviceId = it->getKey();
                        Hash topologyEntry;
                        Hash::Node& hn = topologyEntry.set("device." + deviceId, it->getValue<Hash>());
                        hn.setAttributes(it->getAttributes());
                        connectPotentialAlarmService(topologyEntry);
                        registerPotentialProjectManager(topologyEntry);
                    }
                }

                m_dataConnection->startAsync(bind_weak(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));

                m_loggerConsumer = getConnection();
                m_loggerConsumer->startReadingLogs(bind_weak(&karabo::devices::GuiServerDevice::logHandler, this, _1, _2),
                                                   consumer::ErrorNotifier());

                m_guiDebugProducer = getConnection();

                startDeviceInstantiation();
                startNetworkMonitor();
                startForwardingLogs();
                // TODO: remove this once "fast slot reply policy" is enforced
                const std::vector<std::string>& timingOutClasses = get<std::vector<std::string>>("ignoreTimeoutClasses");
                recalculateTimingOutDevices(remote().getSystemTopology(), timingOutClasses, false);

                updateState(State::ON);

                // Produce some information
                KARABO_LOG_INFO << "GUI Server is up and listening on port: " << get<unsigned int>("port");

            } catch (const Exception& e) {
                updateState(State::ERROR);

                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initialize(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
            if (incomingReconfiguration.has("ignoreTimeoutClasses")) {
                const std::vector<std::string>& timingOutClasses = incomingReconfiguration.get<std::vector<std::string>>("ignoreTimeoutClasses");
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


        void GuiServerDevice::recalculateTimingOutDevices(const karabo::util::Hash& topologyEntry, const std::vector <std::string>& timingOutClasses, bool clearSet) {
            boost::mutex::scoped_lock lock(m_timingOutDevicesMutex);
            if (clearSet) m_timingOutDevices.clear();
            if (topologyEntry.has("device")) {
                const karabo::util::Hash& devices = topologyEntry.get<Hash>("device");
                for (karabo::util::Hash::const_iterator it = devices.begin(); it != devices.end(); ++it) {
                    if (std::find(timingOutClasses.begin(), timingOutClasses.end(), it->getAttribute<std::string>("classId")) != timingOutClasses.end()) {
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
            m_loggerMinForwardingPriority = krb_log4cpp::Priority::getPriorityValue(get<std::string>("logForwardingLevel"));

            // One might also want to react on possible changes of "delayOnInput",
            // i.e. change delay value for existing input channels.
            // For now, changing "delayOnInput" will only affect new InputChannels, i.e. _all_ GUI clients requesting
            // data of a specific output channel have to dis- and then reconnect to see the new delay.
        }

        void GuiServerDevice::startDeviceInstantiation() {
            // NOTE: This timer is a rate limiter for device instantiations
            m_deviceInitTimer.expires_from_now(boost::posix_time::milliseconds(get<int>("waitInitDevice")));
            m_deviceInitTimer.async_wait(bind_weak(&karabo::devices::GuiServerDevice::initSingleDevice, this, boost::asio::placeholders::error));
        }

        void GuiServerDevice::startNetworkMonitor() {
            m_networkStatsTimer.expires_from_now(boost::posix_time::seconds(get<int>("networkPerformance.sampleInterval")));
            m_networkStatsTimer.async_wait(bind_weak(&karabo::devices::GuiServerDevice::collectNetworkStats, this, boost::asio::placeholders::error));
        }


        void GuiServerDevice::startForwardingLogs() {
            m_forwardLogsTimer.expires_from_now(boost::posix_time::milliseconds(get<int>("forwardLogInterval")));
            m_forwardLogsTimer.async_wait(bind_weak(&karabo::devices::GuiServerDevice::forwardLogs, this, boost::asio::placeholders::error));
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


        void GuiServerDevice::forwardLogs(const boost::system::error_code& error) {
            if (error) {
                KARABO_LOG_FRAMEWORK_ERROR << "Log forwarding was cancelled!";
                return;
            }
            boost::mutex::scoped_lock lock(m_forwardLogsMutex);
            if (m_logCache.size() > 0) {

                Hash h("type", "log");
                std::vector<Hash>& messages = h.bindReference<std::vector < Hash >> ("messages");
                messages = std::move(m_logCache); // use r-value assignment operator to avoid a copy
                m_logCache.clear(); // because std::move left it in a valid, but undefined state
                safeAllClientsWrite(h, REMOVE_OLDEST);
            }
            startForwardingLogs();
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

                channel->readAsyncHash(bind_weak(&karabo::devices::GuiServerDevice::onLoginMessage, this, _1, channel, _2));

                string const version = karabo::util::Version::getVersion();
                Hash systemInfo("type", "brokerInformation");
                systemInfo.set("topic", m_topic);
                systemInfo.set("hostname", get<std::string>("hostName"));
                systemInfo.set("hostport", get<unsigned int>("port"));
                systemInfo.set("deviceId", getInstanceId());
                systemInfo.set("readOnly", m_isReadOnly);
                systemInfo.set("version", version);

                channel->writeAsync(systemInfo);

                // Re-register acceptor socket (allows handling multiple clients)
                m_dataConnection->startAsync(bind_weak(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));


            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onConnect(): " << e.userFriendlyMsg();
                m_dataConnection->startAsync(bind_weak(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));
            }
        }

        void GuiServerDevice::registerConnect(const karabo::util::Version& version, const karabo::net::Channel::Pointer & channel) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = ChannelData(version); // keeps channel information
            // Update the number of clients connected
            set("connectedClientCount", static_cast<unsigned int>(m_channels.size()));
        }


        void GuiServerDevice::onLoginMessage(const karabo::net::ErrorCode& e, const karabo::net::Channel::Pointer & channel, karabo::util::Hash& info) {
            if (e) {
                channel->close();
                return;
            }
            try {
                if (!info.has("type")) {
                    KARABO_LOG_FRAMEWORK_WARN << "Ignoring request that lacks type specification: " << info;
                    return;
                }
                const string& type = info.get<string > ("type");
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

        void GuiServerDevice::onLogin(const karabo::net::Channel::Pointer & channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onLogin";
                // Check valid login
                const Version clientVersion(hash.get<string>("version"));
                auto weakChannel = WeakChannelPointer(channel);
                if (clientVersion >= Version(get<std::string>("minClientVersion"))) {
                    KARABO_LOG_INFO << "Login request of user: " << hash.get<string > ("username")
                            << " (version " << clientVersion.getString() << ")";
                    registerConnect(clientVersion, channel);
                    // TODO: Add user authentication and subscribe `onRead` on success
                    channel->readAsyncHash(bind_weak(&karabo::devices::GuiServerDevice::onRead, this, _1, weakChannel, _2));
                    sendSystemTopology(weakChannel);
                    return;
                }
                const std::string message("Your GUI client has version '" + hash.get<string>("version")
                                            + "', but the minimum required is: "
                                            + get<std::string>("minClientVersion"));
                const Hash h("type", "notification", "message", message);
                safeClientWrite(weakChannel, h);
                KARABO_LOG_FRAMEWORK_WARN << "Refused login request of user '" << hash.get<string > ("username")
                        << "' using GUI client version " << clientVersion.getString()
                        << " (from " << getChannelAddress(channel) << ")";
                auto timer(boost::make_shared<boost::asio::deadline_timer>(karabo::net::EventLoop::getIOService()));
                timer->expires_from_now(boost::posix_time::milliseconds(500));
                timer->async_wait(bind_weak(&GuiServerDevice::deferredDisconnect, this,
                                            boost::asio::placeholders::error, weakChannel, timer));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onLogin(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onRead(const karabo::net::ErrorCode& e, WeakChannelPointer channel, karabo::util::Hash& info) {
            if (e) {
                EventLoop::getIOService().post(bind_weak(&GuiServerDevice::onError, this, e, channel));
                return;
            }

            try {
                // GUI communication scenarios
                if (info.has("type")) {
                    const string& type = info.get<string > ("type");
                    if (m_isReadOnly && violatesReadOnly(type, info)) {
                        // not allowed, bail out and inform client
                        const std::string message("Action '" + type + "' is not allowed on GUI servers in readOnly mode!");
                        const Hash h("type", "notification", "message", message);
                        safeClientWrite(channel, h);
                    } else if (violatesClientConfiguration(type, channel)) {
                        // not allowed, bail out and inform client
                        const std::string message("Action '" + type + "' is not allowed on this GUI client version. Please upgrade your GUI client");
                        const Hash h("type", "notification", "message", message);
                        safeClientWrite(channel, h);
                    } else if (type == "requestFromSlot") {
                        onRequestFromSlot(channel, info);
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


        bool GuiServerDevice::violatesReadOnly(const std::string& type, const karabo::util::Hash& info) {
            KARABO_LOG_FRAMEWORK_DEBUG << "violatesReadOnly " << info;
            if (m_writeCommands.find(type) != m_writeCommands.end()) {
                return true;
            } else if (type == "requestFromSlot" && info.has("slot") && info.get<string>("slot") != "requestScene" && info.get<string>("slot") != "slotGetScene") {
                // requestFromSlot must have a 'slot' argument. If not, it should fail somewhere else.
                return true;
            } else if (type == "requestGeneric" && info.has("slot") && info.get<string>("slot") != "requestScene" && info.get<string>("slot") != "slotGetScene") {
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
                    if (itChannelData != m_channels.end()){
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

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGuiError(): " << e.userFriendlyMsg();
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
                    const int timeoutSec = std::max(input.get<int>("timeout"), m_timeout.load()); // load() for template resolution
                    requestor.timeout(timeoutSec * 1000); // convert to ms
                }
            }
        }


        void GuiServerDevice::onReconfigure(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onReconfigure";
                const string& deviceId = hash.get<string > ("deviceId");
                const Hash& config = hash.get<Hash > ("configuration");
                // TODO Supply user specific context
                if (hash.has("reply") && hash.get<bool>("reply")) {
                    auto requestor = request(deviceId, "slotReconfigure", config);
                    setTimeout(requestor, hash, "deviceId");
                    auto successHandler = bind_weak(&GuiServerDevice::forwardReconfigureReply, this, true, channel, hash);
                    auto failureHandler = bind_weak(&GuiServerDevice::forwardReconfigureReply, this, false, channel, hash);
                    requestor.receiveAsync(successHandler, failureHandler);
                } else {
                    call(deviceId, "slotReconfigure", config);
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onReconfigure(): " << e.what();
            }
        }


        void GuiServerDevice::forwardReconfigureReply(bool success, WeakChannelPointer channel, const Hash& input) {
            Hash h("type", "reconfigureReply",
                   "success", success,
                   "input", input);
            if (!success) {
                // Failure, so can get access to exception causing it:
                std::set<std::string> paths;
                input.get<Hash>("configuration").getPaths(paths);
                std::string& failTxt = h.set("failureReason", "Failure on request to reconfigure '" + toString(paths) += "' of "
                                             "device '" + input.get<std::string>("deviceId") + "'")
                        .getValue<std::string>();
                try {
                    throw;
                } catch (const karabo::util::TimeoutException& te) {
                    // TODO: currently ignoring also naughty classes. Remove this once this is enforced.
                    const bool ignoreTimeout = !input.has("timeout") || skipExecutionTimeout(input.get<std::string>("deviceId"));
                    // if the input hash has no timeout key or comes from a "naughty" class, declare success
                    if (ignoreTimeout) {
                        h.set("success", true);
                    }
                    failTxt += ". Request not answered within ";
                    if (ignoreTimeout) {
                        // default timeout is in ms. Convert to minutes
                        (failTxt += toString(karabo::xms::SignalSlotable::Requestor::m_defaultAsyncTimeout/60000.f)) += " minutes.";
                    } else {
                        // Not 100% precise if "timeout" got reconfigured after request was sent...
                        const int timeout = std::max(input.get<int>("timeout"), m_timeout.load());
                        (failTxt += toString(timeout)) += " seconds.";
                    }
                    karabo::util::Exception::clearTrace();
                    KARABO_LOG_FRAMEWORK_WARN << failTxt;
                } catch (const std::exception& e) {
                    (failTxt += ", details:\n") += e.what();
                    KARABO_LOG_FRAMEWORK_WARN << failTxt;
                }
            }
            safeClientWrite(channel, h);
        }

        void GuiServerDevice::onExecute(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onExecute " << hash;
                const string& deviceId = hash.get<string > ("deviceId");
                const string& command = hash.get<string > ("command");
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
            Hash h("type", "executeReply",
                   "success", success,
                   "input", input);
            if (!success) {
                // Failure, so can get access to exception causing it:
                std::string& failTxt = h.set("failureReason", "Failure on request to execute '" + input.get<std::string>("command")
                                             + "' on device '" + input.get<std::string>("deviceId") + "'")
                        .getValue<std::string>();
                try {
                    throw;
                } catch (const karabo::util::TimeoutException& te) {
                    // TODO: currently ignoring also naughty classes. Remove this once this is enforced.
                    const bool ignoreTimeout = !input.has("timeout") || skipExecutionTimeout(input.get<std::string>("deviceId"));
                    // if the input hash has no timeout key or comes from a "naughty" class, declare success
                    if (ignoreTimeout) {
                        h.set("success", true);
                    }
                    failTxt += ". Request not answered within ";
                    if (ignoreTimeout) {
                        // default timeout is in ms. Convert to minutes
                        (failTxt += toString(karabo::xms::SignalSlotable::Requestor::m_defaultAsyncTimeout/60000.f)) += " minutes.";
                    } else {
                        // Not 100% precise if "timeout" got reconfigured after request was sent...
                        const int timeout = std::max(input.get<int>("timeout"), m_timeout.load());
                        (failTxt += toString(timeout)) += " seconds.";
                    }
                    karabo::util::Exception::clearTrace();
                    KARABO_LOG_FRAMEWORK_WARN << failTxt;
                } catch (const std::exception& e) {
                    (failTxt += ", details:\n") += e.what();
                    KARABO_LOG_FRAMEWORK_WARN << failTxt;
                }
            }
            safeClientWrite(channel, h);
        }


        void GuiServerDevice::onInitDevice(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            try {

                const string& serverId = hash.get<string>("serverId");
                const string& deviceId = hash.get<string>("deviceId");
                KARABO_LOG_FRAMEWORK_DEBUG << "onInitDevice: Queuing request to start device instance \""
                                           << deviceId << "\" on server \"" << serverId << "\"";

                if (!deviceId.empty() && hash.has("schemaUpdates")) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Schema updates were provided for device " << deviceId;

                    AttributeUpdates attrUpdates;
                    attrUpdates.eventMask = 0;
                    attrUpdates.updates = hash.get<std::vector<Hash> >("schemaUpdates");

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
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onInitDevice(): " << e.userFriendlyMsg();
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

                    KARABO_LOG_FRAMEWORK_DEBUG << "initSingleDevice: Requesting to start device instance \""
                                               << deviceId << "\" on server \"" << serverId << "\"";
                    // initReply both as success and failure handler, identified by boolean flag as last argument
                    request(serverId, "slotStartDevice", inst.hash)
                        .receiveAsync<bool, string>(bind_weak(&karabo::devices::GuiServerDevice::initReply,
                                                              this, inst.channel, deviceId, inst.hash, _1, _2, false),
                                                    bind_weak(&karabo::devices::GuiServerDevice::initReply,
                                                              this, inst.channel, deviceId, inst.hash, false, "", true));

                    m_pendingDeviceInstantiations.pop();
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initSingleDevice(): " << e.userFriendlyMsg();
            }

            // Always restart the timer!
            startDeviceInstantiation();
        }


        void GuiServerDevice::initReply(WeakChannelPointer channel, const string& givenDeviceId, const karabo::util::Hash& givenConfig,
                                        bool success, const string& message,
                                        bool isFailureHandler) {
            try {

                KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting init reply - "
                        << (isFailureHandler ? "" : "not ") << "as failureHandler";

                Hash h("type", "initReply", "deviceId", givenDeviceId, "success", success, "message", message);
                if (isFailureHandler) {
                    // Called as a failure handler, so can re-throw
                    try {
                        throw;
                    } catch (const std::exception& e) {
                        // Set or extend failure message
                        std::string& msg = h.get<std::string>("message");
                        if (!msg.empty()) msg += ": "; // as failure handler, msg should be empty, but adding does not harm
                        msg += e.what();
                    }
                }
                safeClientWrite(channel, h);

                const NewInstanceAttributeUpdateEvents event = (isFailureHandler || !success ?
                                                                INSTANCE_GONE_EVENT :
                                                                DEVICE_SERVER_REPLY_EVENT);
                tryToUpdateNewInstanceAttributes(givenDeviceId, event);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initReply " << e.what();
            }
        }


        void GuiServerDevice::safeClientWrite(const WeakChannelPointer channel, const karabo::util::Hash& message, int prio) {
            boost::mutex::scoped_lock lock(m_channelMutex);
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
                const string& deviceId = hash.get<string > ("deviceId");

                Hash config = remote().getConfigurationNoWait(deviceId);

                if (!config.empty()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceConfiguration for '" << deviceId << "': direct answer";
                    // Can't we just use 'config' instead of 'remote().get(deviceId)'?
                    Hash h("type", "deviceConfigurations", "configurations", Hash(deviceId, remote().get(deviceId)));
                    safeClientWrite(channel, h);
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceConfiguration for '" << deviceId << "': expect later answer";
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetDeviceConfiguration(): " << e.what();
            }
        }


        void GuiServerDevice::onKillServer(const karabo::util::Hash& info) {
            try {
                string serverId = info.get<string > ("serverId");
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillServer : \"" << serverId << "\"";
                // TODO Supply user specific context
                call(serverId, "slotKillServer");
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onKillServer(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onKillDevice(const karabo::util::Hash& info) {
            try {
                string deviceId = info.get<string > ("deviceId");
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillDevice : \"" << deviceId << "\"";
                call(deviceId, "slotKillDevice");
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onKillDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onStartMonitoringDevice(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                bool registerFlag = false;
                const string& deviceId = info.get<string > ("deviceId");

                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    karabo::net::Channel::Pointer chan = channel.lock();
                    ChannelIterator it = m_channels.find(chan);
                    if (it != m_channels.end()) {
                        it->second.visibleInstances.insert(deviceId);
                    }
                }

                {
                    boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                    // Increase count of device in visible devices map
                    const int newCount = ++m_monitoredDevices[deviceId];
                    if (newCount == 1) registerFlag = true;
                    KARABO_LOG_FRAMEWORK_DEBUG << "onStartMonitoringDevice " << deviceId << " (" << newCount << ")";
                }

                if (registerFlag) { // Fresh device on the shelf
                    remote().registerDeviceForMonitoring(deviceId);
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
                bool unregisterFlag = false;
                const string& deviceId = info.get<string > ("deviceId");

                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    karabo::net::Channel::Pointer chan = channel.lock();
                    ChannelIterator it = m_channels.find(chan);
                    if (it != m_channels.end()) it->second.visibleInstances.erase(deviceId);
                }

                {
                    boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                    const int newCount = --m_monitoredDevices[deviceId]; // prefix decrement!
                    if (newCount <= 0){
                        // Erase instance from the monitored list
                        unregisterFlag = true;
                        m_monitoredDevices.erase(deviceId);
                    }
                    KARABO_LOG_FRAMEWORK_DEBUG << "onStopMonitoringDevice " << deviceId << " (" << newCount << ")";
                }

                if (unregisterFlag) {
                    // Disconnect signal/slot from broker
                    remote().unregisterDeviceFromMonitoring(deviceId);
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onStopMonitoringDevice(): " << e.what();
            }
        }


        void GuiServerDevice::onGetClassSchema(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                string serverId = info.get<string > ("serverId");
                string classId = info.get<string> ("classId");
                Schema schema = remote().getClassSchemaNoWait(serverId, classId);
                if (!schema.empty()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Schema available, direct answer";
                    Hash h("type", "classSchema", "serverId", serverId,
                           "classId", classId, "schema", schema);
                    safeClientWrite(channel, h);
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetClassSchema : serverId=\"" << serverId << "\", classId=\"" << classId << "\": direct answer";
                } else {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    karabo::net::Channel::Pointer chan = channel.lock();
                    if (chan) {
                        ChannelIterator itChannelData = m_channels.find(chan);
                        if (itChannelData != m_channels.end()){
                            itChannelData->second.requestedClassSchemas[serverId].insert(classId);
                        }
                    }
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetClassSchema : serverId=\"" << serverId << "\", classId=\"" << classId << "\": expect later answer";
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetClassSchema(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onGetDeviceSchema(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                const string& deviceId = info.get<string > ("deviceId");

                Schema schema = remote().getDeviceSchemaNoWait(deviceId);

                if (!schema.empty()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceSchema for '" << deviceId << "': direct answer";
                    Hash h("type", "deviceSchema", "deviceId", deviceId, "schema", schema);
                    safeClientWrite(channel, h);
                } else {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    karabo::net::Channel::Pointer chan = channel.lock();
                    if (chan) {
                        ChannelIterator itChannelData = m_channels.find(chan);
                        if (itChannelData != m_channels.end()){
                            itChannelData->second.requestedDeviceSchemas.insert(deviceId);
                        }
                    }
                    KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceSchema for '" << deviceId << "': expect later answer";
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
                const string& deviceId = info.get<string > ("deviceId");
                const string& property = info.get<string > ("property");
                const string& t0 = info.get<string > ("t0");
                const string& t1 = info.get<string > ("t1");
                int maxNumData = 0;
                if (info.has("maxNumData")) maxNumData = info.getAs<int>("maxNumData");
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetPropertyHistory: " << deviceId << "." << property << ", "
                        << t0 << " - " << t1 << " (" << maxNumData << " points)";

                Hash args("from", t0, "to", t1, "maxNumData", maxNumData);

                const std::string& readerId(getDataReaderId(deviceId));
                auto okHandler = bind_weak(&karabo::devices::GuiServerDevice::propertyHistory, this, channel, true, _1, _2, _3);
                auto failHandler = bind_weak(&karabo::devices::GuiServerDevice::propertyHistory, this, channel, false,
                                             deviceId, property, vector<Hash>());
                request(readerId, "slotGetPropertyHistory", deviceId, property, args)
                        .receiveAsync<string, string, vector<Hash> >(okHandler, failHandler);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetPropertyHistory(): " << e.what();
            }
        }


        void GuiServerDevice::propertyHistory(WeakChannelPointer channel, bool success, const std::string& deviceId,
                                              const std::string& property, const std::vector<karabo::util::Hash>& data) {
            try {

                Hash h("type", "propertyHistory", "deviceId", deviceId,
                       "property", property, "data", data, "success", success);
                std::string& reason = h.bindReference<std::string>("failureReason");

                if (success) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting property history: "
                            << deviceId << "." << property << " " << data.size();
                } else {
                    // Failure handler - figure out what went wrong:
                    try {
                        throw;
                    } catch (const std::exception& e) {
                        reason = e.what();
                        KARABO_LOG_FRAMEWORK_INFO << "Property history request to "
                                << deviceId << "." << property << " failed: " << reason;
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

                const std::string & readerId(getDataReaderId(deviceId));
                const bool& preview(info.has("preview") ? info.get<bool>("preview"): false);

                auto handler = bind_weak(&karabo::devices::GuiServerDevice::configurationFromPast, this,
                                         channel, deviceId, time, preview, _1, _2, _3, _4);
                auto failureHandler = bind_weak(&karabo::devices::GuiServerDevice::configurationFromPastError, this,
                                                channel, deviceId, time);
                // Two minutes timeout since current implementation of slotGetConfigurationFromPast:
                // The amount of data it has to read depends on the time when the device (more precisely: its datalogger)
                // was started the last time before the point in time that you requested and all the parameter updates
                // in between these two time points.
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


        void GuiServerDevice::configurationFromPast(WeakChannelPointer channel,
                                                    const std::string& deviceId, const std::string& time,
                                                    const bool& preview,
                                                    const karabo::util::Hash& config, const karabo::util::Schema& /*schema*/,
                                                    const bool configAtTimepoint,
                                                    const std::string &configTimepoint) {
            try {

                KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting configuration from past: " << deviceId << " @ " << time;

                Hash h("type", "configurationFromPast", "deviceId", deviceId, "time", time, "preview", preview);
                if (config.empty()) {
                    // Currently (Oct 2018) DataLogReader::getConfigurationFromPast does not reply errors, but empty
                    // configuration if it could not fulfill the request, e.g. because the device was not online at the
                    // requested time.
                    h.set("success", false);
                    h.set("reason", "Received empty configuration:\nLikely '" + deviceId
                          + "' has not been online (or not logging) until the requested time '" + time + "'.");
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


        void GuiServerDevice::configurationFromPastError(WeakChannelPointer channel,
                                                         const std::string& deviceId, const std::string& time) {
            // Log failure reason
            std::string failureReason;
            std::string details;
            try {
                throw; // Error handlers are called within a try block, so we can rethrow the caught exception
            } catch (const karabo::util::TimeoutException&) {
                failureReason = "Request timed out:\nProbably the data logging infrastructure is not available.";
            } catch (const std::exception& e) {
                failureReason = "Request to configuration from past failed.";
                details = e.what(); // Only for log, hide from GUI client
            }
            KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting configuration from past failed: "
                    << deviceId << " @ " << time << " : " << failureReason << " " << details;

            try {
                const Hash h("type", "configurationFromPast", "deviceId", deviceId, "time", time,
                             "success", false, "reason", failureReason);
                safeClientWrite(channel, h, REMOVE_OLDEST);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in configurationFromPastError: " << e.what();
            }
        }


        std::string GuiServerDevice::getDataReaderId(const std::string& deviceId) const {
            const string loggerId = DATALOGGER_PREFIX + deviceId;
            boost::mutex::scoped_lock lock(m_loggerMapMutex);
            if (m_loggerMap.has(loggerId)) {
                static int i = 0;
                return DATALOGREADER_PREFIX + toString(i++ % DATALOGREADERS_PER_SERVER) += "-" + m_loggerMap.get<string>(loggerId);
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Cannot determine DataLogReaderId: No '"
                        << loggerId << "' in map for '" << deviceId << "'"; // Full details in log file, ...
                throw KARABO_PARAMETER_EXCEPTION("Cannot determine DataLogReader"); // ...less for exception.
                return std::string(); // please the compiler
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
                    if (!channelSet.insert(channel).second) {
                        KARABO_LOG_FRAMEWORK_WARN << "A GUI client wants to subscribe a second time to output channel: "
                                << channelName;
                    }
                    // Mark as ready - no matter whether ready already before...
                    m_readyNetworkConnections[channelName][channel] = true;
                    if (notYetRegistered) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Register to monitor '" << channelName << "'";

                        auto dataHandler = bind_weak(&GuiServerDevice::onNetworkData, this, channelName, _1, _2);
                        // Channel configuration - we rely on defaults as: "dataDistribution" == copy, "onSlowness" == drop
                        const Hash cfg("delayOnInput", get<int>("delayOnInput"));
                        if (!remote().registerChannelMonitor(channelName, dataHandler, cfg)) {
                            KARABO_LOG_FRAMEWORK_WARN << "Already monitoring '" << channelName << "'!";
                            // Should we remote().unregisterChannelMonitor' and try again? But problem never seen...
                        }
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Do not register to monitor '" << channelName << "' "
                                << "since " << channelSet.size() - 1u << " client(s) already registered."; // -1: the new one
                    }
                } else { // i.e. un-subscribe
                    if (0 == channelSet.erase(channel)) {
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
                            KARABO_LOG_FRAMEWORK_WARN << "Failed to unregister '" << channelName << "'"; // Did it ever work?
                        }
                        m_networkConnections.erase(channelName); // Caveat: Makes 'channelSet' a dangling reference...
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Do not unregister to monitor '" << channelName << "' "
                                << "since " << channelSet.size() << " client(s) still interested";

                    }
                }
            } catch (const std::exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onSubscribeNetwork(): " << e.what();
            }
        }


        void GuiServerDevice::onRequestNetwork(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                const string& channelName = info.get<string>("channelName");
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestNetwork for " << channelName;
                boost::mutex::scoped_lock lock(m_networkMutex);
                m_readyNetworkConnections[channelName][channel] = true;
            } catch (const std::exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestNetwork: " << e.what();
            }
        }


        void GuiServerDevice::onNetworkData(const std::string& channelName,
                                            const karabo::util::Hash& data, const karabo::xms::InputChannel::MetaData& meta) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onNetworkData ....";

                Hash h("type", "networkData", "name", channelName);
                // Assign timestamp and aggressively try to avoid any copies by move-assign the 'data' to the payload
                // that we send to the clients. For that we cast const away from 'data' and in fact modify it!
                // That is safe, see comment in InputChannel::triggerIOEvent() which calls this method.
                Hash::Node& dataNode = h.set("data", Hash());
                dataNode.getValue<Hash>() = std::move(const_cast<Hash&> (data));
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
            } catch (const std::exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onNetworkData: " << e.what();
            }
        }


        void GuiServerDevice::sendSystemTopology(WeakChannelPointer channel) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "sendSystemTopology";
                KARABO_LOG_FRAMEWORK_DEBUG << remote().getSystemTopology();
                Hash h("type", "systemTopology", "systemTopology", remote().getSystemTopology());
                safeClientWrite(channel, h);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in sendSystemTopology(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
            // topologyEntry is an empty Hash at path <type>.<instanceId> with all the instanceInfo as attributes
            try {
                const std::string& type = topologyEntry.begin()->getKey();
                if (type == "device") {
                    const std::vector<std::string>& timingOutClasses = get<std::vector<std::string>>("ignoreTimeoutClasses");
                    recalculateTimingOutDevices(topologyEntry, timingOutClasses, false);
                    const Hash& deviceHash = topologyEntry.get<Hash>(type);
                    const std::string& instanceId = deviceHash.begin()->getKey();
                    // Check whether someone already noted interest in it
                    bool registerMonitor = false;
                    {
                        boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                        registerMonitor = (m_monitoredDevices.find(instanceId) != m_monitoredDevices.end());
                    }
                    if (registerMonitor) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Connecting to device " << instanceId << " which is going to be visible in a GUI client";
                        remote().registerDeviceForMonitoring(instanceId);
                    }
                    if (instanceId == get<std::string>("dataLogManagerId")) {
                        // The corresponding 'connect' is done by SignalSlotable's automatic reconnect feature.
                        // Even this request might not be needed since the logger manager emits the corresponding signal.
                        // But we cannot be 100% sure that our 'connect' has been registered in time.
                        requestNoWait(get<std::string>("dataLogManagerId"), "slotGetLoggerMap", "", "slotLoggerMap");
                    }

                    tryToUpdateNewInstanceAttributes(instanceId, INSTANCE_NEW_EVENT);

                    connectPotentialAlarmService(topologyEntry);
                    registerPotentialProjectManager(topologyEntry);
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in instanceNewHandler(): " << e.userFriendlyMsg();
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


        void GuiServerDevice::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& /*instInfo*/) {
            try {
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);

                    // Removes the instance from channel
                    for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                        // it->first->writeAsync(h);
                        it->second.visibleInstances.erase(instanceId);
                        it->second.requestedDeviceSchemas.erase(instanceId);
                        it->second.requestedClassSchemas.erase(instanceId);
                    }
                }

                {
                    // Erase instance from the attribute update map (maybe)
                    boost::mutex::scoped_lock lock(m_pendingAttributesMutex);
                    m_pendingAttributeUpdates.erase(instanceId);
                }

                {
                    // Erase instance from the monitored list
                    boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                    std::map<std::string, int>::iterator jt = m_monitoredDevices.find(instanceId);
                    if (jt != m_monitoredDevices.end()) {
                        m_monitoredDevices.erase(jt);
                    }
                }

                {
                    // Erase all bookmarks (InputChannel pointers) associated with instance that gone
                    // GF: There is no real need for this: If a client is interested in an output channel of this
                    //     device, better just rely on the automatic reconnect once the device is back.
                    //     (Currently [before 2.3.0] the GUI assumes that it has to give an extra command for this
                    //      channel if the device is back.)
                    boost::mutex::scoped_lock lock(m_networkMutex);
                    NetworkMap::const_iterator mapIter = m_networkConnections.cbegin();
                    while (mapIter != m_networkConnections.cend()) {
                        const std::string& channelName = mapIter->first;
                        // If channelName lacks the ':', channelInstanceId will be the full channelName.
                        const std::string channelInstanceId(channelName, 0, channelName.find_first_of(':'));
                        if (channelInstanceId == instanceId) {
                            KARABO_LOG_FRAMEWORK_DEBUG << "Remove connection to input channel: " << channelName;
                            // Use the reference 'channelName' before invalidating it by invalidating mapIter:
                            remote().unregisterChannelMonitor(channelName);
                            m_readyNetworkConnections.erase(channelName);
                            mapIter = m_networkConnections.erase(mapIter);
                        } else {
                           ++mapIter;
                        }
                    }
                }
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


        void GuiServerDevice::devicesChangedHandler(const karabo::util::Hash& what) {
            try {
                // Gathers all devices with configuration updates in 'what'.
                std::vector<std::string> updatedDevices;
                updatedDevices.reserve(what.size());
                what.getKeys(updatedDevices);

                boost::mutex::scoped_lock lock(m_channelMutex);
                // Loop on all clients
                for (ConstChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {

                    if (!it->first || !it->first->isOpen()) continue;

                    Hash configs;
                    for (const std::string &deviceId : updatedDevices) {
                        // Optimization: send only updates for devices the client is interested in.
                        if (it->second.visibleInstances.find(deviceId) != it->second.visibleInstances.end()) {
                            configs.set(deviceId, what.get<Hash>(deviceId));
                        }
                    }
                    if (!configs.empty()) {
                        Hash h("type", "deviceConfigurations");
                        h.bindReference<Hash>("configurations") = std::move(configs);
                        KARABO_LOG_FRAMEWORK_DEBUG << "Sending configuration updates to GUI client:\n" << h;
                        it->first->writeAsync(h);
                    }
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in devicesChangesHandler(): " << e.what();
            }
        }


        void GuiServerDevice::classSchemaHandler(const std::string& serverId, const std::string& classId, const karabo::util::Schema& classSchema) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "classSchemaHandler: serverId: \""<< serverId << "\" - classId :\"" << classId << "\"";

                Hash h("type", "classSchema", "serverId", serverId,
                       "classId", classId, "schema", classSchema);

                boost::mutex::scoped_lock lock(m_channelMutex);
                for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    auto itReq = it->second.requestedClassSchemas.find(serverId);
                    if (itReq != it->second.requestedClassSchemas.end()) {
                        if (itReq->second.find(classId) != itReq->second.end()){
                            // If e.g. a schema of an non-existing plugin was requested the schema could well be empty
                            // In this case we would not answer, but we still must clean the requestedClassSchemas map
                            if (!classSchema.empty()) {
                                if (it->first && it->first->isOpen()){
                                    it->first->writeAsync(h);
                                }
                            }
                            itReq->second.erase(classId);
                            // remove from the server key if all classSchema requests are fulfilled
                            if (itReq->second.empty()) it->second.requestedClassSchemas.erase(itReq);
                        }
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in classSchemaHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::schemaUpdatedHandler(const std::string& deviceId, const karabo::util::Schema& schema) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Sending schema updated for '" << deviceId << "'";

                if (schema.empty()) {
                    KARABO_LOG_FRAMEWORK_WARN << "Going to send an empty schema for deviceId \""<< deviceId << "\".";
                }

                Hash h("type", "deviceSchema", "deviceId", deviceId,
                       "schema", schema);

                boost::mutex::scoped_lock lock(m_channelMutex);
                // Loop on all clients
                for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    // Optimization: write only to clients subscribed to deviceId
                    if ((it->second.visibleInstances.find(deviceId) != it->second.visibleInstances.end())  // if instance is visible
                        || (it->second.requestedDeviceSchemas.find(deviceId) != it->second.requestedDeviceSchemas.end())) {  // if instance is requested
                        if (it->first && it->first->isOpen()){
                            it->first->writeAsync(h);
                        }
                        it->second.requestedDeviceSchemas.erase(deviceId);
                    }
                }


            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in schemaUpdatedHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::logHandler(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {
            try {
                // Filter the messages before caching!
                boost::mutex::scoped_lock lock(m_forwardLogsMutex);
                const std::vector<util::Hash>& inMessages = body->get<std::vector<util::Hash> >("messages");
                for (const util::Hash& msg : inMessages) {
                    const krb_log4cpp::Priority::Value priority(krb_log4cpp::Priority::getPriorityValue(msg.get<std::string>("type")));
                    // The lower the number, the higher the priority
                    if (priority <= m_loggerMinForwardingPriority) {
                        m_logCache.push_back(msg);
                    }
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in logHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onError(const karabo::net::ErrorCode& errorCode, WeakChannelPointer channel) {
            KARABO_LOG_INFO << "onError : TCP socket got error : " << errorCode.value() << " -- \"" << errorCode.message() << "\",  Close connection to a client";

            try {
                karabo::net::Channel::Pointer chan = channel.lock();
                std::set<std::string> deviceIds; // empty set
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    ChannelIterator it = m_channels.find(chan);
                    if (it != m_channels.end()) {
                        it->first->close(); // This closes socket and unregisters channel from connection
                        deviceIds.swap(it->second.visibleInstances); // copy to the empty set
                        // Remove channel as such
                        m_channels.erase(it);
                    } else {
                        KARABO_LOG_FRAMEWORK_WARN << "Trying to disconnect non-existing client channel at "
                                << chan.get() << " (address " << getChannelAddress(chan) << ").";
                    }
                    KARABO_LOG_FRAMEWORK_INFO << m_channels.size() << " client(s) left.";

                    // Update the number of clients connected
                    set("connectedClientCount", static_cast<unsigned int>(m_channels.size()));
                }

                // Now check all devices that this channel had interest in and decrement counter.
                // Unregister monitor if no-one interested anymore.
                {
                    boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                    for (std::set<std::string>::const_iterator jt = deviceIds.begin(); jt != deviceIds.end(); ++jt) {
                        const std::string& deviceId = *jt;
                        const int numLeft = --m_monitoredDevices[deviceId]; // prefix---: decrement and then assign
                        KARABO_LOG_FRAMEWORK_DEBUG << "stopMonitoringDevice (GUI gone) " << deviceId << " " << numLeft;
                        if (numLeft <= 0) {
                            //  erase the monitor entry to avoid unnecessary monitoring
                            m_monitoredDevices.erase(deviceId);
                            remote().unregisterDeviceMonitor(deviceId);
                        }
                    }
                }

                {
                    boost::mutex::scoped_lock lock(m_networkMutex);
                    NetworkMap::iterator iter = m_networkConnections.begin();
                    while (iter != m_networkConnections.end()) {
                        std::set<WeakChannelPointer>& channelSet = iter->second;
                        channelSet.erase(channel); // no matter whether in or not...
                        // Remove from readiness structures
                        for (auto itPair = m_readyNetworkConnections.begin(); itPair != m_readyNetworkConnections.end();) {
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
            KARABO_LOG_FRAMEWORK_INFO << "Debug info requested by slotDumpToLog:\n" << getDebugInfo(karabo::util::Hash());
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
                            const std::vector<std::string> monitoredDevices(it->second.visibleInstances.begin(), it->second.visibleInstances.end());
                            TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(it->first);

                            data.set(clientAddr, Hash("queueInfo", tcpChannel->queueInfo(),
                                                      "monitoredDevices", monitoredDevices,
                                                      // Leave string and bool vectors for the pipeline connections to be filled in below
                                                      "pipelineConnections", std::vector<std::string>(),
                                                      "pipelineConnectionsReadiness", std::vector<bool>(),
                                                      "clientVersion", it->second.clientVersion.getString()));
                        }
                    }

                    // Then add pipeline information to the client connection infos
                    {
                        boost::mutex::scoped_lock lock(m_networkMutex);
                        for (auto mapIter = m_networkConnections.begin(); mapIter != m_networkConnections.end(); ++mapIter) {
                            const std::string& channelName = mapIter->first;
                            const std::set<WeakChannelPointer>& channelSet = mapIter->second;
                            for (const WeakChannelPointer& channel : channelSet) {
                                Channel::Pointer channelPtr = channel.lock(); // promote to shared pointer
                                if (channelPtr) {
                                    const std::string clientAddr = getChannelAddress(channelPtr);
                                    std::vector<std::string>& pipelineConnections = data.get<std::vector<std::string> >(clientAddr + ".pipelineConnections");
                                    pipelineConnections.push_back(channelName);
                                    std::vector<bool>& pipelinesReady = data.get < std::vector<bool> >(clientAddr + ".pipelineConnectionsReadiness");
                                    pipelinesReady.push_back(m_readyNetworkConnections[channelName][channel]);
                                } // else - client might have gone meanwhile...
                            }
                        }
                    }
                }

                if (info.empty() || info.has("devices")) {
                    // monitored devices
                    Hash& monitoredDevices = data.bindReference<Hash>("monitoredDeviceConfigs");
                    {
                        boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);

                        for (auto it = m_monitoredDevices.begin(); it != m_monitoredDevices.end(); ++it) {
                            Hash config = remote().getConfigurationNoWait(it->first);
                            if (config.empty()) {
                                // It's important to know if `getConfigurationNoWait` returned an empty config!
                                monitoredDevices.set(it->first, Hash("configMissing", true));
                            } else {
                                monitoredDevices.set(it->first, config);
                            }
                        }
                    }
                }

                if (info.empty() || info.has("topology")) {
                    // system topology
                    data.set("systemTopology", remote().getSystemTopology());
            }

            return data;
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
                const auto& senderInfo = getSenderInfo("slotDisconnectClient");
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
                    request(deviceId, "slotUpdateSchemaAttributes", it->second.updates).receiveAsync<Hash>(
                        bind_weak(&GuiServerDevice::onUpdateNewInstanceAttributesHandler, this, deviceId, _1));
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in sending attribute update " << e.userFriendlyMsg();
            }
        }

        void GuiServerDevice::onUpdateNewInstanceAttributesHandler(const std::string& deviceId, const Hash& response) {
             try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Handling attribute update response from "<<deviceId;
                if(!response.get<bool>("success")) {
                    KARABO_LOG_ERROR<<"Schema attribute update failed for device: "<< deviceId;
                }

                boost::mutex::scoped_lock lock(m_pendingAttributesMutex);
                if (m_pendingAttributeUpdates.erase(deviceId) == 0) {
                   KARABO_LOG_ERROR<<"Received non-requested attribute update response from: "<< deviceId;
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in receiving attribute update response: " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::slotAlarmSignalsUpdate(const std::string& alarmServiceId, const std::string& type, const karabo::util::Hash& updateRows) {
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
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in broad casting alarms(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onAcknowledgeAlarm(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onAcknowledgeAlarm : info ...\n" << info;
                const std::string& alarmServiceId = info.get<std::string>("alarmInstanceId");
                call(alarmServiceId, "slotAcknowledgeAlarm", info.get<Hash>("acknowledgedRows"));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onAcknowledgeAlarm(): " << e.userFriendlyMsg();
            }
        };


        void GuiServerDevice::onRequestAlarms(WeakChannelPointer channel, const karabo::util::Hash& info, const bool replyToAllClients) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestAlarms : info ...\n" << info;

                const std::string& requestedInstance = info.get<std::string>("alarmInstanceId");
                request(requestedInstance, "slotRequestAlarmDump")
                        .receiveAsync<karabo::util::Hash>(bind_weak(&GuiServerDevice::onRequestedAlarmsReply, this, channel, _1, replyToAllClients));

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestAlarms(): " << e.userFriendlyMsg();
            }
        };


        void GuiServerDevice::onRequestedAlarmsReply(WeakChannelPointer channel, const karabo::util::Hash& reply, const bool replyToAllClients) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestedAlarmsReply : info ...\n" << reply;
                // Flushes all the instance changes that are waiting for the next throttler cycle to be dispatched.
                // This is done to guarantee that the clients will receive those instance changes before the alarm
                // updates. An alarm info, for instance, may refer to a device whose instanceNew event was being
                // held by the Throttler.
                remote().flushThrottledInstanceChanges();
                Hash h("type", "alarmInit", "instanceId", reply.get<std::string>("instanceId"), "rows", reply.get<Hash>("alarms"));
                if (replyToAllClients) {
                    safeAllClientsWrite(h, LOSSLESS);
                } else {
                    safeClientWrite(channel, h, LOSSLESS);
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestedAlarmsReply(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onUpdateAttributes(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onUpdateAttributes : info ...\n" << info;
                const std::string& instanceId = info.get<std::string>("instanceId");
                const std::vector<Hash>& updates = info.get<std::vector<Hash> >("updates");
                request(instanceId, "slotUpdateSchemaAttributes", updates)
                        .receiveAsync<Hash>(bind_weak(&GuiServerDevice::onRequestedAttributeUpdate, this, channel, _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onUpdateAttributes(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onRequestedAttributeUpdate(WeakChannelPointer channel, const karabo::util::Hash& reply) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestedAttributeUpdate : success ...\n" << reply.get<bool>("success");
                Hash h("type", "attributesUpdated", "reply", reply);
                safeClientWrite(channel, h, LOSSLESS);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestedAttributeUpdate(): " << e.userFriendlyMsg();
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
                             bind_weak(&GuiServerDevice::onRequestAlarms, this,
                                       WeakChannelPointer(), Hash("alarmInstanceId", instanceId), true));
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


        void GuiServerDevice::slotProjectUpdate(const Hash& info, const std::string & instanceId) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "slotProjectUpdate : info ...\n" << info;
                Hash h("type", "projectUpdate", "info", info);
                safeAllClientsWrite(h);
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in slotProjectUpdate: " << e.what();
            }
        }


        void GuiServerDevice::typeAndInstanceFromTopology(const karabo::util::Hash& topologyEntry, std::string& type, std::string& instanceId) {
            if (topologyEntry.empty()) return;

            type = topologyEntry.begin()->getKey(); // fails if empty...
            // TODO let device client return also instanceId as first argument
            // const ref is fine even for temporary std::string
            instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                          topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));


        }


        std::vector<std::string> GuiServerDevice::getKnownProjectManagers() const {
            boost::shared_lock<boost::shared_mutex> lk(m_projectManagerMutex);
            return std::vector<std::string>(m_projectManagers.begin(), m_projectManagers.end());
        }


        void GuiServerDevice::onRequestGeneric(WeakChannelPointer channel, const karabo::util::Hash& info){
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
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Error in generic request to slot with info " << info << "...\n" << e.what();
            }
        }


        void GuiServerDevice::forwardHashReply(bool success, WeakChannelPointer channel, const Hash& info, const Hash& reply) {
            const std::string replyType(info.has("replyType") ? info.get<std::string>("replyType") : "requestGeneric");
            const Hash& request(info.has("empty") ? Hash(): info);

            Hash h("type", replyType,
                   "success", success,
                   "request", request,
                   "reply", reply,
                   "reason", "");

            if (!success) {
                std::ostringstream oss;
                oss << "Failure on request to " << info.get<std::string>("instanceId") << "." << info.get<std::string>("slot");
                std::string& failTxt = h.get<std::string>("reason"); // modify via reference!
                failTxt = oss.str();
                try {
                    throw;
                } catch (const karabo::util::TimeoutException& te) {
                    failTxt += ", not answered within ";
                    if (info.has("timeout")) {
                        // Not 100% precise if "timeout" got reconfigured after request was sent...
                        const int timeout = std::max(info.get<int>("timeout"), m_timeout.load()); // load() for template resolution
                        failTxt += toString(timeout);
                    } else {
                        failTxt += toString(karabo::xms::SignalSlotable::Requestor::m_defaultAsyncTimeout / 1000.f);
                    }
                    failTxt += " seconds.";
                    karabo::util::Exception::clearTrace();
                    KARABO_LOG_FRAMEWORK_WARN << failTxt;
                } catch (const std::exception& e) {
                    (failTxt += "... details: ") += e.what();
                    KARABO_LOG_FRAMEWORK_WARN << failTxt;
                }
            }
            safeClientWrite(channel, h);
        }


        void GuiServerDevice::onProjectBeginUserSession(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectBeginUserSession : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectBeginUserSession", "Project manager does not exist: Begin User Session failed.")) return;
                const std::string& token = info.get<std::string>("token");
                request(projectManager, "slotBeginUserSession", token)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectBeginUserSession", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectBeginUserSession(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectEndUserSession(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectEndUserSession : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectEndUserSession", "Project manager does not exist: End User Session failed.")) return;
                const std::string& token = info.get<std::string>("token");
                request(projectManager, "slotEndUserSession", token)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectEndUserSession", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectEndUserSession(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectSaveItems(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectSaveItems : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectSaveItems", "Project manager does not exist: Project items cannot be saved.")) return;
                const std::string& token = info.get<std::string>("token");
                const std::vector<Hash>& items = info.get<std::vector<Hash> >("items");
                const std::string& client = (info.has("client") ? info.get<std::string>("client") : std::string());
                request(projectManager, "slotSaveItems", token, items, client)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectSaveItems", _1));

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectSaveItems(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectLoadItems(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectLoadItems : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectLoadItems", "Project manager does not exist: Project items cannot be loaded.")) return;
                const std::string& token = info.get<std::string>("token");
                const std::vector<Hash>& items = info.get<std::vector<Hash> >("items");
                request(projectManager, "slotLoadItems", token, items)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectLoadItems", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectLoadItems(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectListProjectManagers(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                Hash h("type", "projectListProjectManagers", "reply", getKnownProjectManagers());
                safeClientWrite(channel, h, LOSSLESS);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectListProjectManagers(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectListItems(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectListItems : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectListItems", "Project manager does not exist: Project list cannot be retrieved.")) return;
                const std::string& token = info.get<std::string>("token");
                const std::string& domain = info.get<std::string>("domain");
                const std::vector<std::string>& item_types = info.get<std::vector < std::string >> ("item_types");
                request(projectManager, "slotListItems", token, domain, item_types)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectListItems", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectListItems(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectListDomains(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectListDomains : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectListDomains", "Project manager does not exist: Domain list cannot be retrieved.")) return;
                const std::string& token = info.get<std::string>("token");
                request(projectManager, "slotListDomains", token)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectListDomains", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectListDomains(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectUpdateAttribute(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectUpdateAttribute : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectUpdateAttribute", "Project manager does not exist: Cannot update project attribute (trash).")) return;
                const std::string& token = info.get<std::string>("token");
                const std::vector<Hash>& items = info.get<std::vector<Hash> >("items");
                request(projectManager, "slotUpdateAttribute", token, items)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectUpdateAttribute", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectUpdateAttribute(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::forwardReply(WeakChannelPointer channel, const std::string& replyType,
                                           /*const karabo::net::ErrorCode& e,*/const karabo::util::Hash& reply) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "forwardReply : " << replyType;
                Hash h("type", replyType, "reply", reply);
                safeClientWrite(channel, h, LOSSLESS);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in forwarding reply of type '" << replyType << "': " << e;
            }
        }

        void GuiServerDevice::forwardRequestReply(WeakChannelPointer channel, const karabo::util::Hash& reply, const std::string& token) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "forwardRequestReply for token : "<< token;
                Hash h("reply", reply, "token", token, "success", true, "type", "requestFromSlot");
                safeClientWrite(channel, h, LOSSLESS);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in forwarding request reply for token '" << token << "': " << e;
            }
        }


        bool GuiServerDevice::checkProjectManagerId(WeakChannelPointer channel, const std::string& deviceId, const std::string & type, const std::string & reason) {
            boost::shared_lock<boost::shared_mutex> lk(m_projectManagerMutex);
            if (m_projectManagers.find(deviceId) != m_projectManagers.end()) return true;
            Hash h("type", type, "reply", Hash("success", false, "reason", reason));
            safeClientWrite(channel, h, LOSSLESS);
            return false;

        }

        void GuiServerDevice::onRequestFromSlot(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            Hash failureInfo;
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestFromSlot";
                // verify that hash is sane on top level
                failureInfo.set("deviceId", hash.has("deviceId"));
                failureInfo.set("slot", hash.has("slot"));
                failureInfo.set("args", hash.has("args"));
                failureInfo.set("token", hash.has("token"));
                const string& deviceId = hash.get<string > ("deviceId");
                const string& slot = hash.get<string > ("slot");
                const Hash& arg_hash = hash.get<Hash > ("args");
                const string& token = hash.get<string > ("token");
                request(deviceId, slot, arg_hash).template receiveAsync<Hash>(bind_weak(&GuiServerDevice::forwardRequestReply, this, channel, _1, token),
                                                                              bind_weak(&GuiServerDevice::onRequestFromSlotErrorHandler, this, channel, failureInfo, token));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequest() with args: "<<hash<<": " << e.userFriendlyMsg();
                failureInfo.set("replied_error", e.what());
                Hash reply("success", false, "info", failureInfo, "token", (hash.has("token")? hash.get<std::string>("token"): "undefined"), "type", "requestFromSlot");
                safeClientWrite(channel, reply, LOSSLESS);
            }
        }

        void GuiServerDevice::onRequestFromSlotErrorHandler(WeakChannelPointer channel, const karabo::util::Hash& info, const std::string& token) {
            try {
                throw;
            } catch (const TimeoutException& te) {
                // act on timeout, e.g.
                KARABO_LOG_FRAMEWORK_ERROR << te.what();
                Hash failureInfo(info);
                failureInfo.set("replied_error", te.what());
                Hash reply("success", false, "info", failureInfo, "token", token, "type", "requestFromSlot");
                safeClientWrite(channel, reply, LOSSLESS);
            } catch (const RemoteException& re) {
                // act on remote error, e.g.
                KARABO_LOG_FRAMEWORK_ERROR << re.what();
                Hash failureInfo(info);
                failureInfo.set("replied_error", re.what());
                Hash reply("success", false, "info", failureInfo, "token", token, "type", "requestFromSlot");
                safeClientWrite(channel, reply, LOSSLESS);
            }
        }


        std::string GuiServerDevice::getChannelAddress(const karabo::net::Channel::Pointer& channel) const {
            TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(channel);
            std::string addr = tcpChannel->remoteAddress();

            // convert periods to underscores, so that this can be used as a Hash key...
            std::transform(addr.begin(), addr.end(), addr.begin(), [](char c){ return c == '.' ? '_' : c; });

            return addr;
        }

    }
}
