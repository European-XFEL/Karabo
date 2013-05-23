/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/regex.hpp>

#include "HashDatabase.hh"
#include "GuiServerDevice.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::io;

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

        }


        GuiServerDevice::GuiServerDevice(const Hash& input) : Device<OkErrorFsm>(input) {

            GLOBAL_SLOT1(slotNewNode, Hash)
            GLOBAL_SLOT1(slotNewDeviceServerInstance, Hash)
            GLOBAL_SLOT1(slotNewDeviceClass, Hash)
            GLOBAL_SLOT1(slotNewDeviceInstance, Hash)
            GLOBAL_SLOT1(slotUpdateDeviceServerInstance, Hash)
            GLOBAL_SLOT1(slotUpdateDeviceInstance, Hash)

            GLOBAL_SLOT3(slotChanged, Hash, string, string)
            GLOBAL_SLOT4(slotErrorFound, string, string, string, string)
            GLOBAL_SLOT4(slotWarning, string, string, string, string)
            GLOBAL_SLOT4(slotAlarm, string, string, string, string)
            GLOBAL_SLOT3(slotSchemaUpdatedToGui, string, string, string)


            Hash config;
            config.set("port", input.get<unsigned int>("port"));
            config.set("type", "server");
            config.set("serializationType", "text");
            m_dataConnection = Connection::create("Tcp", config);
            m_ioService = m_dataConnection->getIOService();
            m_textSerializer = TextSerializer<Hash>::create("Xml"); // for reading      
            //m_binarySerializer = Format<Hash>::create("Bin"); // for writing changes

            m_loggerConnection = BrokerConnection::createChoice("loggerConnection", input);
            m_loggerIoService = m_loggerConnection->getIOService();

        }


        void GuiServerDevice::okStateOnEntry() {
            m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));
            // Use one thread currently (you may start this multiple time for having more threads doing the work)
            boost::thread(boost::bind(&karabo::net::IOService::run, m_ioService));

            // Start the logging thread
            m_loggerChannel = m_loggerConnection->start();
            m_loggerChannel->setFilter("target = 'log'");
            m_loggerChannel->readAsyncStringHash(boost::bind(&karabo::core::GuiServerDevice::onLog, this, _1, _2, _3));
            boost::thread(boost::bind(&karabo::net::BrokerIOService::work, m_loggerIoService));
        }


        void GuiServerDevice::onConnect(karabo::net::Channel::Pointer channel) {
            KARABO_LOG_DEBUG << "Incoming connection";
            channel->readAsyncHashString(boost::bind(&karabo::core::GuiServerDevice::onRead, this, _1, _2, _3));
            channel->setErrorHandler(boost::bind(&karabo::core::GuiServerDevice::onError, this, _1, _2));
            m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));
            registerConnect(channel);
        }


        void GuiServerDevice::onRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body) {
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
                } else if (type == "createNewDeviceClassPlugin") {
                    onCreateNewDeviceClassPlugin(header, body);
                }

            } else {
                KARABO_LOG_WARN << "Ignoring request";
            }
            channel->readAsyncHashString(boost::bind(&karabo::core::GuiServerDevice::onRead, this, _1, _2, _3));
        }


        void GuiServerDevice::onLogin(karabo::net::Channel::Pointer channel, const std::string& body) {
            Hash bodyHash;
            m_textSerializer->load(bodyHash, body);
            // Check valid login
            KARABO_LOG_INFO << "Login request of user: " << bodyHash.get<string > ("username");
            // if ok
            sendCurrentIds(channel);
            // else sendBadLogin
        }


        void GuiServerDevice::onRefreshInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header) {
            string instanceId = header.get<string > ("instanceId");
            call(instanceId, "slotRefresh");
        }


        void GuiServerDevice::onReconfigure(const karabo::util::Hash& header, const std::string& body) {
            Hash bodyHash;
            m_textSerializer->load(bodyHash, body);
            string instanceId = header.get<string > ("instanceId");
            call(instanceId, "slotReconfigure", bodyHash);
        }


        void GuiServerDevice::onInitDevice(const karabo::util::Hash& header, const std::string& body) {
            Hash config;
            m_textSerializer->load(config, body);
            string instanceId = header.get<string > ("instanceId");
            KARABO_LOG_INFO << "Incoming request to start device instance on server " << instanceId;
            call(instanceId, "slotStartDevice", config);
        }


        void GuiServerDevice::onSlotCommand(const karabo::util::Hash& header, const std::string& body) {
            Hash config;
            m_textSerializer->load(config, body);
            string instanceId = header.get<string > ("instanceId");
            string slotName = config.get<string > ("name");
            call(instanceId, slotName, config);
        }


        void GuiServerDevice::onNewVisibleDeviceInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header) {
            string instanceId = header.get<string > ("instanceId");
            boost::mutex::scoped_lock lock(m_channelMutex);
            std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if (it != m_channels.end()) {
                it->second.insert(instanceId);
            }
            KARABO_LOG_INFO << "onNewVisibleDeviceInstance " << instanceId;
            // TODO: optimize further in doing the signal/slot connect here
            call(instanceId, "slotRefresh");
        }


        void GuiServerDevice::onRemoveVisibleDeviceInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header) {
            string instanceId = header.get<string > ("instanceId");
            boost::mutex::scoped_lock lock(m_channelMutex);
            std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if (it != m_channels.end()) {
                it->second.erase(instanceId);
            }
        }


        void GuiServerDevice::onKillDeviceServerInstance(const karabo::util::Hash& header, const std::string& body) {
            string instanceId = header.get<string > ("instanceId");
            call(instanceId, "slotKillDeviceServerInstance");
        }


        void GuiServerDevice::onKillDeviceInstance(const karabo::util::Hash& header, const std::string& body) {
            KARABO_LOG_DEBUG << "Broadcasting availability of new device-server instance";
            string deviceServerInstanceId = header.get<string > ("devSrvInsId");
            string deviceInstanceId = header.get<string > ("devInsId");
            call(deviceServerInstanceId, "slotKillDeviceInstance", deviceInstanceId);
        }


        void GuiServerDevice::onCreateNewDeviceClassPlugin(const karabo::util::Hash& header, const std::string& body) {
            KARABO_LOG_DEBUG << "Broadcasting availability of new device class";
            Hash bodyHash;
            m_textSerializer->load(bodyHash, body);
            string devSrvInsId = bodyHash.get<string > ("devSrvInsId");
            string devClaId = bodyHash.get<string > ("devClaId");
            string newDevClaId = bodyHash.get<string > ("newDevClaId");
            call("*", "slotCreateNewDeviceClassPlugin", devSrvInsId, devClaId, newDevClaId);
        }


        void GuiServerDevice::slotNewNode(const karabo::util::Hash& row) {
            KARABO_LOG_DEBUG << "Broadcasting availability of new nodes";
            Hash header("type", "newNode");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, row);
            }
        }


        void GuiServerDevice::slotNewDeviceServerInstance(const karabo::util::Hash& row) {
            KARABO_LOG_DEBUG << "Broadcasting availability of new device-server instance";
            Hash header("type", "newDeviceServerInstance");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, row);
            }
        }


        void GuiServerDevice::slotNewDeviceClass(const karabo::util::Hash& row) {
            KARABO_LOG_DEBUG << "Broadcasting availability of new device class";
            Hash header("type", "newDeviceClass");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, row);
            }
        }


        void GuiServerDevice::slotNewDeviceInstance(const karabo::util::Hash& row) {
            KARABO_LOG_DEBUG << "Broadcasting availability of new device instance";
            Hash header("type", "newDeviceInstance");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, row);
            }
        }


        void GuiServerDevice::slotUpdateDeviceServerInstance(const karabo::util::Hash& row) {
            KARABO_LOG_DEBUG << "Broadcasting update of device server instance";
            Hash header("type", "updateDeviceServerInstance");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, row);
            }
        }


        void GuiServerDevice::slotUpdateDeviceInstance(const karabo::util::Hash& row) {
            KARABO_LOG_DEBUG << "Broadcasting update of device instance";
            Hash header("type", "updateDeviceInstance");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, row);
            }
        }


        void GuiServerDevice::slotChanged(const karabo::util::Hash& what, const std::string& instanceId, const std::string& classId) {
            boost::smatch sm;
            boost::regex re(".*(image).*", boost::regbase::icase);
            Hash modified(what);
            for (Hash::iterator it = modified.begin(); it != modified.end(); it++) {
                if (!boost::regex_search(it->getKey(), sm, re))
                    continue; // this is not an image:  failed to follow naming convention
                if (!it->is<Hash>())
                    continue; // this is not an image:  no Hash containing image structure
                Hash& input = it->getValue<Hash>();
                if (!input.has("dims"))
                    continue; // this is not an image:  no 'dims' key
                if (!input.has("pixelArray"))
                    continue; // this is not an image:  no 'pixelArray' key

                vector<unsigned int>& dims = input.get<vector<unsigned int> >("dims");
                if (dims.size() < 2)
                    continue; // empty image?

                //                unsigned int& dimX = dims[0];
                //                unsigned int& dimY = dims[1];

                vector<unsigned char>& vdata = input.get<vector<unsigned char> >("pixelArray");

                string& fmt = input.get<string > ("format");
                //cout << "slotChanged: pixelArray: len = " << vdata.size() << ", dimX = " << dimX << ", dimY = " << dimY << " -- " << fmt << endl;

                vector<string> vhdr;
                boost::split(vhdr, fmt, boost::is_any_of("-"));
                string& imgType = vhdr[0];
                unsigned int bytesPerPixel = boost::lexical_cast<unsigned int>(vhdr[1]);
                //                unsigned int bitsPerPixel = boost::lexical_cast<unsigned int>(vhdr[2]);
                bool msbFlag = vhdr[3] == "MSB" ? true : false;

                vector<unsigned char> dataImage;

                // support for grayscale images
                if (imgType == "GRAY") {
                    if (bytesPerPixel == 1) {
                        size_t size = vdata.size();
                        unsigned pmax = 0, pmin = 255;
                        for (size_t i = 0; i < size; i++) {
                            unsigned pix = vdata[i];
                            if (pmax < pix) pmax = pix;
                            if (pmin > pix) pmin = pix;
                        }
                        for (size_t i = 0; i < size; i++) {
                            unsigned char pix = vdata[i];
                            pix = int((unsigned(pix) - pmin) * 255 / (pmax - pmin)); // normalization
                            dataImage.push_back(pix);
                            dataImage.push_back(pix);
                            dataImage.push_back(pix);
                            dataImage.push_back(0xFF);
                        }
                    } else if (bytesPerPixel == 2) {
                        size_t size = vdata.size() / 2;
                        unsigned short* sdata = reinterpret_cast<unsigned short*> (&vdata[0]);
                        unsigned pmax = 0, pmin = 65535;
                        if (msbFlag) {
                            for (size_t i = 0; i < size; i++) {
                                sdata[i] = sdata[i] << 8 | sdata[i] >> 8; // swap bytes
                                if (pmax < sdata[i]) pmax = sdata[i];
                                if (pmin > sdata[i]) pmin = sdata[i];
                            }
                        } else {
                            for (size_t i = 0; i < size; i++) {
                                if (pmax < sdata[i]) pmax = sdata[i];
                                if (pmin > sdata[i]) pmin = sdata[i];
                            }
                        }

                        for (size_t i = 0; i < size; i++) {
                            unsigned char pix = int((sdata[i] - pmin) * 255 / (pmax - pmin)); // normalization
                            dataImage.push_back(pix);
                            dataImage.push_back(pix);
                            dataImage.push_back(pix);
                            dataImage.push_back(0xFF);
                        }
                    } else
                        return;
                    // update pixelArray in the hash
                    input.set("pixelArray", dataImage);
                }
                // At the moment we don't touch color images!!!
                // TODO:  color image's support
            }
            Hash header("type", "change", "instanceId", instanceId, "classId", classId);
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                // Optimization: broadcast only to visible DeviceInstances
                //std::string body;
                if (it->second.find(instanceId) != it->second.end()) {
                    //m_textSerializer->save(modified, body);
                    it->first->write(header, modified);
                }
            }
        }


        void GuiServerDevice::onLog(karabo::net::BrokerChannel::Pointer channel, const std::string& logMessage, const karabo::util::Hash& header) {
            Hash h("type", "log");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(h, logMessage);
            }
        }


        void GuiServerDevice::slotErrorFound(const std::string& timeStamp, const std::string& shortMessage, const std::string& detailedMessage, const std::string& instanceId) {
            KARABO_LOG_DEBUG << "Broadcasting ERROR in system";
            Hash header("type", "error");
            // Add message together in logMessage pattern (... | ... | ... #)
            std::string message = timeStamp + " | ERROR | " + instanceId + " | " + shortMessage + " | " + detailedMessage + "#";
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, message);
            }
        }


        void GuiServerDevice::slotWarning(const std::string& timeStamp, const std::string& warnMessage, const std::string& instanceId, const std::string& priority) {
            KARABO_LOG_DEBUG << "Broadcasting WARNING in system";
            Hash header("type", "warning");
            // Add message together in logMessage pattern (... | ... | ... #)
            std::string message = timeStamp + " | WARNING | " + instanceId + " | " + warnMessage + " | Priority: " + priority + "#";
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, message);
            }
        }


        void GuiServerDevice::slotAlarm(const std::string& timeStamp, const std::string& alarmMessage, const std::string& instanceId, const std::string& priority) {
            KARABO_LOG_DEBUG << "Broadcasting ALARM in system";
            Hash header("type", "alarm");
            // Add message together in logMessage pattern (... | ... | ... #)
            std::string message = timeStamp + " | ALARM | " + instanceId + " | " + alarmMessage + " | Priority: " + priority + "#";
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, message);
            }
        }


        void GuiServerDevice::slotSchemaUpdatedToGui(const std::string& schema, const std::string& instanceId, const std::string& classId) {
            KARABO_LOG_DEBUG << "Broadcasting updated schema to system";
            Hash header("type", "schemaUpdated", "instanceId", instanceId, "classId", classId);
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, schema);
            }
        }


        void GuiServerDevice::sendCurrentIds(karabo::net::Channel::Pointer channel) {

            KARABO_LOG_DEBUG << "Providing instance information...";

            Hash body;

            Hash& root = body.bindReference<Hash > ("Root");
            HashDatabase::ResultType result;

            vector<Hash >& node = root.bindReference<vector<Hash> >("Node");
            KARABO_DB_SELECT(result, "id,name", "Node", true);
            node = result;
            result.clear();

            vector<Hash>& deviceServerInstance = root.bindReference<vector<Hash> >("DeviceServerInstance");
            KARABO_DB_SELECT(result, "id,instanceId,status,nodId", "DeviceServerInstance", true);
            deviceServerInstance = result;
            result.clear();

            vector<Hash >& deviceClass = root.bindReference<vector<Hash> >("DeviceClass");
            KARABO_DB_SELECT(result, "id,name,schema,devSerInsId", "DeviceClass", true);
            deviceClass = result;
            result.clear();

            vector<Hash >& deviceInstance = root.bindReference<vector<Hash> >("DeviceInstance");
            KARABO_DB_SELECT(result, "id,instanceId,devClaId,schema", "DeviceInstance", true);
            deviceInstance = result;
            result.clear();

            m_channelMutex.lock();
            channel->write(Hash("type", "currentInstances"), body);
            m_channelMutex.unlock();
        }


        void GuiServerDevice::registerConnect(const karabo::net::Channel::Pointer & channel) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = std::set<std::string > ();
        }


        void GuiServerDevice::onError(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& errorCode) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            // TODO Fork on error message
            std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if (it != m_channels.end()) {
                it->first->close(); // This closes socket and unregisters channel from connection
                m_channels.erase(it);
            }
        }
    }
}
