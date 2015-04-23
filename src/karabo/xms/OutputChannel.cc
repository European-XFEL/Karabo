/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "OutputChannel.hh"
#include "Data.hh"

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
        }


        OutputChannel::OutputChannel(const karabo::util::Hash& config) : m_sharedInputIndex(0) {

            config.get("distributionMode", m_distributionMode);
            config.get("noInputShared", m_onNoSharedInputChannelAvailable);
            config.get("hostname", m_hostname);
            if (m_hostname == "default") m_hostname = boost::asio::ip::host_name();
            config.get("compression", m_compression);

            KARABO_LOG_FRAMEWORK_DEBUG << "NoInputShared: " << m_onNoSharedInputChannelAvailable;

            // Memory related
            m_channelId = MemoryType::registerChannel();
            m_chunkId = MemoryType::registerChunk(m_channelId);

            KARABO_LOG_FRAMEWORK_DEBUG << "Outputting data on channel " << m_channelId << " and chunk " << m_chunkId;

            // Data networking
            // TODO: Use port 0
            int tryAgain = 50; // Try maximum 50 times to start a server
            while (tryAgain > 0) {
                try {
                    m_ownPort = Statics::generateServerPort();
                    karabo::util::Hash h("type", "server", "port", m_ownPort, "compressionUsageThreshold", m_compression * 10E06);
                    m_dataConnection = karabo::net::Connection::create("Tcp", h);
                    m_dataConnection->setErrorHandler(boost::bind(&karabo::xms::OutputChannel::onTcpConnectionError, this, m_dataConnection, _1));
                    m_dataIOService = m_dataConnection->getIOService();
                    m_dataConnection->startAsync(boost::bind(&karabo::xms::OutputChannel::onTcpConnect, this, _1));

                    // Start data thread
                    m_dataThread = boost::thread(boost::bind(&karabo::net::IOService::run, m_dataIOService));

                } catch (...) {
                    if (tryAgain > 0) {
                        tryAgain--;
                        if (m_dataThread.joinable()) {
                            m_dataIOService->stop();
                            m_dataThread.join();
                        }
                        continue;
                    } else {
                        throw KARABO_NETWORK_EXCEPTION("Could not start TcpServer for output channel");
                    }
                }
                tryAgain = 0;
                KARABO_LOG_FRAMEWORK_DEBUG << "Started DeviceOutput-Server listening on port: " << m_ownPort;
            }
        }


        OutputChannel::~OutputChannel() {
            if (m_dataThread.joinable()) {
                m_dataConnection->stop();
                m_dataIOService->stop();
                m_dataThread.join();
            }
            MemoryType::unregisterChannel(m_channelId);
        }


        void OutputChannel::setInstanceId(const std::string& instanceId) {
            m_instanceId = instanceId;
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


        void OutputChannel::onTcpConnect(TcpChannelPointer channel) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Connection established";
            channel->setErrorHandler(boost::bind(&karabo::xms::OutputChannel::onTcpChannelError, this, channel, _1));
            channel->readAsyncHash(boost::bind(&karabo::xms::OutputChannel::onTcpChannelRead, this, channel, _1));
            m_dataConnection->startAsync(boost::bind(&karabo::xms::OutputChannel::onTcpConnect, this, _1));
        }


        void OutputChannel::onTcpConnectionError(karabo::net::Connection::Pointer conn, const karabo::net::ErrorCode& error) {
            KARABO_LOG_FRAMEWORK_ERROR << "Tcp connection error, code: " << error.value() << ", message: " << error.message();
        }


        void OutputChannel::onTcpChannelError(const TcpChannelPointer& channel, const karabo::net::ErrorCode& error) {
            KARABO_LOG_FRAMEWORK_ERROR << "Tcp channel error, code: " << error.value() << ", message: " << error.message();
            if (error.value() == 2) { // End of file
                // Unregister channel
                onInputGone(channel);
            }
        }


        void OutputChannel::onTcpChannelRead(TcpChannelPointer channel, const karabo::util::Hash& message) {

            std::string reason;
            if (message.has("reason")) message.get<std::string > ("reason", reason);

            if (reason == "hello") {

                /* The hello message is expected to have:
                 *     instanceId (std::string)
                 *     memoryLocation (std::string) [local/remote]
                 *     dataDistribution (std::string) [distribute/copy]
                 *     onSlowness (std::string) [queue/drop/wait/error]
                 */

                std::string instanceId = message.get<std::string > ("instanceId");
                std::string memoryLocation = message.get<std::string > ("memoryLocation");
                std::string dataDistribution = message.get<std::string > ("dataDistribution");
                std::string onSlowness = message.get<std::string > ("onSlowness");

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
                    std::string instanceId = message.get<std::string > ("instanceId");
                    KARABO_LOG_FRAMEWORK_DEBUG << "InstanceId " << instanceId << " has updated...";
                    onInputAvailable(instanceId);
                }

            }
            channel->readAsyncHash(boost::bind(&karabo::xms::OutputChannel::onTcpChannelRead, this, channel, _1));
        }


        void OutputChannel::onInputAvailable(const std::string& instanceId) {

            for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                if (channelInfo.get<std::string>("instanceId") == instanceId) {
                    if (!channelInfo.get<std::deque<int> >("queuedChunks").empty()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Writing queued (shared) data to instance " << instanceId;
                        distributeQueue(channelInfo);
                        return;
                    }
                    pushShareNext(instanceId);
                    KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT New (shared) input on instance " << instanceId << " available for writing ";
                    this->triggerIOEvent();
                    return;
                }
            }
            for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {
                karabo::util::Hash& channelInfo = m_registeredCopyInputs[i];
                if (channelInfo.get<std::string>("instanceId") == instanceId) {
                    if (!channelInfo.get<std::deque<int> >("queuedChunks").empty()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Writing queued (copied) data to instance " << instanceId;
                        copyQueue(channelInfo);
                        return;
                    }
                    pushCopyNext(instanceId);
                    KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT New (copied) input on instance " << instanceId << " available for writing ";
                    this->triggerIOEvent();
                    return;
                }
            }
            KARABO_LOG_FRAMEWORK_WARN << "OUTPUT An input channel wants to connect (" << instanceId << ") that was not registered before.";
        }


        void OutputChannel::onInputGone(const TcpChannelPointer& channel) {

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
                if (m_ioEventHandler) m_ioEventHandler(shared_from_this());
            } catch (karabo::util::Exception& e) {
                KARABO_RETHROW;
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

            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT update()";

            // If no data was written return
            if (MemoryType::size(m_channelId, m_chunkId) == 0) return;

            // Take current chunkId for sending
            unsigned int chunkId = m_chunkId;

            // This will increase the usage counts for this chunkId
            // by the number of all interested connected inputs
            registerWritersOnChunk(chunkId);

            // We are done with this chunkId, it will stay alive only until
            // all inputs are served (see above)
            MemoryType::unregisterChunk(m_channelId, chunkId);

            // Register new chunkId for writing to
            m_chunkId = MemoryType::registerChunk(m_channelId);

            // Distribute chunk(s)
            distribute(chunkId);

            // Copy chunk(s)
            copy(chunkId);

        }


        void OutputChannel::signalEndOfStream() {

            // If there is still some data in the pipe, put it out
            if (MemoryType::size(m_channelId, m_chunkId) > 0) update();

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
            if (!m_registeredSharedInputs.empty()) MemoryType::incrementChunkUsage(m_channelId, chunkId);
            for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) MemoryType::incrementChunkUsage(m_channelId, chunkId);
            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Registered " << MemoryType::getChunkStatus(m_channelId, chunkId) << " uses for [" << m_channelId << "][" << chunkId << "]";
        }


        void OutputChannel::unregisterWriterFromChunk(int chunkId) {
            MemoryType::decrementChunkUsage(m_channelId, chunkId);
            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT " << MemoryType::getChunkStatus(m_channelId, chunkId) << " uses left for [" << m_channelId << "][" << chunkId << "]";
        }


        void OutputChannel::distribute(unsigned int chunkId) {

            // If no shared input channels are registered at all, we do not go on
            if (m_registeredSharedInputs.empty()) return;

            // Next input
            unsigned int sharedInputIdx = getNextSharedInputIdx();

            if (m_distributionMode == "round-robin") {

                karabo::util::Hash channelInfo = m_registeredSharedInputs[sharedInputIdx];
                std::string instanceId = channelInfo.get<std::string>("instanceId");

                if (hasSharedInput(instanceId)) { // Found
                    eraseSharedInput(instanceId);
                    if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Now distributing data (local)";
                        distributeLocal(chunkId, channelInfo);
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Now distributing data (remote)";
                        distributeRemote(chunkId, channelInfo);
                    }
                } else { // Not found
                    if (m_onNoSharedInputChannelAvailable == "drop") {
                        unregisterWriterFromChunk(chunkId);
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Dropping (shared) data package with chunkId: " << chunkId;

                    } else if (m_onNoSharedInputChannelAvailable == "throw") {
                        unregisterWriterFromChunk(chunkId);
                        throw KARABO_IO_EXCEPTION("Can not write data because no (shared) input is available");

                    } else if (m_onNoSharedInputChannelAvailable == "queue") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Queuing (shared) data package with chunkId: " << chunkId;
                        // TODO Mutex !!!
                        m_registeredSharedInputs[sharedInputIdx].get<std::deque<int> >("queuedChunks").push_back(chunkId);

                    } else if (m_onNoSharedInputChannelAvailable == "wait") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Waiting for available (shared) input channel...";
                        while (!hasSharedInput(instanceId)) {
                            boost::this_thread::sleep(boost::posix_time::millisec(1));
                        }
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT found (shared) input channel after waiting, distributing now";
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
                    std::string instanceId = popShareNext();
                    for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                        const karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                        if (instanceId == channelInfo.get<std::string>("instanceId")) {
                            if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                                distributeLocal(chunkId, channelInfo);
                            } else {
                                distributeRemote(chunkId, channelInfo);
                            }
                            break;
                        }
                    }
                } else { // Not found
                    if (m_onNoSharedInputChannelAvailable == "drop") {
                        unregisterWriterFromChunk(chunkId);
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Dropping (shared) data package with chunkId: " << chunkId;
                    } else if (m_onNoSharedInputChannelAvailable == "throw") {
                        unregisterWriterFromChunk(chunkId);
                        throw KARABO_IO_EXCEPTION("Can not write data because no (shared) input is available");
                    } else if (m_onNoSharedInputChannelAvailable == "queue") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Queuing (shared) data package with chunkId: " << chunkId;
                        // TODO Mutex !!!
                        m_registeredSharedInputs[sharedInputIdx].get<std::deque<int> >("queuedChunks").push_back(chunkId);
                    } else if (m_onNoSharedInputChannelAvailable == "wait") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Waiting for available (shared) input channel...";
                        while (m_shareNext.empty()) {
                            boost::this_thread::sleep(boost::posix_time::millisec(1));
                        }
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT found (shared) input channel after waiting, distributing now";
                        std::string instanceId = popShareNext();
                        for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                            const karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                            if (instanceId == channelInfo.get<std::string>("instanceId")) {
                                if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                                    distributeLocal(chunkId, channelInfo);
                                } else {
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
            m_sharedInputIndex = (++m_sharedInputIndex) % m_registeredSharedInputs.size();
            return m_sharedInputIndex;
        }


        void OutputChannel::distributeLocal(unsigned int chunkId, const InputChannelInfo& channelInfo) {

            const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

            // Synchronous write as it takes no time here
            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Now writing out (local memory)";
            tcpChannel->write(karabo::util::Hash("channelId", m_channelId, "chunkId", chunkId), std::vector<char>());

            // The input channel will decrement the chunkId usage, as he uses the same memory location
            // unregisterWriterFromChunk(chunkId);
        }


        void OutputChannel::distributeRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {

            const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

            karabo::util::Hash header;
            std::vector<char> data;
            MemoryType::readAsContiguosBlock(data, header, m_channelId, chunkId);

            tcpChannel->write(header, data); // Blocks whilst writing

            unregisterWriterFromChunk(chunkId);
            //MemoryType::decrementChannelUsage(m_channelId, chunkId); // Later use this one!

        }


        void OutputChannel::copy(unsigned int chunkId) {

            // If no copied input channels are registered at all, we do not go on
            if (m_registeredCopyInputs.empty()) return;

            for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {

                karabo::util::Hash& channelInfo = m_registeredCopyInputs[i];
                std::string instanceId = channelInfo.get<std::string>("instanceId");
                std::string onSlowness = channelInfo.get<std::string>("onSlowness");

                if (hasCopyInput(instanceId)) {
                    eraseCopyInput(instanceId);
                    if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                        copyLocal(chunkId, channelInfo);
                    } else {
                        copyRemote(chunkId, channelInfo);
                    }
                } else if (onSlowness == "drop") {
                    KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Dropping (copied) data package for " << instanceId;
                } else if (onSlowness == "throw") {
                    throw KARABO_IO_EXCEPTION("Can not write (copied) data because input channel of " + instanceId + " was too late");
                } else if (onSlowness == "queue") {
                    KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Queuing (copied) data package for " << instanceId;
                    m_registeredCopyInputs[i].get<std::deque<int> >("queuedChunks").push_back(chunkId);
                } else if (onSlowness == "wait") {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Data (copied) is waiting for input channel of " << instanceId << " to be available";
                    while (!hasCopyInput(instanceId)) boost::this_thread::sleep(boost::posix_time::millisec(1));
                    KARABO_LOG_FRAMEWORK_DEBUG << "Found channel, copying now";
                    eraseCopyInput(instanceId);
                    if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                        copyLocal(chunkId, channelInfo);
                    } else {
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
            MemoryType::readAsContiguosBlock(data, header, m_channelId, chunkId);

            tcpChannel->write(header, data);

            unregisterWriterFromChunk(chunkId);
        }


    }
}
