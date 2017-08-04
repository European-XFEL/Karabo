/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <exception>
#include <boost/pointer_cast.hpp>
#include "OutputChannel.hh"
#include "karabo/net/TcpChannel.hh"
#include "karabo/util/MetaTools.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace xms {

        KARABO_REGISTER_FOR_CONFIGURATION(OutputChannel);


        void OutputChannel::expectedParameters(karabo::util::Schema& expected) {
            using namespace karabo::util;


            STRING_ELEMENT(expected).key("distributionMode")
                    .displayedName("Distribution Mode")
                    .description("Describes the policy of how to fan-out data to multiple (shared) input channels")
                    .options("load-balanced,round-robin")
                    .assignmentOptional().defaultValue("load-balanced")
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("noInputShared")
                    .displayedName("No Input (Shared)")
                    .description("What to do if currently no share-input channel is available for writing to")
                    .options("drop,queue,throw,wait")
                    .assignmentOptional().defaultValue("wait")
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("hostname")
                    .displayedName("Hostname")
                    .description("The hostname to which connecting clients will be routed to")
                    .assignmentOptional().defaultValue("default")
                    .commit();


            INT32_ELEMENT(expected).key("compression")
                    .displayedName("Compression")
                    .description("Configures when the data is compressed (-1 = off, 0 = always, >0 = threshold in MB")
                    .expertAccess()
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .assignmentOptional().defaultValue(-1)
                    .commit();
            
            INT32_ELEMENT(expected).key("port")
                    .displayedName("Port")
                    .description("Port number for TCP connection")
                    .expertAccess()
                    .assignmentOptional().defaultValue(0)
                    .init()
                    .commit();
        }


        OutputChannel::OutputChannel(const karabo::util::Hash& config) : m_sharedInputIndex(0), m_port(0) {
            //KARABO_LOG_FRAMEWORK_DEBUG << "*** OutputChannel::OutputChannel CTOR ***";
            config.get("distributionMode", m_distributionMode);
            config.get("noInputShared", m_onNoSharedInputChannelAvailable);
            config.get("hostname", m_hostname);
            config.get("port", m_port);
            if (m_hostname == "default") m_hostname = boost::asio::ip::host_name();
            config.get("compression", m_compression);

            KARABO_LOG_FRAMEWORK_DEBUG << "NoInputShared: " << m_onNoSharedInputChannelAvailable;

            // Memory related
            try {
                m_channelId = Memory::registerChannel();
                m_chunkId = Memory::registerChunk(m_channelId);
            } catch (...) {
                KARABO_RETHROW
            }

            KARABO_LOG_FRAMEWORK_DEBUG << "Outputting data on channel " << m_channelId << " and chunk " << m_chunkId;

            // Data networking
            // TODO: Use port 0
            int tryAgain = 50; // Try maximum 50 times to start a server
            while (tryAgain >= 0) {
                try {
                    //m_ownPort = Statics::generateServerPort();
                    karabo::util::Hash h("type", "server", "port", m_port, "compressionUsageThreshold", m_compression * 1E6);
                    m_dataConnection = karabo::net::Connection::create("Tcp", h);
                    // Cannot yet use bind_weak - we are still in constructor :-(.
                    m_ownPort = m_dataConnection->startAsync(boost::bind(&karabo::xms::OutputChannel::onTcpConnect, this, _1, _2));
                } catch (const std::exception& ex) {
                    if (m_port != 0) { // if output channel is started with defined port number
                        throw KARABO_NETWORK_EXCEPTION(std::string("Could not start TcpServer for output channel (\"")
                                + toString(m_channelId) + "\", port = " + toString(m_port) + ") : " + ex.what());
                    }
                    if (tryAgain > 0) {
                        tryAgain--;
                        continue;
                    } else {
                        throw KARABO_NETWORK_EXCEPTION(std::string("Could not start TcpServer for output channel (\"")
                                + toString(m_channelId) + "\", port = " + toString(m_ownPort) + ") : " + ex.what());
                    }
                }
                KARABO_LOG_FRAMEWORK_DEBUG << "Started DeviceOutput-Server listening on port: " << m_ownPort;
                break;
            }
        }


        OutputChannel::~OutputChannel() {
            //KARABO_LOG_FRAMEWORK_DEBUG << "*** OutputChannel::~OutputChannel() DTOR ***";
            for (std::set<TcpChannelPointer>::iterator it = m_dataChannels.begin(); it != m_dataChannels.end(); ++it) {
                if (*it) (*it)->close();
            }
            m_dataChannels.clear();
            if (m_dataConnection) m_dataConnection->stop();
            Memory::unregisterChannel(m_channelId);
        }


        void OutputChannel::setInstanceIdAndName(const std::string& instanceId, const std::string& name) {
            m_instanceId = instanceId;
            m_channelName = name;
        }


        const std::string& OutputChannel::getInstanceId() const {
            return m_instanceId;
        }


        void OutputChannel::registerIOEventHandler(const boost::function<void (const OutputChannel::Pointer&)>& ioEventHandler) {
            m_ioEventHandler = ioEventHandler;
        }


        karabo::util::Hash OutputChannel::getInformation() const {
            return karabo::util::Hash("connectionType", "tcp", "hostname", m_hostname, "port", m_ownPort);
        }


        void OutputChannel::onTcpConnect(const karabo::net::ErrorCode& ec, const TcpChannelPointer& channel) {
            using namespace karabo::net;
            
            if (ec) {
                KARABO_LOG_FRAMEWORK_DEBUG << "onTcpConnect received error code " << ec.value() << " (i.e. '"
                        << ec.message() << "').";
                return;
            }
            
            m_dataChannels.insert(channel);
            TcpChannel::Pointer tch = boost::dynamic_pointer_cast<TcpChannel>(channel);
            KARABO_LOG_FRAMEWORK_DEBUG << "***** Connection established to socket " << tch->socket().native() << " *****";
            channel->readAsyncHash(bind_weak(&karabo::xms::OutputChannel::onTcpChannelRead, this, _1, channel, _2));
            m_dataConnection->startAsync(bind_weak(&karabo::xms::OutputChannel::onTcpConnect, this, _1, _2));
        }


        void OutputChannel::onTcpChannelError(const karabo::net::ErrorCode& error, const TcpChannelPointer& channel) {
            using namespace karabo::net;
            TcpChannel::Pointer tch = boost::dynamic_pointer_cast<TcpChannel>(channel);
            KARABO_LOG_FRAMEWORK_INFO << "Tcp channel (socket " << tch->socket().native()
                    << ") error on \"" << m_instanceId << "\", code #" << error.value() << " -- \""
                    << error.message() << "\".  Channel closed.";
            // Unregister channel
            onInputGone(channel);
            m_dataChannels.erase(channel);
            if (channel) channel->close();
        }


        void OutputChannel::onTcpChannelRead(const karabo::net::ErrorCode& ec, const TcpChannelPointer& channel, const karabo::util::Hash& message) {
            if (ec) {
                onTcpChannelError(ec, channel);
                return;
            }
            
            std::string reason;
            if (message.has("reason")) message.get<std::string > ("reason", reason);

            if (reason == "hello") {

                /* The hello message is expected to have:
                 *     instanceId (std::string)
                 *     memoryLocation (std::string) [local/remote]
                 *     dataDistribution (std::string) [shared/copy]
                 *     onSlowness (std::string) [queue/drop/wait/throw]
                 */

                const std::string& instanceId = message.get<std::string > ("instanceId");
                const std::string& memoryLocation = message.get<std::string > ("memoryLocation");
                const std::string& dataDistribution = message.get<std::string > ("dataDistribution");
                const std::string& onSlowness = message.get<std::string > ("onSlowness");

                karabo::util::Hash info;
                info.set("instanceId", instanceId);
                info.set("memoryLocation", memoryLocation);
                info.set("tcpChannel", channel);
                info.set("onSlowness", onSlowness);
                info.set("queuedChunks", std::deque<int>());

                if (dataDistribution == "shared") {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Registering shared-input channel of instance: " << instanceId;
                    m_registeredSharedInputs.push_back(info);
                } else {
                    bool isNew = true;
                    for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {
                        karabo::util::Hash& channelInfo = m_registeredCopyInputs[i];
                        if (channelInfo.get<std::string>("instanceId") == instanceId) {
                            KARABO_LOG_FRAMEWORK_DEBUG << "Registering copy-input channel of (used before) instance: " << instanceId;
                            channelInfo.set("memoryLocation", memoryLocation);
                            channelInfo.set("tcpChannel", channel);
                            channelInfo.set("onSlowness", onSlowness);
                            isNew = false;
                            break;
                        }
                    }
                    if (isNew) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Registering copy-input channel of (new) instance: " << instanceId;
                        m_registeredCopyInputs.push_back(info);
                    }
                }
                onInputAvailable(instanceId); // Immediately register for reading

            } else if (reason == "update") {

                if (message.has("instanceId")) {
                    const std::string& instanceId = message.get<std::string > ("instanceId");
                    KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT of '" << this->getInstanceId() << "': instanceId "
                            << instanceId << " has updated...";
                    onInputAvailable(instanceId);
                }

            }
            channel->readAsyncHash(bind_weak(&karabo::xms::OutputChannel::onTcpChannelRead, this, _1, channel, _2));
        }


        void OutputChannel::onInputAvailable(const std::string& instanceId) {

            for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                if (channelInfo.get<std::string>("instanceId") == instanceId) {
                    if (!channelInfo.get<std::deque<int> >("queuedChunks").empty()) {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Writing queued (shared) data to instance " << instanceId;
                        distributeQueue(channelInfo);
                        return;
                    }
                    pushShareNext(instanceId);
                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " New (shared) input on instance " << instanceId << " available for writing ";
                    this->triggerIOEvent();
                    return;
                }
            }
            for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {
                karabo::util::Hash& channelInfo = m_registeredCopyInputs[i];
                if (channelInfo.get<std::string>("instanceId") == instanceId) {
                    if (!channelInfo.get<std::deque<int> >("queuedChunks").empty()) {
                        KARABO_LOG_FRAMEWORK_TRACE << debugId() << " Writing queued (copied) data to instance " << instanceId;
                        copyQueue(channelInfo);
                        return;
                    }
                    pushCopyNext(instanceId);
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " New (copied) input on instance " << instanceId << " available for writing ";
                    this->triggerIOEvent();
                    return;
                }
            }
            KARABO_LOG_FRAMEWORK_WARN << this->debugId() << " An input channel wants to connect (" << instanceId << ") that was not registered before.";
        }


        void OutputChannel::onInputGone(const TcpChannelPointer& channel) {
            KARABO_LOG_FRAMEWORK_DEBUG << "*** OutputChannel::onInputGone ***";
            // SHARED Inputs
            for (InputChannels::iterator it = m_registeredSharedInputs.begin(); it != m_registeredSharedInputs.end(); ++it) {
                if (it->get<TcpChannelPointer>("tcpChannel") == channel) {
                    std::string instanceId = it->get<std::string>("instanceId");

                    KARABO_LOG_FRAMEWORK_DEBUG << "Connected (shared) input on instanceId " << instanceId << " disconnected";
                    std::deque<int> tmp = it->get<std::deque<int> >("queuedChunks");
                    // Delete from registry
                    m_registeredSharedInputs.erase(it);

                    if (!m_registeredSharedInputs.empty()) { // There are other shared input channels available
                        // Append queued chunks to other shared input
                        unsigned int idx = getNextSharedInputIdx();
                        std::deque<int>& src = m_registeredSharedInputs[idx].get<std::deque<int> >("queuedChunks");
                        src.insert(src.end(), tmp.begin(), tmp.end());
                    }

                    // Delete from input queue
                    InputChannelQueue::iterator jt = std::find(m_shareNext.begin(), m_shareNext.end(), instanceId);
                    if (jt != m_shareNext.end()) {
                        boost::mutex::scoped_lock lock(m_nextInputMutex);
                        m_shareNext.erase(jt);
                    }
                    return;
                }
            }

            // COPY Inputs
            for (InputChannels::iterator it = m_registeredCopyInputs.begin(); it != m_registeredCopyInputs.end(); ++it) {
                if (it->get<TcpChannelPointer>("tcpChannel") == channel) {
                    std::string instanceId = it->get<std::string>("instanceId");
                    //boost::mutex::scoped_lock lock(m_)
                    KARABO_LOG_FRAMEWORK_DEBUG << "Connected (copy) input on instanceId " << instanceId << " disconnected";
                    m_registeredCopyInputs.erase(it);

                    // Delete from input queue
                    InputChannelQueue::iterator jt = std::find(m_copyNext.begin(), m_copyNext.end(), instanceId);
                    if (jt != m_copyNext.end()) {
                        boost::mutex::scoped_lock lock(m_nextInputMutex);
                        m_copyNext.erase(jt);
                    }
                    return;
                }
            }
            KARABO_LOG_FRAMEWORK_WARN << "OUTPUT An input channel wants to disconnect that was not registered before.";
        }


        void OutputChannel::triggerIOEvent() {
            try {
                OutputChannel::Pointer self = shared_from_this();
                if (m_ioEventHandler) m_ioEventHandler(self);
            } catch (karabo::util::Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "\"triggerIOEvent\" Exception code #" << e.userFriendlyMsg() << " -- " << e.detailedMsg();
                KARABO_RETHROW;
            } catch (const boost::bad_weak_ptr& e) {
                KARABO_LOG_FRAMEWORK_INFO << "\"triggerIOEvent\" call is too late: OutputChannel destroyed already -- " << e.what();
            } catch (const std::exception& ex) {
                KARABO_LOG_FRAMEWORK_ERROR << "\"triggerIOEvent\" exception -- " << ex.what();
                //throw KARABO_SYSTEM_EXCEPTION(string("\"triggerIOEvent\" exception -- ") + ex.what());
            }
        }


        void OutputChannel::distributeQueue(karabo::util::Hash& channelInfo) {
            std::deque<int>& chunkIds = channelInfo.get<std::deque<int> >("queuedChunks");
            int chunkId = chunkIds.front();
            chunkIds.pop_front();
            KARABO_LOG_FRAMEWORK_DEBUG << "Distributing from queue: " << chunkId;
            if (channelInfo.get<std::string > ("memoryLocation") == "local") distributeLocal(chunkId, channelInfo);
            else distributeRemote(chunkId, channelInfo);
        }


        void OutputChannel::copyQueue(karabo::util::Hash& channelInfo) {
            std::deque<int>& chunkIds = channelInfo.get<std::deque<int> >("queuedChunks");
            int chunkId = chunkIds.front();
            chunkIds.pop_front();
            KARABO_LOG_FRAMEWORK_DEBUG << "Copying from queue: " << chunkId;
            if (channelInfo.get<std::string > ("memoryLocation") == "local") copyLocal(chunkId, channelInfo);
            else copyRemote(chunkId, channelInfo);
        }


        void OutputChannel::pushShareNext(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            m_shareNext.push_back(instanceId);
        }


        std::string OutputChannel::popShareNext() {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            std::string info = m_shareNext.front();
            m_shareNext.pop_front();
            return info;
        }


        bool OutputChannel::hasSharedInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            return (std::find(m_shareNext.begin(), m_shareNext.end(), instanceId) != m_shareNext.end());
        }


        void OutputChannel::eraseSharedInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            InputChannelQueue::iterator it = std::find(m_shareNext.begin(), m_shareNext.end(), instanceId);
            if (it != m_shareNext.end()) m_shareNext.erase(it);
        }


        void OutputChannel::pushCopyNext(const std::string& info) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            m_copyNext.push_back(info);
        }


        std::string OutputChannel::popCopyNext() {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            std::string info = m_copyNext.front();
            m_copyNext.pop_front();
            return info;
        }


        bool OutputChannel::hasCopyInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            return std::find(m_copyNext.begin(), m_copyNext.end(), instanceId) != m_copyNext.end();
        }


        void OutputChannel::eraseCopyInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            InputChannelQueue::iterator it = std::find(m_copyNext.begin(), m_copyNext.end(), instanceId);
            if (it != m_copyNext.end()) m_copyNext.erase(it);
        }


        bool OutputChannel::canCompute() const {
            return true;
        }


        void OutputChannel::update() {

            // m_channelId is unique per _process_...
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT " << m_channelId << " of '" << this->getInstanceId() << "' update()";

            // If no data was written return
            if (Memory::size(m_channelId, m_chunkId) == 0) return;

            // Take current chunkId for sending
            unsigned int chunkId = m_chunkId;

            // This will increase the usage counts for this chunkId
            // by the number of all interested connected inputs
            registerWritersOnChunk(chunkId);

            // We are done with this chunkId, it will stay alive only until
            // all inputs are served (see above)
            Memory::unregisterChunk(m_channelId, chunkId);

            // Register new chunkId for writing to
            m_chunkId = Memory::registerChunk(m_channelId);

            // Distribute chunk(s)
            distribute(chunkId);

            // Copy chunk(s)
            copy(chunkId);

        }


        void OutputChannel::signalEndOfStream() {

            // If there is still some data in the pipe, put it out
            if (Memory::size(m_channelId, m_chunkId) > 0) update();

            // Wait until all queued data is fetched
            bool doWait = true;
            while (doWait) {
                doWait = false;
                for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                    if (!m_registeredSharedInputs[i].get<std::deque<int> >("queuedChunks").empty()) {
                        doWait = true;
                    }
                }
                if (!doWait) break;
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            }


            for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                const karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");
                tcpChannel->write(karabo::util::Hash("endOfStream", true), std::vector<char>());
            }
            for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {
                const karabo::util::Hash& channelInfo = m_registeredCopyInputs[i];
                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");
                tcpChannel->write(karabo::util::Hash("endOfStream", true), std::vector<char>());
            }

        }


        void OutputChannel::registerWritersOnChunk(unsigned int chunkId) {
            // Only one of the shared inputs will be provided with data
            if (!m_registeredSharedInputs.empty()) Memory::incrementChunkUsage(m_channelId, chunkId);
            for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) Memory::incrementChunkUsage(m_channelId, chunkId);
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT Registered " << Memory::getChunkStatus(m_channelId, chunkId) << " uses for [" << m_channelId << "][" << chunkId << "]";
        }


        void OutputChannel::unregisterWriterFromChunk(int chunkId) {
            Memory::decrementChunkUsage(m_channelId, chunkId);
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT " << Memory::getChunkStatus(m_channelId, chunkId) << " uses left for [" << m_channelId << "][" << chunkId << "]";
        }


        void OutputChannel::distribute(unsigned int chunkId) {

            // If no shared input channels are registered at all, we do not go on
            if (m_registeredSharedInputs.empty()) return;

            // Next input
            const unsigned int sharedInputIdx = getNextSharedInputIdx();

            if (m_distributionMode == "round-robin") {

                const karabo::util::Hash& channelInfo = m_registeredSharedInputs[sharedInputIdx];
                const std::string& instanceId = channelInfo.get<std::string>("instanceId");

                if (hasSharedInput(instanceId)) { // Found
                    eraseSharedInput(instanceId);
                    if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Now distributing data (local)";
                        distributeLocal(chunkId, channelInfo);
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Now distributing data (remote)";
                        distributeRemote(chunkId, channelInfo);
                    }
                } else { // Not found
                    if (m_onNoSharedInputChannelAvailable == "drop") {
                        unregisterWriterFromChunk(chunkId);
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Dropping (shared) data package with chunkId: " << chunkId;

                    } else if (m_onNoSharedInputChannelAvailable == "throw") {
                        unregisterWriterFromChunk(chunkId);
                        throw KARABO_IO_EXCEPTION("Can not write data because no (shared) input is available");

                    } else if (m_onNoSharedInputChannelAvailable == "queue") {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing (shared) data package with chunkId: " << chunkId;
                        // TODO Mutex !!!
                        m_registeredSharedInputs[sharedInputIdx].get<std::deque<int> >("queuedChunks").push_back(chunkId);

                    } else if (m_onNoSharedInputChannelAvailable == "wait") {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Waiting for available (shared) input channel...";
                        while (!hasSharedInput(instanceId)) {
                            boost::this_thread::sleep(boost::posix_time::millisec(1));
                        }
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " found (shared) input channel after waiting, distributing now";
                        if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                            distributeLocal(chunkId, channelInfo);
                        } else {
                            distributeRemote(chunkId, channelInfo);
                        }
                    } else {
                        // We should never be here!!
                        throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured, ask BH");
                    }
                }
            } else if (m_distributionMode == "load-balanced") {
                if (!m_shareNext.empty()) { // Found
                    const std::string instanceId = popShareNext();
                    for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                        const karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                        if (instanceId == channelInfo.get<std::string>("instanceId")) {
                            if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                                KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Distributing data (local)";
                                distributeLocal(chunkId, channelInfo);
                            } else {
                                KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Distributing data (remote)";
                                distributeRemote(chunkId, channelInfo);
                            }
                            break;
                        }
                    }
                } else { // Not found
                    if (m_onNoSharedInputChannelAvailable == "drop") {
                        unregisterWriterFromChunk(chunkId);
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Dropping (shared) data package with chunkId: " << chunkId;
                    } else if (m_onNoSharedInputChannelAvailable == "throw") {
                        unregisterWriterFromChunk(chunkId);
                        throw KARABO_IO_EXCEPTION("Can not write data because no (shared) input is available");
                    } else if (m_onNoSharedInputChannelAvailable == "queue") {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing (shared) data package with chunkId: " << chunkId;
                        // TODO Mutex !!!
                        m_registeredSharedInputs[sharedInputIdx].get<std::deque<int> >("queuedChunks").push_back(chunkId);
                    } else if (m_onNoSharedInputChannelAvailable == "wait") {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Waiting for available (shared) input channel...";
                        while (m_shareNext.empty()) {
                            boost::this_thread::sleep(boost::posix_time::millisec(1));
                        }
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " found (shared) input channel after waiting, distributing now";
                        std::string instanceId = popShareNext();
                        for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                            const karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                            if (instanceId == channelInfo.get<std::string>("instanceId")) {
                                if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now distributing data (local)";
                                    distributeLocal(chunkId, channelInfo);
                                } else {
                                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now distributing data (remote)";
                                    distributeRemote(chunkId, channelInfo);
                                }
                                break;
                            }
                        }
                    } else {
                        // We should never be here!!
                        throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured, ask BH");
                    }
                }
            }
        }


        unsigned int OutputChannel::getNextSharedInputIdx() {
            // TODO Check modulo 0
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            ++m_sharedInputIndex;
            m_sharedInputIndex %= m_registeredSharedInputs.size();
            return m_sharedInputIndex;
        }


        void OutputChannel::distributeLocal(unsigned int chunkId, const InputChannelInfo& channelInfo) {

            const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

            // Synchronous write as it takes no time here
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT Now distributing (local memory)";
            tcpChannel->write(karabo::util::Hash("channelId", m_channelId, "chunkId", chunkId), std::vector<char>());

            // The input channel will decrement the chunkId usage, as he uses the same memory location
            // unregisterWriterFromChunk(chunkId);
        }


        void OutputChannel::distributeRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {

            const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

            karabo::util::Hash header;
            std::vector<char> data;
            Memory::readAsContiguousBlock(data, header, m_channelId, chunkId);

            tcpChannel->write(header, data); // Blocks whilst writing

            unregisterWriterFromChunk(chunkId);
            //Memory::decrementChannelUsage(m_channelId, chunkId); // Later use this one!

        }


        void OutputChannel::copy(unsigned int chunkId) {

            // If no copied input channels are registered at all, we do not go on
            if (m_registeredCopyInputs.empty()) return;

            for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {

                const karabo::util::Hash& channelInfo = m_registeredCopyInputs[i];
                const std::string& instanceId = channelInfo.get<std::string>("instanceId");
                const std::string& onSlowness = channelInfo.get<std::string>("onSlowness");

                if (hasCopyInput(instanceId)) {
                    eraseCopyInput(instanceId);
                    if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Now copying data (local)";
                        copyLocal(chunkId, channelInfo);
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Now copying data (remote)";
                        copyRemote(chunkId, channelInfo);
                    }
                } else if (onSlowness == "drop") {
                    unregisterWriterFromChunk(chunkId);
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Dropping (copied) data package for " << instanceId;
                } else if (onSlowness == "throw") {
                    unregisterWriterFromChunk(chunkId);
                    throw KARABO_IO_EXCEPTION("Can not write (copied) data because input channel of " + instanceId + " was too late");
                } else if (onSlowness == "queue") {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing (copied) data package for " << instanceId;
                    m_registeredCopyInputs[i].get<std::deque<int> >("queuedChunks").push_back(chunkId);
                } else if (onSlowness == "wait") {
                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Data (copied) is waiting for input channel of "
                            << instanceId << " to be available";
                    while (!hasCopyInput(instanceId)) boost::this_thread::sleep(boost::posix_time::millisec(1));
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " found (copied) input channel after waiting, copying now";
                    eraseCopyInput(instanceId);
                    if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now copying data (local)";
                        copyLocal(chunkId, channelInfo);
                    } else {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now copying data (remote)";
                        copyRemote(chunkId, channelInfo);
                    }
                }
            }
        }


        void OutputChannel::copyLocal(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {
            const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

            // Synchronous write as it takes no time here
            // Writing no data signals input to read from memory
            tcpChannel->write(karabo::util::Hash("channelId", m_channelId, "chunkId", chunkId), std::vector<char>());

            // NOTE: The input channel will decrement the chunkId usage, as he uses the same memory location
            // Having the next line commented is thus correct!!!
            //unregisterWriterFromChunk(chunkId);
        }


        void OutputChannel::copyRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {

            const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

            karabo::util::Hash header;
            std::vector<char> data;
            Memory::readAsContiguousBlock(data, header, m_channelId, chunkId);

            tcpChannel->write(header, data);

            unregisterWriterFromChunk(chunkId);
        }


        std::string OutputChannel::debugId() const {
            // m_channelId is unique per process and not per instance
            return std::string((("OUTPUT " + util::toString(m_channelId) += " of '") += this->getInstanceId()) += "'");
        }

        void OutputChannel::write(const karabo::util::Hash& data,  const OutputChannel::MetaData& metaData) {
             Memory::write(data, m_channelId, m_chunkId, metaData);
        }

        void OutputChannel::write(const karabo::util::Hash& data) {
            OutputChannel::MetaData meta(/*source*/ m_instanceId+":"+m_channelName, /*timestamp*/ karabo::util::Timestamp());
            Memory::write(data, m_channelId, m_chunkId, meta);
        }

        void OutputChannel::write(const karabo::util::Hash::Pointer& data,  const OutputChannel::MetaData& metaData) {
            write(*data, metaData);
        }

        void OutputChannel::write(const karabo::util::Hash::Pointer& data) {
            write(*data);
        }
    }
}
