/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_FILEWRAPDEVICEOUTPUT_HH
#define	KARABO_XMS_FILEWRAPDEVICEOUTPUT_HH

#include <fstream>
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
         * The FileWrapDeviceOutput class.
         */
        class FileWrapDeviceOutput : public Output<std::string > {
            
            
            typedef boost::shared_ptr<karabo::net::Channel> TcpChannelPointer;
            typedef std::pair<TcpChannelPointer, std::string> TcpChannelInfo;
            typedef std::map<std::string, TcpChannelInfo> TcpChannelMap;
            typedef std::deque< std::pair<unsigned int, TcpChannelInfo> > WriteNext;
            
           
        public:

            KARABO_CLASSINFO(FileWrapDeviceOutput, "FileWrapDeviceOutput", "1.0")

            /**
             * Default constructor.
             */
            FileWrapDeviceOutput() {
            };

            /**
             * Destructor.
             */
            virtual ~FileWrapDeviceOutput() {
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

                STRING_ELEMENT(expected).key("fanOutMode")
                        .displayedName("Fan Out Mode")
                        .description("Fan out mode")
                        .options("copy,distribute")
                        .assignmentOptional().defaultValue("distribute")
                        .init()
                        .commit();
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash& input) {

                input.get("fanOutMode", m_fanOutMode);
                m_channelId = Memory<std::vector<char> >::registerChannel();

                // Data networking
                int tryAgain = 5; // Try maximum 5 times to start a server
                while (tryAgain > 0) {
                    try {
                        m_ownPort = Statics::generateServerPort();
                        karabo::util::Hash h("Tcp.type", "server", "Tcp.port", m_ownPort);
                        m_dataConnection = karabo::net::Connection::create(h);
                        m_dataConnection->setErrorHandler(boost::bind(&karabo::xms::FileWrapDeviceOutput::onTcpConnectionError, this, _1, _2));
                        m_dataIOService = m_dataConnection->getIOService();
                        m_dataConnection->startAsync(boost::bind(&karabo::xms::FileWrapDeviceOutput::onTcpConnect, this, _1));
                    } catch (const karabo::util::NetworkException& e) {
                        if (tryAgain > 0) {
                            tryAgain--;
                            continue;
                        } else {
                            throw NETWORK_EXCEPTION("Could not start TcpServer for output channel");
                        }
                    }
                    tryAgain = 0;
                    std::cout << "Started DeviceOutput-Server listening on port: " << m_ownPort << std::endl;
                }

                // Start data thread
                m_dataThread = boost::thread(boost::bind(&karabo::net::IOService::run, m_dataIOService));

                // No active chunk yet
                m_activeChunk = -1;

            }

            karabo::util::Hash getInformation() const {
                return karabo::util::Hash("connectionType", "tcp", "hostname", boost::asio::ip::host_name(), "port", m_ownPort);
            }

            void write(const std::string& filename) {
                std::ifstream file(filename.c_str(), std::ios::binary);
                std::vector<char> buffer;
                //fileContents.reserve(10000);
                buffer.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
                Memory<std::vector<char> >::write(buffer, m_channelId, m_activeChunk);
            }

            void onTcpConnect(TcpChannelPointer channel) {
                std::cout << "Connection established" << std::endl;
                channel->setErrorHandler(boost::bind(&karabo::xms::FileWrapDeviceOutput::onTcpChannelError, this, _1, _2));
                channel->readAsyncHash(boost::bind(&karabo::xms::FileWrapDeviceOutput::onTcpChannelRead, this, _1, _2));
                m_dataConnection->startAsync(boost::bind(&karabo::xms::FileWrapDeviceOutput::onTcpConnect, this, _1));
            }

            void onTcpConnectionError(TcpChannelPointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelError(TcpChannelPointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelRead(TcpChannelPointer channel, const karabo::util::Hash& message) {

                // Associate instanceId with channel
                if (message.has("instanceId") && message.has("memoryLocation")) {
                    std::string instanceId = message.get<std::string > ("instanceId");
                    std::string memoryLocation = message.get<std::string > ("memoryLocation");
                    std::cout << "Registering input channel of instance: " << instanceId << std::endl;
                    m_instanceId2Channel[instanceId] = std::make_pair(channel, memoryLocation);
                    onInputAvailable(instanceId); // Immediately register for reading
                }
                channel->readAsyncHash(boost::bind(&karabo::xms::FileWrapDeviceOutput::onTcpChannelRead, this, _1, _2));
            }

            void onTcpWriteComplete(TcpChannelPointer channel) {
                if (m_fanOutMode == "distribute") {
                    m_writeQueue.erase(channel);
                } else if (m_fanOutMode == "copy") {
                    m_count--;
                    if (m_count == 0) {
                        std::cout << "All copies sent" << std::endl;
                    }
                }
            }

            void onInputAvailable(const std::string& instanceId) {

                std::cout << "New input on instance " << instanceId << " available for writing " << std::endl;

                m_mutex.lock();
                TcpChannelMap::const_iterator it = m_instanceId2Channel.find(instanceId);
                if (it != m_instanceId2Channel.end()) {
                    // Create new chunk in memory
                    unsigned int chunkId = Memory<std::vector<char> >::registerChunk(m_channelId);
                    TcpChannelInfo channelInfo = m_instanceId2Channel[instanceId];
                    m_writeNext.push_back(std::make_pair(chunkId, channelInfo));
                } else {
                    std::cout << "LOW-LEVEL-DEBUG: An input channel wants to connect, that was not registered before." << std::endl;
                }
                m_mutex.unlock();

                this->triggerIOEvent();
            }

            bool canCompute() {
                boost::mutex::scoped_lock lock(m_mutex);
                if (m_activeChunk == -1 && m_writeNext.empty()) return false;
                else {
                    if (m_activeChunk == -1) {
                        m_activeChunk = m_writeNext[0].first;
                        m_activeTcpChannel = m_writeNext[0].second.first;
                        m_activeMemoryLocation = m_writeNext[0].second.second;
                        m_writeNext.pop_front();

                        // DEBUG
                        std::cout << "New active chunk: " << m_activeChunk
                                << ", new active channel: " << m_activeTcpChannel
                                << ", new active memoryLocation: " << m_activeMemoryLocation << std::endl;


                    }
                    return true;
                }
            }

            void update() {

                std::cout << "onComputeFinished" << std::endl;

                if (m_fanOutMode == "distribute") {
                    if (m_activeMemoryLocation == "local") {
                        distributeLocal();
                    } else {
                        distributeRemote();
                    }
                } else if (m_fanOutMode == "copy") {
                    m_count = m_writeNext.size();
                    Memory<std::vector<char> >::readAsContiguosBlock(m_buffer, m_header, m_channelId, m_activeChunk);
                    for (size_t i = 0; m_writeNext.size(); ++i) {
                        m_writeNext[i].second.first->writeAsyncVectorHash(m_buffer, m_header, boost::bind(&karabo::xms::FileWrapDeviceOutput::onTcpWriteComplete, this, _1));
                    }
                }

                // Invalidate active chunk
                m_activeChunk = -1;
            }

            void distributeLocal() {
                m_activeTcpChannel->write(std::vector<char>(), karabo::util::Hash("channelId", m_channelId, "chunkId", m_activeChunk));
            }

            void distributeRemote() {
                std::pair< std::vector<char>, karabo::util::Hash>& entry = m_writeQueue[m_activeTcpChannel];
                Memory<std::vector<char> >::readAsContiguosBlock(entry.first, entry.second, m_channelId, m_activeChunk);
                std::cout << "Going to distribute " << entry.first.size() << " bytes of data" << std::endl;
                std::cout << "With header: " << entry.second << std::endl;
                m_activeTcpChannel->writeAsyncVectorHash(entry.first, entry.second, boost::bind(&karabo::xms::FileWrapDeviceOutput::onTcpWriteComplete, this, _1));
                //m_activeTcpChannel->write(entry.first, entry.second);
            }




        private: // members

            // Server related
            unsigned int m_ownPort;

            karabo::net::Connection::Pointer m_dataConnection;
            //TcpChannelPointer m_dataChannel;
            karabo::net::IOService::Pointer m_dataIOService;
            boost::thread m_dataThread;



            std::string m_fanOutMode;

            TcpChannelMap m_instanceId2Channel;

            WriteNext m_writeNext;

            boost::mutex m_mutex;

            // Distribute out
            std::map<TcpChannelPointer, std::pair< std::vector<char>, karabo::util::Hash> > m_writeQueue;

            // Copy out
            std::vector<char> m_buffer;
            karabo::util::Hash m_header;
            int m_count;

            // Active output
            int m_activeChunk;
            TcpChannelPointer m_activeTcpChannel;
            std::string m_activeMemoryLocation;

            unsigned int m_channelId;


        private: // functions

        };

    }
}

#endif
