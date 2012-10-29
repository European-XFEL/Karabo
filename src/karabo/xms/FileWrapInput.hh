/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_FILEWRAPDEVICEINPUT_HH
#define	KARABO_XMS_FILEWRAPDEVICEINPUT_HH

#include <fstream>

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
         * The FileWrapDeviceInput class.
         */
        class FileWrapDeviceInput : public Input<std::string > {
        public:

            KARABO_CLASSINFO(FileWrapDeviceInput, "FileWrapDeviceInput", "1.0")

            /**
             * Default constructor.
             */
            FileWrapDeviceInput() {
            }
            
            virtual ~FileWrapDeviceInput() {}

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;


                VECTOR_STRING_ELEMENT(expected).key("connectedOutputChannels")
                        .displayedName("Connected Output Channels")
                        .description("Defines the inter-device connectivity for p-2-p data transfer (use format: <instanceId>/<channelName>)")
                        .assignmentMandatory()
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
                    boost::split(tmp, connectedOutputChannels[i], boost::is_any_of("/"));
                    m_connectedOutputChannels.push_back(karabo::util::Hash("instanceId", tmp[0], "channelId", tmp[1]));
                }

                m_channelId = Memory<std::vector<char> >::registerChannel();
                m_activeChunk = Memory<std::vector<char> >::registerChunk(m_channelId);
                m_inactiveChunk = Memory<std::vector<char> >::registerChunk(m_channelId);
            }

            std::vector<karabo::util::Hash> getConnectedOutputChannels() {
                return m_connectedOutputChannels;
            }

            void read(std::string& filename, size_t idx) {
                std::vector<char> buffer;
                Memory<std::vector<char> >::read(buffer, idx, m_channelId, m_activeChunk);

                std::ofstream os(filename.c_str(), std::ios::binary);
                os.write(const_cast<const char*> (&buffer[0]), buffer.size());
                os.close();
            }

            size_t size() const {
                return Memory<std::vector<char> >::size(m_channelId, m_activeChunk);
            }

            void connectNow(const std::string& instanceId, const karabo::util::Hash& outputChannelInfo) {

                std::string connectionType = outputChannelInfo.get<std::string > ("connectionType");
                std::string memoryLocation = outputChannelInfo.get<std::string > ("memoryLocation");

                if (connectionType == "tcp") {

                    // Prepare connection configuration given output channel information
                    karabo::util::Hash config = prepareConnectionConfiguration(outputChannelInfo);

                    karabo::net::Connection::Pointer tcpConnection = karabo::net::Connection::create(config); // Instantiate
                    startConnection(tcpConnection, instanceId, memoryLocation); // Start

                    if (!m_tcpIoService) {
                        m_tcpIoService = tcpConnection->getIOService();
                        m_tcpIoServiceThread = boost::thread(boost::bind(&karabo::net::IOService::run, m_tcpIoService));
                    } else {
                        tcpConnection->setIOService(m_tcpIoService);
                    }

                }
            }

            karabo::util::Hash prepareConnectionConfiguration(const karabo::util::Hash& serverInfo) const {
                const std::string& hostname = serverInfo.get<std::string > ("hostname");
                const unsigned int& port = serverInfo.get<unsigned int>("port");
                return karabo::util::Hash("Tcp.type", "client", "Tcp.hostname", hostname, "Tcp.port", port);
            }

            void startConnection(karabo::net::Connection::Pointer connection, const std::string& instanceId, const std::string& memoryLocation) {
                //connection->setErrorHandler(&karabo::xms::FileWrapDeviceInput<std::vector<char> >::onTcpConnectionError, this, _1, _2);
                karabo::net::Channel::Pointer channel = connection->start();
                channel->setErrorHandler(boost::bind(&karabo::xms::FileWrapDeviceInput::onTcpChannelError, this, _1, _2));
                channel->write(karabo::util::Hash("instanceId", instanceId, "memoryLocation", memoryLocation)); // Say hello!
                channel->readAsyncVectorHash(boost::bind(&karabo::xms::FileWrapDeviceInput::onTcpChannelRead, this, _1, _2, _3));
                m_tcpConnections.push_back(connection); // TODO check whether really needed
            }

            void onTcpConnectionError(karabo::net::Channel::Pointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelError(karabo::net::Channel::Pointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelRead(karabo::net::Channel::Pointer channel, const std::vector<char>& data, const karabo::util::Hash& header) {
                //boost::mutex::scoped_lock lock(m_mutex);
                std::cout << "Receiving " << data.size() << " bytes of data" << std::endl;
                if (data.size() == 0 && header.has("channelId") && header.has("chunkId")) { // Local memory
                    std::cout << "READING FROM LOCAL MEMORY" << std::endl;
                    unsigned int channelId = header.get<unsigned int>("channelId");
                    unsigned int chunkId = header.get<unsigned int>("chunkId");
                    Memory<std::vector<char> >::writeChunk(Memory<std::vector<char> >::readChunk(channelId, chunkId), m_channelId, m_inactiveChunk);
                } else {
                    std::cout << "READING FROM REMOTE MEMORY (over tcp)" << std::endl;
                    Memory<std::vector<char> >::writeAsContiguosBlock(data, header, m_channelId, m_inactiveChunk);
                }
                if (Memory<std::vector<char> >::size(m_channelId, m_inactiveChunk) < this->getMinimumNumberOfData()) {
                    std::cout << "can read more data" << std::endl;
                    //this->triggerCanReadEvent();
                } else if (Memory<std::vector<char> >::size(m_channelId, m_activeChunk) == 0) {
                    std::swap(m_activeChunk, m_inactiveChunk);
                    std::cout << "swapped buffers, can read more" << std::endl;
                    //this->triggerCanReadEvent();
                    //this->triggerIOEvent();
                }

                channel->readAsyncVectorHash(boost::bind(&karabo::xms::FileWrapDeviceInput::onTcpChannelRead, this, _1, _2, _3));
            }

            bool canCompute() {
                boost::mutex::scoped_lock lock(m_mutex);
                std::cout << "Current size of async read: " << Memory<std::vector<char> >::size(m_channelId, m_activeChunk) << std::endl;
                return Memory<std::vector<char> >::size(m_channelId, m_activeChunk) >= this->getMinimumNumberOfData();
            }

            void update() {
                // Clear active chunk
                Memory<std::vector<char> >::clearChunk(m_channelId, m_activeChunk);
                std::swap(m_activeChunk, m_inactiveChunk);
            }

        private: // members

            std::vector<karabo::util::Hash> m_connectedOutputChannels;

            unsigned int m_channelId;

            boost::mutex m_mutex;

            int m_activeChunk;
            int m_inactiveChunk;

            karabo::net::IOService::Pointer m_tcpIoService;
            std::deque<karabo::net::Connection::Pointer> m_tcpConnections;
            boost::thread m_tcpIoServiceThread;



        private: // functions

            bool needsDeviceConnection() const {
                return true;
            }


        };

    }
}

#endif
