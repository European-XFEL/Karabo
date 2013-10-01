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
            
            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();

        }


        GuiServerDevice::GuiServerDevice(const Hash& input) : Device<OkErrorFsm>(input) {

            GLOBAL_SLOT2(slotChanged, Hash /*configuration*/, string /*deviceId*/)
            GLOBAL_SLOT2(slotSchemaUpdated, Schema /*description*/, string /*deviceId*/)
            GLOBAL_SLOT4(slotNotification, string /*type*/, string /*shortMsg*/, string /*detailedMsg*/, string /*deviceId*/)

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
            try {
                // Register handlers
                remote().registerInstanceNewMonitor(boost::bind(&karabo::core::GuiServerDevice::instanceNewHandler, this, _1));
                remote().registerInstanceUpdatedMonitor(boost::bind(&karabo::core::GuiServerDevice::instanceUpdatedHandler, this, _1));
                remote().registerInstanceGoneMonitor(boost::bind(&karabo::core::GuiServerDevice::instanceGoneHandler, this, _1));

                m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));
                // Use one thread currently (you may start this multiple time for having more threads doing the work)
                boost::thread(boost::bind(&karabo::net::IOService::run, m_ioService));

                // Start the logging thread
                m_loggerChannel = m_loggerConnection->start();
                m_loggerChannel->setFilter("target = 'log'");
                m_loggerChannel->readAsyncStringHash(boost::bind(&karabo::core::GuiServerDevice::logHandler, this, _1, _2, _3));
                boost::thread(boost::bind(&karabo::net::BrokerIOService::work, m_loggerIoService));
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in okStateOnEntry(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onConnect(karabo::net::Channel::Pointer channel) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Incoming connection";
                channel->readAsyncHashString(boost::bind(&karabo::core::GuiServerDevice::onRead, this, _1, _2, _3));
                channel->setErrorHandler(boost::bind(&karabo::core::GuiServerDevice::onError, this, _1, _2));
                // Re-register acceptor socket (allows handling multiple clients)
                m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));
                registerConnect(channel);
                
                Hash header("type", "brokerInformation");
                Hash body;
                body.set("host", this->getConnection()->getBrokerHostname());
                body.set("port", this->getConnection()->getBrokerPort());
                body.set("topic", this->getConnection()->getBrokerTopic());
                channel->write(header, body);
                
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onConnect(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::registerConnect(const karabo::net::Channel::Pointer & channel) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = std::set<std::string > (); // maps channel to visible instances
        }


        void GuiServerDevice::onRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body) {
            try {
                // GUI communication scenarios
                if (header.has("type")) {
                    string type = header.get<string > ("type");
                    if (type == "login") {
                        onLogin(channel, body);
                    } else if (type == "reconfigure") {
                        onReconfigure(header, body);
                    } else if (type == "execute") {
                        onExecute(header, body);
                    } else if (type == "initDevice") {
                        onInitDevice(header, body);
                    } else if (type == "refreshInstance") {
                        onRefreshInstance(channel, header);
                    } else if (type == "killServer") {
                        onKillServer(header, body);
                    } else if (type == "killDevice") {
                        onKillDevice(header, body);
                    } else if (type == "newVisibleDevice") {
                        onNewVisibleDevice(channel, header);
                    } else if (type == "removeVisibleDevice") {
                        onRemoveVisibleDevice(channel, header);
                    } else if (type == "getClassSchema") {
                        onGetClassSchema(channel, header, body);
                    } else if (type == "getDeviceSchema") {
                        onGetDeviceSchema(channel, header, body);
                    }
                } else {
                    KARABO_LOG_WARN << "Ignoring request";
                }
                channel->readAsyncHashString(boost::bind(&karabo::core::GuiServerDevice::onRead, this, _1, _2, _3));
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onRead(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onLogin(karabo::net::Channel::Pointer channel, const std::string& body) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onLogin";
                Hash bodyHash;
                m_textSerializer->load(bodyHash, body);
                // Check valid login
                KARABO_LOG_INFO << "Login request of user: " << bodyHash.get<string > ("username");
                // if ok
                sendSystemTopology(channel);
                // else sendBadLogin
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onLogin(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::sendSystemTopology(karabo::net::Channel::Pointer channel) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "sendSystemTopology";
                channel->write(Hash("type", "systemTopology"), remote().getSystemTopology());
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in sendSystemTopology(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onReconfigure(const karabo::util::Hash& header, const std::string& body) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onReconfigure";
                Hash bodyHash;
                m_textSerializer->load(bodyHash, body);
                string deviceId = header.get<string > ("deviceId");
                // TODO Supply user specific context
                call(deviceId, "slotReconfigure", bodyHash);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onReconfigure(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onExecute(const karabo::util::Hash& header, const std::string& body) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onExecute";
                Hash config;
                m_textSerializer->load(config, body);
                string deviceId = header.get<string > ("deviceId");
                string command = config.get<string > ("command");
                // TODO Supply user specific context
                call(deviceId, command, config);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onExecute(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onInitDevice(const karabo::util::Hash& header, const std::string& body) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onInitDevice";
                Hash config;
                m_textSerializer->load(config, body);
                string serverId = header.get<string > ("serverId");
                KARABO_LOG_INFO << "Incoming request to start device instance on server " << serverId;
                call(serverId, "slotStartDevice", config);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onInitDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onRefreshInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRefreshInstance";
                string deviceId = header.get<string > ("deviceId");
                Hash h("type", "configurationChanged", "deviceId", deviceId);
                Hash b;
                Hash& tmp = b.bindReference<Hash>("device." + deviceId + ".configuration");
                tmp = remote().get(deviceId);
                preprocessImageData(tmp);
                channel->write(h, b);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onRefreshInstance(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onKillServer(const karabo::util::Hash& header, const std::string& body) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillServer";
                string serverId = header.get<string > ("serverId");
                // TODO Supply user specific context
                call(serverId, "slotKillServer");
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onKillServer(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onKillDevice(const karabo::util::Hash& header, const std::string& body) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onKillDevice";
                string deviceId = header.get<string > ("deviceId");
                call(deviceId, "slotKillDevice");
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onKillDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onNewVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onNewVisibleDevice";
                string deviceId = header.get<string > ("deviceId");
                boost::mutex::scoped_lock lock(m_channelMutex);
                std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                if (it != m_channels.end()) {
                    it->second.insert(deviceId);
                }
//                Hash h("type", "configurationChanged", "deviceId", deviceId);
//                Hash b;
//                Hash& tmp = b.bindReference<Hash>("device." + deviceId + ".configuration");
//                tmp = remote().get(deviceId);
//                preprocessImageData(tmp);
//                channel->write(h, b);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onNewVisibleDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onRemoveVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onRemoveVisibleDevice";
                string deviceId = header.get<string > ("deviceId");
                boost::mutex::scoped_lock lock(m_channelMutex);
                std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
                if (it != m_channels.end()) it->second.erase(deviceId);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onRemoveVisibleDevice(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onGetClassSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetClassSchema";
                string serverId = header.get<string > ("serverId");
                string classId = header.get<string> ("classId");
                boost::mutex::scoped_lock lock(m_channelMutex);
                Hash h("type", "classDescription");
                Hash b("server." + serverId + ".classes." + classId + ".description", remote().getClassSchema(serverId, classId));
                channel->write(h, b);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onGetClassSchema(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onGetDeviceSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "onGetDeviceSchema";
                string deviceId = header.get<string > ("deviceId");
                boost::mutex::scoped_lock lock(m_channelMutex);
                Hash h("type", "deviceSchema", "deviceId", deviceId);
                Hash b("device." + deviceId + ".description", remote().getDeviceSchema(deviceId));
                channel->write(h, b);
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in onGetDeviceSchema(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting availability of new instance";
                Hash header("type", "instanceNew");
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(header, topologyEntry);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in instanceNewHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::instanceUpdatedHandler(const karabo::util::Hash& topologyEntry) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting instance updated";
                Hash header("type", "instanceUpdated");
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(header, topologyEntry);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in instanceUpdatedHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::instanceGoneHandler(const std::string& instanceId) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting instance gone";
                Hash header("type", "instanceGone");
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(header, instanceId);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in instanceUpdatedHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::preprocessImageData(karabo::util::Hash& modified) {

            try {
                for (Hash::iterator it = modified.begin(); it != modified.end(); it++) {
                    if (it->hasAttribute("image")) {
                        
                        // Create a RawImageData object which shares the data of the hash
                        karabo::xip::RawImageData img(it->getValue<Hash>(), true); // Must share data

                        if (img.getDimensions().rank() < 2) continue;

                        // Support for grayscale images
                        if (img.getEncoding() == Encoding::GRAY) {
                            KARABO_LOG_DEBUG << "Preprocessing image";
                            
                            size_t size = img.size();
                            vector<unsigned char> qtImage(size * 4); // Have to blow up for RGBA

                            if (img.getChannelSpace() == ChannelSpace::u_8_1) {
                                KARABO_LOG_DEBUG << "u_8_1";
                                unsigned char* data = reinterpret_cast<unsigned char*>(img.dataPointer());
                                unsigned char pmax = 0, pmin = 0xFF;
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = data[i];
                                    if (pmax < pix) pmax = pix;
                                    if (pmin > pix) pmin = pix;
                                }

                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                    norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin) // normalization
                                        pix = static_cast<unsigned char>(norm * (data[i] - pmin));
                                    
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }

                            } else if (img.getChannelSpace() == ChannelSpace::s_8_1) {
                                char* data = img.dataPointer();
                                unsigned char pmax = 0, pmin = 0xFF;
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = data[i] + 0x80; // back to 0-0xFF range
                                    if (pmax < pix) pmax = pix;
                                    if (pmin > pix) pmin = pix;
                                }

                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                    norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin) // normalization
                                        pix = static_cast<unsigned char>(norm * (data[i] + 0x80 - pmin));

                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }


                            } else if (img.getChannelSpace() == ChannelSpace::u_16_2) {
                                unsigned short* data = reinterpret_cast<unsigned short*>(img.dataPointer());
                                unsigned short pmax = 0, pmin = 0xFFFF;

                                if (img.isBigEndian()) {
                                    for (size_t i = 0; i < size; i++) {
                                        data[i] = data[i] << 8 | data[i] >> 8; // swap bytes
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                } else {
                                    for (size_t i = 0; i < size; i++) {
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                }

                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                    norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin) // normalization
                                        pix = static_cast<unsigned char>(norm * (data[i] - pmin));

                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }
				
                            } else if (img.getChannelSpace() == ChannelSpace::s_16_2) {
                                short* data = reinterpret_cast<short*>(img.dataPointer());
                                unsigned short pmax = 0, pmin = 0xFFFF;

                                if (img.isBigEndian()) {
                                    for (size_t i = 0; i < size; i++) {
                                        data[i] = data[i] << 8 | data[i] >> 8; // swap bytes
                                        unsigned short pix = data[i] + 0x8000; // go back to 0-0xFFFF range
                                        if (pmax < pix) pmax = pix;
                                        if (pmin > pix) pmin = pix;
                                    }
                                } else {
                                    for (size_t i = 0; i < size; i++) {
                                        unsigned short pix = data[i] + 0x8000; // go back to 0-0xFFFF range
                                        if (pmax < pix) pmax = pix;
                                        if (pmin > pix) pmin = pix;
                                    }
                                }

                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin) // normalization
                                        pix = static_cast<unsigned char>(norm * (data[i] + 0x8000 - pmin));

                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }


                            } else if (img.getChannelSpace() == ChannelSpace::u_32_4) {
                                unsigned int* data = reinterpret_cast<unsigned int*>(img.dataPointer());
                                unsigned int pmax = 0, pmin = 0xFFFFFFFF;

                                if (img.isBigEndian()) {
                                    for (size_t i = 0; i < size; i++) {
                                        data[i] = ((data[i]<<24)&0xFF000000)
                                                + ((data[i]<< 8)&0x00FF0000)
                                                + ((data[i]>> 8)&0x0000FF00)
                                                + ((data[i]>>24)&0x000000FF);  // swap bytes
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                } else {
                                    for (size_t i = 0; i < size; i++) {
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                }

                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                    norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin) // normalization
                                        pix = static_cast<unsigned char>(norm * (data[i] - pmin));

                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }


                            } else if (img.getChannelSpace() == ChannelSpace::s_32_4) {
                                int* data = reinterpret_cast<int*>(img.dataPointer());
                                unsigned int pmax = 0, pmin = 0xFFFFFFFF;

                                if (img.isBigEndian()) {
                                    for (size_t i = 0; i < size; i++) {
                                        data[i] = ((data[i]<<24)&0xFF000000)
                                                + ((data[i]<< 8)&0x00FF0000)
                                                + ((data[i]>> 8)&0x0000FF00)
                                                + ((data[i]>>24)&0x000000FF);  // swap bytes
                                        unsigned int pix = data[i] + 0x80000000; // go back to 0-0xFFFFFFFF range
                                        if (pmax < pix) pmax = pix;
                                        if (pmin > pix) pmin = pix;
                                    }
                                } else {
                                    for (size_t i = 0; i < size; i++) {
                                        unsigned int pix = data[i] + 0x80000000; // go back to 0-0xFFFFFFFF range
                                        if (pmax < pix) pmax = pix;
                                        if (pmin > pix) pmin = pix;
                                    }
                                }

                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                    norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin) // normalization
                                        pix = static_cast<unsigned char>(norm * (data[i] + 0x80000000 - pmin));

                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }


                           } else if (img.getChannelSpace() == ChannelSpace::u_64_8) {
                                unsigned long* data = reinterpret_cast<unsigned long*>(img.dataPointer());
                                unsigned long pmax = 0, pmin = 0xFFFFFFFFFFFFFFFF;

                                if (img.isBigEndian()) {
                                    for (size_t i = 0; i < size; i++) {
                                        data[i] = ((data[i]<<56)&0xFF00000000000000)
                                                + ((data[i]<<40)&0x00FF000000000000)
                                                + ((data[i]<<24)&0x0000FF0000000000)
                                                + ((data[i]<< 8)&0x000000FF00000000)
                                                + ((data[i]>> 8)&0x00000000FF000000)
                                                + ((data[i]>>24)&0x0000000000FF0000)
                                                + ((data[i]>>40)&0x000000000000FF00)
                                                + ((data[i]>>56)&0x00000000000000FF);  // swap bytes
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                } else {
                                    for (size_t i = 0; i < size; i++) {
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                }

                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                    norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin) // normalization
                                        pix = static_cast<unsigned char>(norm * (data[i] - pmin));

                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }


                           } else if (img.getChannelSpace() == ChannelSpace::s_64_8) {
                                long* data = reinterpret_cast<long*>(img.dataPointer());
                                unsigned long pmax = 0, pmin = 0xFFFFFFFFFFFFFFFF;

                                if (img.isBigEndian()) {
                                    for (size_t i = 0; i < size; i++) {
                                        data[i] = ((data[i]<<56)&0xFF00000000000000)
                                                + ((data[i]<<40)&0x00FF000000000000)
                                                + ((data[i]<<24)&0x0000FF0000000000)
                                                + ((data[i]<< 8)&0x000000FF00000000)
                                                + ((data[i]>> 8)&0x00000000FF000000)
                                                + ((data[i]>>24)&0x0000000000FF0000)
                                                + ((data[i]>>40)&0x000000000000FF00)
                                                + ((data[i]>>56)&0x00000000000000FF);  // swap bytes
                                        unsigned long pix = data[i] + 0x8000000000000000; // go back to 0-0xFFFFFFFFFFFFFFFF range
                                        if (pmax < pix) pmax = pix;
                                        if (pmin > pix) pmin = pix;
                                    }
                                } else {
                                    for (size_t i = 0; i < size; i++) {
                                        unsigned long pix = data[i] + 0x8000000000000000; // go back to 0-0xFFFFFFFFFFFFFFFF range
                                        if (pmax < pix) pmax = pix;
                                        if (pmin > pix) pmin = pix;
                                    }
                                }
                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                    norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin) // normalization
                                        pix = static_cast<unsigned char>(norm * (data[i] + 0x8000000000000000 - pmin));

                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }


                            } else if (img.getChannelSpace() == ChannelSpace::f_32_4) {
                                float* data = reinterpret_cast<float*>(img.dataPointer());
                                float pmax = -std::numeric_limits<float>::max();
                                float pmin = std::numeric_limits<float>::max();

                                for (size_t i = 0; i < size; i++) {
                                    if (pmax < data[i]) pmax = data[i];
                                    if (pmin > data[i]) pmin = data[i];
                                }

                                if (img.isBigEndian()) {
                                    for (size_t i = 0; i < size; i++) {
                                        unsigned int* data_i = (unsigned int*)(data+i);
                                        *data_i = (*data_i<<24)&0xFF000000
                                                + (*data_i<< 8)&0x00FF0000
                                                + (*data_i>> 8)&0x0000FF00
                                                + (*data_i>>24)&0x000000FF;  // swap bytes
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                } else {
                                    for (size_t i = 0; i < size; i++) {
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                }

                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                    norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                    unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin)
                                        pix = static_cast<unsigned char>(norm * (data[i] - pmin)); // normalization

                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }


                           } else if (img.getChannelSpace() == ChannelSpace::f_64_8) {
                                double* data = reinterpret_cast<double*>(img.dataPointer());
                                double pmax = -std::numeric_limits<double>::max();
                                double pmin = std::numeric_limits<double>::max();

                                for (size_t i = 0; i < size; i++) {
                                    if (pmax < data[i]) pmax = data[i];
                                    if (pmin > data[i]) pmin = data[i];
                                }

                                if (img.isBigEndian()) {
                                    for (size_t i = 0; i < size; i++) {
                                        unsigned long* data_i = (unsigned long*)(data+i);
                                        *data_i = (*data_i<<56)&0xFF00000000000000
                                                + (*data_i<<40)&0x00FF000000000000
                                                + (*data_i<<24)&0x0000FF0000000000
                                                + (*data_i<< 8)&0x000000FF00000000
                                                + (*data_i>> 8)&0x00000000FF000000
                                                + (*data_i>>24)&0x0000000000FF0000
                                                + (*data_i>>40)&0x000000000000FF00
                                                + (*data_i>>56)&0x00000000000000FF;  // swap bytes
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                } else {
                                    for (size_t i = 0; i < size; i++) {
                                        if (pmax < data[i]) pmax = data[i];
                                        if (pmin > data[i]) pmin = data[i];
                                    }
                                }

                                size_t index = 0;
                                double norm = 1.0;
                                if (pmax > pmin)
                                    norm = (double)0xFF / (pmax-pmin);
                                for (size_t i = 0; i < size; i++) {
                                 unsigned char pix = 0x7F; // arbitrary
                                    if (pmax > pmin) pix = static_cast<unsigned char>(norm * (data[i] - pmin)); // normalization

                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = pix;
                                    qtImage[index++] = 0xFF;
                                }


                            } else {
                                return;
                            }

                            // update data in the hash
                            img.setData(qtImage); // Update data
                            img.setEncoding(Encoding::RGBA); // Update encoding

                        }
                        // TODO: At the moment we don't touch color images!!!
                        // TODO: Color image's support
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in preprocessImageData(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::slotChanged(const karabo::util::Hash& what, const std::string & deviceId) {
            try {
                Hash modified(what);
                preprocessImageData(modified);

                Hash header("type", "configurationChanged", "deviceId", deviceId);
                Hash body("device." + deviceId + ".configuration", modified);

                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    // Optimization: broadcast only to visible DeviceInstances
                    //std::string body;
                    if (it->second.find(deviceId) != it->second.end()) {
                        //m_textSerializer->save(modified, body);
                        it->first->write(header, body);
                    }
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in slotChanged(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string & deviceId) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting schema updated";
                Hash header("type", "schemaUpdated", "deviceId", deviceId);
                Hash body("device." + deviceId + ".description", schema);
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(header, body);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in slotSchemaUpdated(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::slotNotification(const std::string& type, const std::string& shortMessage, const std::string& detailedMessage, const std::string & deviceId) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting notification";
                Hash header("type", "notification", "deviceId", deviceId);
                Hash body("type", type, "shortMsg", shortMessage, "detailedMsg", detailedMessage);
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(header, body);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in slotNotification(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::logHandler(karabo::net::BrokerChannel::Pointer channel, const std::string& logMessage, const karabo::util::Hash & header) {
            try {
                Hash h("type", "log");
                boost::mutex::scoped_lock lock(m_channelMutex);
                // Broadcast to all GUIs
                typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
                for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                    it->first->write(h, logMessage);
                }
            } catch (const Exception& e) {
                KARABO_LOG_ERROR << "Problem in logHandler(): " << e.userFriendlyMsg();
            }
        }


        void GuiServerDevice::onError(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode & errorCode) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            KARABO_LOG_INFO << "Network notification: " << errorCode.message();
            // TODO Fork on error message
            std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if (it != m_channels.end()) {
                it->first->close(); // This closes socket and unregisters channel from connection
                m_channels.erase(it);
            }
        }
    }
}
