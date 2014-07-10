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

namespace karabo {
    namespace core {


        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, GuiServerDevice)

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

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();

            // Slow beats on GuiServer
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();
        }


        GuiServerDevice::GuiServerDevice(const Hash& input) : Device<OkErrorFsm>(input) {
           
            GLOBAL_SLOT4(slotNotification, string /*type*/, string /*shortMsg*/, string /*detailedMsg*/, string /*deviceId*/)
            SLOT3(slotPropertyHistory, string /*deviceId*/, string /*property*/, vector<Hash> /*data*/)

            Hash config;
            config.set("port", input.get<unsigned int>("port"));
            config.set("type", "server");
            config.set("serializationType", "binary"); // Will lead to binary header hashes
            m_dataConnection = Connection::create("Tcp", config);
            m_ioService = m_dataConnection->getIOService();
            m_serializer = BinarySerializer<Hash>::create("Bin"); // for reading      

            m_loggerConnection = BrokerConnection::createChoice("loggerConnection", input);
            m_loggerIoService = m_loggerConnection->getIOService();

            // This creates a connection in order to forward exceptions happened in the GUI
            m_guiDebugConnection = BrokerConnection::create("Jms", Hash("destinationName", "karaboGuiDebug"));
        }


        GuiServerDevice::~GuiServerDevice() {
            m_ioService->stop();
            m_dataConnection->stop();
            m_loggerIoService->stop();
        }


        void GuiServerDevice::okStateOnEntry() {
            try {
                // Register handlers
                remote().registerInstanceNewMonitor(boost::bind(&karabo::core::GuiServerDevice::instanceNewHandler, this, _1));
                remote().registerInstanceUpdatedMonitor(boost::bind(&karabo::core::GuiServerDevice::instanceUpdatedHandler, this, _1));
                remote().registerInstanceGoneMonitor(boost::bind(&karabo::core::GuiServerDevice::instanceGoneHandler, this, _1, _2));
                remote().registerSchemaUpdatedMonitor(boost::bind(&karabo::core::GuiServerDevice::schemaUpdatedHandler, this, _1, _2));
                remote().registerClassSchemaMonitor(boost::bind(&karabo::core::GuiServerDevice::classSchemaHandler, this, _1, _2, _3));

                // Connect the history slot
                connect("Karabo_FileDataLogger_0", "signalPropertyHistory", "", "slotPropertyHistory");

                m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));
                // Use one thread currently (you may start this multiple time for having more threads doing the work)
                boost::thread(boost::bind(&karabo::net::IOService::run, m_ioService));

                // Start the logging thread
                m_loggerChannel = m_loggerConnection->start();
                m_loggerChannel->setFilter("target = 'log'");
                m_loggerChannel->readAsyncStringHash(boost::bind(&karabo::core::GuiServerDevice::logHandler, this, _1, _2, _3));
                boost::thread(boost::bind(&karabo::net::BrokerIOService::work, m_loggerIoService));

                // Start the guiDebugChannel
                m_guiDebugChannel = m_guiDebugConnection->start();

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in okStateOnEntry(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onConnect(karabo::net::Channel::Pointer channel) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Incoming connection";
                channel->readAsyncHash(boost::bind(&karabo::core::GuiServerDevice::onRead, this, _1, _2));
                channel->setErrorHandler(boost::bind(&karabo::core::GuiServerDevice::onError, this, _1, _2));
                // Re-register acceptor socket (allows handling multiple clients)
                m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));
                registerConnect(channel);

                Hash brokerInfo("type", "brokerInformation");
                brokerInfo.set("host", this->getConnection()->getBrokerHostname());
                brokerInfo.set("port", this->getConnection()->getBrokerPort());
                brokerInfo.set("topic", this->getConnection()->getBrokerTopic());
                channel->write(brokerInfo);

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onConnect(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::registerConnect(const karabo::net::Channel::Pointer & channel) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = std::set<std::string > (); // maps channel to visible instances
        }


        void GuiServerDevice::onRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
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
                    } else if (type == "initDevice") {
                        onInitDevice(info);
                    } else if (type == "refreshInstance") {
                        onRefreshInstance(channel, info);
                    } else if (type == "killServer") {
                        onKillServer(info);
                    } else if (type == "killDevice") {
                        onKillDevice(info);
                    } else if (type == "newVisibleDevice") {
                        onNewVisibleDevice(channel, info);
                    } else if (type == "removeVisibleDevice") {
                        onRemoveVisibleDevice(channel, info);
                    } else if (type == "getClassSchema") {
                        onGetClassSchema(channel, info);
                    } else if (type == "getDeviceSchema") {
                        onGetDeviceSchema(channel, info);
                    } else if (type == "getFromPast") {
                        onGetFromPast(channel, info);
                    } else if (type == "error") {
                        onGuiError(info);
                    }
                } else {
                    KARABO_LOG_WARN << "Ignoring request";
                }
                channel->readAsyncHash(boost::bind(&karabo::core::GuiServerDevice::onRead, this, _1, _2));
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onRead(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onGuiError(const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGuiError";
                m_guiDebugChannel->write(hash, Hash() /*empty header*/);

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onGuiError(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onLogin(karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onLogin";
                // Check valid login
                KARABO_LOG_INFO << "Login request of user: " << hash.get<string > ("username");
                // if ok
                sendSystemTopology(channel);
                // else sendBadLogin
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onLogin(): " << e.userFriendlyMsg();
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
                KARABO_LOG_ERROR << "Problem in onReconfigure(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onExecute(const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onExecute";
                string deviceId = hash.get<string > ("deviceId");
                string command = hash.get<string > ("command");
                // TODO Supply user specific context
                call(deviceId, command, Hash());
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onExecute(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onInitDevice(const karabo::util::Hash& hash) {
            try {

                KARABO_LOG_FRAMEWORK_DEBUG << "onInitDevice";
                cout << hash << endl;
                string serverId = hash.get<string > ("serverId");                
                KARABO_LOG_INFO << "Incoming request to start device instance on server " << serverId;
                call(serverId, "slotStartDevice", hash);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onInitDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onRefreshInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& hash) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRefreshInstance";
                string deviceId = hash.get<string > ("deviceId");

                Hash config = remote().getConfigurationNoWait(deviceId);

                if (!config.empty()) {
                    Hash h("type", "configurationChanged", "deviceId", deviceId, "configuration", remote().get(deviceId));
                    channel->write(h);
                }

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onRefreshInstance(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onKillServer(const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillServer";
                string serverId = info.get<string > ("serverId");
                // TODO Supply user specific context
                call(serverId, "slotKillServer");
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onKillServer(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onKillDevice(const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillDevice";
                string deviceId = info.get<string > ("deviceId");
                call(deviceId, "slotKillDevice");
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onKillDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onNewVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                string deviceId = info.get<string > ("deviceId");
                boost::mutex::scoped_lock lock(m_channelMutex);
                std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                if (it != m_channels.end()) {
                    it->second.insert(deviceId);
                }

                // Increase count of device in visible devices map
                m_visibleDevices[deviceId]++;
                KARABO_LOG_FRAMEWORK_DEBUG << "onNewVisibleDevice " << deviceId << " " << m_visibleDevices[deviceId];

                if (m_visibleDevices[deviceId] == 1) { // Fresh device on the shelf
                    remote().registerDeviceMonitor(deviceId, boost::bind(&karabo::core::GuiServerDevice::deviceChangedHandler, this, _1, _2));
                }

                // Send back fresh information about device
                // TODO This could check a dirty-flag whether the device changed since last time seen
                onRefreshInstance(channel, info);

            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onNewVisibleDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onRemoveVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                string deviceId = info.get<string > ("deviceId");

                boost::mutex::scoped_lock lock(m_channelMutex);
                std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                if (it != m_channels.end()) it->second.erase(deviceId);

                m_visibleDevices[deviceId]--;
                KARABO_LOG_FRAMEWORK_DEBUG << "onRemoveVisibleDevice " << deviceId << " " << m_visibleDevices[deviceId];

                if (m_visibleDevices[deviceId] == 0) {
                    // Disconnect signal/slot from broker
                    remote().unregisterDeviceMonitor(deviceId);
                }


            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onRemoveVisibleDevice(): " << e.userFriendlyMsg();
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
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    Hash h("type", "classSchema", "serverId", serverId,
                                   "classId", classId, "schema", schema);
                    channel->write(h);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onGetClassSchema(): " << e.userFriendlyMsg();
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
                    boost::mutex::scoped_lock lock(m_channelMutex);
                    Hash h("type", "deviceSchema", "deviceId", deviceId,
                           "schema", schema);
                    if (!config.empty()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Adding configuration, too";
                        h.set("configuration", config);
                    }
                    channel->write(h);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onGetDeviceSchema(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onGetFromPast(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetFromPast";
                string deviceId = info.get<string > ("deviceId");
                string property = info.get<string > ("property");
                string t0 = info.get<string > ("t0");
                string t1 = info.get<string > ("t1");
                int maxNumData = 0;
                if (info.has("maxNumData")) maxNumData = info.getAs<int>("maxNumData");

                Hash args("from", t0, "to", t1, "maxNumData", maxNumData);
                call("Karabo_FileDataLogger_0", "slotGetPropertyHistory", deviceId, property, args);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onGetFromPast(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::slotPropertyHistory(const std::string& deviceId, const std::string& property, const std::vector<karabo::util::Hash>& data) {
            try {

                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting property history";

                Hash h("type", "propertyHistory", "deviceId", deviceId,
                       "property", property, "data", data);

                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs (which is shit here, but the current solution...)
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;

                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    if (it->second.find(deviceId) != it->second.end()) {
                        it->first->write(h);
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in slotPropertyHistory " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::sendSystemTopology(karabo::net::Channel::Pointer channel) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "sendSystemTopology";
                KARABO_LOG_FRAMEWORK_DEBUG << remote().getSystemTopology();
                Hash systemTopology("type", "systemTopology", "systemTopology", remote().getSystemTopology());
                channel->write(systemTopology);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in sendSystemTopology(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting availability of new instance";
                Hash instanceInfo("type", "instanceNew", "topologyEntry", topologyEntry);
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(instanceInfo);
                }
                // TODO let device client return also deviceId as first argument
                if (topologyEntry.has("device")) {
                    string deviceId = topologyEntry.get<Hash>("device").begin()->getKey();
                    // Check whether someone already noted interest in this deviceId
                    if (m_visibleDevices.find(deviceId) != m_visibleDevices.end()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Connecting to device " << deviceId << " which is going to be visible in a GUI client";
                        remote().registerDeviceMonitor(deviceId, boost::bind(&karabo::core::GuiServerDevice::deviceChangedHandler, this, _1, _2));
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in instanceNewHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::instanceUpdatedHandler(const karabo::util::Hash& topologyEntry) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting instance updated";
                Hash instanceInfo("type", "instanceUpdated", "topologyEntry", topologyEntry);
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(instanceInfo);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in instanceUpdatedHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting instance gone";
                std::string type("unknown");
                if (instanceInfo.has("type")) instanceInfo.get("type", type);
                Hash h("type", "instanceGone", "instanceId", instanceId, "instanceType", type);
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(h);
                    // Set all visibilities to 0
                    std::map<std::string, int>::iterator jt = m_visibleDevices.find(instanceId);
                    if (jt != m_visibleDevices.end()) {
                        m_visibleDevices.erase(jt);
                    }
                    // and remove the instance from channel
                    it->second.erase(instanceId);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in instanceGoneHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::deviceChangedHandler(const std::string& deviceId, const karabo::util::Hash& what) {
            try {
                Hash h("type", "configurationChanged", "deviceId", deviceId, "configuration", what);

                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    // Optimization: broadcast only to visible DeviceInstances
                    if (it->second.find(deviceId) != it->second.end()) {
                        it->first->write(h);
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in slotChanged(): " << e.userFriendlyMsg();
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
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(h);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onGetClassSchema(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::schemaUpdatedHandler(const std::string& deviceId, const karabo::util::Schema& schema) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting schema updated";

                if (schema.empty()) {
                    KARABO_LOG_FRAMEWORK_WARN << "Going to send an empty schema, should not happen...";
                }

                Hash h("type", "schemaUpdated", "deviceId", deviceId,
                       "schema", schema);

                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    // TODO This could be optimized by selecting the proper clients
                    it->first->write(h);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in slotSchemaUpdated(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::slotNotification(const std::string& type, const std::string& shortMessage, const std::string& detailedMessage, const std::string & deviceId) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting notification";
                Hash notificationInfo("type", "notification", "deviceId", deviceId,
                                      "messageType", type, "shortMsg", shortMessage,
                                      "detailedMsg", detailedMessage);

                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(notificationInfo);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in slotNotification(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::logHandler(karabo::net::BrokerChannel::Pointer channel, const std::string& logMessage, const karabo::util::Hash & header) {
            try {
                Hash instanceInfo("type", "log", "message", logMessage);
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(instanceInfo);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in logHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onError(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& errorCode) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            KARABO_LOG_INFO << "Network notification: " << errorCode.message();
            // TODO Fork on error message
            std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if (it != m_channels.end()) {
                it->first->close(); // This closes socket and unregisters channel from connection
                // Remove all previously visible devices
                const std::set<std::string>& deviceIds = it->second;
                for (std::set<std::string>::const_iterator jt = deviceIds.begin(); jt != deviceIds.end(); jt++) {
                    const std::string& deviceId = *jt;
                    m_visibleDevices[deviceId]--;
                    KARABO_LOG_FRAMEWORK_DEBUG << "removeVisibleDevice (GUI gone) " << deviceId << " " << m_visibleDevices[deviceId];
                    if (m_visibleDevices[deviceId] == 0) {
                        // Disconnect signal/slot from broker
                        remote().unregisterDeviceMonitor(deviceId);
                    }
                }
                // Remove channel as such
                m_channels.erase(it);
            }
        }
    }
}
