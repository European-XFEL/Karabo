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

using namespace std;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::io;
using namespace karabo::xms;



namespace karabo {
    namespace devices {

        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, GuiServerDevice)

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
                    .description("Some delay before informing output channel about readiness for next data.")
                    .assignmentOptional().defaultValue(500)
                    .reconfigurable()
                    .minInc(200) // => not faster than 5 Hz, but allow as slow as desired
                    .unit(Unit::SECOND)
                    .metricPrefix(MetricPrefix::MILLI)
                    .commit();

            INT32_ELEMENT(expected).key("fastDataQueueCapacity")
                    .displayedName("Fast Data forwarding queue size")
                    .description("The number of fast data (cameras, other big data) messages to store in the forwarding ring buffer. NOTE: Will be applied to newly connected clients only")
                    .assignmentOptional().defaultValue(5) // 5Hz * 1 second
                    .reconfigurable()
                    .minExc(0).maxInc(100)
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
                    .minInc(100).maxInc(10000) // => roughly between 10 Hz and 0.1 Hz
                    .commit();

            INT32_ELEMENT(expected).key("waitInitDevice")
                    .displayedName("Instantiate wait time")
                    .description("Time interval between the instantiation of devices.")
                    .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                    .assignmentOptional().defaultValue(500)
                    .reconfigurable()
                    .minExc(200).maxInc(5000) // NOTE: Not _too_ fast. The device instantiation timer is always running!
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
                    .assignmentOptional().defaultValue("ERROR")
                    .reconfigurable()
                    .commit();

            VECTOR_STRING_ELEMENT(expected).key("p2pDevices")
                    .displayedName("P2p Devices")
                    .description("Point-to-point connections should be established for these devices. NOTE: Only evaluated at start up or for new devices.")
                    .assignmentOptional().defaultValue(std::vector<std::string>())
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


        }


        GuiServerDevice::GuiServerDevice(const Hash& config)
        : Device<>(config)
        , m_deviceInitTimer(EventLoop::getIOService())
        , m_networkStatsTimer(EventLoop::getIOService()) {

            KARABO_INITIAL_FUNCTION(initialize)

            KARABO_SLOT(slotLoggerMap, Hash /*loggerMap*/)
            KARABO_SLOT(slotAlarmSignalsUpdate, std::string, std::string, karabo::util::Hash );
            KARABO_SLOT(slotRunConfigSourcesUpdate, karabo::util::Hash, std::string);
            KARABO_SLOT(slotDumpDebugInfo, karabo::util::Hash);
            KARABO_SIGNAL("signalClientSignalsAlarmUpdate", Hash);
            KARABO_SIGNAL("signalClientRequestsAlarms");

            Hash h;
            h.set("port", config.get<unsigned int>("port"));
            h.set("type", "server");
            h.set("serializationType", "binary"); // Will lead to binary header hashes
            m_dataConnection = Connection::create("Tcp", h);
            m_serializer = BinarySerializer<Hash>::create("Bin"); // for reading

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
                remote().registerInstanceUpdatedMonitor(boost::bind(&karabo::devices::GuiServerDevice::instanceUpdatedHandler, this, _1));
                remote().registerInstanceGoneMonitor(boost::bind(&karabo::devices::GuiServerDevice::instanceGoneHandler, this, _1, _2));
                remote().registerSchemaUpdatedMonitor(boost::bind(&karabo::devices::GuiServerDevice::schemaUpdatedHandler, this, _1, _2));
                remote().registerClassSchemaMonitor(boost::bind(&karabo::devices::GuiServerDevice::classSchemaHandler, this, _1, _2, _3));

                // If someone manages to bind_weak(&karabo::devices::GuiServerDevice::requestNoWait<>, this, ...),
                // we would not need loggerMapConnectedHandler...
                asyncConnect(karabo::util::DATALOGMANAGER_ID, "signalLoggerMap", "", "slotLoggerMap",
                             bind_weak(&karabo::devices::GuiServerDevice::loggerMapConnectedHandler, this));

                // scan topology to treat alarm services, project managers and run configurators
                // NOTE: If instanceNewHandler would be registered before enableInstanceTracking(),
                //       this code would probably not be needed here, but just in instanceNewHandler!
                const Hash& topology = remote().getSystemTopology();
                const boost::optional<const Hash::Node&> devices = topology.find("device");
                if (devices) {
                    const Hash& deviceEntry = devices->getValue<Hash>();
                    for (Hash::const_iterator it = deviceEntry.begin(); it != deviceEntry.end(); ++it) {
                        const std::string& deviceId = it->getKey();
                        Hash topologyEntry; // prepare input as instanceNewHandler expects it
                        Hash::Node& hn = topologyEntry.set("device." + deviceId, it->getValue<Hash>());
                        hn.setAttributes(it->getAttributes());
                        checkForConnectingP2p(deviceId);
                        connectPotentialAlarmService(topologyEntry);
                        registerPotentialProjectManager(topologyEntry);
                        connectPotentialRunConfigurator(topologyEntry);
                    }
                }

                m_dataConnection->startAsync(bind_weak(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));

                m_loggerConsumer = getConnection()->createConsumer(m_topic, "target = 'log'");
                m_loggerConsumer->startReading(bind_weak(&karabo::devices::GuiServerDevice::logHandler, this, _1, _2));

                m_guiDebugProducer = getConnection()->createProducer();

                startDeviceInstantiation();
                startNetworkMonitor();

                updateState(State::ON);

                // Produce some information
                KARABO_LOG_INFO << "GUI Server is up and listening on port: " << get<unsigned int>("port");

            } catch (const Exception& e) {
                updateState(State::ERROR);

                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initialize(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::loggerMapConnectedHandler() {
            requestNoWait(karabo::util::DATALOGMANAGER_ID, "slotGetLoggerMap", "", "slotLoggerMap");
        }


        void GuiServerDevice::checkForConnectingP2p(const std::string& deviceId) {
            const std::vector<std::string> p2pDevices = get<std::vector<std::string> >("p2pDevices");
            if (std::find(p2pDevices.begin(), p2pDevices.end(), deviceId) != p2pDevices.end()) {
                KARABO_LOG_FRAMEWORK_DEBUG;

                auto successHandler = [deviceId] () {
                    KARABO_LOG_FRAMEWORK_INFO << "Going to establish p2p to '" << deviceId << "'";
                };
                auto failureHandler = [deviceId] () {
                    try {
                        throw;
                    } catch (const std::exception& e) {
                        // As of now (2018-01-03), this is expected for middlelayer...
                        KARABO_LOG_FRAMEWORK_WARN << "Cannot establish p2p to '" << deviceId << "' since:\n"
                                << e.what();
                    }
                };
                asyncConnectP2p(deviceId, successHandler, failureHandler);
            }
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
                for (auto it = m_networkConnections.begin(); it != m_networkConnections.end(); ) {
                    karabo::xms::InputChannel::Pointer channel = it->first;

                    pipeBytesRead += channel->dataQuantityRead();
                    pipeBytesWritten += channel->dataQuantityWritten();

                    // m_networkConnections is a multimap. Go to the next non-duplicate key.
                    do {
                        ++it;
                    } while (it != m_networkConnections.end() && channel != it->first);
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
                // priority 2 bound to FAST_DATA traffic with REMOVE_OLDEST policy with a small queue
                channel->setAsyncChannelPolicy(FAST_DATA, "REMOVE_OLDEST", get<int>("fastDataQueueCapacity"));
                // priority 3 bound to REMOVE_OLDEST dropping policy
                channel->setAsyncChannelPolicy(REMOVE_OLDEST, "REMOVE_OLDEST", get<int>("lossyDataQueueCapacity"));
                // priority 4 should be LOSSLESS
                channel->setAsyncChannelPolicy(LOSSLESS, "LOSSLESS");

                channel->readAsyncHash(bind_weak(&karabo::devices::GuiServerDevice::onRead, this, _1, WeakChannelPointer(channel), _2));

                Hash brokerInfo("type", "brokerInformation");
                // TODO Add to gui code
                //brokerInfo.set("url", getConnection()->getBrokerUrl());
                brokerInfo.set("host", "exfl-broker.desy.de"); // TODO Kill once it's clear the GUI does not need that
                brokerInfo.set("port", 7777); // TODO Kill once it's clear the GUI does not need that
                brokerInfo.set("topic", m_topic);
                channel->writeAsync(brokerInfo);

                // Register channel
                registerConnect(channel);

                // Re-register acceptor socket (allows handling multiple clients)
                m_dataConnection->startAsync(bind_weak(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));


            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onConnect(): " << e.userFriendlyMsg();
                m_dataConnection->startAsync(bind_weak(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));
            }
        }


        void GuiServerDevice::registerConnect(const karabo::net::Channel::Pointer & channel) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = std::set<std::string > (); // maps channel to visible instances
            // Update the number of clients connected
            set("connectedClientCount", static_cast<unsigned int>(m_channels.size()));
        }


        void GuiServerDevice::onRead(const karabo::net::ErrorCode& e, WeakChannelPointer channel, karabo::util::Hash& info) {
            if (e) {
                EventLoop::getIOService().post(bind_weak(&GuiServerDevice::onError, this, e, channel));
                return;
            }

            try {
                // GUI communication scenarios
                if (info.has("type")) {
                    string type = info.get<string > ("type");
                    if (type == "requestFromSlot") {
                        onRequestFromSlot(channel, info);
                    } else if (type == "login") {
                        onLogin(channel, info);
                    } else if (type == "reconfigure") {
                        onReconfigure(info);
                    } else if (type == "execute") {
                        onExecute(info);
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
                    } else if (type == "subscribeNetwork") {
                        onSubscribeNetwork(channel, info);
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
                    } else if (type == "runConfigSourcesInGroup") {
                        onRunConfigSourcesInGroup(channel, info);
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "Ignoring request";
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRead(): " << e.userFriendlyMsg();
            }

            // Read the next Hash from the client
            karabo::net::Channel::Pointer chan = channel.lock();
            if (chan) {
                chan->readAsyncHash(bind_weak(&karabo::devices::GuiServerDevice::onRead, this, _1, channel, _2));
            }
        }


        void GuiServerDevice::onGuiError(const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGuiError";
                m_guiDebugProducer->write("karaboGuiDebug", Hash()/*empty header*/, hash);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGuiError(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onLogin(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onLogin";
                // Check valid login
                KARABO_LOG_INFO << "Login request of user: " << hash.get<string > ("username");

                vector<unsigned int> versionParts = karabo::util::fromString<unsigned int, vector>(hash.get<string>("version"), ".");
                if (versionParts.size() >= 2) {
                    unsigned int major = versionParts[0];
                    unsigned int minor = versionParts[1];
                    // Versions earlier than 1.5.0 of the GUI don't understand a systemVersion message.
                    if ((major >= 1 && minor >= 5) || major >= 2) sendSystemVersion(channel);
                }

                // if ok
                sendSystemTopology(channel);
                // else sendBadLogin
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onLogin(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onReconfigure(const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onReconfigure";
                string deviceId = hash.get<string > ("deviceId");
                Hash config = hash.get<Hash > ("configuration");
                // TODO Supply user specific context
                call(deviceId, "slotReconfigure", config);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onReconfigure(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onExecute(const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onExecute";
                string deviceId = hash.get<string > ("deviceId");
                string command = hash.get<string > ("command");
                // TODO Supply user specific context
                call(deviceId, command);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onExecute(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onInitDevice(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            try {

                const string& serverId = hash.get<string>("serverId");
                const string& deviceId = hash.get<string>("deviceId");
                KARABO_LOG_FRAMEWORK_DEBUG << "onInitDevice: Queueing request to start device instance \""
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

                    request(serverId, "slotStartDevice", inst.hash)
                        .receiveAsync<bool, string>(bind_weak(&karabo::devices::GuiServerDevice::initReply,
                                                              this, inst.channel, deviceId, inst.hash, _1, _2));

                    m_pendingDeviceInstantiations.pop();
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initSingleDevice(): " << e.userFriendlyMsg();
            }

            // Always restart the timer!
            startDeviceInstantiation();
        }


        void GuiServerDevice::initReply(WeakChannelPointer channel, const string& givenDeviceId, const karabo::util::Hash& givenConfig, bool success, const string& message) {
            try {

                KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting init reply";

                Hash h("type", "initReply", "deviceId", givenDeviceId, "success", success, "message", message);
                safeClientWrite(channel, h);

                tryToUpdateNewInstanceAttributes(givenDeviceId, DEVICE_SERVER_REPLY_EVENT);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initReply " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::safeClientWrite(const WeakChannelPointer channel, const karabo::util::Hash& message, int prio) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            karabo::net::Channel::Pointer chan = channel.lock();
            if (chan && chan->isOpen()) {
                chan->writeAsync(message, prio);
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
                    Hash h("type", "deviceConfiguration", "deviceId", deviceId, "configuration", remote().get(deviceId));
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
                    std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(chan);
                    if (it != m_channels.end()) {
                        it->second.insert(deviceId);
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
                    remote().registerDeviceMonitor(deviceId, bind_weak(&karabo::devices::GuiServerDevice::deviceChangedHandler, this, _1, _2));
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
                    std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(chan);
                    if (it != m_channels.end()) it->second.erase(deviceId);
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
                    remote().unregisterDeviceMonitor(deviceId);
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
                    KARABO_LOG_FRAMEWORK_INFO << "onGetClassSchema : serverId=\"" << serverId << "\", classId=\"" << classId << "\" +";
                } else {
                    KARABO_LOG_FRAMEWORK_INFO << "onGetClassSchema : serverId=\"" << serverId << "\", classId=\"" << classId << "\" ...";
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

                const string loggerId = DATALOGGER_PREFIX + deviceId;
                if (m_loggerMap.has(loggerId)) {
                    static int i = 0;
                    const string readerId = DATALOGREADER_PREFIX + toString(i++ % DATALOGREADERS_PER_SERVER) += "-" + m_loggerMap.get<string>(loggerId);
                    request(readerId, "slotGetPropertyHistory", deviceId, property, args)
                            .receiveAsync<string, string, vector<Hash> >(bind_weak(&karabo::devices::GuiServerDevice::propertyHistory, this, channel, _1, _2, _3));
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "onGetPropertyHistory: No '" << loggerId << "' in map.";
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetPropertyHistory(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::propertyHistory(WeakChannelPointer channel, const std::string& deviceId, const std::string& property, const std::vector<karabo::util::Hash>& data) {
            try {

                KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting property history: "
                        << deviceId << "." << property << " " << data.size();

                Hash h("type", "propertyHistory", "deviceId", deviceId,
                       "property", property, "data", data);

                safeClientWrite(channel, h, REMOVE_OLDEST);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in propertyHistory " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onSubscribeNetwork(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onSubscribeNetwork : channelName = \"" << info.get<string>("channelName") << "\" "
                        << (info.get<bool>("subscribe") ? "+" : "-");

                karabo::net::Channel::Pointer chan = channel.lock();
                string channelName = info.get<string>("channelName");
                bool subscribe = info.get<bool>("subscribe");
                NetworkMap::iterator iter;
                boost::mutex::scoped_lock lock(m_networkMutex);

                for (iter = m_networkConnections.begin(); iter != m_networkConnections.end(); ++iter) {
                    if (channelName == iter->second.name) {
                        if (subscribe) {
                            if (chan == iter->second.channel) {
                                KARABO_LOG_FRAMEWORK_WARN << "Skip subscription to output channel '" << channelName
                                        << "' for GUI on channel " " : It is already subscribed for GUI on channel "
                                        << chan.get();
                                return;
                            }
                            NetworkConnection nc;
                            nc.name = channelName;
                            nc.channel = chan;
                            m_networkConnections.insert(NetworkMap::value_type(iter->first, nc));
                            return;
                        } else {
                            if (iter->second.channel == chan) {
                                m_networkConnections.erase(iter);
                                return;
                            }
                        }
                    }
                }

                if (!subscribe) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "trying to unsubscribe from non-subscribed channel " << channelName;
                    return;
                }

                Hash h("connectedOutputChannels", channelName, "dataDistribution", "copy",
                       "onSlowness", "drop", "delayOnInput", get<int>("delayOnInput"));
                InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", h);
                input->setInstanceId(m_instanceId);
                input->registerInputHandler(bind_weak(&GuiServerDevice::onNetworkData, this, _1));

                connectInputChannel(input); // asynchronous
                NetworkConnection nc;
                nc.name = channelName;
                nc.channel = chan;
                m_networkConnections.insert(NetworkMap::value_type(input, nc));

            } catch (const std::exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onSubscribeNetwork(): " << e.what();
            }
        }


        void GuiServerDevice::onNetworkData(const InputChannel::Pointer& input) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onNetworkData ....";

                Hash h("type", "networkData");
                for (size_t i = 0; i < input->size(); ++i) {
                    Hash& data = h.bindReference<Hash>("data"); // overwrites if "data" already there
                    input->read(data, i);

                    boost::mutex::scoped_lock lock(m_networkMutex);
                    pair<NetworkMap::iterator, NetworkMap::iterator> range = m_networkConnections.equal_range(input);
                    for (; range.first != range.second; ++range.first) {
                        h.set("name", range.first->second.name);
                        safeClientWrite(WeakChannelPointer(range.first->second.channel), h, FAST_DATA);
                    }
                }
            } catch (const Exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onNetworkData: " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::sendSystemVersion(WeakChannelPointer channel) {
            try {
                string const version = karabo::util::Version::getVersion();
                KARABO_LOG_FRAMEWORK_DEBUG << "sendSystemVersion";
                KARABO_LOG_FRAMEWORK_DEBUG << version;
                Hash h("type", "systemVersion", "version", version);
                safeClientWrite(channel, h);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in sendSystemVersion(): " << e.userFriendlyMsg();
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
            // topologyEntry is prepared in DeviceClient::prepareTopologyEntry:
            // an empty Hash at path <type>.<instanceId> with all the instanceInfo as attributes
            try {

                std::string type, instanceId;
                typeAndInstanceFromTopology(topologyEntry, type, instanceId);

                KARABO_LOG_FRAMEWORK_INFO << "instanceNewHandler --> instanceId: '" << instanceId
                        << "', type: '" << type << "'";

                Hash h("type", "instanceNew", "topologyEntry", topologyEntry);
                safeAllClientsWrite(h);

                if (type == "device") {
                    // Check whether someone already noted interest in it
                    bool registerMonitor = false;
                    {
                        boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                        registerMonitor = (m_monitoredDevices.find(instanceId) != m_monitoredDevices.end());
                    }
                    if (registerMonitor) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Connecting to device " << instanceId << " which is going to be visible in a GUI client";
                        remote().registerDeviceMonitor(instanceId, bind_weak(&karabo::devices::GuiServerDevice::deviceChangedHandler, this, _1, _2));
                    }
                    if (instanceId == karabo::util::DATALOGMANAGER_ID) {
                        // The corresponding 'connect' is done by SignalSlotable's automatic reconnect feature.
                        // Even this request might not be needed since the logger manager emits the corresponding signal.
                        // But we cannot be 100% sure that our 'connect' has been registered in time.
                        requestNoWait(karabo::util::DATALOGMANAGER_ID, "slotGetLoggerMap", "", "slotLoggerMap");
                    }

                    tryToUpdateNewInstanceAttributes(instanceId, INSTANCE_NEW_EVENT);

                    checkForConnectingP2p(instanceId);
                    connectPotentialAlarmService(topologyEntry);
                    registerPotentialProjectManager(topologyEntry);
                    connectPotentialRunConfigurator(topologyEntry);
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in instanceNewHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::instanceUpdatedHandler(const karabo::util::Hash& topologyEntry) {
            try {
                const std::string& type = topologyEntry.begin()->getKey();
                const std::string& instanceId = topologyEntry.begin()->getValue<Hash>().begin()->getKey();
                KARABO_LOG_FRAMEWORK_INFO << "instanceUpdatedHandler --> instanceId: '" << instanceId << "'"
                        << ", type: '" << type << "'";
                Hash h("type", "instanceUpdated", "topologyEntry", topologyEntry);
                safeAllClientsWrite(h);

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in instanceUpdatedHandler(): " << e.what();
            }
        }


        void GuiServerDevice::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            try {
                // const ref is fine even for temporary std::string
                const std::string& type = (instanceInfo.has("type") && instanceInfo.is<std::string>("type") ?
                                           instanceInfo.get<std::string>("type") : std::string("unknown"));
                KARABO_LOG_FRAMEWORK_INFO << "instanceGoneHandler --> instanceId: '" << instanceId
                        << "', type: '" << type << "'";

                Hash h("type", "instanceGone", "instanceId", instanceId, "instanceType", type);
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);

                    // Broadcast to all GUIs
                    for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                        it->first->writeAsync(h);
                        // and remove the instance from channel
                        it->second.erase(instanceId);
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
                    NetworkMap::iterator iter;
                    boost::mutex::scoped_lock lock(m_networkMutex);

                    for (iter = m_networkConnections.begin(); iter != m_networkConnections.end(); ++iter) {
                        std::vector<std::string> tmp;
                        boost::split(tmp, iter->second.name, boost::is_any_of("@:"));
                        if (tmp[0] == instanceId) {
                            KARABO_LOG_FRAMEWORK_DEBUG << "instanceId : " << instanceId << ", channelName : " << iter->second.name;
                            m_networkConnections.erase(iter);
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



            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in instanceGoneHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::deviceChangedHandler(const std::string& deviceId, const karabo::util::Hash& what) {
            try {

                if (what.has("deviceId")) { // only request for full configuration should contain that...
                    KARABO_LOG_FRAMEWORK_DEBUG << "deviceChangedHandler" << ": deviceId = '" << deviceId << "'";
                }

                Hash h("type", "deviceConfiguration", "deviceId", deviceId, "configuration", what);
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Loop on all clients
                for (ConstChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    // Optimization: broadcast only to clients interested in deviceId
                    if (it->second.find(deviceId) != it->second.end()) {
                        if (it->first && it->first->isOpen()) it->first->writeAsync(h);
                    }
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in deviceChangedHandler(): " << e.what();
            }
        }


        void GuiServerDevice::classSchemaHandler(const std::string& serverId, const std::string& classId, const karabo::util::Schema& classSchema) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "classSchemaHandler";

                // If e.g. a schema of an non-existing plugin was requested the schema could well be empty
                // In this case we would not answer
                if (classSchema.empty()) return;

                Hash h("type", "classSchema", "serverId", serverId,
                       "classId", classId, "schema", classSchema);

                // Broadcast to all GUIs
                safeAllClientsWrite(h);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetClassSchema(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::schemaUpdatedHandler(const std::string& deviceId, const karabo::util::Schema& schema) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting schema updated for '" << deviceId << "'";

                if (schema.empty()) {
                    KARABO_LOG_FRAMEWORK_WARN << "Going to send an empty schema, should not happen...";
                }

                Hash h("type", "deviceSchema", "deviceId", deviceId,
                       "schema", schema);

                // Broadcast to all GUIs
                // why bother all and not only those that are interested in deviceId?
                // As in deviceChangedHandle!
                safeAllClientsWrite(h);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in schemaUpdatedHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::logHandler(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {
            try {
                // Filter the messages before forwarding!
                const std::vector<util::Hash>& inMessages = body->get<std::vector<util::Hash> >("messages");
                std::vector<util::Hash> outMessages;

                BOOST_FOREACH(const util::Hash& msg, inMessages) {
                    const krb_log4cpp::Priority::Value priority(krb_log4cpp::Priority::getPriorityValue(msg.get<std::string>("type")));
                    // The lower the number, the higher the priority
                    if (priority <= m_loggerMinForwardingPriority) {
                        outMessages.push_back(msg);
                    }
                }

                if (outMessages.size() > 0){
                    Hash h("type", "log", "messages", outMessages);
                    // Broadcast to all GUIs
                    safeAllClientsWrite(h, REMOVE_OLDEST);
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in logHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onError(const karabo::net::ErrorCode& errorCode, WeakChannelPointer channel) {
            try {

                KARABO_LOG_INFO << "onError : TCP socket got error : " << errorCode.value() << " -- \"" << errorCode.message() << "\",  Close connection to a client";

                // TODO Fork on error message
                karabo::net::Channel::Pointer chan = channel.lock();
                std::set<std::string> deviceIds; // empty set
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(chan);
                    if (it != m_channels.end()) {
                        it->first->close(); // This closes socket and unregisters channel from connection
                        deviceIds.swap(it->second); // copy to the empty set
                        // Remove channel as such
                        m_channels.erase(it);
                    }
                    KARABO_LOG_FRAMEWORK_INFO << m_channels.size() << " client(s) left.";

                    // Update the number of clients connected
                    set("connectedClientCount", static_cast<unsigned int>(m_channels.size()));
                }

                // Now check all devices that this channel had interest in and decrement counter.
                // Keep those devices in deviceIds where no one else is interested.
                {
                    boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                    for (std::set<std::string>::const_iterator jt = deviceIds.begin(); jt != deviceIds.end();) {
                        const std::string& deviceId = *jt;
                        const int numLeft = --m_monitoredDevices[deviceId]; // prefix---: decrement and then assign
                        KARABO_LOG_FRAMEWORK_DEBUG << "stopMonitoringDevice (GUI gone) " << deviceId << " " << numLeft;
                        if (numLeft > 0) {
                            // others still interested - remove from set of devices to be unregistered
                            deviceIds.erase(jt++); // postfix-++: erase from map on the fly & keep valid iterator
                        } else {
                            //  erase the monitor entry to avoid unnecessary monitoring
                            m_monitoredDevices.erase(deviceId);
                            ++jt;
                        }
                    }
                }
                // All devices left in deviceIds have to be unregistered from monitoring.

                for (const std::string& deviceId : deviceIds) {
                    remote().unregisterDeviceMonitor(deviceId);
                }

                {
                    boost::mutex::scoped_lock lock(m_networkMutex);

                    NetworkMap::iterator iter = m_networkConnections.begin();
                    while (iter != m_networkConnections.end()) {
                        if (iter->second.channel == chan) {
                            m_networkConnections.erase(iter++);
                        } else ++iter;
                    }
                    KARABO_LOG_FRAMEWORK_INFO << m_networkConnections.size() << " pipeline channel(s) left.";
                }

            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onError(): " << e.what();
            }
        }


        void GuiServerDevice::slotLoggerMap(const karabo::util::Hash& loggerMap) {
            m_loggerMap = loggerMap;
        }


        void GuiServerDevice::slotDumpDebugInfo(const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "slotDebugInfo : info ...\n" << info;

                Hash data;

                if (info.empty() || info.has("clients")) {
                    // connected clients

                    // Start with the client TCP connections
                    {
                        boost::mutex::scoped_lock lock(m_channelMutex);

                        for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
                            const std::string clientAddr = getChannelAddress(it->first);
                            const std::vector<std::string> monitoredDevices(it->second.begin(), it->second.end());
                            TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(it->first);

                            data.set(clientAddr, Hash("queueInfo", tcpChannel->queueInfo(),
                                                      "monitoredDevices", monitoredDevices,
                                                      // Leave a string vector for the pipeline connections to be filled in below
                                                      "pipelineConnections", std::vector<std::string>()));
                        }
                    }

                    // Then add pipeline information to the client connection infos
                    {
                        boost::mutex::scoped_lock lock(m_networkMutex);

                        // m_networkConnections is a multimap; the same client channel (key) might be encountered more than once
                        for (auto it = m_networkConnections.begin(); it != m_networkConnections.end(); ++it) {
                            const std::string clientAddr = getChannelAddress(it->second.channel);
                            std::vector<std::string>& pipelineConnections = data.get<std::vector<std::string> >(clientAddr + ".pipelineConnections");
                            pipelineConnections.push_back(it->second.name); // the connected device property name
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

                // reply to the caller
                reply(data);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in slotDebugInfo(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::tryToUpdateNewInstanceAttributes(const std::string& deviceId, const int callerMask) {
            try {
                boost::mutex::scoped_lock lock(m_pendingAttributesMutex);
                const auto it = m_pendingAttributeUpdates.find(deviceId);

                if (it != m_pendingAttributeUpdates.end()) {
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

                boost::mutex::scoped_lock(m_pendingAttributesMutex);
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
                // actively ask this previously unknown device to submit its alarms as init messages on all channesl
                asyncConnect(instanceId, "signalAlarmServiceUpdate", "", "slotAlarmSignalsUpdate",
                             bind_weak(&GuiServerDevice::onRequestAlarms, this,
                                       WeakChannelPointer(), Hash("alarmInstanceId", instanceId), true));
            }

        }


        void GuiServerDevice::connectPotentialRunConfigurator(const karabo::util::Hash& topologyEntry) {
            std::string type, instanceId;
            typeAndInstanceFromTopology(topologyEntry, type, instanceId);
            if (topologyEntry.get<Hash>(type).begin()->hasAttribute("classId") &&
                topologyEntry.get<Hash>(type).begin()->getAttribute<std::string>("classId") == "RunConfigurator") {

                // No success handler since no need to to do an initial request for this information(?).
                asyncConnect(instanceId, "signalGroupSourceChanged", "", "slotRunConfigSourcesUpdate");
            }

        }


        void GuiServerDevice::registerPotentialProjectManager(const karabo::util::Hash& topologyEntry) {
            std::string type, instanceId;
            typeAndInstanceFromTopology(topologyEntry, type, instanceId);
            if (topologyEntry.get<Hash>(type).begin()->hasAttribute("classId") &&
                topologyEntry.get<Hash>(type).begin()->getAttribute<std::string>("classId") == "ProjectManager") {
                boost::unique_lock<boost::shared_mutex> lk(m_projectManagerMutex);
                m_projectManagers.insert(instanceId);
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
                request(projectManager, "slotSaveItems", token, items)
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


        void GuiServerDevice::onRunConfigSourcesInGroup(WeakChannelPointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRunConfigSourcesInGroup : info ...\n" << info;
                const std::string& runConfigurator = info.get<std::string>("runConfiguratorId");
                const std::string& group = info.get<std::string>("group");
                request(runConfigurator, "slotGetSourcesInGroup", group)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "runConfigSourcesInGroup", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRunConfigSourcesInGroup(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::slotRunConfigSourcesUpdate(const karabo::util::Hash& info, const std::string& deviceId) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting run config group update";
                Hash h("type", "runConfigSourcesInGroup", "reply", info);
                // Broadcast to all GUIs
                safeAllClientsWrite(h, LOSSLESS);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in broad config group update: " << e.userFriendlyMsg();
            }
        }

        void GuiServerDevice::onRequestFromSlot(WeakChannelPointer channel, const karabo::util::Hash& hash) {
            Hash failureInfo;
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequest";
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
            std::string addr = boost::lexical_cast<std::string>(tcpChannel->socket().remote_endpoint());

            // convert periods to underscores, so that this can be used as a Hash key...
            std::transform(addr.begin(), addr.end(), addr.begin(), [](char c){ return c == '.' ? '_' : c; });

            return addr;
        }

    }
}
