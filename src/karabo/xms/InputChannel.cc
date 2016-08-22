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
#include "karabo/net/EventLoop.hh"

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

            INT32_ELEMENT(expected).key("delayOnInput")
                    .displayedName("Delay on Input channel")
                    .description("Some delay before informing output channel about readiness for next data.")
                    .assignmentOptional().defaultValue(0)
                    .unit(Unit::SECOND)
                    .metricPrefix(MetricPrefix::MILLI)
                    .init()
                    .commit();
        }


        InputChannel::InputChannel(const karabo::util::Hash& config)
            : m_deadline(karabo::net::EventLoop::getIOService())
            , m_isEndOfStream(false)
            , m_respondToEndOfStream(true) {
            parseOutputChannelConfiguration(config);
            config.get("dataDistribution", m_dataDistribution);
            config.get("minData", m_minData);
            config.get("keepDataUntilNew", m_keepDataUntilNew);
            config.get("onSlowness", m_onSlowness);
            config.get("respondToEndOfStream", m_respondToEndOfStream);
            config.get("delayOnInput", m_delayOnInput);

            m_channelId = MemoryType::registerChannel();
            m_inactiveChunk = MemoryType::registerChunk(m_channelId);
            m_activeChunk = MemoryType::registerChunk(m_channelId);

            KARABO_LOG_FRAMEWORK_DEBUG << "Inputting on channel " << m_channelId << " (active) chunkId " << m_activeChunk << " and (inactive) chunkId " << m_inactiveChunk;
        }


        InputChannel::~InputChannel() {
            closeChannelsAndStopConnections();
            if (m_tcpIoService) {
                m_tcpIoService->stop();
                if (m_tcpIoServiceThread.get_id() == boost::this_thread::get_id()) {
                    KARABO_LOG_FRAMEWORK_WARN << BOOST_CURRENT_FUNCTION << ":" << __LINE__ << " : attempt to join itself";
                } else
                    m_tcpIoServiceThread.join();
            }
            MemoryType::unregisterChannel(m_channelId);
            KARABO_LOG_FRAMEWORK_DEBUG << "*** InputChannel::~InputChannel DTOR for channelId = " << m_channelId;
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


        void InputChannel::registerInputHandler(const boost::function<void (const Self::Pointer&)>& ioInputHandler) {
            if (m_dataHandler) {
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Clear "
                        << "data handler per Data since setting one per InputChannel";
                m_dataHandler.clear();
            }
            m_inputHandler = ioInputHandler;
        }


        void InputChannel::registerDataHandler(const boost::function<void (const Data&) >& ioDataHandler) {
            if (m_inputHandler) {
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Clear "
                        << "data handler per InputChannel since setting one per Data";
                m_inputHandler.clear();
            }

            m_dataHandler = ioDataHandler;
        }


        void InputChannel::registerEndOfStreamEventHandler(const boost::function<void (const Self::Pointer&)>& endOfStreamEventHandler) {
            m_endOfStreamHandler = endOfStreamEventHandler;
        }


        std::map<std::string, karabo::util::Hash> InputChannel::getConnectedOutputChannels() {
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


        void InputChannel::connect(const karabo::util::Hash& outputChannelInfo) {

            KARABO_LOG_FRAMEWORK_DEBUG << "connect  on \"" << m_instanceId << "\"  :   outputChannelInfo is ...\n" << outputChannelInfo;

            std::string connectionType = outputChannelInfo.get<std::string > ("connectionType");

            if (connectionType == "tcp") {

                if (outputChannelInfo.has("outputChannelString")) {
                    const string& outputChannelString = outputChannelInfo.get<string>("outputChannelString");
                    boost::mutex::scoped_lock lock(m_outputChannelsMutex);
                    OpenConnections::iterator it = m_openConnections.find(outputChannelString);
                    if (it != m_openConnections.end()) return; // Already connected!
                }

                KARABO_LOG_FRAMEWORK_DEBUG << "connect  on \"" << m_instanceId << "\"   :   No old connection found.  Create one!";

                // Prepare connection configuration given output channel information
                karabo::util::Hash config = prepareConnectionConfiguration(outputChannelInfo);
                // Instantiate connection object
                karabo::net::Connection::Pointer tcpConnection = karabo::net::Connection::create(config);

                // Establish connection (and define sub-type of server)
                startConnectionAsync(tcpConnection, outputChannelInfo);
            }
        }


        void InputChannel::disconnect(const karabo::util::Hash& outputChannelInfo) {

            const std::string& hostname = outputChannelInfo.get<std::string > ("hostname");
            unsigned int port = outputChannelInfo.get<unsigned int>("port");

            string outputChannelString;
            {
                boost::mutex::scoped_lock lock(m_outputChannelsMutex);
                for (ConnectedOutputChannels::const_iterator it = m_connectedOutputChannels.begin(); it != m_connectedOutputChannels.end(); ++it) {
                    if (it->second.empty()) continue;
                    if (it->second.get<string>("hostname") != hostname) continue;
                    if (it->second.get<unsigned int>("port") != port) continue;
                    outputChannelString = it->first;
                    break;
                }
            }
            if (!outputChannelString.empty()) disconnect(outputChannelString);
        }


        void InputChannel::disconnect(const std::string& outputChannelString) {

            if (outputChannelString.empty()) return;

            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            OpenConnections::iterator it = m_openConnections.find(outputChannelString);
            if (it == m_openConnections.end()) return;

            KARABO_LOG_FRAMEWORK_DEBUG << "Disconnecting...";
            it->second.second->close(); // Closes channel
            it->second.first->stop();
            m_openConnections.erase(it);
            ConnectedOutputChannels::iterator ii = m_connectedOutputChannels.find(outputChannelString);
            if (ii == m_connectedOutputChannels.end()) return;
            // Should we clean Hash or keep it?  More safe is to clean.
            ii->second.clear();
        }


        void InputChannel::closeChannelsAndStopConnections() {
            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            for (OpenConnections::iterator it = m_openConnections.begin(); it != m_openConnections.end(); ++it) {
                it->second.second->close();
                it->second.first->stop();
            }
            m_openConnections.clear();
        }


        karabo::util::Hash InputChannel::prepareConnectionConfiguration(const karabo::util::Hash& outputChannelInfo) const {
            const std::string& hostname = outputChannelInfo.get<std::string > ("hostname");
            const unsigned int& port = outputChannelInfo.get<unsigned int>("port");
            karabo::util::Hash h("Tcp.type", "client", "Tcp.hostname", hostname, "Tcp.port", port);
            return h;
        }


        void InputChannel::onConnect(const karabo::net::ErrorCode& ec,
                                     karabo::net::Connection::Pointer connection,
                                     const karabo::util::Hash& outputChannelInfo,
                                     karabo::net::Channel::Pointer channel) {

            KARABO_LOG_FRAMEWORK_DEBUG << "onConnect  :  outputChannelInfo is ...\n" << outputChannelInfo;

            if (ec) {
                onTcpChannelError(ec, channel);
                return;
            }

            const std::string& memoryLocation = outputChannelInfo.get<std::string > ("memoryLocation");
            const std::string& hostname = outputChannelInfo.get<std::string > ("hostname");
            unsigned int port = outputChannelInfo.get<unsigned int>("port");

            channel->write(karabo::util::Hash("reason", "hello", "instanceId", this->getInstanceId(), "memoryLocation", memoryLocation, "dataDistribution", m_dataDistribution, "onSlowness", m_onSlowness)); // Say hello!
            channel->readAsyncHashVector(boost::bind(&karabo::xms::InputChannel::onTcpChannelRead, this, _1, channel, _2, _3));

            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            string outputChannelString;
            if (outputChannelInfo.has("outputChannelString")) {
                outputChannelString = outputChannelInfo.get<string>("outputChannelString");
            } else {
                for (ConnectedOutputChannels::const_iterator it = m_connectedOutputChannels.begin(); it != m_connectedOutputChannels.end(); ++it) {
                    if (!it->second.empty() && it->second.get<string>("hostname") == hostname && it->second.get<unsigned>("port") == port) {
                        outputChannelString = it->first;
                        break;
                    }
                }
            }
            if (outputChannelString.empty())
                throw KARABO_PARAMETER_EXCEPTION("Output Channel String is not registered  in \"ConnectedOutputChannels\"!");
            m_openConnections.insert(std::make_pair(outputChannelString, std::make_pair(connection, channel)));
        }


        void InputChannel::startConnectionAsync(karabo::net::Connection::Pointer connection, const karabo::util::Hash& outputChannelInfo) {
            connection->startAsync(boost::bind(&InputChannel::onConnect, this, _1, connection, outputChannelInfo, _2));
        }


        void InputChannel::onTcpConnectionError(const karabo::net::ErrorCode& error, const karabo::net::Connection::Pointer& connection) {

            string outputChannelString;

            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            for (OpenConnections::iterator ii = m_openConnections.begin(); ii != m_openConnections.end(); ++ii) {
                if (ii->second.first == connection) {
                    ii->second.second->close();
                    connection->stop();
                    outputChannelString = ii->first;
                    m_openConnections.erase(ii);
                    break;
                }
            }

            KARABO_LOG_FRAMEWORK_INFO << "onTcpConnectionError on \"" << m_instanceId << "\"  connected to \""
                    << outputChannelString << "\"  :  code #" << error.value() << " -- \"" << error.message() << "\".  Close channel.";

            if (outputChannelString.empty()) return;
            ConnectedOutputChannels::iterator it = m_connectedOutputChannels.find(outputChannelString);
            if (it == m_connectedOutputChannels.end()) return;
            it->second.clear();
        }


        void InputChannel::onTcpChannelError(const karabo::net::ErrorCode& error, const karabo::net::Channel::Pointer& channel) {

            string outputChannelString;

            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            for (OpenConnections::iterator ii = m_openConnections.begin(); ii != m_openConnections.end(); ++ii) {
                if (ii->second.second == channel) {
                    channel->close();
                    ii->second.first->stop();
                    outputChannelString = ii->first;
                    m_openConnections.erase(ii);
                    break;
                }
            }

            KARABO_LOG_FRAMEWORK_INFO << "onTcpChannelError on \"" << m_instanceId << "\"  connected to \""
                    << outputChannelString << "\"  :  code #" << error.value() << " -- \"" << error.message() << "\".  Close channel.";

            if (outputChannelString.empty()) return;
            ConnectedOutputChannels::iterator it = m_connectedOutputChannels.find(outputChannelString);
            if (it == m_connectedOutputChannels.end()) return;
            it->second.clear();
        }


        void InputChannel::onTcpChannelRead(const karabo::net::ErrorCode& ec,
                                            karabo::net::Channel::Pointer channel,
                                            const karabo::util::Hash& header,
                                            const std::vector<char>& data) {
            if (ec) {
                onTcpChannelError(ec, channel);
                return;
            }

            // Debug helper (m_channelId is unique per process...):
            const std::string debugId((("INPUT " + util::toString(m_channelId) += " of '") += this->getInstanceId()) += "' ");
            KARABO_LOG_FRAMEWORK_DEBUG << debugId << "ENTRY onTcpChannelRead";
            try {
                // set the guard: it is guaranteed that InputChannel object is alive
                // ... or exception brings us out here
                InputChannel::Pointer self = shared_from_this();

                {
                    boost::mutex::scoped_lock lock(m_mutex);
                    m_isEndOfStream = false;

                    if (header.has("endOfStream")) {

                        // Track the channels that sent eos
                        m_eosChannels.insert(channel);

                        KARABO_LOG_FRAMEWORK_DEBUG << debugId << "Received EOS #" << m_eosChannels.size();
                        if (m_respondToEndOfStream) m_isEndOfStream = true;
                        if (this->getMinimumNumberOfData() <= 0) {
                            KARABO_LOG_FRAMEWORK_TRACE << debugId << "Triggering another compute";
                            this->swapBuffers();
                            m_tcpIoService->post(boost::bind(&InputChannel::triggerIOEvent, this));
                        }
                        if (m_eosChannels.size() == m_openConnections.size()) {
                            if (m_respondToEndOfStream) {
                                KARABO_LOG_FRAMEWORK_TRACE << debugId << "Triggering EOS function after reception of " << m_eosChannels.size() << " EOS tokens";
                                m_tcpIoService->post(boost::bind(&InputChannel::triggerEndOfStreamEvent, this));
                            }
                            // Reset eos tracker
                            m_eosChannels.clear();
                        }
                        channel->readAsyncHashVector(boost::bind(&karabo::xms::InputChannel::onTcpChannelRead, this, _1, channel, _2, _3));
                        return;
                    }

                    if (data.size() == 0 && header.has("channelId") && header.has("chunkId")) { // Local memory

                        unsigned int channelId = header.get<unsigned int>("channelId");
                        unsigned int chunkId = header.get<unsigned int>("chunkId");

                        KARABO_LOG_FRAMEWORK_TRACE << debugId << "Reading from local memory [" << channelId << "][" << chunkId << "]";

                        MemoryType::writeChunk(MemoryType::readChunk(channelId, chunkId), m_channelId, m_inactiveChunk);
                        MemoryType::decrementChunkUsage(channelId, chunkId);

                    } else { // TCP data

                        KARABO_LOG_FRAMEWORK_TRACE << debugId << "Reading from remote memory (over tcp)";
                        MemoryType::writeAsContiguosBlock(data, header, m_channelId, m_inactiveChunk);

                    }

                    size_t nInactiveData = MemoryType::size(m_channelId, m_inactiveChunk);
                    size_t nActiveData = MemoryType::size(m_channelId, m_activeChunk);

                    if ((this->getMinimumNumberOfData()) <= 0 || (nInactiveData < this->getMinimumNumberOfData())) { // Not enough data, yet
                        KARABO_LOG_FRAMEWORK_TRACE << debugId << "Can read more data";
                        notifyOutputChannelForPossibleRead(channel);
                    } else if (nActiveData == 0) { // Data complete, second pot still empty

                        this->swapBuffers();
                        notifyOutputChannelForPossibleRead(channel);

                        // No mutex under callback
                        KARABO_LOG_FRAMEWORK_TRACE << debugId << "Triggering IOEvent";
                        m_tcpIoService->post(boost::bind(&InputChannel::triggerIOEvent, this));
                    } else { // Data complete on both pots now
                        if (m_keepDataUntilNew) { // Is false per default
                            m_keepDataUntilNew = false;
                            KARABO_LOG_FRAMEWORK_TRACE << debugId << "Updating";
                            m_tcpIoService->post(boost::bind(&InputChannel::update, this));
                            m_keepDataUntilNew = true;
                        }
                    }
                    channel->readAsyncHashVector(boost::bind(&karabo::xms::InputChannel::onTcpChannelRead, this, _1, channel, _2, _3));
                }
            } catch (const karabo::util::Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onTcpChannelRead " << e;
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onTcpChannelRead (std::exception) : " << e.what();
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "Unknown problem in onTcpChannelRead";
            }

        }


        void InputChannel::triggerIOEvent() {
            try {
                // set the guard: it is guaranteed that InputChannel object is alive
                // ... or exception brings us out here
                InputChannel::Pointer self = shared_from_this();

                // There is either m_inputHandler or m_dataHandler
                // (or neither), see registerInputHandler and registerDataHandler.
                if (m_inputHandler && m_dataHandler) {
                    // Just in case that the above promise is not the case...
                    KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Clear "
                            << "input handler since we have a data handler.";
                    // Clear inputHandler since dataHandler is the recommended
                    // interface (though inputHandler is more general...).
                    m_inputHandler.clear();
                }

                if (m_dataHandler) {
                    for (size_t i = 0; i < this->size(); ++i) {
                        m_dataHandler(this->read(i));
                    }
                    if (m_tcpIoService)
                        m_tcpIoService->post(boost::bind(&InputChannel::update, this));
                    else
                        update();
                }

                if (m_inputHandler) {
                    m_inputHandler(self);
                    // FIXME: Move call to this->update() from m_inputHandler to here!
                }
            } catch (const boost::bad_weak_ptr& e) {
                KARABO_LOG_FRAMEWORK_INFO << "\"triggerIOEvent\" call is too late: InputChannel destroyed already -- " << e.what();
            } catch (const std::exception& ex) {
                KARABO_LOG_FRAMEWORK_ERROR << "\"triggerIOEvent\" exception -- " << ex.what();
                //throw KARABO_SYSTEM_EXCEPTION(string("\"triggerIOEvent\" exception -- ") + ex.what());
            }
        }


        void InputChannel::triggerEndOfStreamEvent() {
            try {
                // set the guard: it is guaranteed that InputChannel object is alive
                // ... or exception brings us out here
                InputChannel::Pointer self = shared_from_this();

                if (m_endOfStreamHandler) {
                    m_endOfStreamHandler(self);
                }
            } catch (const boost::bad_weak_ptr& e) {
            } catch (const std::exception& ex) {
                KARABO_LOG_FRAMEWORK_ERROR << "\"triggerEndOfStreamEvent\" call is problematic -- " << ex.what();
                //throw KARABO_SYSTEM_EXCEPTION(string("\"triggerEndOfStreamEvent\" call is problematic -- ") + ex.what());
            }
        }


        void InputChannel::swapBuffers() {
            boost::mutex::scoped_lock lock(m_swapBuffersMutex);
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
            try {
                // set the guard: it is guaranteed that InputChannel object is alive
                // ... or exception brings us out here
                InputChannel::Pointer self = shared_from_this();

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
            } catch (const boost::bad_weak_ptr& e) {
            } catch (const std::exception& ex) {
                KARABO_LOG_FRAMEWORK_ERROR << "InputChannel::update exception -- " << ex.what();
                //throw KARABO_SYSTEM_EXCEPTION(ex.what());
            }
        }


        void InputChannel::deferredNotificationOfOutputChannelForPossibleRead(const karabo::net::Channel::Pointer& channel) {
            if (channel->isOpen()) {
                KARABO_LOG_FRAMEWORK_TRACE << "INPUT Notifying output channel that " << this->getInstanceId() << " is ready for next read.";
                channel->write(karabo::util::Hash("reason", "update", "instanceId", this->getInstanceId()));
            }
        }


        void InputChannel::notifyOutputChannelForPossibleRead(const karabo::net::Channel::Pointer& channel) {
            if (channel->isOpen()) {
                if (m_delayOnInput <= 0) // no delay
                    deferredNotificationOfOutputChannelForPossibleRead(channel);
                else {
                    m_deadline.expires_from_now(boost::posix_time::seconds(m_delayOnInput));
                    m_deadline.async_wait(boost::bind(&InputChannel::deferredNotificationOfOutputChannelForPossibleRead, this, channel));
                }
            }
        }


        void InputChannel::deferredNotificationsOfOutputChannelsForPossibleRead() {
            for (OpenConnections::const_iterator it = m_openConnections.begin(); it != m_openConnections.end(); ++it) {
                const karabo::net::Channel::Pointer& channel = it->second.second;
                if (channel->isOpen())
                    channel->write(karabo::util::Hash("reason", "update", "instanceId", this->getInstanceId()));
            }
        }


        void InputChannel::notifyOutputChannelsForPossibleRead() {
            if (m_delayOnInput <= 0) // no delay
                deferredNotificationsOfOutputChannelsForPossibleRead();
            else { // wait "asynchronously"
                m_deadline.expires_from_now(boost::posix_time::seconds(m_delayOnInput));
                m_deadline.async_wait(boost::bind(&InputChannel::deferredNotificationsOfOutputChannelsForPossibleRead, this));
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
                        boost::mutex::scoped_lock lock(m_outputChannelsMutex);
                        m_connectedOutputChannels.insert(std::make_pair(connectedOutputChannels[i], Hash()));
                    } else {
                        throw KARABO_PARAMETER_EXCEPTION("Illegal format for connected output channel, expecting <deviceId>:<channelName>");
                    }
                }
            }
        }


        void InputChannel::updateOutputChannelConfiguration(const std::string& outputChannelString, const karabo::util::Hash& config) {
            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            m_connectedOutputChannels[outputChannelString] = config;
        }


        bool InputChannel::needsDeviceConnection() const {
            return true;
        }

    }
}
