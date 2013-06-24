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
        }


        void GuiServerDevice::onConnect(karabo::net::Channel::Pointer channel) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Incoming connection";
            channel->readAsyncHashString(boost::bind(&karabo::core::GuiServerDevice::onRead, this, _1, _2, _3));
            channel->setErrorHandler(boost::bind(&karabo::core::GuiServerDevice::onError, this, _1, _2));
            // Re-register acceptor socket (allows handling multiple clients)
            m_dataConnection->startAsync(boost::bind(&karabo::core::GuiServerDevice::onConnect, this, _1));
            registerConnect(channel);
        }


        void GuiServerDevice::registerConnect(const karabo::net::Channel::Pointer & channel) {
            boost::mutex::scoped_lock lock(m_channelMutex);
            m_channels[channel] = std::set<std::string > (); // maps channel to visible instances
        }


        void GuiServerDevice::onRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body) {

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
        }


        void GuiServerDevice::onLogin(karabo::net::Channel::Pointer channel, const std::string& body) {
            KARABO_LOG_FRAMEWORK_DEBUG << "onLogin";
            Hash bodyHash;
            m_textSerializer->load(bodyHash, body);
            // Check valid login
            KARABO_LOG_INFO << "Login request of user: " << bodyHash.get<string > ("username");
            // if ok
            sendSystemTopology(channel);
            // else sendBadLogin
        }


        void GuiServerDevice::sendSystemTopology(karabo::net::Channel::Pointer channel) {
            KARABO_LOG_FRAMEWORK_DEBUG << "sendSystemTopology";
            channel->write(Hash("type", "systemTopology"), remote().getSystemTopology());
        }


        void GuiServerDevice::onReconfigure(const karabo::util::Hash& header, const std::string& body) {
            KARABO_LOG_FRAMEWORK_DEBUG << "onReconfigure";
            Hash bodyHash;
            m_textSerializer->load(bodyHash, body);
            string deviceId = header.get<string > ("deviceId");
            // TODO Supply user specific context
            call(deviceId, "slotReconfigure", bodyHash);
        }


        void GuiServerDevice::onExecute(const karabo::util::Hash& header, const std::string& body) {
            KARABO_LOG_FRAMEWORK_DEBUG << "";
            Hash config;
            m_textSerializer->load(config, body);
            string deviceId = header.get<string > ("deviceId");
            string command = config.get<string > ("command");
            // TODO Supply user specific context
            call(deviceId, command, config);
        }


        void GuiServerDevice::onInitDevice(const karabo::util::Hash& header, const std::string& body) {
            KARABO_LOG_FRAMEWORK_DEBUG << "";
            Hash config;
            m_textSerializer->load(config, body);
            string serverId = header.get<string > ("serverId");
            KARABO_LOG_INFO << "Incoming request to start device instance on server " << serverId;
            call(serverId, "slotStartDevice", config);
        }


        void GuiServerDevice::onRefreshInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header) {
            KARABO_LOG_FRAMEWORK_DEBUG << "";
            string deviceId = header.get<string > ("deviceId");
            Hash h("type", "configurationChanged", "deviceId", deviceId);
            Hash b("device." + deviceId + ".configuration", remote().get(deviceId));
            channel->write(h, b);
        }


        void GuiServerDevice::onKillServer(const karabo::util::Hash& header, const std::string& body) {
            KARABO_LOG_FRAMEWORK_DEBUG << "";
            string serverId = header.get<string > ("serverId");
            // TODO Supply user specific context
            call(serverId, "slotKillServer");
        }


        void GuiServerDevice::onKillDevice(const karabo::util::Hash& header, const std::string& body) {
            KARABO_LOG_FRAMEWORK_DEBUG << "";
            string deviceId = header.get<string > ("deviceId");
            call(deviceId, "slotKillDevice");
        }


        void GuiServerDevice::onNewVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header) {
            KARABO_LOG_FRAMEWORK_DEBUG << "";
            string deviceId = header.get<string > ("deviceId");
            boost::mutex::scoped_lock lock(m_channelMutex);
            std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if (it != m_channels.end()) {
                it->second.insert(deviceId);
            }
            Hash h("type", "configurationChanged", "deviceId", deviceId);
            Hash b("device." + deviceId + ".configuration", remote().get(deviceId));
            channel->write(h, b);
        }


        void GuiServerDevice::onRemoveVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header) {
            KARABO_LOG_FRAMEWORK_DEBUG << "";
            string deviceId = header.get<string > ("deviceId");
            boost::mutex::scoped_lock lock(m_channelMutex);
            std::map<karabo::net::Channel::Pointer, std::set<std::string> >::iterator it = m_channels.find(channel);
            if (it != m_channels.end()) {
                it->second.erase(deviceId);
            }
        }


        void GuiServerDevice::onGetClassSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body) {
            KARABO_LOG_FRAMEWORK_DEBUG << "";
            string serverId = header.get<string > ("serverId");
            string classId = header.get<string> ("classId");
            boost::mutex::scoped_lock lock(m_channelMutex);
            Hash h("type", "classDescription");
            Hash b("server." + serverId + ".classes." + classId + ".description", remote().getClassSchema(serverId, classId));
            channel->write(h, b);
        }


        void GuiServerDevice::onGetDeviceSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body) {
            KARABO_LOG_FRAMEWORK_DEBUG << "";
            string deviceId = header.get<string > ("deviceId");
            boost::mutex::scoped_lock lock(m_channelMutex);
            Hash h("type", "deviceSchema", "deviceId", deviceId);
            Hash b("device." + deviceId + ".description", remote().getFullSchema(deviceId));
            channel->write(h, b);
        }


        void GuiServerDevice::instanceNewHandler(const karabo::util::Hash& topologyEntry) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting availability of new instance";
            Hash header("type", "instanceNew");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, topologyEntry);
            }
        }


        void GuiServerDevice::instanceUpdatedHandler(const karabo::util::Hash& topologyEntry) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting instance updated";
            Hash header("type", "instanceUpdated");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, topologyEntry);
            }
        }


        void GuiServerDevice::instanceGoneHandler(const std::string& instanceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting instance gone";
            Hash header("type", "instanceGone");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, instanceId);
            }
        }


        void GuiServerDevice::preprocessImageData(karabo::util::Hash& modified) {
            boost::smatch sm;
            boost::regex re(".*(image).*", boost::regbase::icase);
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
        }
        
        void GuiServerDevice::slotChanged(const karabo::util::Hash& what, const std::string& deviceId) {
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
        }


        void GuiServerDevice::slotSchemaUpdated(const karabo::util::Schema& description, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting schema updated";
            Hash header("type", "schemaUpdated", "deviceId", deviceId);
            Hash body("device." + deviceId + ".description", description);
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, body);
            }
        }


        void GuiServerDevice::slotNotification(const std::string& type, const std::string& shortMessage, const std::string& detailedMessage, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Broadcasting notification";
            Hash header("type", "notification", "deviceId", deviceId);
            Hash body("type", type, "shortMsg", shortMessage, "detailedMsg", detailedMessage);
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(header, body);
            }
        }


        void GuiServerDevice::logHandler(karabo::net::BrokerChannel::Pointer channel, const std::string& logMessage, const karabo::util::Hash& header) {
            Hash h("type", "log");
            boost::mutex::scoped_lock lock(m_channelMutex);
            // Broadcast to all GUIs
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator channelIterator;
            for (channelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
                it->first->write(h, logMessage);
            }
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
