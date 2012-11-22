/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_INTERINSTANCEOUTPUT_HH
#define	KARABO_XMS_INTERINSTANCEOUTPUT_HH

#include <boost/asio.hpp>

#include <karabo/net/IOService.hh>
#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>

#include "Statics.hh"
#include "Memory.hh"
#include "Output.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xms {

        /**
         * The DeviceOutput class.
         */
        template <class T>
        class NetworkOutput : public Output<T> {
            typedef boost::shared_ptr<karabo::net::Channel> TcpChannelPointer;

            /*
             * InputChannelInfo (karabo::util::Hash)
             * 
             *     memoryLocation (std::string) [local/remote]
             *     dataDistribution (std::string) [distribute/copy]
             *     tcpChannel (TcpChannelPointer)
             *
             */
            typedef karabo::util::Hash InputChannelInfo;

            /*
             * InputChannels (karabo::util::Hash)
             * 
             * instanceId (karabo::util::Hash)
             *     InputChannelInfo (s.a.)
             *
             */
            typedef karabo::util::Hash InputChannels;

            typedef std::deque< InputChannelInfo > InputChannelQueue;

            typedef std::map<unsigned int, int> CurrentWritersCount;

            typedef std::map<TcpChannelPointer, unsigned int> TcpChannelPointer2ChunkId;


        public:

            KARABO_CLASSINFO(NetworkOutput, "NetworkOutput-" + T::classInfo().getClassId(), "1.0")

            /**
             * Default constructor.
             */
            NetworkOutput() {
            };

            /**
             * Destructor.
             */
            virtual ~NetworkOutput() {
                if (m_dataThread.joinable()) {
                    m_dataConnection->close();
                    m_dataIOService->stop();
                    m_dataThread.join();
                }
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

                STRING_ELEMENT(expected).key("noInputShared")
                        .displayedName("No Input (Shared)")
                        .description("What to do if currently no share-input channel is available for writing to")
                        .options("drop,queue,throw,wait")
                        .assignmentOptional().defaultValue("wait")
                        .init()
                        .commit();

                STRING_ELEMENT(expected).key("noInputCopy")
                        .displayedName("No Input (Copy)")
                        .description("What to do if one (or more) copy-input channel(s) are missing")
                        .options("drop,throw,wait")
                        .assignmentOptional().defaultValue("wait")
                        .init()
                        .commit();
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash& input) {

                input.get("noInputShared", m_onNoSharedInputChannelAvailable);
                input.get("noInputCopy", m_onNoCopiedInputChannelAvailable);
                
                std::cout << "NoInputShared: " << m_onNoSharedInputChannelAvailable << std::endl;
                std::cout << "NoInputCopy: " << m_onNoCopiedInputChannelAvailable << std::endl;

                // Memory related
                m_channelId = Memory<T>::registerChannel();
                m_chunkId = Memory<T>::registerChunk(m_channelId);

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
                            throw NETWORK_EXCEPTION("Could not start TcpServer for output channel");
                        }
                    }
                    tryAgain = 0;
                    std::cout << "Started DeviceOutput-Server listening on port: " << m_ownPort << std::endl;
                }


            }

            karabo::util::Hash getInformation() const {
                return karabo::util::Hash("connectionType", "tcp", "hostname", boost::asio::ip::host_name(), "port", m_ownPort);
            }

            void write(const T& data) {
                Memory<T>::write(data, m_channelId, m_chunkId);
            }

            void onTcpConnect(TcpChannelPointer channel) {
                std::cout << "Connection established" << std::endl;
                channel->setErrorHandler(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpChannelError, this, _1, _2));
                channel->readAsyncHash(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpChannelRead, this, _1, _2));
                m_dataConnection->startAsync(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpConnect, this, _1));
            }

            void onTcpConnectionError(TcpChannelPointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelError(TcpChannelPointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;

            }

            void onTcpChannelRead(TcpChannelPointer channel, const karabo::util::Hash& message) {

                std::string reason;
                if (message.has("reason")) message.get<std::string > ("reason", reason);

                if (reason == "hello") {

                    // Associate instanceId with channel
                    if (message.has("instanceId") && message.has("memoryLocation") && message.has("dataDistribution")) {

                        std::string instanceId = message.get<std::string > ("instanceId");
                        std::string dataDistribution = message.get<std::string > ("dataDistribution");
                        std::string memoryLocation = message.get<std::string > ("memoryLocation");

                        karabo::util::Hash info;
                        info.set("memoryLocation", memoryLocation);
                        info.set("tcpChannel", channel);

                        if (dataDistribution == "shared") {
                            std::cout << "Registering shared-input channel of instance: " << instanceId << std::endl;
                            m_sharedInputs.set(instanceId, info);
                        } else {
                            std::cout << "Registering copy-input channel of instance: " << instanceId << std::endl;
                            m_copiedInputs.set(instanceId, info);
                        }
                        std::cout << "With meta-data: " << info;

                        onInputAvailable(instanceId); // Immediately register for reading
                    }
                } else if (reason == "update") {

                    if (message.has("instanceId")) {
                        std::string instanceId = message.get<std::string > ("instanceId");
                        std::cout << "InstanceId " << instanceId << " has updated..." << std::endl;
                        onInputAvailable(instanceId);
                    }

                }
                channel->readAsyncHash(boost::bind(&karabo::xms::NetworkOutput<T>::onTcpChannelRead, this, _1, _2));
            }

            void onInputAvailable(const std::string& instanceId) {

                if (m_sharedInputs.has(instanceId)) {
                    InputChannelInfo channelInfo = m_sharedInputs.get<karabo::util::Hash > (instanceId);
                    pushShareNext(channelInfo);
                    std::cout << "OUTPUT: New (shared) input on instance " << instanceId << " available for writing " << std::endl;
                    if (m_finishedSharedChunkIds.size() > 0) {
                        //this->autoDistributeQueue();
                        return;
                    }
                } else if (m_copiedInputs.has(instanceId)) {
                    InputChannelInfo channelInfo = m_copiedInputs.get<karabo::util::Hash > (instanceId);
                    pushCopyNext(channelInfo);
                    std::cout << "OUTPUT: New (copied) input on instance " << instanceId << " available for writing " << std::endl;
                } else {
                    std::cout << "OUTPUT: LOW-LEVEL-DEBUG An input channel wants to connect, that was not registered before." << std::endl;
                }
                this->template triggerIOEvent<Output<T> >();
            }

            void pushShareNext(const InputChannelInfo& info) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                m_shareNext.push_back(info);
            }

            InputChannelInfo popShareNext() {
                InputChannelInfo info = m_shareNext.front();
                m_shareNext.pop_front();
                return info;
            }

            void pushCopyNext(const InputChannelInfo& info) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                m_copyNext.push_back(info);
            }

            InputChannelInfo popCopyNext() {
                InputChannelInfo info = m_copyNext.front();
                m_copyNext.pop_front();
                return info;
            }

            bool canCompute() const {
                return true;
//                //boost::mutex::scoped_lock lock(m_nextInputMutex);
//                return !m_shareNext.empty();
//                        }
//                } else 
//                if (m_copiedInputs.size() > 0 && m_copyNext.size() != m_copiedInputs.size()) {
//                    if (m_onNoCopiedInputChannelAvailable != "drop") {
//                        return false;
//                    }
//                }
//                return true;
            }

            void autoDistributeQueue() {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                std::cout << "OUTPUT: Auto-distributing queued data" << std::endl;
                unsigned int chunkId = popSharedChunkId();
                InputChannelInfo channelInfo = popShareNext();
                if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                    distributeLocal(chunkId, channelInfo);
                } else {
                    distributeRemote(chunkId, channelInfo);
                }
            }

            void update() {

                std::cout << "OUTPUT: update" << std::endl;

                // Distribute chunk(s)
                distribute();

                // Copy chunk(s)
                copy();

                m_chunkId = Memory<T>::registerChunk(m_channelId);
            }

            void distribute() {

                bool goOn = checkAndHandleSharedInputs();

                if (goOn) {
                    boost::mutex::scoped_lock lock(m_nextInputMutex);

                    std::cout << "OUTPUT: finishedChunks " << m_finishedSharedChunkIds.size() << " shareNext " << m_shareNext.size() << std::endl;
                    while (!m_finishedSharedChunkIds.empty() && !m_shareNext.empty()) {

                        unsigned int chunkId = popSharedChunkId();
                        InputChannelInfo channelInfo = popShareNext();

                        if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                            distributeLocal(chunkId, channelInfo);
                        } else {
                            distributeRemote(chunkId, channelInfo);
                        }
                    }
                }
            }

            bool checkAndHandleSharedInputs() {

                // If no shared input channels are registered at all, we do not go on
                if (m_sharedInputs.empty()) return false;

                // If shared inputs channels are available for distribution go on
                if (!m_shareNext.empty()) {
                    pushSharedChunkId(m_chunkId);
                    return true;
                }

                // There are shared inputs registered but currently not available -> react as configured
                if (m_onNoSharedInputChannelAvailable == "drop") {
                    std::cout << "OUTPUT: Dropping (shared) data package with chunkId: " << m_chunkId << std::endl;
                    return false;
                }

                if (m_onNoSharedInputChannelAvailable == "throw") {
                    throw IO_EXCEPTION("Can not write accumulated data because no (shared) input is available");
                    return false;
                }

                if (m_onNoSharedInputChannelAvailable == "queue") {
                    std::cout << "OUTPUT: Queuing (shared) data package with chunkId: " << m_chunkId << std::endl;
                    pushSharedChunkId(m_chunkId);
                    return false;
                }

                if (m_onNoSharedInputChannelAvailable == "wait") {
                    std::cout << "OUTPUT: Waiting for available (shared) input channel..." << std::endl;
                    pushSharedChunkId(m_chunkId);
                    while (m_shareNext.empty()) {
                        boost::this_thread::sleep(boost::posix_time::millisec(500));
                    }
                    std::cout << "OUTPUT: found (shared) input channel after waiting, distributing now" << std::endl;
                    return true;
                }

                // We should never be here!!
                throw LOGIC_EXCEPTION("Output channel case internally misconfigured, ask BH");
                return false;
            }

            void pushSharedChunkId(const unsigned int& chunkId) {
                boost::mutex::scoped_lock lock(m_chunkIdsMutex);
                m_finishedSharedChunkIds.push_back(chunkId);
            }

            unsigned int popSharedChunkId() {
                boost::mutex::scoped_lock lock(m_chunkIdsMutex);
                unsigned int chunkId = m_finishedSharedChunkIds.front();
                m_finishedSharedChunkIds.pop_front();
                return chunkId;
            }

            void distributeLocal(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {

                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                // Synchronous write as it takes no time here
                tcpChannel->write(std::vector<char>(), karabo::util::Hash("channelId", m_channelId, "chunkId", chunkId));
            }

            void distributeRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {

                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                registerAsyncWrite(tcpChannel, chunkId);
                const std::pair< std::vector<char>, karabo::util::Hash>& entry = getAsyncWriteData(chunkId);
                std::cout << "OUTPUT: Going to distribute " << entry.first.size() << " bytes of data" << std::endl;
                std::cout << "OUTPUT: With header: " << entry.second << std::endl;
                tcpChannel->writeAsyncVectorHash(entry.first, entry.second, boost::bind(&karabo::xms::NetworkOutput<T>::onWriteCompleted, this, _1));
                //m_activeTcpChannel->write(entry.first, entry.second);
              
            }

            void registerAsyncWrite(const TcpChannelPointer& channel, const unsigned int& chunkId) {

                m_currentWritersCountMutex.lock();

                m_channel2ChunkId[channel] = chunkId;
                CurrentWritersCount::iterator it = m_currentWritersCount.find(chunkId);
                if (it == m_currentWritersCount.end()) { // No one tried to write this chunk, yet
                    m_currentWritersCountMutex.unlock();
                    Memory<T>::cacheAsContiguousBlock(m_channelId, chunkId);
                    m_currentWritersCountMutex.lock();
                }
                m_currentWritersCount[chunkId]++;

                m_currentWritersCountMutex.unlock();
            }

            const std::pair<std::vector<char>, karabo::util::Hash>& getAsyncWriteData(const unsigned int& chunkId) {
                return Memory<T>::readContiguousBlockCache(m_channelId, chunkId);
            }

            void onWriteCompleted(TcpChannelPointer channel) {

                boost::mutex::scoped_lock lock(m_currentWritersCountMutex);

                TcpChannelPointer2ChunkId::iterator it = m_channel2ChunkId.find(channel);
                if (it != m_channel2ChunkId.end()) {
                    unsigned int chunkId = it->second;
                    m_currentWritersCount[chunkId]--;

                    if (m_currentWritersCount[chunkId] == 0) {
                        Memory<T>::clearContiguousBlockCache(m_channelId, chunkId);
                        m_currentWritersCount.erase(chunkId);
                        std::cout << "OUTPUT: Cleared asynchronous write cache" << std::endl;
                        // TODO Delete memory resident data
                        //Memory<T>::clearChunk(m_channelId, chunkId);
                    }
                } else {
                    throw LOGIC_EXCEPTION("Bad async write encountered");
                }
            }

            void copy() {

                bool goOn = checkAndHandleCopiedInputs();

                if (goOn) {
                    boost::mutex::scoped_lock lock(m_nextInputMutex);

                    unsigned int chunkId = popCopiedChunkId();

                    while (!m_copyNext.empty()) {
                        InputChannelInfo channelInfo = popCopyNext();
                        if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                            copyLocal(chunkId, channelInfo);
                        } else {
                            copyRemote(chunkId, channelInfo);
                        }
                    }
                }
            }

            bool checkAndHandleCopiedInputs() {

                // If no copied input channels are registered at all, we do not go on
                if (m_copiedInputs.empty()) return false;

                // If all copied inputs channels are available for distribution go on
                if (m_copyNext.size() == m_copiedInputs.size()) {
                    pushCopiedChunkId(m_chunkId);
                    return true;
                }

                // React as configured
                if (m_onNoCopiedInputChannelAvailable == "drop") {
                    std::cout << "Dropping (copied) data package for " << m_copiedInputs.size() - m_copyNext.size() << " connected inputs" << std::endl;
                    pushCopiedChunkId(m_chunkId);
                    return true;
                }
                if (m_onNoCopiedInputChannelAvailable == "throw") {
                    throw IO_EXCEPTION("Can not write accumulated data because not all (shared) inputs are available");
                    return false;
                }
                //                    if (m_onNoCopiedInputChannelAvailable == "queue") {
                //                        std::cout << "Queuing (copied) data package with chunkId: " << m_chunkId << std::endl;
                //                        return false;
                //                    }
                if (m_onNoCopiedInputChannelAvailable == "wait") {
                    std::cout << "Waiting for all copy input channels to be available... " << std::flush;
                    pushCopiedChunkId(m_chunkId);
                    while (m_copyNext.size() != m_copiedInputs.size()) {
                        boost::this_thread::sleep(boost::posix_time::millisec(500));
                    }
                    std::cout << "found all, copying now" << std::endl;
                    return true;
                }
                return false;

            }

            void pushCopiedChunkId(const unsigned int& chunkId) {
                boost::mutex::scoped_lock lock(m_chunkIdsMutex);
                m_finishedCopiedChunkIds.push_back(chunkId);
            }

            unsigned int popCopiedChunkId() {
                boost::mutex::scoped_lock lock(m_chunkIdsMutex);
                unsigned int chunkId = m_finishedCopiedChunkIds.front();
                m_finishedCopiedChunkIds.pop_front();
                return chunkId;
            }

            void copyLocal(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {
                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                // Synchronous write as it takes no time here
                // Writing no data signals input to read from memory
                tcpChannel->write(std::vector<char>(), karabo::util::Hash("channelId", m_channelId, "chunkId", chunkId));
            }

            void copyRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {

                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                registerAsyncWrite(tcpChannel, chunkId);
                const std::pair< std::vector<char>, karabo::util::Hash>& entry = getAsyncWriteData(chunkId);
                std::cout << "OUTPUT: Going to copy " << entry.first.size() << " bytes of data" << std::endl;
                std::cout << "OUTPUT: With header: " << entry.second << std::endl;
                tcpChannel->writeAsyncVectorHash(entry.first, entry.second, boost::bind(&karabo::xms::NetworkOutput<T>::onWriteCompleted, this, _1));
            }


        private: // members

            // Server related
            unsigned int m_ownPort;

            karabo::net::Connection::Pointer m_dataConnection;
            //TcpChannelPointer m_dataChannel;
            karabo::net::IOService::Pointer m_dataIOService;
            boost::thread m_dataThread;

            std::string m_onNoSharedInputChannelAvailable;
            std::string m_onNoCopiedInputChannelAvailable;

            InputChannels m_sharedInputs;
            InputChannels m_copiedInputs;

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


            // Copy out
            std::vector<char> m_buffer;
            karabo::util::Hash m_header;
            int m_count;

            unsigned int m_channelId;
            unsigned int m_chunkId;
            std::deque<unsigned int> m_finishedSharedChunkIds;
            std::deque<unsigned int> m_finishedCopiedChunkIds;

        };

    }
}

#endif
