/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_XMS_DEVICEINPUT_HH
#define	EXFEL_XMS_DEVICEINPUT_HH

#include <karabo/net/IOService.hh>
#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>

#include "Memory.hh"
#include "Input.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    namespace xms {

        /**
         * The DeviceInput class.
         */
        template <class T>
        class InterInstanceInput : public Input<T> {
            typedef std::set<exfel::net::Connection::Pointer> TcpConnections;
            typedef std::set<exfel::net::Channel::Pointer> TcpChannels;


        public:

            EXFEL_CLASSINFO(InterInstanceInput, "DeviceInput-" + T::classInfo().getClassId(), "1.0")



            /**
             * Default constructor.
             */
            InterInstanceInput() {
            };

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(exfel::util::Schema& expected) {
                using namespace exfel::util;


                VECTOR_STRING_ELEMENT(expected).key("connectedOutputChannels")
                        .displayedName("Connected Output Channels")
                        .description("Defines the inter-device connectivity for p-2-p data transfer (use format: <instanceId>/<channelName>)")
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
            void configure(const exfel::util::Hash & input) {

                std::vector<std::string> connectedOutputChannels;
                input.get("connectedOutputChannels", connectedOutputChannels);
                for (size_t i = 0; i < connectedOutputChannels.size(); ++i) {
                    std::vector<std::string> tmp;
                    boost::split(tmp, connectedOutputChannels[i], boost::is_any_of("/"));
                    m_connectedOutputChannels.push_back(exfel::util::Hash("instanceId", tmp[0], "channelId", tmp[1]));
                }

                input.get("dataDistribution", m_dataDistribution);

                m_channelId = Memory<T>::registerChannel();
                m_activeChunk = Memory<T>::registerChunk(m_channelId);
                m_inactiveChunk = Memory<T>::registerChunk(m_channelId);
            }

            std::vector<exfel::util::Hash> getConnectedOutputChannels() {
                return m_connectedOutputChannels;
            }

            void read(T& data, size_t idx) {
                Memory<T>::read(data, idx, m_channelId, m_activeChunk);
            }

            size_t size() const {
                return Memory<T>::size(m_channelId, m_activeChunk);
            }

            void connectNow(const exfel::util::Hash& outputChannelInfo) {


                std::string connectionType = outputChannelInfo.get<std::string > ("connectionType");
                std::string memoryLocation = outputChannelInfo.get<std::string > ("memoryLocation");

                if (connectionType == "tcp") {

                    // Prepare connection configuration given output channel information
                    exfel::util::Hash config = prepareConnectionConfiguration(outputChannelInfo);
                    exfel::net::Connection::Pointer tcpConnection = exfel::net::Connection::create(config); // Instantiate
                    startConnection(tcpConnection, memoryLocation);
                    
                    if (!m_tcpIoService) { 
                        m_tcpIoService = tcpConnection->getIOService(); // Save IO service for later sharing
                        m_tcpIoServiceThread = boost::thread(boost::bind(&exfel::net::IOService::run, m_tcpIoService));
                    }
                }
            }

            exfel::util::Hash prepareConnectionConfiguration(const exfel::util::Hash& serverInfo) const {
                const std::string& hostname = serverInfo.get<std::string > ("hostname");
                const unsigned int& port = serverInfo.get<unsigned int>("port");
                exfel::util::Hash h("Tcp.type", "client", "Tcp.hostname", hostname, "Tcp.port", port);
                if (m_tcpIoService) h.setFromPath("Tcp.IOService", m_tcpIoService);
                return h;
            }

            void startConnection(exfel::net::Connection::Pointer connection, const std::string& memoryLocation) {
                //connection->setErrorHandler(&exfel::xms::DeviceInput<T>::onTcpConnectionError, this, _1, _2);
                exfel::net::Channel::Pointer channel;
                bool connected = false;
                int sleep = 1;
                while (!connected) {
                    try {
                        channel = connection->start();
                    } catch (exfel::util::NetworkException& e) {
                        std::cout << "Could not connect to desired output channel, retrying in " << sleep << " s.";
                        boost::this_thread::sleep(boost::posix_time::seconds(sleep));
                        sleep += 2;
                        continue;
                    }
                    connected = true;
                }
                channel->setErrorHandler(boost::bind(&exfel::xms::InterInstanceInput<T>::onTcpChannelError, this, _1, _2));
                channel->write(exfel::util::Hash("reason", "hello", "instanceId", this->getInstanceId(), "memoryLocation", memoryLocation, "dataDistribution", m_dataDistribution)); // Say hello!
                channel->readAsyncVectorHash(boost::bind(&exfel::xms::InterInstanceInput<T>::onTcpChannelRead, this, _1, _2, _3));

                m_tcpConnections.insert(connection); // TODO check whether really needed
                m_tcpChannels.insert(channel);
            }

            void onTcpConnectionError(exfel::net::Channel::Pointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelError(exfel::net::Channel::Pointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelRead(exfel::net::Channel::Pointer channel, const std::vector<char>& data, const exfel::util::Hash& header) {
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
                    this->triggerIOEvent();
                }

                channel->readAsyncVectorHash(boost::bind(&exfel::xms::InterInstanceInput<T>::onTcpChannelRead, this, _1, _2, _3));
            }

            bool canCompute() {
                boost::mutex::scoped_lock lock(m_mutex);
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

            void notifyOutputChannelForPossibleRead(const exfel::net::Channel::Pointer& channel) {
                channel->write(exfel::util::Hash("reason", "update", "instanceId", this->getInstanceId()));
            }

            /**
             * Destructor.
             */
            virtual ~InterInstanceInput() {
            }




        private: // members

            std::vector<exfel::util::Hash> m_connectedOutputChannels;
            std::string m_dataDistribution;

            unsigned int m_channelId;

            boost::mutex m_mutex;

            int m_activeChunk;
            int m_inactiveChunk;

            exfel::net::IOService::Pointer m_tcpIoService;
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
