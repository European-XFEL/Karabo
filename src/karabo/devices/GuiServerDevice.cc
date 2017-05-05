/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "GuiServerDevice.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/net/EventLoop.hh"

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

            UINT32_ELEMENT(expected)
                    .key("port")
                    .displayedName("Hostport")
                    .description("Local port for this server")
                    .assignmentOptional().defaultValue(44444)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_GuiServer_0")
                    .commit();

            // Do not archive the archivers (would lead to infinite recursion)
            OVERWRITE_ELEMENT(expected).key("archive")
                    .setNewDefaultValue(false)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue<int>(Schema::AccessLevel::ADMIN)
                    .commit();

            // Slow beats on GuiServer
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
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

            INT32_ELEMENT(expected).key("inputChannelQueueCapacity")
                    .displayedName("Input Channel forwarding queue size")
                    .description("The number of messages to store in the forwarding ring buffer. NOTE: Will be applied to newly connected clients only")
                    .assignmentOptional().defaultValue(100) // 5Hz * 20 seconds
                    .reconfigurable()
                    .minExc(0).maxInc(5000)
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
                    .assignmentOptional().defaultValue(1500)
                    .reconfigurable()
                    .minExc(500).maxInc(5000)
                    .commit();

        }


        GuiServerDevice::GuiServerDevice(const Hash& config) : Device<>(config) {

            KARABO_INITIAL_FUNCTION(initialize)

            KARABO_SLOT4(slotNotification, string /*type*/, string /*shortMsg*/, string /*detailedMsg*/, string /*deviceId*/)
            KARABO_SLOT(slotLoggerMap, Hash /*loggerMap*/)
            KARABO_SLOT(slotAlarmSignalsUpdate, std::string, std::string, karabo::util::Hash );
            KARABO_SLOT(slotRunConfigSourcesUpdate, karabo::util::Hash, std::string);
            KARABO_SIGNAL("signalClientSignalsAlarmUpdate", Hash);
            KARABO_SIGNAL("signalClientRequestsAlarms");

            Hash h;
            h.set("port", config.get<unsigned int>("port"));
            h.set("type", "server");
            h.set("serializationType", "binary"); // Will lead to binary header hashes
            m_dataConnection = Connection::create("Tcp", h);
            m_serializer = BinarySerializer<Hash>::create("Bin"); // for reading

            m_loggerInput = config;
        }


        GuiServerDevice::~GuiServerDevice() {
            if (m_dataConnection) m_dataConnection->stop();
        }


        void GuiServerDevice::initialize() {

            try {

                trackAllInstances();

                // Protect clients from too frequent updates of a single property:
                remote().setDeviceMonitorInterval(get<int>("propertyUpdateInterval"));

                remote().getSystemInformation();

                // Register handlers
                remote().registerInstanceNewMonitor(boost::bind(&karabo::devices::GuiServerDevice::instanceNewHandler, this, _1));
                remote().registerInstanceUpdatedMonitor(boost::bind(&karabo::devices::GuiServerDevice::instanceUpdatedHandler, this, _1));
                remote().registerInstanceGoneMonitor(boost::bind(&karabo::devices::GuiServerDevice::instanceGoneHandler, this, _1, _2));
                remote().registerSchemaUpdatedMonitor(boost::bind(&karabo::devices::GuiServerDevice::schemaUpdatedHandler, this, _1, _2));
                remote().registerClassSchemaMonitor(boost::bind(&karabo::devices::GuiServerDevice::classSchemaHandler, this, _1, _2, _3));

                connect(karabo::util::DATALOGMANAGER_ID, "signalLoggerMap", "", "slotLoggerMap");
                requestNoWait(karabo::util::DATALOGMANAGER_ID, "slotGetLoggerMap", "", "slotLoggerMap");

                //scan topology to find additional alarm services
                const Hash& topology = remote().getSystemTopology();
                const boost::optional<const Hash::Node&> devices = topology.find("device");
                if (devices) {
                    const Hash& deviceEntry = devices->getValue<Hash>();
                    for (Hash::const_iterator it = deviceEntry.begin(); it != deviceEntry.end(); ++it) {
                        Hash topologyEntry; // prepare input for connectPotentialAlarmService
                        Hash::Node& hn = topologyEntry.set("device." + it->getKey(), it->getValue<Hash>());
                        hn.setAttributes(it->getAttributes());
                        connectPotentialAlarmService(topologyEntry);
                        registerPotentialProjectManager(topologyEntry);
                        connectPotentialRunConfigurator(topologyEntry);
                    }
                }


                m_dataConnection->startAsync(boost::bind(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));


                m_loggerConsumer = getConnection()->createConsumer(m_topic, "target = 'log'");
                m_loggerConsumer->readAsync(bind_weak(&karabo::devices::GuiServerDevice::logHandler, this, _1, _2));

                m_guiDebugProducer = getConnection()->createProducer();

                // Produce some information
                KARABO_LOG_INFO << "GUI Server is up and listening on port: " << get<unsigned int>("port");

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initialize(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::postReconfigure() {
            remote().setDeviceMonitorInterval(get<int>("propertyUpdateInterval"));

            // One might also want to react on possible changes of "delayOnInput",
            // i.e. change delay value for existing input channels.
            // For now, changing "delayOnInput" will only affect new InputChannels, i.e. _all_ GUI clients requesting
            // data of a specific output channel have to dis- and then reconnect to see the new delay.
        }

        void GuiServerDevice::onConnect(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel) {
            if (e) return;

            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Incoming connection";

                // Set 2 different queues for publishing (writeAsync) to the GUI client...
                // priority 3 bound to REJECT_OLDEST dropping policy
                channel->setAsyncChannelPolicy(REMOVE_OLDEST, "REMOVE_OLDEST", get<int>("inputChannelQueueCapacity"));
                // priority 4 should be LOSSLESS
                channel->setAsyncChannelPolicy(LOSSLESS, "LOSSLESS");

                channel->readAsyncHash(boost::bind(&karabo::devices::GuiServerDevice::onRead, this, _1, channel, _2));

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
                m_dataConnection->startAsync(boost::bind(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));


            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onConnect(): " << e.userFriendlyMsg();
                m_dataConnection->startAsync(boost::bind(&karabo::devices::GuiServerDevice::onConnect, this, _1, _2));
            }
        }


        void GuiServerDevice::registerConnect(const karabo::net::Channel::Pointer & channel) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = std::set<std::string > (); // maps channel to visible instances
        }


        void GuiServerDevice::onRead(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel, karabo::util::Hash& info) {
            if (e) {
                EventLoop::getIOService().post(bind_weak(&GuiServerDevice::onError, this, e, channel));
                return;
            }

            try {
                // GUI communication scenarios
                if (info.has("type")) {
                    string type = info.get<string > ("type");
                    if (type == "login") {
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
                channel->readAsyncHash(boost::bind(&karabo::devices::GuiServerDevice::onRead, this, _1, channel, _2));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRead(): " << e.userFriendlyMsg();
                channel->readAsyncHash(boost::bind(&karabo::devices::GuiServerDevice::onRead, this, _1, channel, _2));
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


        void GuiServerDevice::onLogin(karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash) {
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


        void GuiServerDevice::onInitDevice(Channel::Pointer channel, const karabo::util::Hash& hash) {
            try {

                const string& serverId = hash.get<string > ("serverId");
                const string& deviceId = hash.get<string > ("deviceId");
                KARABO_LOG_FRAMEWORK_DEBUG << "onInitDevice: request to start device instance \"" << deviceId << "\" on server \"" << serverId << "\"";
                request(serverId, "slotStartDevice", hash)
                        .receiveAsync<bool, string > (boost::bind(&karabo::devices::GuiServerDevice::initReply, this, channel, deviceId, hash, _1, _2));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onInitDevice(): " << e.userFriendlyMsg();
            }
            // NOTE: This sleep() is a rate limiter for device instantiations
            boost::this_thread::sleep(boost::posix_time::milliseconds(get<int>("waitInitDevice")));
        }


        void GuiServerDevice::initReply(Channel::Pointer channel, const string& givenDeviceId, const karabo::util::Hash& givenConfig, bool success, const string& message) {
            try {

                KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting init reply";

                Hash h("type", "initReply", "deviceId", givenDeviceId, "success", success, "message", message);
                safeClientWrite(channel, h);

                // Handle schema updates contained in the Hash supplied to onInitDevice
                if (success && givenConfig.has("schemaUpdates")) {
                    const string& createdDeviceId = message; // "message"" is the created deviceId when success is true.
                    const std::vector<Hash>& schemaUpdates = givenConfig.get<std::vector<Hash> >("schemaUpdates");

                    {
                        boost::mutex::scoped_lock(m_pendingAttributesMutex);
                        std::vector<Hash>& pendingUpdatesSlot = m_pendingAttributeUpdates[createdDeviceId];
                        pendingUpdatesSlot.insert(pendingUpdatesSlot.end(), schemaUpdates.begin(), schemaUpdates.end());
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initReply " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::safeClientWrite(const karabo::net::Channel::Pointer channel, const karabo::util::Hash& message, int prio) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            if (channel && channel->isOpen()) channel->writeAsync(message, prio);
        }


        void GuiServerDevice::safeAllClientsWrite(const karabo::util::Hash& message, int prio) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            for (ConstChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                if (it->first && it->first->isOpen()) it->first->writeAsync(message, prio);
            }
        }


        void GuiServerDevice::onGetDeviceConfiguration(karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceConfiguration";
                string deviceId = hash.get<string > ("deviceId");

                Hash config = remote().getConfigurationNoWait(deviceId);

                if (!config.empty()) {
                    // Can't we just use 'config' instead of 'remote().get(deviceId)'?
                    Hash h("type", "deviceConfiguration", "deviceId", deviceId, "configuration", remote().get(deviceId));
                    safeClientWrite(channel, h);
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetDeviceConfiguration(): " << e.userFriendlyMsg();
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


        void GuiServerDevice::onStartMonitoringDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                bool registerFlag = false;
                string deviceId = info.get<string > ("deviceId");

                KARABO_LOG_FRAMEWORK_DEBUG << "onStartMonitoringDevice : \"" << deviceId << "\"";

                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                    if (it != m_channels.end()) {
                        it->second.insert(deviceId);
                    }
                }

                {
                    boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                    // Increase count of device in visible devices map
                    m_monitoredDevices[deviceId]++;
                    if (m_monitoredDevices[deviceId] == 1) registerFlag = true;
                    KARABO_LOG_FRAMEWORK_DEBUG << "onStartMonitoringDevice " << deviceId << " (" << m_monitoredDevices[deviceId] << ")";
                }

                if (registerFlag) { // Fresh device on the shelf
                    remote().registerDeviceMonitor(deviceId, boost::bind(&karabo::devices::GuiServerDevice::deviceChangedHandler, this, _1, _2));
                }

                // Send back fresh information about device
                // TODO This could check a dirty-flag whether the device changed since last time seen
                onGetDeviceConfiguration(channel, info);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onStartMonitoringDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onStopMonitoringDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {

            try {
                bool unregisterFlag = false;
                const string& deviceId = info.get<string > ("deviceId");

                KARABO_LOG_FRAMEWORK_DEBUG << "onStopMonitoringDevice : \"" << deviceId << "\"";

                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                    if (it != m_channels.end()) it->second.erase(deviceId);
                }

                {
                    boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                    const int newCount = --m_monitoredDevices[deviceId]; // prefix decrement!
                    if (newCount <= 0) unregisterFlag = true;
                    KARABO_LOG_FRAMEWORK_DEBUG << "onStopMonitoringDevice " << deviceId << " (" << newCount << ")";
                }

                if (unregisterFlag) {
                    // Disconnect signal/slot from broker
                    remote().unregisterDeviceMonitor(deviceId);
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onStopMonitoringDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onGetClassSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
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


        void GuiServerDevice::onGetDeviceSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceSchema";
                const string& deviceId = info.get<string > ("deviceId");

                Schema schema = remote().getDeviceSchemaNoWait(deviceId);

                if (!schema.empty()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Schema available, direct answer";
                    Hash h("type", "deviceSchema", "deviceId", deviceId, "schema", schema);
                    safeClientWrite(channel, h);
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetDeviceSchema(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onGetPropertyHistory(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
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
                            .receiveAsync<string, string, vector<Hash> >(boost::bind(&karabo::devices::GuiServerDevice::propertyHistory, this, channel, _1, _2, _3));
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "onGetPropertyHistory: No '" << loggerId << "' in map.";
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetPropertyHistory(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::propertyHistory(karabo::net::Channel::Pointer channel, const std::string& deviceId, const std::string& property, const std::vector<karabo::util::Hash>& data) {
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


        void GuiServerDevice::onSubscribeNetwork(Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onSubscribeNetwork : channelName = \"" << info.get<string>("channelName") << "\" "
                        << (info.get<bool>("subscribe") ? "+" : "-");
                string channelName = info.get<string>("channelName");
                bool subscribe = info.get<bool>("subscribe");
                NetworkMap::iterator iter;
                boost::mutex::scoped_lock lock(m_networkMutex);

                for (iter = m_networkConnections.begin(); iter != m_networkConnections.end(); ++iter) {
                    if (channelName == iter->second.name) {
                        if (subscribe) {
                            if (channel == iter->second.channel) {
                                KARABO_LOG_FRAMEWORK_WARN << "Skip subscription to output channel '" << channelName
                                        << "' for GUI on channel " " : It is already subscribed for GUI on channel "
                                        << channel.get();
                                return;
                            }
                            NetworkConnection nc;
                            nc.name = channelName;
                            nc.channel = channel;
                            m_networkConnections.insert(NetworkMap::value_type(iter->first, nc));
                            return;
                        } else {
                            if (iter->second.channel == channel) {
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
                input->registerInputHandler(boost::bind(&GuiServerDevice::onNetworkData, this, _1));

                connectInputChannel(input); // asynchronous
                NetworkConnection nc;
                nc.name = channelName;
                nc.channel = channel;
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
                        safeClientWrite(range.first->second.channel, h, REMOVE_OLDEST);
                    }
                }
            } catch (const Exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onNetworkData: " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::sendSystemVersion(karabo::net::Channel::Pointer channel) {
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


        void GuiServerDevice::sendSystemTopology(karabo::net::Channel::Pointer channel) {
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
                        remote().registerDeviceMonitor(instanceId, boost::bind(&karabo::devices::GuiServerDevice::deviceChangedHandler, this, _1, _2));
                    }
                    if (instanceId == karabo::util::DATALOGMANAGER_ID) {
                        // The corresponding 'connect' is done by SignalSlotable's automatic reconnect feature.
                        // Even this request might not be needed since the logger manager emits the corresponding signal.
                        // But we cannot be 100% sure that our 'connect' has been registered in time.
                        requestNoWait(karabo::util::DATALOGMANAGER_ID, "slotGetLoggerMap", "", "slotLoggerMap");
                    }

                    updateNewInstanceAttributes(instanceId);

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
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting instance updated";
                Hash h("type", "instanceUpdated", "topologyEntry", topologyEntry);
                safeAllClientsWrite(h);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in instanceUpdatedHandler(): " << e.userFriendlyMsg();
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

                //KARABO_LOG_FRAMEWORK_DEBUG << "deviceChangedHandler" << ": deviceId = \"" << deviceId << "\"";

                Hash h("type", "deviceConfiguration", "deviceId", deviceId, "configuration", what);
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                for (ConstChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    // Optimization: broadcast only to visible DeviceInstances
                    if (it->second.find(deviceId) != it->second.end()) {
                        if (it->first && it->first->isOpen()) it->first->writeAsync(h);
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in deviceChangedHandler(): " << e.userFriendlyMsg();
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
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting schema updated";

                if (schema.empty()) {
                    KARABO_LOG_FRAMEWORK_WARN << "Going to send an empty schema, should not happen...";
                }

                Hash h("type", "deviceSchema", "deviceId", deviceId,
                       "schema", schema);

                // Broadcast to all GUIs
                safeAllClientsWrite(h);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in schemaUpdatedHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::slotNotification(const std::string& type, const std::string& shortMessage, const std::string& detailedMessage, const std::string & deviceId) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting notification";
                Hash h("type", "notification", "deviceId", deviceId,
                       "messageType", type, "shortMsg", shortMessage,
                       "detailedMsg", detailedMessage);

                // Broadcast to all GUIs
                safeAllClientsWrite(h);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in slotNotification(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::logHandler(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body) {
            try {
                Hash h("type", "log", "messages", body->get<std::vector<util::Hash> >("messages"));
                // Broadcast to all GUIs
                safeAllClientsWrite(h, REMOVE_OLDEST);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in logHandler(): " << e.userFriendlyMsg();
            }

            m_loggerConsumer->readAsync(bind_weak(&karabo::devices::GuiServerDevice::logHandler, this, _1, _2));
        }


        void GuiServerDevice::onError(const karabo::net::ErrorCode& errorCode, karabo::net::Channel::Pointer channel) {
            try {

                KARABO_LOG_INFO << "onError : TCP socket got error : " << errorCode.value() << " -- \"" << errorCode.message() << "\",  Close connection to a client";

                // TODO Fork on error message
                std::set<std::string> deviceIds; // empty set
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                    if (it != m_channels.end()) {
                        it->first->close(); // This closes socket and unregisters channel from connection
                        deviceIds.swap(it->second); // copy to the empty set
                        // Remove channel as such
                        m_channels.erase(it);
                    }
                    KARABO_LOG_FRAMEWORK_INFO << m_channels.size() << " client(s) left.";
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
                            ++jt;
                        }
                    }
                }
                // All devices left in deviceIds have to be unregistered from monitoring.


                BOOST_FOREACH(const std::string& deviceId, deviceIds) {
                    remote().unregisterDeviceMonitor(deviceId);
                }

                {
                    boost::mutex::scoped_lock lock(m_networkMutex);

                    NetworkMap::iterator iter = m_networkConnections.begin();
                    while (iter != m_networkConnections.end()) {
                        if (iter->second.channel == channel) {
                            m_networkConnections.erase(iter++);
                        } else ++iter;
                    }
                    KARABO_LOG_FRAMEWORK_INFO << m_networkConnections.size() << " p2p channel(s) left.";
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onError(): " << e.userFriendlyMsg();
            } catch (const std::exception& se) {
                KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in onError(): " << se.what();
            }
        }


        void GuiServerDevice::slotLoggerMap(const karabo::util::Hash& loggerMap) {
            m_loggerMap = loggerMap;
        }


        void GuiServerDevice::updateNewInstanceAttributes(const std::string& deviceId) {
            boost::mutex::scoped_lock(m_pendingAttributesMutex);
            const auto it = m_pendingAttributeUpdates.find(deviceId);

            if (it != m_pendingAttributeUpdates.end()) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Updating schema attributes of device: " << deviceId;

                request(deviceId, "slotUpdateSchemaAttributes", it->second);
                m_pendingAttributeUpdates.erase(it);
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


        void GuiServerDevice::onAcknowledgeAlarm(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onAcknowledgeAlarm : info ...\n" << info;
                const std::string& alarmServiceId = info.get<std::string>("alarmInstanceId");
                call(alarmServiceId, "slotAcknowledgeAlarm", info.get<Hash>("acknowledgedRows"));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onAcknowledgeAlarm(): " << e.userFriendlyMsg();
            }
        };


        void GuiServerDevice::onRequestAlarms(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestAlarms : info ...\n" << info;

                const std::string& requestedInstance = info.get<std::string>("alarmInstanceId");
                request(requestedInstance, "slotRequestAlarmDump")
                        .receiveAsync<karabo::util::Hash>(boost::bind(&GuiServerDevice::onRequestedAlarmsReply, this, channel, _1));

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestAlarms(): " << e.userFriendlyMsg();
            }
        };


        void GuiServerDevice::onRequestedAlarmsReply(karabo::net::Channel::Pointer channel, const karabo::util::Hash& reply) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestedAlarmsReply : info ...\n" << reply;
                Hash h("type", "alarmInit", "instanceId", reply.get<std::string>("instanceId"), "rows", reply.get<Hash>("alarms"));
                if (channel) {//write to a single channel
                    channel->writeAsync(h, LOSSLESS);
                } else { //write to all
                    safeAllClientsWrite(h);
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestedAlarmsReply(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onUpdateAttributes(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onUpdateAttributes : info ...\n" << info;
                const std::string& instanceId = info.get<std::string>("instanceId");
                const std::vector<Hash>& updates = info.get<std::vector<Hash> >("updates");
                request(instanceId, "slotUpdateSchemaAttributes", updates)
                        .receiveAsync<Hash>(boost::bind(&GuiServerDevice::onRequestedAttributeUpdate, this, channel, _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onUpdateAttributes(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onRequestedAttributeUpdate(karabo::net::Channel::Pointer channel, const karabo::util::Hash& reply) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRequestedAttributeUpdate : success ...\n" << reply.get<bool>("success");
                Hash h("type", "attributesUpdated", "reply", reply);
                channel->writeAsync(h, LOSSLESS);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRequestedAttributeUpdate(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::connectPotentialAlarmService(const karabo::util::Hash& topologyEntry) {
            std::string type, instanceId;
            typeAndInstanceFromTopology(topologyEntry, type, instanceId);
            if (topologyEntry.get<Hash>(type).begin()->hasAttribute("classId") &&
                topologyEntry.get<Hash>(type).begin()->getAttribute<std::string>("classId") == "AlarmService") {
                connect(instanceId, "signalAlarmServiceUpdate", "", "slotAlarmSignalsUpdate");
                // actively ask this previously unknown device to submit its alarms as init messages on all channesl
                onRequestAlarms(karabo::net::Channel::Pointer(), Hash("alarmInstanceId", instanceId));
            }

        }


        void GuiServerDevice::connectPotentialRunConfigurator(const karabo::util::Hash& topologyEntry) {
            std::string type, instanceId;
            typeAndInstanceFromTopology(topologyEntry, type, instanceId);
            if (topologyEntry.get<Hash>(type).begin()->hasAttribute("classId") &&
                topologyEntry.get<Hash>(type).begin()->getAttribute<std::string>("classId") == "RunConfigurator") {
                connect(instanceId, "signalGroupSourceChanged", "", "slotRunConfigSourcesUpdate");
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


        void GuiServerDevice::onProjectBeginUserSession(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectBeginUserSession : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectBeginUserSession")) return;
                const std::string& token = info.get<std::string>("token");
                request(projectManager, "slotBeginUserSession", token)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectBeginUserSession", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectBeginUserSession(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectEndUserSession(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectEndUserSession : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectEndUserSession")) return;
                const std::string& token = info.get<std::string>("token");
                request(projectManager, "slotEndUserSession", token)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectEndUserSession", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectEndUserSession(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectSaveItems(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectSaveItems : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectSaveItems")) return;
                const std::string& token = info.get<std::string>("token");
                const std::vector<Hash>& items = info.get<std::vector<Hash> >("items");
                request(projectManager, "slotSaveItems", token, items)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectSaveItems", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectSaveItems(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectLoadItems(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectLoadItems : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectLoadItems")) return;
                const std::string& token = info.get<std::string>("token");
                const std::vector<Hash>& items = info.get<std::vector<Hash> >("items");
                request(projectManager, "slotLoadItems", token, items)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectLoadItems", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectLoadItems(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectListProjectManagers(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                Hash h("type", "projectListProjectManagers", "reply", getKnownProjectManagers());
                channel->writeAsync(h, LOSSLESS);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectListProjectManagers(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectListItems(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectListItems : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectListItems")) return;
                const std::string& token = info.get<std::string>("token");
                const std::string& domain = info.get<std::string>("domain");
                const std::vector<std::string>& item_types = info.get<std::vector < std::string >> ("item_types");
                request(projectManager, "slotListItems", token, domain, item_types)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectListItems", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectListItems(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectListDomains(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectListDomains : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectListDomains")) return;
                const std::string& token = info.get<std::string>("token");
                request(projectManager, "slotListDomains", token)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectListDomains", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectListDomains(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onProjectUpdateAttribute(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onProjectUpdateAttribute : info ...\n" << info;
                const std::string& projectManager = info.get<std::string>("projectManager");
                if (!checkProjectManagerId(channel, projectManager, "projectUpdateAttribute")) return;
                const std::string& token = info.get<std::string>("token");
                const std::vector<Hash>& items = info.get<std::vector<Hash> >("items");
                request(projectManager, "slotUpdateAttribute", token, items)
                        .receiveAsync<Hash>(util::bind_weak(&GuiServerDevice::forwardReply, this, channel, "projectUpdateAttribute", _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onProjectUpdateAttribute(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::forwardReply(karabo::net::Channel::Pointer channel, const std::string& replyType,
                                           /*const karabo::net::ErrorCode& e,*/const karabo::util::Hash& reply) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "forwardReply : " << replyType;
                Hash h("type", replyType, "reply", reply);
                channel->writeAsync(h, LOSSLESS);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in forwarding reply of type '" << replyType << "': " << e;
            }
        }


        bool GuiServerDevice::checkProjectManagerId(karabo::net::Channel::Pointer channel, const std::string& deviceId, const std::string & type) {
            boost::shared_lock<boost::shared_mutex> lk(m_projectManagerMutex);
            if (m_projectManagers.find(deviceId) != m_projectManagers.end()) return true;

            Hash h("type", type, "reply", Hash("success", false, "reason", "Project manager doesn't exist"));
            channel->writeAsync(h, LOSSLESS);
            return false;

        }


        void GuiServerDevice::onRunConfigSourcesInGroup(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
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

    }
}
