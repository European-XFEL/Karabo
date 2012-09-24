/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "HashDatabase.hh"
#include "GuiServerDevice.hh"

using namespace std;
using namespace exfel::util;
using namespace exfel::core;
using namespace exfel::net;
using namespace exfel::io;
using namespace log4cpp;

namespace exfel {
    namespace core {

        EXFEL_REGISTER_FACTORY_CC(Device, GuiServerDevice)

        void GuiServerDevice::expectedParameters(Schema& expected) {

            UINT32_ELEMENT(expected)
                    .key("port")
                    .displayedName("Hostport")
                    .description("Local port for this server")
                    .assignmentOptional().defaultValue(44444)
                    .commit();

            CHOICE_ELEMENT<exfel::net::BrokerConnection > (expected)
                    .key("loggerConnection")
                    .displayedName("Logger Connection")
                    .description("Configuration of the connection for the distributed logging system")
                    .assignmentOptional().defaultValue("Jms")
                    .commit();

        }


        void GuiServerDevice::configure(const Hash& input) {

            SLOT0(slotEndError)

            GLOBAL_SLOT1(slotNewNode, Hash)
            GLOBAL_SLOT1(slotNewDeviceServerInstance, Hash)
            GLOBAL_SLOT1(slotNewDeviceClass, Hash)
            GLOBAL_SLOT1(slotNewDeviceInstance, Hash)
            GLOBAL_SLOT1(slotUpdateDeviceServerInstance, Hash)
            GLOBAL_SLOT1(slotUpdateDeviceInstance, Hash)

            // SLOT2(slotNoTransition, string, string)
            // SLOT2(slotBadReconfiguration, string, string)
            // GLOBAL_SLOT2(slotConnected, string, string)
            GLOBAL_SLOT3(slotChanged, Hash, string, string)
            GLOBAL_SLOT4(slotErrorFound, string, string, string, string)
            GLOBAL_SLOT4(slotWarning, string, string, string, string)
            GLOBAL_SLOT4(slotAlarm, string, string, string, string)
            GLOBAL_SLOT3(slotSchemaUpdatedToGui, string, string, string)


            Hash config;
            config.set("port", input.get<unsigned int>("port"));
            config.set("type", "server");
            config.setFromPath("hashSerialization.Xml.indentation", -1);
            config.setFromPath("hashSerialization.Xml.printDataType", true);
            m_dataConnection = Connection::create("Tcp", config);
            m_ioService = m_dataConnection->getIOService();
            m_xmlSerializer = Format<Hash>::create("Xml"); // for reading
            m_binarySerializer = Format<Hash>::create("Bin"); // for writing changes
            
            m_loggerConnection = BrokerConnection::createChoice("loggerConnection", input);
            m_loggerIoService = m_loggerConnection->getIOService();
           
        }


        void GuiServerDevice::run() {
            startStateMachine();
            runEventLoop();
        }
        
        
        void GuiServerDevice::startServer() {
            log() << Priority::INFO << "Starting the XFEL GuiServer";
            m_dataConnection->startAsync(boost::bind(&exfel::core::GuiServerDevice::onConnect, this, _1));
            // Use one thread currently (you may start this multiple time for having more threads doing the work)
            boost::thread(boost::bind(&exfel::net::IOService::run, m_ioService));
            
             // Start the logging thread
            m_loggerChannel = m_loggerConnection->start();
            m_loggerChannel->setFilter("target = 'log'");
            m_loggerChannel->readAsyncStringHash(boost::bind(&exfel::core::GuiServerDevice::onLog, this, _1, _2, _3));
            boost::thread(boost::bind(&exfel::net::BrokerIOService::work, m_loggerIoService));
        }

        
        void GuiServerDevice::onConnect(exfel::net::Channel::Pointer channel) {
            log() << Priority::INFO << "Incoming connection";
            channel->readAsyncStringHash(boost::bind(&exfel::core::GuiServerDevice::onRead, this, _1, _2, _3));
            channel->setErrorHandler(boost::bind(&exfel::core::GuiServerDevice::onError, this, _1, _2));
            m_dataConnection->startAsync(boost::bind(&exfel::core::GuiServerDevice::onConnect, this, _1));
            registerConnect(channel);
        }
        

        void GuiServerDevice::onRead(exfel::net::Channel::Pointer channel, const std::string& body, const exfel::util::Hash& header) {
            // GUI communication scenarios could go here 
            if (header.has("type")) {
                string type = header.get<string > ("type");
                if (type == "login") {
                    onLogin(channel, body);
                } else if (type == "reconfigure") {
                    onReconfigure(header, body);
                } else if (type == "initDevice") {
                    onInitDevice(header, body);
                } else if (type == "refreshInstance") {
                    onRefreshInstance(channel, header);
                } else if (type == "slotCommand") {
                    onSlotCommand(header, body);
                } else if (type == "newVisibleDeviceInstance") {
                    onNewVisibleDeviceInstance(channel, header);
                } else if (type == "removeVisibleDeviceInstance") {
                    onRemoveVisibleDeviceInstance(channel, header);
                } else if (type == "killDeviceServerInstance") {
                    onKillDeviceServerInstance(header, body);
                } else if (type == "killDeviceInstance") {
                    onKillDeviceInstance(header, body);
                }

            } else {
                log() << Priority::WARN << "Ignoring request";
            }
            channel->readAsyncStringHash(boost::bind(&exfel::core::GuiServerDevice::onRead, this, _1, _2, _3));
        }


        void GuiServerDevice::onLogin(exfel::net::Channel::Pointer channel, const std::string& body) {
            Hash bodyHash = m_xmlSerializer->unserialize(body);
            // Check valid login
            log() << Priority::INFO << "Login request of user: " << bodyHash.get<string > ("username");
            // if ok
            sendCurrentIds(channel);
            // else sendBadLogin
        }


        void GuiServerDevice::onRefreshInstance(exfel::net::Channel::Pointer channel, const exfel::util::Hash& header) {
            string instanceId = header.get<string > ("instanceId");
            call(instanceId, "slotRefresh");
        }


        void GuiServerDevice::onReconfigure(const exfel::util::Hash& header, const std::string& body) {
            Hash bodyHash = m_xmlSerializer->unserialize(body);
            string instanceId = header.get<string > ("instanceId");
            call(instanceId, "slotReconfigure", bodyHash);
        }


        void GuiServerDevice::onInitDevice(const exfel::util::Hash& header, const std::string& body) {
            Hash config = m_xmlSerializer->unserialize(body);
            string instanceId = header.get<string > ("instanceId");
            log() << Priority::INFO << "Incoming request to start device instance on server " << instanceId;
            call(instanceId, "slotStartDevice", config);
        }


        void GuiServerDevice::onSlotCommand(const exfel::util::Hash& header, const std::string& body) {
            Hash config = m_xmlSerializer->unserialize(body);
            string instanceId = header.get<string > ("instanceId");
            string slotName = config.get<string > ("name");
            call(instanceId, slotName, config);
        }


        void GuiServerDevice::onNewVisibleDeviceInstance(exfel::net::Channel::Pointer channel, const exfel::util::Hash& header) {
            string instanceId = header.get<string > ("instanceId");
            boost::mutex::scoped_lock lock(m_channelMutex);
            std::map<exfel::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if ( it != m_channels.end() ) {
                it->second.insert(instanceId);
            }
            // TODO: optimize further in doing the signal/slot connect here
            call(instanceId, "slotRefresh");
        }


        void GuiServerDevice::onRemoveVisibleDeviceInstance(exfel::net::Channel::Pointer channel, const exfel::util::Hash& header){
            string instanceId = header.get<string > ("instanceId");
            boost::mutex::scoped_lock lock(m_channelMutex);
            std::map<exfel::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if ( it != m_channels.end() ) {
                it->second.erase(instanceId);
            }
        }
        
        void GuiServerDevice::onKillDeviceServerInstance(const exfel::util::Hash& header, const std::string& body) {
             string instanceId = header.get<string > ("instanceId");
             call(instanceId, "slotKillDeviceServerInstance");
        }

        void GuiServerDevice::onKillDeviceInstance(const exfel::util::Hash& header, const std::string& body) {
            log() << Priority::INFO << "Broadcasting availability of new device-server instance";
             string deviceServerInstanceId = header.get<string > ("devSrvInsId");
             string deviceInstanceId = header.get<string>("devInsId");
             call(deviceServerInstanceId, "slotKillDeviceInstance", deviceInstanceId);
        }

        void GuiServerDevice::slotNewNode(const exfel::util::Hash& row) {
            log() << Priority::DEBUG << "Broadcasting availability of new nodes";
            Hash header("type", "newNode");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(row, header);
            }
        }


        void GuiServerDevice::slotNewDeviceServerInstance(const exfel::util::Hash& row) {
            log() << Priority::DEBUG << "Broadcasting availability of new device-server instance";
            Hash header("type", "newDeviceServerInstance");
             boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(row, header);
            }
        }


        void GuiServerDevice::slotNewDeviceClass(const exfel::util::Hash& row) {
            log() << Priority::DEBUG << "Broadcasting availability of new device class";
            Hash header("type", "newDeviceClass");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(row, header);
            }
        }


        void GuiServerDevice::slotNewDeviceInstance(const exfel::util::Hash& row) {
            log() << Priority::DEBUG << "Broadcasting availability of new device instance";
            Hash header("type", "newDeviceInstance");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(row, header);
            }
        }


        void GuiServerDevice::slotUpdateDeviceServerInstance(const exfel::util::Hash& row) {
            log() << Priority::DEBUG << "Broadcasting update of device server instance";
            Hash header("type", "updateDeviceServerInstance");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(row, header);
            }
        }


        void GuiServerDevice::slotUpdateDeviceInstance(const exfel::util::Hash& row) {
            log() << Priority::DEBUG << "Broadcasting update of device instance";
            Hash header("type", "updateDeviceInstance");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(row, header);
            }
        }


        void GuiServerDevice::slotChanged(const exfel::util::Hash& what, const std::string& instanceId, const std::string& classId) {
            Hash header("type", "change", "instanceId", instanceId, "classId", classId);
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                // Optimization: broadcast only to visible DeviceInstances
                if ( it->second.find(instanceId) != it->second.end() ) {
                    it->first->write(m_binarySerializer->serialize(what), header);
                }
            }
        }

        
        void GuiServerDevice::onLog(exfel::net::BrokerChannel::Pointer channel, const std::string& logMessage, const exfel::util::Hash& header) {
            Hash h("type", "log");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(logMessage, h);
            }
        }


        void GuiServerDevice::slotErrorFound(const std::string& timeStamp, const std::string& shortMessage, const std::string& detailedMessage, const std::string& instanceId) {
            log() << Priority::DEBUG << "Broadcasting ERROR in system";
            Hash header("type", "error");
            // Add message together in logMessage pattern (... | ... | ... #)
            std::string message = timeStamp + " | ERROR | " + instanceId + " | " + shortMessage + " | " + detailedMessage + "#";
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(message, header);
            }
        }


        void GuiServerDevice::slotWarning(const std::string& timeStamp, const std::string& warnMessage, const std::string& instanceId, const std::string& priority) {
            log() << Priority::WARN << "Broadcasting WARNING in system";
            Hash header("type", "warning");
            // Add message together in logMessage pattern (... | ... | ... #)
            std::string message = timeStamp + " | WARNING | " + instanceId + " | " + warnMessage + " | Priority: " + priority + "#";
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(message, header);
            }
        }

        
        void GuiServerDevice::slotAlarm(const std::string& timeStamp, const std::string& alarmMessage, const std::string& instanceId, const std::string& priority) {
            log() << Priority::DEBUG << "Broadcasting ALARM in system";
            Hash header("type", "alarm");
            // Add message together in logMessage pattern (... | ... | ... #)
            std::string message = timeStamp + " | ALARM | " + instanceId + " | " + alarmMessage + " | Priority: " + priority + "#";
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(message, header);
            }
        }


        void GuiServerDevice::slotSchemaUpdatedToGui(const std::string& schema, const std::string& instanceId, const std::string& classId) {
            log() << Priority::DEBUG << "Broadcasting updated schema to system";
            Hash header("type", "schemaUpdated", "instanceId", instanceId, "classId", classId);
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< exfel::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(schema, header);
            }
        }


        void GuiServerDevice::sendCurrentIds(exfel::net::Channel::Pointer channel) {

            log() << Priority::DEBUG << "Providing instance information...";

            Hash body;

            Hash& root = body.bindReference<Hash > ("Root");
            HashDatabase::ResultType result;

            vector<Hash >& node = root.bindReference<vector<Hash> >("Node");
            EXFEL_DB_SELECT(result, "id,name", "Node", true);
            node = result;
            result.clear();

            vector<Hash>& deviceServerInstance = root.bindReference<vector<Hash> >("DeviceServerInstance");
            EXFEL_DB_SELECT(result, "id,instanceId,status,nodId", "DeviceServerInstance", true);
            deviceServerInstance = result;
            result.clear();

            vector<Hash >& deviceClass = root.bindReference<vector<Hash> >("DeviceClass");
            EXFEL_DB_SELECT(result, "id,name,schema,devSerInsId", "DeviceClass", true);
            deviceClass = result;
            result.clear();

            vector<Hash >& deviceInstance = root.bindReference<vector<Hash> >("DeviceInstance");
            EXFEL_DB_SELECT(result, "id,instanceId,devClaId,schema", "DeviceInstance", true);
            deviceInstance = result;
            result.clear();
            
            m_channelMutex.lock();
            channel->write(body, Hash("type", "currentInstances"));
            m_channelMutex.unlock();
        }


        void GuiServerDevice::registerConnect(const exfel::net::Channel::Pointer & channel) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = std::set<std::string>();
        }


        void GuiServerDevice::onError(exfel::net::Channel::Pointer channel, const std::string & errorMessage) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            // TODO Fork on error message
            std::map<exfel::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if ( it != m_channels.end() ) {
                it->first->close(); // This closes socket and unregisters channel from connection
                m_channels.erase(it);
            }
        }
    }
}
