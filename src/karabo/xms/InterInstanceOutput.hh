/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_XMS_DEVICEOUTPUT_HH
#define	EXFEL_XMS_DEVICEOUTPUT_HH

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
namespace exfel {

    namespace xms {

        /**
         * The DeviceOutput class.
         */
        template <class T>
        class InterInstanceOutput : public Output<T> {
            typedef boost::shared_ptr<exfel::net::Channel> TcpChannelPointer;

            /*
             * InputChannelInfo (exfel::util::Hash)
             * 
             *     memoryLocation (std::string) [local/remote]
             *     dataDistribution (std::string) [distribute/copy]
             *     tcpChannel (TcpChannelPointer)
             *
             */
            typedef exfel::util::Hash InputChannelInfo;

            /*
             * InputChannels (exfel::util::Hash)
             * 
             * instanceId (exfel::util::Hash)
             *     InputChannelInfo (s.a.)
             *
             */
            typedef exfel::util::Hash InputChannels;

            typedef std::deque< InputChannelInfo > InputChannelQueue;

            typedef std::map<unsigned int, int> CurrentWritersCount;

            typedef std::map<TcpChannelPointer, unsigned int> TcpChannelPointer2ChunkId;


        public:

            EXFEL_CLASSINFO(InterInstanceOutput, "DeviceOutput-" + T::classInfo().getClassId(), "1.0")

            /**
             * Default constructor.
             */
            InterInstanceOutput() {
            };

            /**
             * Destructor.
             */
            virtual ~InterInstanceOutput() {
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(exfel::util::Schema& expected) {
                using namespace exfel::util;

                STRING_ELEMENT(expected).key("noInputShared")
                        .displayedName("No Input (Shared)")
                        .description("What to do if currently no share-input channel is available for writing to")
                        .options("drop,queue,throw,wait")
                        .assignmentOptional().defaultValue("queue")
                        .init()
                        .commit();

                STRING_ELEMENT(expected).key("noInputCopy")
                        .displayedName("No Input (Copy)")
                        .description("What to do if one (or more) copy-input channel(s) are missing")
                        .options("drop,throw,wait")
                        .assignmentOptional().defaultValue("drop")
                        .init()
                        .commit();
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const exfel::util::Hash& input) {

                input.get("noInputShared", m_onNoSharedInputChannelAvailable);
                input.get("noInputCopy", m_onNoCopiedInputChannelAvailable);

                // Memory related
                m_channelId = Memory<T>::registerChannel();
                m_chunkId = Memory<T>::registerChunk(m_channelId);

                // Data networking
                int tryAgain = 5; // Try maximum 5 times to start a server
                while (tryAgain > 0) {
                    try {
                        m_ownPort = Statics::generateServerPort();
                        exfel::util::Hash h("Tcp.type", "server", "Tcp.port", m_ownPort);
                        m_dataConnection = exfel::net::Connection::create(h);
                        m_dataConnection->setErrorHandler(boost::bind(&exfel::xms::InterInstanceOutput<T>::onTcpConnectionError, this, _1, _2));
                        m_dataIOService = m_dataConnection->getIOService();
                        m_dataConnection->startAsync(boost::bind(&exfel::xms::InterInstanceOutput<T>::onTcpConnect, this, _1));

                        // Start data thread
                        m_dataThread = boost::thread(boost::bind(&exfel::net::IOService::run, m_dataIOService));

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

            exfel::util::Hash getInformation() const {
                return exfel::util::Hash("connectionType", "tcp", "hostname", boost::asio::ip::host_name(), "port", m_ownPort);
            }

            void write(const T& data) {
                Memory<T>::write(data, m_channelId, m_chunkId);
            }

            void onTcpConnect(TcpChannelPointer channel) {
                std::cout << "Connection established" << std::endl;
                channel->setErrorHandler(boost::bind(&exfel::xms::InterInstanceOutput<T>::onTcpChannelError, this, _1, _2));
                channel->readAsyncHash(boost::bind(&exfel::xms::InterInstanceOutput<T>::onTcpChannelRead, this, _1, _2));
                m_dataConnection->startAsync(boost::bind(&exfel::xms::InterInstanceOutput<T>::onTcpConnect, this, _1));
            }

            void onTcpConnectionError(TcpChannelPointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelError(TcpChannelPointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }
            
            void onTcpChannelRead(TcpChannelPointer channel, const exfel::util::Hash& message) {

                std::string reason;
                if (message.has("reason")) message.get<std::string > ("reason", reason);

                if (reason == "hello") {

                    // Associate instanceId with channel
                    if (message.has("instanceId") && message.has("memoryLocation") && message.has("dataDistribution")) {

                        std::string instanceId = message.get<std::string > ("instanceId");
                        std::string dataDistribution = message.get<std::string > ("dataDistribution");
                        std::string memoryLocation = message.get<std::string > ("memoryLocation");

                        exfel::util::Hash info;
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
                channel->readAsyncHash(boost::bind(&exfel::xms::InterInstanceOutput<T>::onTcpChannelRead, this, _1, _2));
            }

            void onInputAvailable(const std::string& instanceId) {



                if (m_sharedInputs.has(instanceId)) {
                    InputChannelInfo channelInfo = m_sharedInputs.get<exfel::util::Hash > (instanceId);
                    pushShareNext(channelInfo);
                    std::cout << "New (shared) input on instance " << instanceId << " available for writing " << std::endl;
                } else if (m_copiedInputs.has(instanceId)) {
                    InputChannelInfo channelInfo = m_copiedInputs.get<exfel::util::Hash > (instanceId);
                    pushCopyNext(channelInfo);
                    std::cout << "New (copied) input on instance " << instanceId << " available for writing " << std::endl;
                } else {
                    std::cout << "LOW-LEVEL-DEBUG: An input channel wants to connect, that was not registered before." << std::endl;
                }
                this->triggerIOEvent();
            }

            bool canCompute() {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                return !m_shareNext.empty();
            }

            void update() {

                std::cout << "update" << std::endl;

                // Distribute chunk(s)
                distribute();

                // Copy chunk(s)
                copy();

                m_chunkId = Memory<T>::registerChunk(m_channelId);
            }

            void pushShareNext(const InputChannelInfo& info) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                m_shareNext.push_back(info);
            }

            InputChannelInfo popShareNext() {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                InputChannelInfo info = m_shareNext.front();
                m_shareNext.pop_front();
                return info;
            }

            void pushCopyNext(const InputChannelInfo& info) {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                m_copyNext.push_back(info);
            }

            InputChannelInfo popCopyNext() {
                boost::mutex::scoped_lock lock(m_nextInputMutex);
                InputChannelInfo info = m_copyNext.front();
                m_copyNext.pop_front();
                return info;
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

            void distribute() {

                // Save currently written chunkId
                pushSharedChunkId(m_chunkId);

                bool goOn = checkAndHandleSharedInputs();

                if (goOn) {
                    std::cout << "finishedChunks " << m_finishedSharedChunkIds.size() << " shareNext " << m_shareNext.size() << std::endl;
                    while (!m_finishedSharedChunkIds.empty() && !m_shareNext.empty()) {

                        std::cout << " dist" << std::endl;
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
                if (!m_shareNext.empty()) {
                    return true;
                } else {
                    // React as configured
                    if (m_onNoSharedInputChannelAvailable == "drop") {
                        std::cout << "Dropping (shared) data package with chunkId: " << m_chunkId << std::endl;
                        popSharedChunkId();
                        return false;
                    }
                    if (m_onNoSharedInputChannelAvailable == "throw") {
                        throw IO_EXCEPTION("Can not write accumulated data because no (shared) input is available");
                        return false;
                    }
                    if (m_onNoSharedInputChannelAvailable == "queue") {
                        std::cout << "Queuing (shared) data package with chunkId: " << m_chunkId << std::endl;
                        return false;
                    }
                    if (m_onNoSharedInputChannelAvailable == "wait") {
                        std::cout << "Waiting for available input channel..." << std::flush;
                        while (m_shareNext.empty()) {
                            boost::this_thread::sleep(boost::posix_time::millisec(500));
                        }
                        std::cout << "found one, distributing now" << std::endl;
                        return true;
                    }
                    return false;
                }
            }

            void distributeLocal(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {

                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                // Synchronous write as it takes no time here
                tcpChannel->write(std::vector<char>(), exfel::util::Hash("channelId", m_channelId, "chunkId", chunkId));
            }

            void distributeRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {

                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                registerAsyncWrite(tcpChannel, chunkId);
                const std::pair< std::vector<char>, exfel::util::Hash>& entry = getAsyncWriteData(chunkId);
                std::cout << "Going to distribute " << entry.first.size() << " bytes of data" << std::endl;
                std::cout << "With header: " << entry.second << std::endl;
                tcpChannel->writeAsyncVectorHash(entry.first, entry.second, boost::bind(&exfel::xms::InterInstanceOutput<T>::onWriteCompleted, this, _1));
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

            const std::pair<std::vector<char>, exfel::util::Hash>& getAsyncWriteData(const unsigned int& chunkId) {
                return Memory<T>::readContiguousBlockCache(m_channelId, chunkId);
            }

            void onWriteCompleted(TcpChannelPointer channel) {

                boost::mutex::scoped_lock lock(m_currentWritersCountMutex);

                TcpChannelPointer2ChunkId::iterator it = m_channel2ChunkId.find(channel);
                if (it != m_channel2ChunkId.end()) {
                    unsigned int chunkId = it->second;
                    m_currentWritersCount[chunkId]--;

                    if (m_currentWritersCount[chunkId] == 0) {
                        std::cout << "Going to clear asynchronous write cache..." << std::flush;
                        Memory<T>::clearContiguousBlockCache(m_channelId, chunkId);
                        m_currentWritersCount.erase(chunkId);
                        std::cout << "done" << std::endl;
                    }
                } else {
                    throw LOGIC_EXCEPTION("Bad async write encountered");
                }
            }

            void copy() {

                if (!m_copiedInputs.empty()) {

                    // Save currently written chunkId
                    pushCopiedChunkId(m_chunkId);

                    bool goOn = checkAndHandleCopiedInputs();

                    if (goOn) {

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
            }

            bool checkAndHandleCopiedInputs() {
                if (m_copyNext.size() == m_copiedInputs.size()) {
                    return true;
                } else {
                    // React as configured
                    if (m_onNoCopiedInputChannelAvailable == "drop") {
                        std::cout << "Dropping (copied) data package for " << m_copiedInputs.size() - m_copyNext.size() << " connected inputs" << std::endl;
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
                        std::cout << "Waiting for all copy input channels to be available..." << std::flush;
                        while (m_copyNext.size() != m_copiedInputs.size()) {
                            boost::this_thread::sleep(boost::posix_time::millisec(500));
                        }
                        std::cout << "found all, copying now" << std::endl;
                        return true;
                    }
                    return false;
                }
            }

            void copyLocal(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {
                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                // Synchronous write as it takes no time here
                // Writing no data signals input to read from memory
                tcpChannel->write(std::vector<char>(), exfel::util::Hash("channelId", m_channelId, "chunkId", chunkId));
            }

            void copyRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {

                const TcpChannelPointer& tcpChannel = channelInfo.get<TcpChannelPointer > ("tcpChannel");

                registerAsyncWrite(tcpChannel, chunkId);
                const std::pair< std::vector<char>, exfel::util::Hash>& entry = getAsyncWriteData(chunkId);
                std::cout << "Going to copy " << entry.first.size() << " bytes of data" << std::endl;
                std::cout << "With header: " << entry.second << std::endl;
                tcpChannel->writeAsyncVectorHash(entry.first, entry.second, boost::bind(&exfel::xms::InterInstanceOutput<T>::onWriteCompleted, this, _1));
            }


        private: // members

            // Server related
            unsigned int m_ownPort;

            exfel::net::Connection::Pointer m_dataConnection;
            //TcpChannelPointer m_dataChannel;
            exfel::net::IOService::Pointer m_dataIOService;
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

            // Async out
            CurrentWritersCount m_currentWritersCount;
            std::map<TcpChannelPointer, unsigned int> m_channel2ChunkId;


            // Copy out
            std::vector<char> m_buffer;
            exfel::util::Hash m_header;
            int m_count;

            unsigned int m_channelId;
            unsigned int m_chunkId;
            std::deque<unsigned int> m_finishedSharedChunkIds;
            std::deque<unsigned int> m_finishedCopiedChunkIds;


        private: // functions

        };

    }
}

#endif
