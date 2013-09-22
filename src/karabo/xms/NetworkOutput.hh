/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_NETWORKOUTPUT_HH
#define	KARABO_XMS_NETWORKOUTPUT_HH

#include <boost/asio.hpp>
#include <karabo/util.hpp>
#include <karabo/log.hpp>
#include <karabo/io.hpp>
#include <karabo/net.hpp>

#include "Statics.hh"
#include "Memory.hh"


/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xms {

        /**
         * The DeviceOutput class.
         */
        template <class T>
        class NetworkOutput : public karabo::io::Output<T> {

            typedef boost::shared_ptr<karabo::net::Channel> TcpChannelPointer;

            /*
             * InputChannelInfo (karabo::util::Hash)
             * 
             *     instanceId (std::string)
             *     memoryLocation (std::string) [local/remote]
             *     tcpChannel (TcpChannelPointer)
             *     onSlowness (std::string) [queue/drop/wait/throw]
             *     queuedChunks (std::deque<int>)
             *
             */
            typedef karabo::util::Hash InputChannelInfo;

            typedef std::vector<InputChannelInfo> InputChannels;

            typedef std::deque< std::string > InputChannelQueue;

            typedef std::map<unsigned int, int> CurrentWritersCount;

            typedef std::map<TcpChannelPointer, unsigned int> TcpChannelPointer2ChunkId;

            typedef Memory<T> MemoryType;

            // Server related
            unsigned int m_ownPort;

            karabo::net::Connection::Pointer m_dataConnection;
            //TcpChannelPointer m_dataChannel;
            karabo::net::IOService::Pointer m_dataIOService;
            boost::thread m_dataThread;

            std::string m_onNoSharedInputChannelAvailable;
            std::string m_distributionMode;

            InputChannels m_registeredSharedInputs;
            InputChannels m_registeredCopyInputs;

            unsigned int m_sharedInputIndex;

            InputChannelQueue m_shareNext;
            InputChannelQueue m_copyNext;

            boost::mutex m_nextInputMutex;
            boost::mutex m_chunkIdsMutex;
            boost::mutex m_currentWritersCountMutex;
            boost::mutex m_onTcpReadMutex;
            boost::mutex m_updateMutex;

            // Async out
            CurrentWritersCount m_currentWritersCount;
            std::map<TcpChannelPointer, unsigned int> m_channel2ChunkId;

            unsigned int m_channelId;
            unsigned int m_chunkId;

            std::map<int, int> m_writersOnChunk;

            bool m_isEndOfStream;



        public:

            KARABO_CLASSINFO(NetworkOutput, "Network-" + karabo::io::getIODataType<T>(), "1.0")

            virtual ~NetworkOutput() {
                if (m_dataThread.joinable()) {
                    m_dataConnection->stop();
                    m_dataIOService->stop();
                    m_dataThread.join();
                }
                MemoryType::clearChannel(m_channelId);
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
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
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            NetworkOutput(const karabo::util::Hash& config) : karabo::io::Output<T>(config), m_sharedInputIndex(0), m_isEndOfStream(false) {

                config.get("distributionMode", m_distributionMode);
                config.get("noInputShared", m_onNoSharedInputChannelAvailable);

                KARABO_LOG_FRAMEWORK_DEBUG << "NoInputShared: " << m_onNoSharedInputChannelAvailable;

                // Memory related
                m_channelId = MemoryType::registerChannel();
                m_chunkId = MemoryType::registerChunk(m_channelId);

                // Data networking
                int tryAgain = 5; // Try maximum 5 times to start a server
                while (tryAgain > 0) {
                    try {
                        m_ownPort = Statics::generateServerPort();
                        karabo::util::Hash h("Tcp.type", "server", "Tcp.port", m_ownPort);
                        m_dataConnection = karabo::net::Connection::create(h);
                        m_dataConnection->setErrorHandler(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpConnectionError, this, _1, _2));
                        m_dataIOService = m_dataConnection->getIOService();
                        m_dataConnection->startAsync(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpConnect, this, _1));

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

            karabo::util::Hash getInformation() const {
                return karabo::util::Hash("connectionType", "tcp", "hostname", boost::asio::ip::host_name(), "port", m_ownPort);
            }

            void write(const T& data) {
                MemoryType::write(data, m_channelId, m_chunkId);
            }

            void onTcpConnect(TcpChannelPointer channel) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Connection established";
                channel->setErrorHandler(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpChannelError, this, _1, _2));
                channel->readAsyncHash(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpChannelRead, this, _1, _2));
                m_dataConnection->startAsync(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpConnect, this, _1));
            }

            void onTcpConnectionError(TcpChannelPointer, const karabo::net::ErrorCode& error) {
                KARABO_LOG_FRAMEWORK_ERROR << error.message();
            }

            void onTcpChannelError(TcpChannelPointer, const karabo::net::ErrorCode& error) {
                KARABO_LOG_FRAMEWORK_ERROR << error.message();
            }

            void onTcpChannelRead(TcpChannelPointer channel, const karabo::util::Hash& message) {

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
                        KARABO_LOG_FRAMEWORK_DEBUG << "Registering copy-input channel of instance: " << instanceId;
                        m_registeredCopyInputs.push_back(info);
                    }
                    onInputAvailable(instanceId); // Immediately register for reading

                } else if (reason == "update") {

                    if (message.has("instanceId")) {
                        std::string instanceId = message.get<std::string > ("instanceId");
                        KARABO_LOG_FRAMEWORK_DEBUG << "InstanceId " << instanceId << " has updated...";
                        onInputAvailable(instanceId);
                    }

                }
                channel->readAsyncHash(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpChannelRead, this, _1, _2));
            }

            void onInputAvailable(const std::string& instanceId) {

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
                        //this->template triggerIOEvent<karabo::io::Output<T> >();
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
                        //this->template triggerIOEvent<karabo::io::Output<T> >();
                        this->triggerIOEvent();
                        return;
                    }
                }
                KARABO_LOG_FRAMEWORK_WARN << "OUTPUT An input channel wants to connect (" << instanceId << ") that was not registered before.";
            }

            void distributeQueue(karabo::util::Hash& channelInfo) {
                std::deque<int>& chunkIds = channelInfo.get<std::deque<int> >("queuedChunks");
                int chunkId = chunkIds.front();
                chunkIds.pop_front();
                if (channelInfo.get<std::string > ("memoryLocation") == "local") distributeLocal(chunkId, channelInfo);
                else distributeRemote(chunkId, channelInfo);
            }

            void copyQueue(karabo::util::Hash& channelInfo) {
                std::deque<int>& chunkIds = channelInfo.get<std::deque<int> >("queuedChunks");
                int chunkId = chunkIds.front();
                chunkIds.pop_front();
                if (channelInfo.get<std::string > ("memoryLocation") == "local") copyLocal(chunkId, channelInfo);
                else copyRemote(chunkId, channelInfo);
            }

            void pushShareNext(const std::string& instanceId) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                m_shareNext.push_back(instanceId);
            }

            std::string popShareNext() {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                std::string info = m_shareNext.front();
                m_shareNext.pop_front();
                return info;
            }

            bool hasSharedInput(const std::string& instanceId) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                return (std::find(m_shareNext.begin(), m_shareNext.end(), instanceId) != m_shareNext.end());
            }

            void eraseSharedInput(const std::string& instanceId) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                InputChannelQueue::iterator it = std::find(m_shareNext.begin(), m_shareNext.end(), instanceId);
                if (it != m_shareNext.end()) m_shareNext.erase(it);
            }

            void pushCopyNext(const std::string& info) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                m_copyNext.push_back(info);
            }

            std::string popCopyNext() {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                std::string info = m_copyNext.front();
                m_copyNext.pop_front();
                return info;
            }

            bool hasCopyInput(const std::string& instanceId) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                return std::find(m_copyNext.begin(), m_copyNext.end(), instanceId) != m_copyNext.end();
            }

            void eraseCopyInput(const std::string& instanceId) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                InputChannelQueue::iterator it = std::find(m_copyNext.begin(), m_copyNext.end(), instanceId);
                if (it != m_copyNext.end()) m_copyNext.erase(it);
            }

            bool canCompute() const {
                return true;
            }

            void update() {

                KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT update()";

                // If no data was written return
                if (MemoryType::size(m_channelId, m_chunkId) == 0) return;

                // Unset endOfStream flag
                m_isEndOfStream = false;

                registerWritersOnChunk();

                // Distribute chunk(s)
                distribute();

                // Copy chunk(s)
                copy();

                m_chunkId = MemoryType::registerChunk(m_channelId);
            }

            void signalEndOfStream() {

                // End of stream should be send only exactly once
                if (m_isEndOfStream) return;

                // If there is still some data in the pipe, put it out
                if (MemoryType::size(m_channelId, m_chunkId) > 0) update();
                
                // Update sets m_isEndOfStream to false -> we have to set it here, now
                // It is also needed to switch to synchronous TCP write as we have to now wait until all left data
                // was sent.
                m_isEndOfStream = true;
                
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

            void registerWritersOnChunk() {
                int nWriters = 0;
                if (!m_registeredSharedInputs.empty()) nWriters++;
                nWriters += m_registeredCopyInputs.size();
                m_writersOnChunk[m_chunkId] = nWriters;
                KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Registered " << nWriters << " for chunkId " << m_chunkId;
            }

            void unregisterWriterFromChunk(int chunkId) {
                m_writersOnChunk[chunkId]--;
                KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT " << m_writersOnChunk[chunkId] << " Writers  left for chunkId " << chunkId;
                if (m_writersOnChunk[chunkId] == 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Releasing memory for chunk " << chunkId;
                    MemoryType::clearChunk(m_channelId, chunkId);
                }
            }

            void distribute() {

                // If no shared input channels are registered at all, we do not go on
                if (m_registeredSharedInputs.empty()) return;

                // Next input
                m_sharedInputIndex = (++m_sharedInputIndex) % m_registeredSharedInputs.size();

                if (m_distributionMode == "round-robin") {

                    karabo::util::Hash channelInfo = m_registeredSharedInputs[m_sharedInputIndex];
                    std::string instanceId = channelInfo.get<std::string>("instanceId");

                    if (hasSharedInput(instanceId)) { // Found
                        eraseSharedInput(instanceId);
                        if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                            distributeLocal(m_chunkId, channelInfo);
                        } else {
                            distributeRemote(m_chunkId, channelInfo);
                        }
                    } else { // Not found
                        if (m_onNoSharedInputChannelAvailable == "drop") {
                            unregisterWriterFromChunk(m_chunkId);
                            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Dropping (shared) data package with chunkId: " << m_chunkId;

                        } else if (m_onNoSharedInputChannelAvailable == "throw") {
                            unregisterWriterFromChunk(m_chunkId);
                            throw KARABO_IO_EXCEPTION("Can not write data because no (shared) input is available");

                        } else if (m_onNoSharedInputChannelAvailable == "queue") {
                            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Queuing (shared) data package with chunkId: " << m_chunkId;
                            // TODO Mutex !!!
                            m_registeredSharedInputs[m_sharedInputIndex].get<std::deque<int> >("queuedChunks").push_back(m_chunkId);

                        } else if (m_onNoSharedInputChannelAvailable == "wait") {
                            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Waiting for available (shared) input channel...";
                            while (!hasSharedInput(instanceId)) {
                                boost::this_thread::sleep(boost::posix_time::millisec(500));
                            }
                            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT found (shared) input channel after waiting, distributing now";
                            if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                                distributeLocal(m_chunkId, channelInfo);
                            } else {
                                distributeRemote(m_chunkId, channelInfo);
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
                                    distributeLocal(m_chunkId, channelInfo);
                                } else {
                                    distributeRemote(m_chunkId, channelInfo);
                                }
                                break;
                            }
                        }
                    } else { // Not found
                        if (m_onNoSharedInputChannelAvailable == "drop") {
                            unregisterWriterFromChunk(m_chunkId);
                            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Dropping (shared) data package with chunkId: " << m_chunkId;
                        } else if (m_onNoSharedInputChannelAvailable == "throw") {
                            unregisterWriterFromChunk(m_chunkId);
                            throw KARABO_IO_EXCEPTION("Can not write data because no (shared) input is available");
                        } else if (m_onNoSharedInputChannelAvailable == "queue") {
                            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Queuing (shared) data package with chunkId: " << m_chunkId;
                            // TODO Mutex !!!
                            m_registeredSharedInputs[m_sharedInputIndex].get<std::deque<int> >("queuedChunks").push_back(m_chunkId);
                        } else if (m_onNoSharedInputChannelAvailable == "wait") {
                            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Waiting for available (shared) input channel...";
                            while (m_shareNext.empty()) {
                                boost::this_thread::sleep(boost::posix_time::millisec(500));
                            }
                            KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT found (shared) input channel after waiting, distributing now";
                            std::string instanceId = popShareNext();
                            for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                                const karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                                if (instanceId == channelInfo.get<std::string>("instanceId")) {
                                    if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                                        distributeLocal(m_chunkId, channelInfo);
                                    } else {
                                        distributeRemote(m_chunkId, channelInfo);
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

            void distributeLocal(const unsigned int& chunkId, const InputChannelInfo & channelInfo) {

                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                // Synchronous write as it takes no time here
                KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Now writing out (local memory)";
                tcpChannel->write(karabo::util::Hash("channelId", m_channelId, "chunkId", chunkId), std::vector<char>());
                //unregisterWriterFromChunk(chunkId);
            }

            void distributeRemote(const unsigned int& chunkId, const InputChannelInfo & channelInfo) {

                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");
                
                if (m_isEndOfStream) { // write synchronously
                    karabo::util::Hash header;
                    std::vector<char> data;
                    MemoryType::readAsContiguosBlock(data, header, m_channelId, chunkId);
                    tcpChannel->write(header, data);
                } else {
                    registerAsyncWrite(tcpChannel, chunkId);
                    const std::pair< std::vector<char>, karabo::util::Hash>& entry = getAsyncWriteData(chunkId);
                    KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Going to distribute " << entry.first.size() << " bytes of data";
                    KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT With header: " << entry.second;
                    tcpChannel->writeAsyncHashVector(entry.second, entry.first, boost::bind(&karabo::xms::NetworkOutput<T>::onWriteCompleted, this, _1));
                }
            }

            void registerAsyncWrite(const TcpChannelPointer& channel, const unsigned int& chunkId) {

                m_currentWritersCountMutex.lock();

                m_channel2ChunkId[channel] = chunkId;
                CurrentWritersCount::iterator it = m_currentWritersCount.find(chunkId);
                if (it == m_currentWritersCount.end()) { // No one tried to write this chunk, yet
                    m_currentWritersCountMutex.unlock();
                    MemoryType::cacheAsContiguousBlock(m_channelId, chunkId);
                    m_currentWritersCountMutex.lock();
                }
                m_currentWritersCount[chunkId]++;

                m_currentWritersCountMutex.unlock();
            }

            const std::pair<std::vector<char>, karabo::util::Hash>& getAsyncWriteData(const unsigned int& chunkId) {
                return MemoryType::readContiguousBlockCache(m_channelId, chunkId);
            }

            void onWriteCompleted(TcpChannelPointer channel) {

                boost::mutex::scoped_lock lock(m_currentWritersCountMutex);

                TcpChannelPointer2ChunkId::iterator it = m_channel2ChunkId.find(channel);
                if (it != m_channel2ChunkId.end()) {
                    unsigned int chunkId = it->second;
                    m_currentWritersCount[chunkId]--;

                    if (m_currentWritersCount[chunkId] == 0) {
                        MemoryType::clearContiguousBlockCache(m_channelId, chunkId);
                        m_currentWritersCount.erase(chunkId);
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Cleared asynchronous write cache";
                        unregisterWriterFromChunk(chunkId);
                    }
                } else {
                    throw KARABO_LOGIC_EXCEPTION("Bad async write encountered");
                }
            }

            void copy() {

                // If no copied input channels are registered at all, we do not go on
                if (m_registeredCopyInputs.empty()) return;

                for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {

                    karabo::util::Hash channelInfo = m_registeredCopyInputs[i];
                    std::string instanceId = channelInfo.get<std::string>("instanceId");
                    std::string onSlowness = channelInfo.get<std::string>("onSlowness");

                    if (hasCopyInput(instanceId)) {
                        eraseCopyInput(instanceId);
                        if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                            copyLocal(m_chunkId, channelInfo);
                        } else {
                            copyRemote(m_chunkId, channelInfo);
                        }
                    } else if (onSlowness == "drop") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Dropping (copied) data package for " << instanceId;
                    } else if (onSlowness == "throw") {
                        throw KARABO_IO_EXCEPTION("Can not write (copied) data because input channel of " + instanceId + " was too late");
                    } else if (onSlowness == "queue") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Queuing (copied) data package for " << instanceId;
                        m_registeredCopyInputs[i].get<std::deque<int> >("queuedChunks").push_back(m_chunkId);
                    } else if (onSlowness == "wait") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Data (copied) is waiting for input channel of " << instanceId << " to be available";
                        while (!hasCopyInput(instanceId)) boost::this_thread::sleep(boost::posix_time::millisec(500));
                        KARABO_LOG_FRAMEWORK_DEBUG << "Found channel, copying now";
                        eraseCopyInput(instanceId);
                        if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                            copyLocal(m_chunkId, channelInfo);
                        } else {
                            copyRemote(m_chunkId, channelInfo);
                        }
                    }
                }
            }

            void copyLocal(const unsigned int& chunkId, const InputChannelInfo & channelInfo) {
                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                // Synchronous write as it takes no time here
                // Writing no data signals input to read from memory
                tcpChannel->write(karabo::util::Hash("channelId", m_channelId, "chunkId", chunkId), std::vector<char>());
            }

            void copyRemote(const unsigned int& chunkId, const InputChannelInfo & channelInfo) {

                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");
 
                if (m_isEndOfStream) { // write synchronously
                    karabo::util::Hash header;
                    std::vector<char> data;
                    MemoryType::readAsContiguosBlock(data, header, m_channelId, chunkId);
                    tcpChannel->write(header, data);
                } else {
                    registerAsyncWrite(tcpChannel, chunkId);
                    const std::pair< std::vector<char>, karabo::util::Hash>& entry = getAsyncWriteData(chunkId);
                    KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT Going to copy " << entry.first.size() << " bytes of data";
                    KARABO_LOG_FRAMEWORK_DEBUG << "OUTPUT With header: " << entry.second;
                    tcpChannel->writeAsyncHashVector(entry.second, entry.first, boost::bind(&karabo::xms::NetworkOutput<T>::onWriteCompleted, this, _1));
                }
            }
        };

    }
}

#endif
