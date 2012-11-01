/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_DEVICEINPUT_HH
#define	KARABO_XMS_DEVICEINPUT_HH

#include <karabo/net/IOService.hh>
#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>

#include "Memory.hh"
#include "Input.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xms {

        /**
         * The DeviceInput class.
         */
        template <class T>
        class NetworkInput : public Input<T> {
            typedef std::set<karabo::net::Connection::Pointer> TcpConnections;
            typedef std::set<karabo::net::Channel::Pointer> TcpChannels;


        public:

            KARABO_CLASSINFO(NetworkInput, "NetworkInput-" + T::classInfo().getClassId(), "1.0")



            /**
             * Default constructor.
             */
            NetworkInput() {
            };
            
            virtual ~NetworkInput() {
                // Close all connections
                for (TcpConnections::iterator it = m_tcpConnections.begin(); it != m_tcpConnections.end(); ++it) {
                    (*it)->close();
                }
                if (m_tcpIoServiceThread.joinable()) {
                    m_tcpIoService->stop();
                    m_tcpIoServiceThread.join();
                }
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
                        .assignmentMandatory()
                        .init()
                        .commit();

                STRING_ELEMENT(expected).key("dataDistribution")
                        .displayedName("Data Distribution")
                        .description("The way data is fetched from the connected output channels (shared/copy)")
                        .options("copy,shared")
                        .assignmentOptional().defaultValue("copy")
                        .init()
                        .commit();
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash & input) {

                std::vector<std::string> connectedOutputChannels;
                input.get("connectedOutputChannels", connectedOutputChannels);
                for (size_t i = 0; i < connectedOutputChannels.size(); ++i) {
                    std::vector<std::string> tmp;
                    boost::split(tmp, connectedOutputChannels[i], boost::is_any_of("@"));
                    m_connectedOutputChannels.push_back(karabo::util::Hash("instanceId", tmp[0], "channelId", tmp[1]));
                }

                input.get("dataDistribution", m_dataDistribution);

                m_channelId = Memory<T>::registerChannel();
                m_activeChunk = Memory<T>::registerChunk(m_channelId);
                m_inactiveChunk = Memory<T>::registerChunk(m_channelId);
            }

            std::vector<karabo::util::Hash> getConnectedOutputChannels() {
                return m_connectedOutputChannels;
            }

            void read(T& data, size_t idx) {
                Memory<T>::read(data, idx, m_channelId, m_activeChunk);
            }

            size_t size() const {
                return Memory<T>::size(m_channelId, m_activeChunk);
            }

            void connectNow(const karabo::util::Hash& outputChannelInfo) {


                std::string connectionType = outputChannelInfo.get<std::string > ("connectionType");
                std::string memoryLocation = outputChannelInfo.get<std::string > ("memoryLocation");

                if (connectionType == "tcp") {

                    // Prepare connection configuration given output channel information
                    karabo::util::Hash config = prepareConnectionConfiguration(outputChannelInfo);
                    karabo::net::Connection::Pointer tcpConnection = karabo::net::Connection::create(config); // Instantiate
                    startConnection(tcpConnection, memoryLocation);
                    
                    if (!m_tcpIoService) { 
                        m_tcpIoService = tcpConnection->getIOService(); // Save IO service for later sharing
                        m_tcpIoServiceThread = boost::thread(boost::bind(&karabo::net::IOService::run, m_tcpIoService));
                    }
                }
            }

            karabo::util::Hash prepareConnectionConfiguration(const karabo::util::Hash& serverInfo) const {
                const std::string& hostname = serverInfo.get<std::string > ("hostname");
                const unsigned int& port = serverInfo.get<unsigned int>("port");
                karabo::util::Hash h("Tcp.type", "client", "Tcp.hostname", hostname, "Tcp.port", port);
                if (m_tcpIoService) h.setFromPath("Tcp.IOService", m_tcpIoService);
                return h;
            }

            void startConnection(karabo::net::Connection::Pointer connection, const std::string& memoryLocation) {
                //connection->setErrorHandler(&karabo::xms::DeviceInput<T>::onTcpConnectionError, this, _1, _2);
                karabo::net::Channel::Pointer channel;
                bool connected = false;
                int sleep = 1;
                while (!connected) {
                    try {
                        channel = connection->start();
                    } catch (karabo::util::NetworkException& e) {
                        std::cout << "Could not connect to desired output channel, retrying in " << sleep << " s.";
                        boost::this_thread::sleep(boost::posix_time::seconds(sleep));
                        sleep += 2;
                        continue;
                    }
                    connected = true;
                }
                channel->setErrorHandler(boost::bind(&karabo::xms::NetworkInput<T>::onTcpChannelError, this, _1, _2));
                channel->write(karabo::util::Hash("reason", "hello", "instanceId", this->getInstanceId(), "memoryLocation", memoryLocation, "dataDistribution", m_dataDistribution)); // Say hello!
                channel->readAsyncVectorHash(boost::bind(&karabo::xms::NetworkInput<T>::onTcpChannelRead, this, _1, _2, _3));

                m_tcpConnections.insert(connection); // TODO check whether really needed
                m_tcpChannels.insert(channel);
            }

            void onTcpConnectionError(karabo::net::Channel::Pointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelError(karabo::net::Channel::Pointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelRead(karabo::net::Channel::Pointer channel, const std::vector<char>& data, const karabo::util::Hash& header) {
                std::cout << "Receiving " << data.size() << " bytes of data" << std::endl;
                if (data.size() == 0 && header.has("channelId") && header.has("chunkId")) { // Local memory
                    std::cout << "READING FROM LOCAL MEMORY" << std::endl;
                    unsigned int channelId = header.get<unsigned int>("channelId");
                    unsigned int chunkId = header.get<unsigned int>("chunkId");
                    Memory<T>::writeChunk(Memory<T>::readChunk(channelId, chunkId), m_channelId, m_inactiveChunk);
                } else {
                    std::cout << "READING FROM REMOTE MEMORY (over tcp)" << std::endl;
                    Memory<T>::writeAsContiguosBlock(data, header, m_channelId, m_inactiveChunk);
                }
                if (Memory<T>::size(m_channelId, m_inactiveChunk) < this->getMinimumNumberOfData()) {
                    std::cout << "can read more data" << std::endl;
                    notifyOutputChannelForPossibleRead(channel);
                } else if (Memory<T>::size(m_channelId, m_activeChunk) == 0) {
                    std::swap(m_activeChunk, m_inactiveChunk);
                    std::cout << "swapped buffers, can read more" << std::endl;
                    notifyOutputChannelForPossibleRead(channel);
                    this->template triggerIOEvent< Input<T> >();
                }

                channel->readAsyncVectorHash(boost::bind(&karabo::xms::NetworkInput<T>::onTcpChannelRead, this, _1, _2, _3));
            }

            bool canCompute() const {
                //boost::mutex::scoped_lock lock(m_mutex);
                std::cout << "Current size of async read: " << Memory<T>::size(m_channelId, m_activeChunk) << std::endl;
                return Memory<T>::size(m_channelId, m_activeChunk) >= this->getMinimumNumberOfData();
            }

            void update() {
                // Clear active chunk
                Memory<T>::clearChunk(m_channelId, m_activeChunk);
                std::swap(m_activeChunk, m_inactiveChunk);

                // Notify all connected output channels for another read
                //notifyOutputChannelsForPossibleRead();
            }

            void notifyOutputChannelsForPossibleRead() {
                for (TcpChannels::const_iterator it = m_tcpChannels.begin(); it != m_tcpChannels.end(); ++it) {
                    notifyOutputChannelForPossibleRead(*it);
                }
            }

            void notifyOutputChannelForPossibleRead(const karabo::net::Channel::Pointer& channel) {
                channel->write(karabo::util::Hash("reason", "update", "instanceId", this->getInstanceId()));
            }


        private: // members

            std::vector<karabo::util::Hash> m_connectedOutputChannels;
            std::string m_dataDistribution;

            unsigned int m_channelId;

            boost::mutex m_mutex;

            int m_activeChunk;
            int m_inactiveChunk;

            karabo::net::IOService::Pointer m_tcpIoService;
            boost::thread m_tcpIoServiceThread;

            TcpConnections m_tcpConnections;
            TcpChannels m_tcpChannels;

        private: // functions

            bool needsDeviceConnection() const {
                return true;
            }
        };


    }
}

#endif
