/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_NETWORKINPUT_HH
#define	KARABO_XMS_NETWORKINPUT_HH

#include <karabo/net.hpp>
#include <karabo/io.hpp>
#include <karabo/log.hpp>
#include <karabo/util.hpp>
#include "Memory.hh"

/**
 * The main Karabo namespace
 */
namespace karabo {

    namespace xms {

        /**
         * The DeviceInput class.
         */
        template <class T>
        class NetworkInput : public karabo::io::Input<T> {

            typedef std::set<karabo::net::Connection::Pointer> TcpConnections;
            typedef std::set<karabo::net::Channel::Pointer> TcpChannels;
            typedef Memory<T> MemoryType;


        public:

            KARABO_CLASSINFO(NetworkInput, "Network-" + karabo::io::getIODataType<T>(), "1.0")

            virtual ~NetworkInput() {
                // Close all connections
                for (TcpConnections::iterator it = m_tcpConnections.begin(); it != m_tcpConnections.end(); ++it) {
                    (*it)->stop();
                }
                if (m_tcpIoServiceThread.joinable()) {
                    m_tcpIoService->stop();
                    m_tcpIoServiceThread.join();
                }
                MemoryType::unregisterChannel(m_channelId);
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;


                VECTOR_STRING_ELEMENT(expected).key("connectedOutputChannels")
                        .displayedName("Connected Output Channels")
                        .description("Defines the inter-device connectivity for p-2-p data transfer (use format: <instanceId>@<channelName>)")
                        .assignmentOptional().noDefaultValue()
                        .reconfigurable()
                        .commit();

                STRING_ELEMENT(expected).key("dataDistribution")
                        .displayedName("Data Distribution")
                        .description("The way data is fetched from the connected output channels (shared/copy)")
                        .options("copy,shared")
                        .assignmentOptional().defaultValue("copy")
                        .reconfigurable()
                        .commit();

                STRING_ELEMENT(expected).key("onSlowness")
                        .displayedName("On Slowness")
                        .description("Policy for what to do if this input is too slow for the fed data rate (only used in copy mode)")
                        .options("drop,throw,wait,queue")
                        .assignmentOptional().defaultValue("wait")
                        .reconfigurable()
                        .commit();

                UINT32_ELEMENT(expected).key("minData")
                        .displayedName("Minimum number input packets")
                        .description("The number of elements to be read before any computation is started (0 = all, -1 = none/any)")
                        .assignmentOptional().defaultValue(1)
                        .reconfigurable()
                        .commit();

                BOOL_ELEMENT(expected).key("keepDataUntilNew")
                        .displayedName("Keep data until new")
                        .description("If true, keeps data until new data from an connected output is provided. "
                                     "If new data is available the previous chunk is automatically deleted and the new one is made available for reading")
                        .assignmentOptional().defaultValue(false)
                        .reconfigurable()
                        .commit();



                BOOL_ELEMENT(expected).key("respondToEndOfStream")
                        .displayedName("Respond to end-of-stream")
                        .description("Determines whether this input should forward a end-of-stream event to its parent device.")
                        .assignmentOptional().defaultValue(true)
                        .reconfigurable()
                        .commit();



            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            NetworkInput(const karabo::util::Hash& config) : karabo::io::Input<T>(config), m_isEndOfStream(false), m_respondToEndOfStream(true) {
                parseOutputChannelConfiguration(config);
                config.get("dataDistribution", m_dataDistribution);
                config.get("minData", m_minData);
                config.get("keepDataUntilNew", m_keepDataUntilNew);
                config.get("onSlowness", m_onSlowness);
                config.get("respondToEndOfStream", m_respondToEndOfStream);

                m_channelId = Memory<T>::registerChannel();
                m_inactiveChunk = Memory<T>::registerChunk(m_channelId);
                m_activeChunk = Memory<T>::registerChunk(m_channelId);
                

                KARABO_LOG_FRAMEWORK_DEBUG << "Inputting on Channel " << m_channelId << " and chunkId " << m_activeChunk << " and " << m_inactiveChunk;

            }

            void parseOutputChannelConfiguration(const karabo::util::Hash& config) {
                if (config.has("connectedOutputChannels")) {
                    std::vector<std::string> connectedOutputChannels;
                    config.get("connectedOutputChannels", connectedOutputChannels);
                    for (size_t i = 0; i < connectedOutputChannels.size(); ++i) {
                        std::vector<std::string> tmp;
                        boost::split(tmp, connectedOutputChannels[i], boost::is_any_of("@:"));
                        m_connectedOutputChannels.push_back(karabo::util::Hash("instanceId", tmp[0], "channelId", tmp[1]));
                    }
                }
            }

            void reconfigure(const karabo::util::Hash& config) {
                parseOutputChannelConfiguration(config);
                if (config.has("dataDistribution")) config.get("dataDistribution", m_dataDistribution);
                if (config.has("minData")) config.get("minData", m_minData);
                if (config.has("keepDataUntilNew")) config.get("keepDataUntilNew", m_keepDataUntilNew);
                if (config.has("onSlowness")) config.get("onSlowness", m_onSlowness);
                if (config.has("respondToEndOfStream")) config.get("respondToEndOfStream", m_respondToEndOfStream);
            }

            std::vector<karabo::util::Hash> getConnectedOutputChannels() {
                return m_connectedOutputChannels;
            }

            void read(T& data, size_t idx) {
                boost::mutex::scoped_lock lock(m_swapBuffersMutex);
                Memory<T>::read(data, idx, m_channelId, m_activeChunk);
            }

            size_t size() const {
                return Memory<T>::size(m_channelId, m_activeChunk);
            }

            unsigned int getMinimumNumberOfData() const {
                return m_minData;
            }

            void connectNow(const karabo::util::Hash& outputChannelInfo) {


                std::string connectionType = outputChannelInfo.get<std::string > ("connectionType");
                std::string memoryLocation = outputChannelInfo.get<std::string > ("memoryLocation");

                if (connectionType == "tcp") {

                    // Prepare connection configuration given output channel information
                    karabo::util::Hash config = prepareConnectionConfiguration(outputChannelInfo);
                    karabo::net::Connection::Pointer tcpConnection = karabo::net::Connection::create(config); // Instantiate

                    if (!m_tcpIoService) {
                        // Get IO service and save for later sharing
                        m_tcpIoService = tcpConnection->getIOService();
                        // Establish connection (and define sub-type of server)
                        startConnection(tcpConnection, memoryLocation);
                        m_tcpIoServiceThread = boost::thread(boost::bind(&karabo::net::IOService::run, m_tcpIoService));
                    } else {
                        tcpConnection->setIOService(m_tcpIoService);
                        startConnection(tcpConnection, memoryLocation);
                    }
                }
            }

            karabo::util::Hash prepareConnectionConfiguration(const karabo::util::Hash& serverInfo) const {
                const std::string& hostname = serverInfo.get<std::string > ("hostname");
                const unsigned int& port = serverInfo.get<unsigned int>("port");
                karabo::util::Hash h("Tcp.type", "client", "Tcp.hostname", hostname, "Tcp.port", port);
                return h;
            }

            void startConnection(karabo::net::Connection::Pointer connection, const std::string& memoryLocation) {
                karabo::net::Channel::Pointer channel;
                bool connected = false;
                int sleep = 1;
                while (!connected) {
                    try {
                        channel = connection->start();
                    } catch (karabo::util::NetworkException& e) {
                        KARABO_LOG_FRAMEWORK_INFO << "Could not connect to desired output channel, retrying in " << sleep << " s.";
                        boost::this_thread::sleep(boost::posix_time::seconds(sleep));
                        sleep += 2;
                        continue;
                    }
                    connected = true;
                }
                channel->setErrorHandler(boost::bind(&karabo::xms::NetworkInput<T>::onTcpChannelError, this, _1, _2));
                channel->write(karabo::util::Hash("reason", "hello", "instanceId", this->getInstanceId(), "memoryLocation", memoryLocation, "dataDistribution", m_dataDistribution, "onSlowness", m_onSlowness)); // Say hello!
                channel->readAsyncHashVector(boost::bind(&karabo::xms::NetworkInput<T>::onTcpChannelRead, this, _1, _2, _3));

                m_tcpConnections.insert(connection); // TODO check whether really needed
                m_tcpChannels.insert(channel);
            }

            void onTcpConnectionError(karabo::net::Channel::Pointer, const karabo::net::ErrorCode& error) {
                KARABO_LOG_FRAMEWORK_ERROR << error.value() << ": " << error.message();
            }

            void onTcpChannelError(karabo::net::Channel::Pointer, const karabo::net::ErrorCode& error) {
                std::cout << error.message() << std::endl;
            }

            void onTcpChannelRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::vector<char>& data) {

                boost::mutex::scoped_lock lock(m_mutex);

                m_isEndOfStream = false;

                if (header.has("endOfStream")) {
                    
                    KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Received EOS";
                    if (m_respondToEndOfStream) m_isEndOfStream = true;
                    if (this->getMinimumNumberOfData() <= 0) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Triggering another compute";
                        this->swapBuffers();
                        this->triggerIOEvent();
                    }

                    if (m_respondToEndOfStream) KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Triggering EOS function";
                    if (m_respondToEndOfStream) this->triggerEndOfStreamEvent();

                    channel->readAsyncHashVector(boost::bind(&karabo::xms::NetworkInput<T>::onTcpChannelRead, this, _1, _2, _3));
                    return;
                }


                if (data.size() == 0 && header.has("channelId") && header.has("chunkId")) { // Local memory
                                        
                    unsigned int channelId = header.get<unsigned int>("channelId");
                    unsigned int chunkId = header.get<unsigned int>("chunkId");

                    KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Reading from local memory [" << channelId << "][" << chunkId << "]";
                    
                    MemoryType::writeChunk(Memory<T>::readChunk(channelId, chunkId), m_channelId, m_inactiveChunk);
                    MemoryType::decrementChunkUsage(channelId, chunkId);
                    
                } else { // TCP data
                    
                    KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Reading from remote memory (over tcp)";
                    MemoryType::writeAsContiguosBlock(data, header, m_channelId, m_inactiveChunk);
                    
                }

                size_t nInactiveData = MemoryType::size(m_channelId, m_inactiveChunk);
                size_t nActiveData = MemoryType::size(m_channelId, m_activeChunk);
                
                if ((this->getMinimumNumberOfData()) <= 0 || (nInactiveData < this->getMinimumNumberOfData())) { // Not enough data, yet
                    KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Can read more data";
                    notifyOutputChannelForPossibleRead(channel);
                } else if (nActiveData == 0) { // Data complete, second pot still empty
                    
                    this->swapBuffers();
                    notifyOutputChannelForPossibleRead(channel);

                    // No mutex under callback
                    KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Triggering IOEvent";
                    m_mutex.unlock();
                    this->triggerIOEvent();
                    m_mutex.lock();

                } else { // Data complete on both pots now
                    if (m_keepDataUntilNew) { // Is false per default
                        m_keepDataUntilNew = false;
                        KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Updating";
                        m_mutex.unlock();
                        update();
                        m_mutex.lock();
                        m_keepDataUntilNew = true;
                    }
                }

                channel->readAsyncHashVector(boost::bind(&karabo::xms::NetworkInput<T>::onTcpChannelRead, this, _1, _2, _3));
            }

            void swapBuffers() {
                //boost::mutex::scoped_lock lock(m_swapBuffersMutex);
                std::swap(m_activeChunk, m_inactiveChunk);
            }

            bool canCompute() const {
                //KARABO_LOG_FRAMEWORK_DEBUG << "INPUT: Current size of async read data cache: " << Memory<T>::size(m_channelId, m_activeChunk);
                //KARABO_LOG_FRAMEWORK_DEBUG << "INPUT: Is end of stream? " << m_isEndOfStream;
                //KARABO_LOG_FRAMEWORK_DEBUG << "INPUT: MinData " << this->getMinimumNumberOfData();
                if ((this->getMinimumNumberOfData() == -1) && !m_isEndOfStream) return true;

                if (m_isEndOfStream && (Memory<T>::size(m_channelId, m_activeChunk) == 0)) return false;

                if (!m_isEndOfStream && (this->getMinimumNumberOfData() <= 0)) return false;

                return Memory<T>::size(m_channelId, m_activeChunk) >= this->getMinimumNumberOfData();
            }

            void update() {

                boost::mutex::scoped_lock lock(m_mutex);

                if (m_keepDataUntilNew) return;

                // Clear active chunk
                MemoryType::clearChunkData(m_channelId, m_activeChunk);

                // Swap buffers               
                swapBuffers();

                // Fetch number of data pieces
                size_t nActiveData = Memory<T>::size(m_channelId, m_activeChunk);
                
                // Notify all connected output channels for another read
                if (nActiveData >= this->getMinimumNumberOfData()) {
                    notifyOutputChannelsForPossibleRead();
                }
            }

            void notifyOutputChannelsForPossibleRead() {
                for (TcpChannels::const_iterator it = m_tcpChannels.begin(); it != m_tcpChannels.end(); ++it) {
                    notifyOutputChannelForPossibleRead(*it);
                }
            }

            void notifyOutputChannelForPossibleRead(const karabo::net::Channel::Pointer& channel) {
                KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Notifying output channel that " << this->getInstanceId() << " is ready for next read.";
                channel->write(karabo::util::Hash("reason", "update", "instanceId", this->getInstanceId()));
            }

            bool respondsToEndOfStream() {
                return m_respondToEndOfStream;
            }


        private: // members

            std::vector<karabo::util::Hash> m_connectedOutputChannels;
            std::string m_dataDistribution;
            unsigned int m_minData;
            bool m_keepDataUntilNew;
            std::string m_onSlowness;

            unsigned int m_channelId;

            boost::mutex m_mutex;
            boost::mutex m_swapBuffersMutex;

            int m_activeChunk;
            int m_inactiveChunk;

            karabo::net::IOService::Pointer m_tcpIoService;
            boost::thread m_tcpIoServiceThread;

            TcpConnections m_tcpConnections;
            TcpChannels m_tcpChannels;

            bool m_isEndOfStream;
            bool m_respondToEndOfStream;


        private: // functions

            bool needsDeviceConnection() const {
                return true;
            }
        };


    }
}

#endif
