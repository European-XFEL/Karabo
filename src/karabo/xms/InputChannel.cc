/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 *
 * Modified to new concepts: April 17, 2015
 */

#include "InputChannel.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace xms {

        KARABO_REGISTER_FOR_CONFIGURATION(InputChannel);

        //        KARABO_REGISTER_FOR_CONFIGURATION(AbstractInput, Input<Hash >, InputChannel<Hash>)
        //        KARABO_REGISTER_FOR_CONFIGURATION(Input<Hash >, InputChannel<Hash>)
        //                
        //        KARABO_REGISTER_FOR_CONFIGURATION(AbstractInput, Input<std::vector<char> >, InputChannel<std::vector<char> >)
        //        KARABO_REGISTER_FOR_CONFIGURATION(Input<std::vector<char> >, InputChannel<std::vector<char> >)


        void InputChannel::expectedParameters(karabo::util::Schema& expected) {
            using namespace karabo::util;


            VECTOR_STRING_ELEMENT(expected).key("connectedOutputChannels")
                    .displayedName("Connected Output Channels")
                    .description("Defines the inter-device connectivity for p-2-p data transfer (use format: <instanceId>:<channelName>)")
                    .assignmentOptional().noDefaultValue()
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("dataDistribution")
                    .displayedName("Data Distribution")
                    .description("The way data is fetched from the connected output channels (shared/copy)")
                    .options("copy,shared")
                    .assignmentOptional().defaultValue("copy")
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("onSlowness")
                    .displayedName("On Slowness")
                    .description("Policy for what to do if this input is too slow for the fed data rate (only used in copy mode)")
                    .options("drop,throw,wait,queue")
                    .assignmentOptional().defaultValue("wait")
                    .init()
                    .commit();

            UINT32_ELEMENT(expected).key("minData")
                    .displayedName("Minimum number input packets")
                    .description("The number of elements to be read before any computation is started (0 = all, 0xFFFFFFFF = none/any)")
                    .assignmentOptional().defaultValue(1)
                    .init()
                    .commit();

            BOOL_ELEMENT(expected).key("keepDataUntilNew")
                    .displayedName("Keep data until new")
                    .description("If true, keeps data until new data from an connected output is provided. "
                    "If new data is available the previous chunk is automatically deleted and the new one is made available for reading")
                    .assignmentOptional().defaultValue(false)
                    .init()
                    .commit();

            BOOL_ELEMENT(expected).key("respondToEndOfStream")
                    .displayedName("Respond to end-of-stream")
                    .description("Determines whether this input should forward a end-of-stream event to its parent device.")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .commit();
        }


        InputChannel::InputChannel(const karabo::util::Hash& config) : m_isEndOfStream(false), m_respondToEndOfStream(true) {
            parseOutputChannelConfiguration(config);
            config.get("dataDistribution", m_dataDistribution);
            config.get("minData", m_minData);
            config.get("keepDataUntilNew", m_keepDataUntilNew);
            config.get("onSlowness", m_onSlowness);
            config.get("respondToEndOfStream", m_respondToEndOfStream);

            m_channelId = MemoryType::registerChannel();
            m_inactiveChunk = MemoryType::registerChunk(m_channelId);
            m_activeChunk = MemoryType::registerChunk(m_channelId);


            KARABO_LOG_FRAMEWORK_DEBUG << "Inputting on channel " << m_channelId << " (active) chunkId " << m_activeChunk << " and (inactive) chunkId " << m_inactiveChunk;

        }


        InputChannel::~InputChannel() {
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


        void InputChannel::reconfigure(const karabo::util::Hash& config) {
            parseOutputChannelConfiguration(config);
            if (config.has("dataDistribution")) config.get("dataDistribution", m_dataDistribution);
            if (config.has("minData")) config.get("minData", m_minData);
            if (config.has("keepDataUntilNew")) config.get("keepDataUntilNew", m_keepDataUntilNew);
            if (config.has("onSlowness")) config.get("onSlowness", m_onSlowness);
            if (config.has("respondToEndOfStream")) config.get("respondToEndOfStream", m_respondToEndOfStream);
        }


        void InputChannel::setInstanceId(const std::string& instanceId) {
            m_instanceId = instanceId;
        }


        const std::string& InputChannel::getInstanceId() const {
            return m_instanceId;
        }


        void InputChannel::registerIOEventHandler(const boost::function<void (const Self::Pointer&)>& ioEventHandler) {
            m_dataAvailableHandler = ioEventHandler;
        }


        void InputChannel::registerEndOfStreamEventHandler(const boost::function<void (const Self::Pointer&)>& endOfStreamEventHandler) {
            m_endOfStreamHandler = endOfStreamEventHandler;
        }


        std::vector<karabo::util::Hash> InputChannel::getConnectedOutputChannels() {
            return m_connectedOutputChannels;
        }


        void InputChannel::read(karabo::util::Hash& data, size_t idx) {
            boost::mutex::scoped_lock lock(m_swapBuffersMutex);
            MemoryType::read(data, idx, m_channelId, m_activeChunk);
        }
        
        karabo::util::Hash::Pointer InputChannel::read(size_t idx) {
            boost::mutex::scoped_lock lock(m_swapBuffersMutex);
            return MemoryType::read(idx, m_channelId, m_activeChunk);
        }              
        
        size_t InputChannel::size() {
            return MemoryType::size(m_channelId, m_activeChunk);
        }

        unsigned int InputChannel::getMinimumNumberOfData() const {
            return m_minData;
        }


        void InputChannel::connectNow(const karabo::util::Hash& outputChannelInfo) {
            connect(outputChannelInfo);
        }


        void InputChannel::connect(const karabo::util::Hash& outputChannelInfo) {

            std::string connectionType = outputChannelInfo.get<std::string > ("connectionType");

            if (connectionType == "tcp") {

                // Prepare connection configuration given output channel information
                karabo::util::Hash config = prepareConnectionConfiguration(outputChannelInfo);
                karabo::net::Connection::Pointer tcpConnection = karabo::net::Connection::create(config); // Instantiate

                if (!m_tcpIoService) {
                    // Get IO service and save for later sharing
                    m_tcpIoService = tcpConnection->getIOService();
                    // Establish connection (and define sub-type of server)
                    startConnection(tcpConnection, outputChannelInfo);
                    m_tcpIoServiceThread = boost::thread(boost::bind(&karabo::net::IOService::run, m_tcpIoService));
                } else {
                    tcpConnection->setIOService(m_tcpIoService);
                    startConnection(tcpConnection, outputChannelInfo);
                }
            }
        }


        void InputChannel::disconnect(const karabo::util::Hash& outputChannelInfo) {

            const std::string& hostname = outputChannelInfo.get<std::string > ("hostname");
            const std::string& port = outputChannelInfo.getAs<std::string>("port");

            TcpChannels::iterator it = m_tcpChannels.find(hostname + port);
            if (it != m_tcpChannels.end()) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Disconnecting...";
                it->second->close(); // Closes channel

                // TODO think about erasing it here
            }
        }


        karabo::util::Hash InputChannel::prepareConnectionConfiguration(const karabo::util::Hash& outputChannelInfo) const {
            const std::string& hostname = outputChannelInfo.get<std::string > ("hostname");
            const unsigned int& port = outputChannelInfo.get<unsigned int>("port");
            karabo::util::Hash h("Tcp.type", "client", "Tcp.hostname", hostname, "Tcp.port", port);
            return h;
        }


        void InputChannel::startConnection(karabo::net::Connection::Pointer connection, const karabo::util::Hash& outputChannelInfo) {

            const std::string& memoryLocation = outputChannelInfo.get<std::string > ("memoryLocation");
            const std::string& hostname = outputChannelInfo.get<std::string > ("hostname");
            const std::string& port = outputChannelInfo.getAs<std::string>("port");

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
            channel->setErrorHandler(boost::bind(&karabo::xms::InputChannel::onTcpChannelError, this, channel, _1));
            channel->write(karabo::util::Hash("reason", "hello", "instanceId", this->getInstanceId(), "memoryLocation", memoryLocation, "dataDistribution", m_dataDistribution, "onSlowness", m_onSlowness)); // Say hello!
            channel->readAsyncHashVector(boost::bind(&karabo::xms::InputChannel::onTcpChannelRead, this, channel, _1, _2));

            m_tcpConnections.insert(connection); // TODO check whether really needed
            m_tcpChannels.insert(std::make_pair(hostname + port, channel));
        }


        void InputChannel::onTcpConnectionError(karabo::net::Channel::Pointer, const karabo::net::ErrorCode& error) {
            KARABO_LOG_FRAMEWORK_ERROR << error.value() << ": " << error.message();
        }


        void InputChannel::onTcpChannelError(karabo::net::Channel::Pointer, const karabo::net::ErrorCode& error) {
            KARABO_LOG_FRAMEWORK_INFO << error.value() << ": " << error.message();
        }


        void InputChannel::onTcpChannelRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::vector<char>& data) {

            boost::mutex::scoped_lock lock(m_mutex);

            m_isEndOfStream = false;

            if (header.has("endOfStream")) {

                // Track the channels that sent eos
                m_eosChannels.insert(channel);

                KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Received EOS #" << m_eosChannels.size();
                if (m_respondToEndOfStream) m_isEndOfStream = true;
                if (this->getMinimumNumberOfData() <= 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Triggering another compute";
                    this->swapBuffers();
                    this->triggerIOEvent();
                }

                if (m_eosChannels.size() == m_tcpChannels.size()) {
                    if (m_respondToEndOfStream) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Triggering EOS function after reception of " << m_eosChannels.size() << " EOS tokens";
                        this->triggerEndOfStreamEvent();
                    }
                    // Reset eos tracker
                    m_eosChannels.clear();
                }

                channel->readAsyncHashVector(boost::bind(&karabo::xms::InputChannel::onTcpChannelRead, this, channel, _1, _2));
                return;
            }

            if (data.size() == 0 && header.has("channelId") && header.has("chunkId")) { // Local memory

                unsigned int channelId = header.get<unsigned int>("channelId");
                unsigned int chunkId = header.get<unsigned int>("chunkId");

                KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Reading from local memory [" << channelId << "][" << chunkId << "]";

                MemoryType::writeChunk(MemoryType::readChunk(channelId, chunkId), m_channelId, m_inactiveChunk);
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
                m_mutex.unlock(); // TODO scoped in case of exception thrown is callback code
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

            channel->readAsyncHashVector(boost::bind(&karabo::xms::InputChannel::onTcpChannelRead, this, channel, _1, _2));
        }


        void InputChannel::triggerIOEvent() {
            if (m_dataAvailableHandler) {
                m_dataAvailableHandler(shared_from_this());
            }
        }


        void InputChannel::triggerEndOfStreamEvent() {
            if (m_endOfStreamHandler) {
                m_endOfStreamHandler(shared_from_this());
            }
        }


        void InputChannel::swapBuffers() {
            //boost::mutex::scoped_lock lock(m_swapBuffersMutex);
            std::swap(m_activeChunk, m_inactiveChunk);
        }


        bool InputChannel::canCompute() const {
            //KARABO_LOG_FRAMEWORK_DEBUG << "INPUT: Current size of async read data cache: " << MemoryType::size(m_channelId, m_activeChunk);
            //KARABO_LOG_FRAMEWORK_DEBUG << "INPUT: Is end of stream? " << m_isEndOfStream;
            //KARABO_LOG_FRAMEWORK_DEBUG << "INPUT: MinData " << this->getMinimumNumberOfData();
            if ((this->getMinimumNumberOfData() == 0xFFFFFFFF)) {
                if (m_isEndOfStream && m_respondToEndOfStream) {
                    return false;
                }
                return true;
            }

            if (m_isEndOfStream && (MemoryType::size(m_channelId, m_activeChunk) == 0)) return false;

            if (!m_isEndOfStream && (this->getMinimumNumberOfData() <= 0)) return false;

            return MemoryType::size(m_channelId, m_activeChunk) >= this->getMinimumNumberOfData();
        }


        void InputChannel::update() {

            boost::mutex::scoped_lock lock(m_mutex);

            if (m_keepDataUntilNew) return;

            // Clear active chunk
            MemoryType::clearChunkData(m_channelId, m_activeChunk);

            // Swap buffers               
            swapBuffers();

            // Fetch number of data pieces
            size_t nActiveData = MemoryType::size(m_channelId, m_activeChunk);

            // Notify all connected output channels for another read
            if (nActiveData >= this->getMinimumNumberOfData()) {
                notifyOutputChannelsForPossibleRead();
            }
        }


        void InputChannel::notifyOutputChannelForPossibleRead(const karabo::net::Channel::Pointer& channel) {
            KARABO_LOG_FRAMEWORK_DEBUG << "INPUT Notifying output channel that " << this->getInstanceId() << " is ready for next read.";
            channel->write(karabo::util::Hash("reason", "update", "instanceId", this->getInstanceId()));
        }


        void InputChannel::notifyOutputChannelsForPossibleRead() {
            for (TcpChannels::const_iterator it = m_tcpChannels.begin(); it != m_tcpChannels.end(); ++it) {
                notifyOutputChannelForPossibleRead(it->second);
            }
        }


        bool InputChannel::respondsToEndOfStream() {
            return m_respondToEndOfStream;
        }


        void InputChannel::parseOutputChannelConfiguration(const karabo::util::Hash& config) {
            if (config.has("connectedOutputChannels")) {
                std::vector<std::string> connectedOutputChannels;
                config.get("connectedOutputChannels", connectedOutputChannels);
                for (size_t i = 0; i < connectedOutputChannels.size(); ++i) {
                    std::vector<std::string> tmp;
                    boost::split(tmp, connectedOutputChannels[i], boost::is_any_of("@:"));
                    if (tmp.size() == 2) {
                        m_connectedOutputChannels.push_back(karabo::util::Hash("instanceId", tmp[0], "channelId", tmp[1]));
                    } else {
                        throw KARABO_PARAMETER_EXCEPTION("Illegal format for connected output channel, expecting <deviceId>:<channelName>");
                    }
                }
            }
        }


        bool InputChannel::needsDeviceConnection() const {
            return true;
        }

    }
}
