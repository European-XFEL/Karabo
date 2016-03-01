/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "GuiServerDevice.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::io;
using namespace karabo::xip;
using namespace karabo::xms;

#define DATALOGGER_PREFIX "DataLogger-"
#define DATALOGREADER_PREFIX "DataLogReader"
#define DATALOGREADERS_PER_SERVER 2

namespace karabo {
    namespace core {

        
        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, GuiServerDevice)

        void GuiServerDevice::expectedParameters(Schema& expected) {

            UINT32_ELEMENT(expected)
                    .key("port")
                    .displayedName("Hostport")
                    .description("Local port for this server")
                    .assignmentOptional().defaultValue(44444)
                    .commit();

            CHOICE_ELEMENT(expected)
                    .key("loggerConnection")
                    .displayedName("Logger Connection")
                    .description("Configuration of the connection for the distributed logging system")
                    .appendNodesOfConfigurationBase<BrokerConnection>()
                    .assignmentOptional().defaultValue("Jms")
                    .commit();

            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_GuiServer_0")
                    .commit();

            // Do not archive the archivers (would lead to infinite recursion)
            OVERWRITE_ELEMENT(expected).key("archive")
                    .setNewDefaultValue(false)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(4)
                    .commit();

            // Slow beats on GuiServer
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();

            INT32_ELEMENT(expected).key("delayOnInput")
                    .displayedName("Delay on Input channel")
                    .description("Some delay before informing output channel about readiness for next data.")
                    .assignmentOptional().defaultValue(500)
                    .unit(Unit::SECOND)
                    .metricPrefix(MetricPrefix::MILLI)
                    .init()
                    .commit();

            INT32_ELEMENT(expected).key("propertyUpdateInterval")
                    .displayedName("Property update interval")
                    .description("Minimum interval between subsequent property updates forwarded to clients.")
                    .unit(Unit::SECOND).metricPrefix(MetricPrefix::MILLI)
                    .assignmentOptional().defaultValue(500)
                    .minExc(100).maxInc(1000) // not faster than 10 Hz, 1 Hz is already slow
                    .init()
                    .commit();

        }


        GuiServerDevice::GuiServerDevice(const Hash& config) : Device<>(config) {

            KARABO_INITIAL_FUNCTION(initialize)

            KARABO_SLOT4(slotNotification, string /*type*/, string /*shortMsg*/, string /*detailedMsg*/, string /*deviceId*/)
            KARABO_SLOT(slotLoggerMap, Hash /*loggerMap*/)

            Hash h;
            h.set("port", config.get<unsigned int>("port"));
            h.set("type", "server");
            h.set("serializationType", "binary"); // Will lead to binary header hashes
            m_dataConnection = Connection::create("Tcp", h);
            m_ioService = m_dataConnection->getIOService();
            m_serializer = BinarySerializer<Hash>::create("Bin"); // for reading
            
            m_loggerInput = config;
        }


        GuiServerDevice::~GuiServerDevice() {
            m_ioService->stop();
            m_dataConnection->stop();
            m_loggerIoService->stop();
        }


        void GuiServerDevice::initialize() {
            // Inherit from device connection settings
            string hostname = getConnection()->getBrokerHostname();
            unsigned int port = getConnection()->getBrokerPort();
            const vector<string>& brokers = getConnection()->getBrokerHosts();
            string host = hostname + ":" + toString(port);


            m_loggerInput.set("loggerConnection.Jms.hostname", host);
            m_loggerInput.set("loggerConnection.Jms.port", port);
            m_loggerInput.set("loggerConnection.Jms.brokerHosts", brokers);

            m_loggerConnection = BrokerConnection::createChoice("loggerConnection", m_loggerInput);
            m_loggerIoService = m_loggerConnection->getIOService();

            // This creates a connection in order to forward exceptions happened in the GUI
            m_guiDebugConnection = BrokerConnection::create("Jms", Hash("destinationName", "karaboGuiDebug",
                                                                        "hostname", host,
                                                                        "port", port,
                                                                        "brokerHosts", brokers));
            try {

                trackAllInstances();

                // Protect clients from too frequent updates of a single property:
                remote().setDeviceMonitorInterval(this->get<int>("propertyUpdateInterval"));

                remote().getSystemInformation();

                // Register handlers
                remote().registerInstanceNewMonitor(boost::bind(&karabo::core::GuiServerDevice::instanceNewHandler, this, _1));
                remote().registerInstanceUpdatedMonitor(boost::bind(&karabo::core::GuiServerDevice::instanceUpdatedHandler, this, _1));
                remote().registerInstanceGoneMonitor(boost::bind(&karabo::core::GuiServerDevice::instanceGoneHandler, this, _1, _2));
                remote().registerSchemaUpdatedMonitor(boost::bind(&karabo::core::GuiServerDevice::schemaUpdatedHandler, this, _1, _2));
                remote().registerClassSchemaMonitor(boost::bind(&karabo::core::GuiServerDevice::classSchemaHandler, this, _1, _2, _3));

                connect("Karabo_DataLoggerManager_0", "signalLoggerMap", "", "slotLoggerMap", RECONNECT, true);
                requestNoWait("Karabo_DataLoggerManager_0", "slotGetLoggerMap", "", "slotLoggerMap");

                m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));
                // Use one thread currently (you may start this multiple time for having more threads doing the work)
                boost::thread(boost::bind(&karabo::net::IOService::run, m_ioService));

                // Start the logging thread
                m_loggerConnection->start();
                m_loggerChannel = m_loggerConnection->createChannel();
                m_loggerChannel->setFilter("target = 'log'");
                m_loggerChannel->readAsyncHashHash(boost::bind(&karabo::core::GuiServerDevice::logHandler, this, _1, _2, _3));
                boost::thread(boost::bind(&karabo::net::BrokerIOService::work, m_loggerIoService));

                // Start the guiDebugChannel
                m_guiDebugConnection->start();
                m_guiDebugChannel = m_guiDebugConnection->createChannel();

                // Produce some information
                KARABO_LOG_INFO << "GUI Server is up and listening on port: " << get<unsigned int>("port");

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in initialize(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onConnect(karabo::net::Channel::Pointer channel) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Incoming connection";
                
                // Set 2 different queues for publishing (writeAsync) to the GUI client...
                // priority 3 bound to REJECT_OLDEST dropping policy
                channel->setAsyncChannelPolicy(REMOVE_OLDEST, "REMOVE_OLDEST");
                // priority 4 should be LOSSLESS
                channel->setAsyncChannelPolicy(LOSSLESS, "LOSSLESS");
                
                channel->readAsyncHash(boost::bind(&karabo::core::GuiServerDevice::onRead, this, channel, _1));
                channel->setErrorHandler(boost::bind(&karabo::core::GuiServerDevice::onError, this, channel, _1));


                Hash brokerInfo("type", "brokerInformation");
                brokerInfo.set("host", this->getConnection()->getBrokerHostname());
                brokerInfo.set("port", this->getConnection()->getBrokerPort());
                brokerInfo.set("topic", this->getConnection()->getBrokerTopic());
                channel->writeAsync(brokerInfo);

                // Register channel
                registerConnect(channel);

                // Re-register acceptor socket (allows handling multiple clients)
                m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));


            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onConnect(): " << e.userFriendlyMsg();
                m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));
            }
        }


        void GuiServerDevice::registerConnect(const karabo::net::Channel::Pointer & channel) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = std::set<std::string > (); // maps channel to visible instances
        }


        void GuiServerDevice::onRead(karabo::net::Channel::Pointer channel, karabo::util::Hash& info) {
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
                    } else if (type == "getAvailableProjects") {
                        onGetAvailableProjects(channel);
                    } else if (type == "newProject") {
                        onNewProject(channel, info);
                    } else if (type == "loadProject") {
                        onLoadProject(channel, info);
                    } else if (type == "saveProject") {
                        onSaveProject(channel, info);
                    } else if (type == "closeProject") {
                        onCloseProject(channel, info);
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "Ignoring request";
                }
                channel->readAsyncHash(boost::bind(&karabo::core::GuiServerDevice::onRead, this, channel, _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onRead(): " << e.userFriendlyMsg();
                channel->readAsyncHash(boost::bind(&karabo::core::GuiServerDevice::onRead, this, channel, _1));
            }
        }


        void GuiServerDevice::onGuiError(const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGuiError";
                m_guiDebugChannel->write(Hash()/*empty header*/, hash);

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
                    if ((major >= 1 && minor >= 5) || major >=2) sendSystemVersion(channel);
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

                KARABO_LOG_FRAMEWORK_DEBUG << "onInitDevice";
                //cout << hash << endl;
                string serverId = hash.get<string > ("serverId");
                string deviceId = hash.get<string > ("deviceId");
                KARABO_LOG_INFO << "Incoming request to start device instance \"" <<  deviceId << "\" on server \"" << serverId << "\"";
                request(serverId, "slotStartDevice", hash)
                        .receiveAsync<bool, string > (boost::bind(&karabo::core::GuiServerDevice::initReply, this, channel, hash.get<string>("deviceId"), _1, _2));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onInitDevice(): " << e.userFriendlyMsg();
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
        }


        void GuiServerDevice::initReply(Channel::Pointer channel, const string& deviceId, bool success, const string& message) {
            try {

                KARABO_LOG_FRAMEWORK_DEBUG << "Unicasting init reply";

                Hash h("type", "initReply", "deviceId", deviceId, "success", success, "message", message);
                safeClientWrite(channel, h);

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
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillServer";
                string serverId = info.get<string > ("serverId");
                // TODO Supply user specific context
                call(serverId, "slotKillServer");
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onKillServer(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onKillDevice(const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillDevice";
                string deviceId = info.get<string > ("deviceId");
                call(deviceId, "slotKillDevice");
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onKillDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onStartMonitoringDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                bool registerFlag = false;
                string deviceId = info.get<string > ("deviceId");

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
                    remote().registerDeviceMonitor(deviceId, boost::bind(&karabo::core::GuiServerDevice::deviceChangedHandler, this, _1, _2));
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
                string deviceId = info.get<string > ("deviceId");

                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                    if (it != m_channels.end()) it->second.erase(deviceId);
                }

                {
                    boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                    m_monitoredDevices[deviceId]--;
                    if (m_monitoredDevices[deviceId] == 0) unregisterFlag = true;
                    KARABO_LOG_FRAMEWORK_DEBUG << "onStopMonitoringDevice " << deviceId << " (" << m_monitoredDevices[deviceId] << ")";
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
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetClassSchema";
                string serverId = info.get<string > ("serverId");
                string classId = info.get<string> ("classId");
                Schema schema = remote().getClassSchemaNoWait(serverId, classId);
                if (!schema.empty()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Schema available, direct answer";
                    Hash h("type", "classSchema", "serverId", serverId,
                           "classId", classId, "schema", schema);
                    safeClientWrite(channel, h);
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetClassSchema(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onGetDeviceSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceSchema";
                string deviceId = info.get<string > ("deviceId");

                Schema schema = remote().getDeviceSchemaNoWait(deviceId);
                Hash config = remote().getConfigurationNoWait(deviceId);

                if (!schema.empty()) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Schema available, direct answer";
                    Hash h("type", "deviceSchema", "deviceId", deviceId,
                           "schema", schema);
                    if (!config.empty()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Adding configuration, too";
                        h.set("configuration", config);
                    }
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
                            .receiveAsync<string, string, vector<Hash> >(boost::bind(&karabo::core::GuiServerDevice::propertyHistory, this, channel, _1, _2, _3));
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
                KARABO_LOG_FRAMEWORK_DEBUG << "onSubscribeNetwork:\n" << info;
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

                // TODO: This is a synchronous connect (using request-receive-with-timeout) and it may take ages
                // attempting to connect to non-existing device ... 
                // But it may fail if network or broker is so busy that cannot reply in time ...
                // ... maybe we need asynchronous connect here
                connectInputChannel(input, 1); // 1 attempt
                NetworkConnection nc;
                nc.name = channelName;
                nc.channel = channel;
                m_networkConnections.insert(NetworkMap::value_type(input, nc));

                //connectInputChannelAsync(input, boost::bind(&GuiServerDevice::onInputChannelConnected, this, input, channel, channelName));
            } catch (const Exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onSubscribeNetwork(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onInputChannelConnected(const InputChannel::Pointer& input, const Channel::Pointer& channel, const std::string& channelName) {
            boost::mutex::scoped_lock lock(m_networkMutex);
            // TODO: Revise the code below ... avoid double insertion because of asynchrony.
            NetworkConnection nc;
            nc.name = channelName;
            nc.channel = channel;

            m_networkConnections.insert(NetworkMap::value_type(input, nc));
        }


        void GuiServerDevice::onNetworkData(const InputChannel::Pointer& input) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onNetworkData";

                for (size_t i = 0; i < input->size(); ++i) {
                    Hash::Pointer data = input->read(i);

                    boost::mutex::scoped_lock lock(m_networkMutex);
                    pair<NetworkMap::iterator, NetworkMap::iterator> range = m_networkConnections.equal_range(input);
                    for (; range.first != range.second; range.first++) {
                        Hash h("type", "networkData", "name", range.first->second.name, "data", data);
                        //cout << h << endl;
                        safeClientWrite(range.first->second.channel, h, REMOVE_OLDEST);
                    }
                }
                input->update();
            } catch (const Exception &e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onNetworkData: " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onGetAvailableProjects(karabo::net::Channel::Pointer channel) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetAvailableProjects";

                request("Karabo_ProjectManager", "slotGetAvailableProjects")
                        .receiveAsync<karabo::util::Hash >(boost::bind(&karabo::core::GuiServerDevice::availableProjects, this, channel, _1));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onGetAvailableProjects(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::availableProjects(karabo::net::Channel::Pointer channel, const karabo::util::Hash& projects) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting available projects";
                KARABO_LOG_FRAMEWORK_DEBUG << projects;
                Hash h("type", "availableProjects", "availableProjects", projects);
                safeClientWrite(channel, h);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in availableProjects " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onNewProject(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onNewProject";

                string author = info.get<string > ("author");
                string projectName = info.get<string > ("name");
                vector<char> data = info.get<vector<char> > ("data");

                request("Karabo_ProjectManager", "slotNewProject", author, projectName, data)
                        .receiveAsync<string, bool, vector<char> >(boost::bind(&karabo::core::GuiServerDevice::projectNew, this, channel, _1, _2, _3));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onNewProject(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::projectNew(karabo::net::Channel::Pointer channel,
                const std::string& projectName,
                bool success,
                const std::vector<char>& data) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "projectNew " << projectName;

                Hash h("type", "projectNew");
                h.set("name", projectName);
                h.set("success", success);
                h.set("data", data);
                safeClientWrite(channel, h);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in projectNew(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onLoadProject(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onLoadProject";

                string userName = info.get<string > ("user");
                string projectName = info.get<string > ("name");

                request("Karabo_ProjectManager", "slotLoadProject", userName, projectName)
                        .receiveAsync<string, Hash, vector<char> >(boost::bind(&karabo::core::GuiServerDevice::projectLoaded, this, channel, _1, _2, _3));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onLoadProject(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::projectLoaded(karabo::net::Channel::Pointer channel, const std::string& projectName,
                const karabo::util::Hash& metaData, const std::vector<char>& data) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "projectLoaded";

                Hash h("type", "projectLoaded");
                h.set("name", projectName);
                h.set("metaData", metaData);
                h.set("buffer", data);
                safeClientWrite(channel, h);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in projectLoaded: " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onSaveProject(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onSaveProject";

                string userName = info.get<string > ("user");
                string projectName = info.get<string > ("name");
                vector<char> data = info.get<vector<char> > ("data");

                request("Karabo_ProjectManager", "slotSaveProject", userName, projectName, data)
                        .receiveAsync<string, bool, vector<char> >(boost::bind(&karabo::core::GuiServerDevice::projectSaved, this, channel, _1, _2, _3));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onSaveProject(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::projectSaved(karabo::net::Channel::Pointer channel,
                const std::string& projectName,
                bool success,
                const std::vector<char>& data) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "projectSaved " << projectName;

                Hash h("type", "projectSaved");
                h.set("name", projectName);
                h.set("success", success);
                h.set("data", data);
                safeClientWrite(channel, h);
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in projectSaved(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onCloseProject(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onCloseProject";

                string userName = info.get<string > ("user");
                string projectName = info.get<string > ("name");

                request("Karabo_ProjectManager", "slotCloseProject", userName, projectName)
                        .receiveAsync<string, bool, vector<char> >(boost::bind(&karabo::core::GuiServerDevice::projectClosed, this, channel, _1, _2, _3));
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onCloseProject(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::projectClosed(karabo::net::Channel::Pointer channel,
                const std::string& projectName,
                bool success,
                const std::vector<char>& data) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "projectClosed " << projectName;
                Hash h("type", "projectClosed");
                h.set("name", projectName);
                h.set("success", success);
                h.set("data", data);
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                    if (it != m_channels.end()) {
                        channel->writeAsync(h); // send back to requestor
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in projectClosed(): " << e.userFriendlyMsg();
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
            try {

                const std::string& type = topologyEntry.begin()->getKey(); // fails if empty...
                // const ref is fine even for temporary std::string
                const std::string& instanceId = (topologyEntry.has(type) && topologyEntry.is<Hash>(type) ?
                                                 topologyEntry.get<Hash>(type).begin()->getKey() : std::string("?"));
                KARABO_LOG_FRAMEWORK_INFO << "instanceNewHandler --> instanceId: '" << instanceId
                        << "', type: '" << type << "'";


                Hash h("type", "instanceNew", "topologyEntry", topologyEntry);
                safeAllClientsWrite(h);

                // TODO let device client return also deviceId as first argument
                if (topologyEntry.has("device")) {
                    string deviceId = topologyEntry.get<Hash>("device").begin()->getKey();
                    // Check whether someone already noted interest in this deviceId
                    if (m_monitoredDevices.find(deviceId) != m_monitoredDevices.end()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Connecting to device " << deviceId << " which is going to be visible in a GUI client";
                        remote().registerDeviceMonitor(deviceId, boost::bind(&karabo::core::GuiServerDevice::deviceChangedHandler, this, _1, _2));
                    }
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


        void GuiServerDevice::logHandler(karabo::net::BrokerChannel::Pointer channel,
                const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& data) {
            try {
                Hash h("type", "log", "messages", data->get<std::vector<util::Hash> >("messages"));
                // Broadcast to all GUIs
                safeAllClientsWrite(h, REMOVE_OLDEST);

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in logHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onError(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& errorCode) {
            try {
                if (errorCode.value() != 2 && errorCode.value() != 32 && errorCode.value() != 104) {
                    // NOT End of file or Broken pipe: an unknown reason why connection to GUI client stopped.
                    KARABO_LOG_ERROR << "Tcp channel error, code: " << errorCode.value() << ", message: " << errorCode.message();
                    return;
                }

                KARABO_LOG_INFO << "TCP socket got \"" << errorCode.message() << "\", client dropped the connection";

                // TODO Fork on error message
                std::set<std::string> deviceIds; // empty set
                {
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                    if (it != m_channels.end()) {
                        it->first->close(); // This closes socket and unregisters channel from connection
                        deviceIds = it->second; // copy to the empty set
                        // Remove channel as such
                        m_channels.erase(it);
                    }
                    KARABO_LOG_FRAMEWORK_INFO << m_channels.size() << " client(s) left.";
                }

                {
                    boost::mutex::scoped_lock lock(m_monitoredDevicesMutex);
                    // Remove all previously visible devices
                    for (std::set<std::string>::const_iterator jt = deviceIds.begin(); jt != deviceIds.end(); jt++) {
                        const std::string& deviceId = *jt;
                        m_monitoredDevices[deviceId]--;
                        KARABO_LOG_FRAMEWORK_DEBUG << "stopMonitoringDevice (GUI gone) " << deviceId << " " << m_monitoredDevices[deviceId];
                        if (m_monitoredDevices[deviceId] == 0) {
                            // Disconnect signal/slot from broker
                            remote().unregisterDeviceMonitor(deviceId);
                        }
                    }
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
    }
}
