/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_XIP_DEVICEOUTPUT_HH
#define	EXFEL_XIP_DEVICEOUTPUT_HH

#include <boost/asio.hpp>

#include <exfel/net/IOService.hh>
#include <exfel/net/Connection.hh>
#include <exfel/net/Channel.hh>

#include "Statics.hh"
#include "Memory.hh"
#include "Output.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    namespace xip {

        /**
         * The DeviceOutput class.
         */
        template <class T>
        class DeviceOutput : public Output<T> {


            typedef boost::shared_ptr<exfel::net::Channel> TcpChannelPointer;
            typedef std::pair<TcpChannelPointer, std::string> TcpChannelInfo;
            typedef std::map<std::string, TcpChannelInfo> TcpChannelMap;
            typedef std::deque< std::pair<unsigned int, TcpChannelInfo> > WriteNext;

        public:

            EXFEL_CLASSINFO(DeviceOutput, "DeviceOutput-" + T::classInfo().getClassId(), "1.0")

            /**
             * Default constructor.
             */
            DeviceOutput() {
            };

            /**
             * Destructor.
             */
            virtual ~DeviceOutput() {
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(exfel::util::Schema& expected) {
                using namespace exfel::util;

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
            void configure(const exfel::util::Hash& input) {

                input.get("fanOutMode", m_fanOutMode);
                m_channelId = Memory<T>::registerChannel();

                // Data networking
                int tryAgain = 5; // Try maximum 5 times to start a server
                while (tryAgain > 0) {
                    try {
                        m_ownPort = Statics::generateServerPort();
                        exfel::util::Hash h("Tcp.type", "server", "Tcp.port", m_ownPort);
                        m_dataConnection = exfel::net::Connection::create(h);
                        m_dataConnection->setErrorHandler(boost::bind(&exfel::xip::DeviceOutput<T>::onTcpConnectionError, this, _1, _2));
                        m_dataIOService = m_dataConnection->getIOService();
                        m_dataConnection->startAsync(boost::bind(&exfel::xip::DeviceOutput<T>::onTcpConnect, this, _1));
                    } catch (const exfel::util::NetworkException& e) {
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
                m_dataThread = boost::thread(boost::bind(&exfel::net::IOService::run, m_dataIOService));

                // No active chunk yet
                m_activeChunk = -1;

            }

            exfel::util::Hash getInformation() const {
                return exfel::util::Hash("connectionType", "tcp", "hostname", boost::asio::ip::host_name(), "port", m_ownPort);
            }

            void write(const T& data) {
                Memory<T>::write(data, m_channelId, m_activeChunk);
            }

            void onTcpConnect(TcpChannelPointer channel) {
                std::cout << "Connection established" << std::endl;
                channel->setErrorHandler(boost::bind(&exfel::xip::DeviceOutput<T>::onTcpChannelError, this, _1, _2));
                channel->readAsyncHash(boost::bind(&exfel::xip::DeviceOutput<T>::onTcpChannelRead, this, _1, _2));
                m_dataConnection->startAsync(boost::bind(&exfel::xip::DeviceOutput<T>::onTcpConnect, this, _1));
            }

            void onTcpConnectionError(TcpChannelPointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelError(TcpChannelPointer, const std::string& errorMessage) {
                std::cout << errorMessage << std::endl;
            }

            void onTcpChannelRead(TcpChannelPointer channel, const exfel::util::Hash& message) {

                // Associate instanceId with channel
                if (message.has("instanceId") && message.has("memoryLocation")) {
                    std::string instanceId = message.get<std::string>("instanceId");
                    std::string memoryLocation = message.get<std::string>("memoryLocation");
                    std::cout << "Registering input channel of instance: " << instanceId << std::endl;
                    m_instanceId2Channel[instanceId] = std::make_pair(channel, memoryLocation);
                    onInputAvailable(instanceId); // Immediately register for reading
                }
                channel->readAsyncHash(boost::bind(&exfel::xip::DeviceOutput<T>::onTcpChannelRead, this, _1, _2));
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

                {
                    boost::mutex::scoped_lock lock(m_mutex);
                    TcpChannelMap::const_iterator it = m_instanceId2Channel.find(instanceId);
                    if (it != m_instanceId2Channel.end()) {
                        // Create new chunk in memory
                        unsigned int chunkId = Memory<T>::registerChunk(m_channelId);
                        TcpChannelInfo channelInfo = m_instanceId2Channel[instanceId];
                        m_writeNext.push_back(std::make_pair(chunkId, channelInfo));
                    } else {
                        std::cout << "LOW-LEVEL-DEBUG: An input channel wants to connect, that was not registered before." << std::endl;
                    }
                }

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

            void onComputeFinished() {

                std::cout << "onComputeFinished" << std::endl;

                if (m_fanOutMode == "distribute") {
                    if (m_activeMemoryLocation == "local") {
                        distributeLocal();
                    } else {
                        distributeRemote();
                    }
                } else if (m_fanOutMode == "copy") {
                    m_count = m_writeNext.size();
                    Memory<T>::readAsContiguosBlock(m_buffer, m_header, m_channelId, m_activeChunk);
                    for (size_t i = 0; m_writeNext.size(); ++i) {
                        m_writeNext[i].second.first->writeAsyncVectorHash(m_buffer, m_header, boost::bind(&exfel::xip::DeviceOutput<T>::onTcpWriteComplete, this, _1));
                    }
                }

                // Invalidate active chunk
                m_activeChunk = -1;
            }

            void distributeLocal() {
                m_activeTcpChannel->write(std::vector<char>(), exfel::util::Hash("channelId", m_channelId, "chunkId", m_activeChunk));
            }

            void distributeRemote() {
                std::pair< std::vector<char>, exfel::util::Hash>& entry = m_writeQueue[m_activeTcpChannel];
                Memory<T>::readAsContiguosBlock(entry.first, entry.second, m_channelId, m_activeChunk);
                std::cout << "Going to distribute " << entry.first.size() << " bytes of data" << std::endl;
                std::cout << "With header: " << entry.second << std::endl;
                m_activeTcpChannel->writeAsyncVectorHash(entry.first, entry.second, boost::bind(&exfel::xip::DeviceOutput<T>::onTcpWriteComplete, this, _1));
                //m_activeTcpChannel->write(entry.first, entry.second);
            }




        private: // members

            // Server related
            unsigned int m_ownPort;

            exfel::net::Connection::Pointer m_dataConnection;
            //TcpChannelPointer m_dataChannel;
            exfel::net::IOService::Pointer m_dataIOService;
            boost::thread m_dataThread;



            std::string m_fanOutMode;

            TcpChannelMap m_instanceId2Channel;

            WriteNext m_writeNext;

            boost::mutex m_mutex;

            // Distribute out
            std::map<TcpChannelPointer, std::pair< std::vector<char>, exfel::util::Hash> > m_writeQueue;

            // Copy out
            std::vector<char> m_buffer;
            exfel::util::Hash m_header;
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
