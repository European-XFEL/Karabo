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
#include "karabo/util/MetaTools.hh"

#include <boost/system/error_code.hpp>

using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::net;

using std::string;

namespace karabo {
    namespace xms {

        KARABO_REGISTER_FOR_CONFIGURATION(InputChannel);

        void InputChannel::expectedParameters(karabo::util::Schema& expected) {
            using namespace karabo::util;


            VECTOR_STRING_ELEMENT(expected).key("connectedOutputChannels")
                    .displayedName("Connected Output Channels")
                    .description("Defines the inter-device connectivity for pipeline data transfer (use format: <instanceId>:<channelName>)")
                    .assignmentOptional().defaultValue(std::vector<std::string>())
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
                    .options("drop,wait,queue,queueDrop")
                    .assignmentOptional().defaultValue("drop")
                    .init()
                    .commit();

            UINT32_ELEMENT(expected).key("minData")
                    .displayedName("Minimum number input packets")
                    .description("The number of elements to be read before any computation is started - not respected "
                                 "for last data before endOfStream is received (0 = all until endOfStream)")
                    .assignmentOptional().defaultValue(1)
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
            : m_strand(boost::make_shared<Strand>(karabo::net::EventLoop::getIOService()))
            , m_deadline(karabo::net::EventLoop::getIOService())
            , m_respondToEndOfStream(true) {
            reconfigure(config, false);

            m_channelId = Memory::registerChannel();
            m_inactiveChunk = Memory::registerChunk(m_channelId);
            m_activeChunk = Memory::registerChunk(m_channelId);

            KARABO_LOG_FRAMEWORK_DEBUG << "Inputting on channel " << m_channelId << " (active) chunkId " << m_activeChunk << " and (inactive) chunkId " << m_inactiveChunk;
        }



        InputChannel::~InputChannel() {
            closeChannelsAndStopConnections();
            Memory::unregisterChannel(m_channelId);
            KARABO_LOG_FRAMEWORK_DEBUG << "*** InputChannel::~InputChannel DTOR for channelId = " << m_channelId;
        }


        void InputChannel::reconfigure(const karabo::util::Hash& config, bool allowMissing) {
            parseOutputChannelConfiguration(config);
            // If allowMissing is false, the lack of a key will throw an exception - that is on purpose!
            if (!allowMissing || config.has("dataDistribution")) config.get("dataDistribution", m_dataDistribution);
            if (!allowMissing || config.has("minData")) config.get("minData", m_minData);
            if (!allowMissing || config.has("onSlowness")) config.get("onSlowness", m_onSlowness);
            if (!allowMissing || config.has("respondToEndOfStream")) config.get("respondToEndOfStream", m_respondToEndOfStream);
            if (!allowMissing || config.has("delayOnInput")) config.get("delayOnInput", m_delayOnInput);
        }


        void InputChannel::setInstanceId(const std::string& instanceId) {
            m_instanceId = instanceId;
        }


        const std::string& InputChannel::getInstanceId() const {
            return m_instanceId;
        }


        void InputChannel::registerInputHandler(const InputHandler& ioInputHandler) {
            if (m_dataHandler) {
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Clear "
                        << "data handler per Data since setting one per InputChannel";
                m_dataHandler.clear();
            }
            m_inputHandler = ioInputHandler;
        }


        void InputChannel::registerDataHandler(const DataHandler& ioDataHandler) {
            if (m_inputHandler) {
                KARABO_LOG_FRAMEWORK_WARN << this->getInstanceId() << ": Clear "
                        << "data handler per InputChannel since setting one per Data";
                m_inputHandler.clear();
            }

            m_dataHandler = ioDataHandler;
        }


        void InputChannel::registerEndOfStreamEventHandler(const InputHandler& endOfStreamEventHandler) {
            m_endOfStreamHandler = endOfStreamEventHandler;
        }

        size_t InputChannel::dataQuantityRead() {
            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            size_t bytesRead = 0;
            for (auto it = m_openConnections.begin(); it != m_openConnections.end(); ++it) {
                bytesRead += it->second.second->dataQuantityRead();
            }
            return bytesRead;
        }

        size_t InputChannel::dataQuantityWritten() {
            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            size_t bytesWritten = 0;
            for (auto it = m_openConnections.begin(); it != m_openConnections.end(); ++it) {
                bytesWritten += it->second.second->dataQuantityWritten();
            }
            return bytesWritten;
        }

        std::map<std::string, karabo::util::Hash> InputChannel::getConnectedOutputChannels() {
            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            return m_connectedOutputChannels;
        }


        const InputChannel::MetaData& InputChannel::read(karabo::util::Hash& data, size_t idx) {
            Memory::read(data, idx, m_channelId, m_activeChunk);
            return m_metaDataList[idx];
        }


        karabo::util::Hash::Pointer InputChannel::read(size_t idx) {
            karabo::util::Hash::Pointer hash(new karabo::util::Hash());
            Memory::read(*hash, idx, m_channelId, m_activeChunk);
            return hash;
        }


        karabo::util::Hash::Pointer InputChannel::read(size_t idx, InputChannel::MetaData& metaData) {
            auto hash = boost::make_shared<karabo::util::Hash>();
            Memory::read(*hash, idx, m_channelId, m_activeChunk);
            metaData = m_metaDataList[idx];
            return hash;
        }


        size_t InputChannel::size() {
            return Memory::size(m_channelId, m_activeChunk);
        }


        unsigned int InputChannel::getMinimumNumberOfData() const {
            return m_minData;
        }


        void InputChannel::connect(const karabo::util::Hash& outputChannelInfo,
                                   const boost::function<void (const karabo::net::ErrorCode&)>& handler) {
            connectImpl(outputChannelInfo, handler, false);
        }


        void InputChannel::connectImpl(const karabo::util::Hash& outputChannelInfo,
                                       const boost::function<void (const karabo::net::ErrorCode&)>& handler,
                                       bool forceNew) {
            namespace bse = boost::system::errc;
            KARABO_LOG_FRAMEWORK_DEBUG << "connect  on \"" << m_instanceId << "\"  :   outputChannelInfo is ...\n" << outputChannelInfo;

            karabo::net::ErrorCode ec;
            const std::string& connectionType = outputChannelInfo.get<std::string > ("connectionType");
            if (connectionType != "tcp") {
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << " does not support connection type '" << connectionType << "'";
                ec = bse::make_error_code(bse::protocol_not_supported);
            } else if (!outputChannelInfo.has("outputChannelString")) {
                // onConnect(..) will need it - bail out early
                KARABO_LOG_FRAMEWORK_WARN << "InputChannel with id " << getInstanceId()
                        << ": outputChannelInfo for connect(..) lacks key 'outputChannelString'";
                ec = bse::make_error_code(bse::invalid_argument);
            } else if (!outputChannelInfo.has("memoryLocation")) {
                // onConnect(..) will need it - bail out early
                KARABO_LOG_FRAMEWORK_WARN << "InputChannel with id " << getInstanceId()
                        << ": outputChannelInfo for connect(..) lacks key 'memoryLocation'";
                ec = bse::make_error_code(bse::argument_out_of_domain);
            } else {
                const string& outputChannelString = outputChannelInfo.get<string>("outputChannelString");
                boost::mutex::scoped_lock lock(m_outputChannelsMutex);
                if (m_connectedOutputChannels.find(outputChannelString) == m_connectedOutputChannels.end()) {
                    KARABO_LOG_FRAMEWORK_WARN << "InputChannel with id " << getInstanceId()
                            << " not configured to connect to " << outputChannelString;
                    ec = bse::make_error_code(bse::argument_out_of_domain);
                } else if (m_openConnections.find(outputChannelString) != m_openConnections.end()) {
                    // Can happen from interfering connect/reconnect of SignalSlotable if sender and receiver
                    // are instantiated concurrently.
                    KARABO_LOG_FRAMEWORK_INFO << "InputChannel with id " << getInstanceId()
                            << " already connected to " << outputChannelString;
                    ec = bse::make_error_code(bse::already_connected);
                } else {
                    auto it = m_connectionsBeingSetup.find(outputChannelString);
                    if (!forceNew && it != m_connectionsBeingSetup.end()) {
                        // Currently someone is already trying to connect us - let's just hijack that try
                        KARABO_LOG_FRAMEWORK_DEBUG << "connect  on \"" << m_instanceId << "\" - use previous try";
                        it->second.push(std::make_pair(outputChannelInfo, handler));
                    } else {
                        m_connectionsBeingSetup[outputChannelString]; // mark as being tried - for further tries later
                        KARABO_LOG_FRAMEWORK_DEBUG << "connect  on \"" << m_instanceId << "\" - create connection!";

                        // Prepare connection configuration given output channel information
                        karabo::util::Hash config = prepareConnectionConfiguration(outputChannelInfo);
                        // Instantiate connection object
                        karabo::net::Connection::Pointer connection = karabo::net::Connection::create(config);

                        // Establish connection (and define sub-type of server)
                        connection->startAsync(karabo::util::bind_weak(&InputChannel::onConnect, this, _1, connection,
                                                                       outputChannelInfo, _2, handler));
                    }
                    return; // avoid calling handler that is otherwise posted below
                }
            }

            // OK, end up here in error condition only - call handler (if given) with failure code
            if (handler) {
                EventLoop::getIOService().post(boost::bind(handler, ec));
            }
        }


        void InputChannel::disconnect(const karabo::util::Hash& outputChannelInfo) {

            if (outputChannelInfo.has("outputChannelString")) {
                disconnect(outputChannelInfo.get<std::string>("outputChannelString"));
                return;
            } else {
                // Not directly configured, so try host/port
                const std::string& hostname = outputChannelInfo.get<std::string > ("hostname");
                unsigned int port = outputChannelInfo.get<unsigned int>("port");
                boost::mutex::scoped_lock lock(m_outputChannelsMutex);
                for (ConnectedOutputChannels::const_iterator it = m_connectedOutputChannels.begin(); it != m_connectedOutputChannels.end(); ++it) {
                    if (it->second.get<string>("hostname") != hostname) continue;
                    if (it->second.get<unsigned int>("port") != port) continue;
                    disconnectImpl(it->first);
                    return;
                }
            }

            KARABO_LOG_FRAMEWORK_ERROR << "disconnect: input neither defines outputChannelString nor hostname/port "
                    << "matching any active connection.";
        }


        void InputChannel::disconnect(const std::string& outputChannelString) {
            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            disconnectImpl(outputChannelString);
        }


        void InputChannel::disconnectImpl(std::string outputChannelString) {
            // For safety, take 'outputChannelString' as a value and not as a const reference:
            // It might come from an iterator looping over m_openConnections - then it would be invalid
            // after m_openConnections.erase(it) below

            // If someone is still waiting for connection, tell that this was canceled
            auto itBeingSetup = m_connectionsBeingSetup.find(outputChannelString);
            if (itBeingSetup != m_connectionsBeingSetup.end()) {
                ConnectionQueue& pending = itBeingSetup->second;
                while (!pending.empty()) {
                    const auto& handler = pending.front().second;
                    if (handler) handler(boost::system::errc::make_error_code(boost::system::errc::operation_canceled));
                    pending.pop();
                }
                m_connectionsBeingSetup.erase(itBeingSetup);
            }

            OpenConnections::iterator it = m_openConnections.find(outputChannelString);
            if (it == m_openConnections.end()) return; // see below

            KARABO_LOG_FRAMEWORK_DEBUG << "Disconnecting... " << outputChannelString;
            it->second.second->close(); // Closes channel
            it->second.first->stop();
            m_openConnections.erase(it);

            // Also clean stored info in m_connectedOutputChannels - but we keep the keys stable to the
            // initially configured list of (to be [!]) connected output channels.
            // (Note: Only things in m_connectedOutputChannels can get into m_openConnections, so operator[] is safe.)
            m_connectedOutputChannels[outputChannelString].clear();
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


        void InputChannel::onConnect(karabo::net::ErrorCode ec,
                                     karabo::net::Connection::Pointer connection,
                                     const karabo::util::Hash& outputChannelInfo,
                                     karabo::net::Channel::Pointer channel,
                                     const boost::function<void (const karabo::net::ErrorCode&)>& handler) {

            KARABO_LOG_FRAMEWORK_DEBUG << "onConnect  :  outputChannelInfo is ...\n" << outputChannelInfo;
            if (!ec) { // succeeded so far
                try {
                    // synchronous write could throw if connection already broken again
                    channel->write(karabo::util::Hash("reason", "hello", "instanceId", this->getInstanceId(),
                                                      "memoryLocation", outputChannelInfo.get<std::string > ("memoryLocation"),
                                                      "dataDistribution", m_dataDistribution,
                                                      "onSlowness", m_onSlowness)); // Say hello!
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_WARN << getInstanceId() << ": connecting failed while writing hello: " << e.what();
                    ec = boost::system::errc::make_error_code(boost::system::errc::operation_canceled);
                }
            }

            const string& outputChannelString = outputChannelInfo.get<string>("outputChannelString");
            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            auto itNextTries = m_connectionsBeingSetup.find(outputChannelString);
            if (itNextTries == m_connectionsBeingSetup.end()) {
                // TODO: In case of success so far, we should not have tried to write hello above!
                KARABO_LOG_FRAMEWORK_INFO << "onConnect for " << outputChannelString << ": No preparation found, "
                        << "likely disconnected before, so cut connection.";
                lock.unlock();
                if (handler) {
                    if (!ec) ec = boost::system::errc::make_error_code(boost::system::errc::operation_canceled);
                    handler(ec);
                }
                return;
            }
            if (ec) { // failed
                ConnectionQueue& nextTries = itNextTries->second;
                if (!nextTries.empty()) {
                    KARABO_LOG_FRAMEWORK_WARN << "Connecting to " << outputChannelString << " failed (" << ec.message()
                            << "), attempt to connect once more";
                    // Try again with more recent outputChannelInfo
                    // (retrieve pair of Hash and handler from queue without copy since pop()-ed from queue anyway)
                    const ConnectionQueue::value_type outInfo_handler_pair(std::move(nextTries.front()));
                    nextTries.pop();
                    lock.unlock();
                    // forceNew to 'true' avoids adding to m_connectionsBeingSetup again:
                    connectImpl(outInfo_handler_pair.first, outInfo_handler_pair.second, true);
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "Connecting to " << outputChannelString << " failed finally ("
                            << ec.message() << ")";
                    // Just unmark as being tried (invalidates 'nextTries'!)
                    m_connectionsBeingSetup.erase(outputChannelString);
                    lock.unlock();
                }
                // Call handler if given - but without mutex lock as unlock()-ed above
                if (handler) handler(ec);
                return;
            }

            channel->readAsyncHashVectorBufferSetPointer(util::bind_weak(&karabo::xms::InputChannel::onTcpChannelRead, this,
                                                                         _1, net::Channel::WeakPointer(channel), _2, _3));

            m_openConnections[outputChannelString] = std::make_pair(connection, channel);

            // Call handler(s) if given - but without mutex lock, so move from m_connectionsBeingSetup all other tries
            ConnectionQueue otherTries(std::move(m_connectionsBeingSetup[outputChannelString]));
            m_connectionsBeingSetup.erase(outputChannelString);
            lock.unlock();
            KARABO_LOG_FRAMEWORK_DEBUG << "Call " << 1u + otherTries.size() << " handler(s) for success " << ec;
            if (handler) handler(ec);
            while (!otherTries.empty()) {
                const auto& otherHandler = otherTries.front().second;
                if (otherHandler) otherHandler(ec);
                otherTries.pop();
            }
        }


        void InputChannel::onTcpChannelError(const karabo::net::ErrorCode& error, const karabo::net::Channel::Pointer& channel) {

            bool runEndOfStream = false;
            {
                boost::mutex::scoped_lock lock(m_outputChannelsMutex);
                for (auto it = m_eosChannels.begin(); it != m_eosChannels.end(); ) {
                    net::Channel::Pointer ptr = it->lock();
                    if (!ptr // should not happen, but if it does clean up nevertheless
                        || ptr == channel) {
                        it = m_eosChannels.erase(it);
                    } else {
                        ++it;
		    }
                }
                // If the only output (of several) that did not yet provide endOfStream disconnects, trigger eos handling
                runEndOfStream = (!m_openConnections.empty() && m_eosChannels.size() == m_openConnections.size());
            }

            if (runEndOfStream) {
                boost::mutex::scoped_lock twoPotsLock(m_twoPotsMutex);
                // FIXME: When fixing the handling of multiple output channels, check that eos is really called
                //        (might not be the case when no more data arrives - as expected after EOS...)
                Memory::setEndOfStream(m_channelId, m_inactiveChunk, true);
            }

            if (!channel) {
                // Should never come here...
                KARABO_LOG_FRAMEWORK_WARN << "onTcpChannelError on '" << m_instanceId << "' called for empty channel "
                        << "together with error code #" << error.value() << " -- '" << error.message() << "'.";
                return;
            }

            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            for (OpenConnections::iterator ii = m_openConnections.begin(); ii != m_openConnections.end(); ++ii) {
                if (ii->second.second == channel) {
                    const std::string& outputChannelString = ii->first;
                    KARABO_LOG_FRAMEWORK_INFO << "onTcpChannelError on \"" << m_instanceId << "\"  connected to \""
                            << outputChannelString << "\"  :  code #" << error.value() << " -- \"" << error.message() << "\".  Disconnect.";
                    disconnectImpl(outputChannelString);
                    return;
                }
            }

            // Should never come here:
            KARABO_LOG_FRAMEWORK_ERROR << "onTcpChannelError on \"" << m_instanceId << "\"  for untracked connection: "
                    << "code #" << error.value() << " -- \"" << error.message() << "\".  Stop connection.";
            channel->getConnection()->stop();
        }


        void InputChannel::onTcpChannelRead(const karabo::net::ErrorCode& ec,
                                            karabo::net::Channel::WeakPointer channel,
                                            const karabo::util::Hash& header,
                                            const std::vector<karabo::io::BufferSet::Pointer>& data) {
            net::Channel::Pointer channelPtr = channel.lock();
            if (ec || !channelPtr) {
                onTcpChannelError(ec, channelPtr);
                return;
            }

            // Trace helper (m_channelId is unique per process...):
            const std::string debugId((("INPUT " + util::toString(m_channelId) += " of '") += this->getInstanceId()) += "' ");
            KARABO_LOG_FRAMEWORK_DEBUG << debugId << "ENTRY onTcpChannelRead  header is ...\n" << header << "\nand data.size=" << data.size();

            const std::string traceId("(" + boost::lexical_cast<std::string>(boost::this_thread::get_id()) + ": onTcpChannelRead) ");

            try {
                bool treatEndOfStream = false;
                if (header.has("endOfStream")) {
                    boost::mutex::scoped_lock lock(m_outputChannelsMutex);
                    m_eosChannels.insert(channel);
                    if (m_eosChannels.size() < m_openConnections.size()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << debugId << "Received EOS #" << m_eosChannels.size() << ", await "
                                << m_openConnections.size() - m_eosChannels.size() << " more.";
                    } else {
                        treatEndOfStream = true;
                    }
                }

                boost::mutex::scoped_lock twoPotsLock(m_twoPotsMutex);

                if (header.has("channelId") && header.has("chunkId")) {
                    // Local memory
                    unsigned int channelId = header.get<unsigned int>("channelId");
                    unsigned int chunkId = header.get<unsigned int>("chunkId");
                    KARABO_LOG_FRAMEWORK_TRACE << traceId << "Reading from local memory [" << channelId << "][" << chunkId << "]";
                    Memory::writeChunk(Memory::readChunk(channelId, chunkId), m_channelId, m_inactiveChunk, Memory::getMetaData(channelId, chunkId));
                    Memory::decrementChunkUsage(channelId, chunkId);
                } else { // TCP data
                    KARABO_LOG_FRAMEWORK_TRACE << traceId << "Reading from remote memory (over tcp)";
                    Memory::writeFromBuffers(data, header, m_channelId, m_inactiveChunk);
                }
                // Due to minData needs or multi-input, we may have a chunk marked as endOfStream that also contains data!
                Memory::setEndOfStream(m_channelId, m_inactiveChunk, treatEndOfStream);

                if (this->getMinimumNumberOfData() == 0 && !treatEndOfStream) { // should keep reading until EOS (minData == 0 means that)
                    KARABO_LOG_FRAMEWORK_TRACE << traceId << "Can read more data since 'all' requested";
                    twoPotsLock.unlock();
                    notifyOutputChannelForPossibleRead(channel);
                } else {
                    size_t nInactiveData = Memory::size(m_channelId, m_inactiveChunk);
                    if (nInactiveData < this->getMinimumNumberOfData() && !treatEndOfStream) {
                        // requests more data
                        twoPotsLock.unlock();
                        notifyOutputChannelForPossibleRead(channel);
                    } else {
                        // Data complete,...
                        size_t nActiveData = Memory::size(m_channelId, m_activeChunk);
                        if (nActiveData == 0 && !Memory::isEndOfStream(m_channelId, m_activeChunk)) { // ...second pot still empty,...
                            KARABO_LOG_FRAMEWORK_TRACE << traceId << "At nActiveData == 0; will swap buffers for active and inactive data.";
                            std::swap(m_activeChunk, m_inactiveChunk);
                            twoPotsLock.unlock(); // before synchronous tcp write in notifyOutputChannelForPossibleRead
                            // ...so process first one and...
                            m_strand->post(util::bind_weak(&InputChannel::triggerIOEvent, this));
                            // ...request more data (after posting triggerIOEvent in case tcp write throws)
                            notifyOutputChannelForPossibleRead(channel);
                            KARABO_LOG_FRAMEWORK_TRACE << traceId << "Triggering IOEvent";
                        } else { // Data complete on both pots now
                            // triggerIOEvent will be called by the update of the triggerIOEvent
                            // that is processing the active pot now
                            KARABO_LOG_FRAMEWORK_DEBUG << "Do not trigger IOEvent with pot sizes " << nActiveData << "/"
                                    << nInactiveData;
                        }
                    }
                }
                channelPtr->readAsyncHashVectorBufferSetPointer(util::bind_weak(&karabo::xms::InputChannel::onTcpChannelRead, this,
                                                                                _1, channel, _2, _3));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onTcpChannelRead (std::exception) : " << e.what();
            }

        }


        /**
         * Prepares metadata for the active chunk.
         *
         * @details prepareMetaData assumes that it has exclusive access to the m_activeChunk member variable when it's
         * called. Is up to the caller to guarantee that assumption.
         */
        void InputChannel::prepareMetaData() {

            m_metaDataList = Memory::getMetaData(m_channelId, m_activeChunk);

            m_sourceMap.clear();
            m_trainIdMap.clear();
            m_reverseMetaDataMap.clear();
            unsigned int i = 0;
            for (auto it = m_metaDataList.cbegin(); it != m_metaDataList.cend(); ++it) {
                m_sourceMap.emplace(it->getSource(), i);
                m_trainIdMap.emplace(it->getTimestamp().getTrainId(), i);
                m_reverseMetaDataMap.emplace(i, *it);
                i++;
            }
        }


        const std::vector<InputChannel::MetaData>& InputChannel::getMetaData() const {
            return m_metaDataList;
        }


        std::vector<unsigned int> InputChannel::sourceToIndices(const std::string& source) const {
            const std::pair<std::multimap<std::string, unsigned int>::const_iterator, std::multimap<std::string, unsigned int>::const_iterator> indices = m_sourceMap.equal_range(source);
            std::vector<unsigned int> ret;
            for (auto it = std::get<0>(indices); it != std::get<1>(indices); ++it) {
                ret.push_back(it->second);
            }
            return ret;
        }


        std::vector<unsigned int> InputChannel::trainIdToIndices(unsigned long long trainId) const {
            const std::pair<std::multimap<unsigned long long, unsigned int>::const_iterator, std::multimap<unsigned long long, unsigned int>::const_iterator> indices = m_trainIdMap.equal_range(trainId);
            std::vector<unsigned int> ret;
            for (auto it = std::get<0>(indices); it != std::get<1>(indices); ++it) {
                ret.push_back(it->second);
            }
            return ret;
        }


        const InputChannel::MetaData& InputChannel::indexToMetaData(unsigned int index) const {
            auto it = m_reverseMetaDataMap.find(index);
            if (it != m_reverseMetaDataMap.end()) {
                return it->second;
            } else {
                throw KARABO_LOGIC_EXCEPTION("No meta data available for given index");
            }
            
        }


        void InputChannel::triggerIOEvent() {

            bool treatEndOfStream = false;
            {
                boost::mutex::scoped_lock twoPotsLock(m_twoPotsMutex);
                const std::string traceId("(" + boost::lexical_cast<std::string>(boost::this_thread::get_id()) + ": triggerIOEvent) ");

                // Cache need for endOfStream handling since cleared in clearChunkData
                treatEndOfStream = Memory::isEndOfStream(m_channelId, m_activeChunk);
                try {
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
                    KARABO_LOG_FRAMEWORK_TRACE << traceId << "Will prepare Metadata";
                    
                    prepareMetaData(); 
                    if (m_dataHandler) {
                        Hash data;
                        for (size_t i = 0; i < this->size(); ++i) {
                            read(data, i); // clears data internally before filling TODO: doc that all read(..) need 'active' mutex)
                            // Note: Optimisation in GuiServerDevice::onNetworkData will cast const away from its 'data'
                            // argument, apply std::move and (move-)assign it to another Hash to avoid copies of big
                            // vectors (the data of an NDArray is anyway not copied, but shared...).
                            // That is safe as long as this loop does nothing with 'data' after calling the data handler.
                            m_dataHandler(data, m_metaDataList[i]);
                        }
                    } else if (m_inputHandler && size() > 0) { // size could be zero if only endOfStream
                        KARABO_LOG_FRAMEWORK_TRACE << traceId << "Calling inputHandler";
                        m_inputHandler(shared_from_this());
                    }
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Exception in 'triggerIOEvent' for instance '"
                            << m_instanceId << "': " << e.what();
                }

                // Whatever handlers (even none or one that throws): we are done with the data.
                try {
                    // Clear active chunk
                    Memory::clearChunkData(m_channelId, m_activeChunk);

                    // Swap buffers
                    KARABO_LOG_FRAMEWORK_TRACE << traceId << "Will call swapBuffers after processing input";
                    size_t nInactiveData = Memory::size(m_channelId, m_inactiveChunk);
                    if (nInactiveData < this->getMinimumNumberOfData()
                        && !Memory::isEndOfStream(m_channelId, m_inactiveChunk)) {
                        // Too early to process inactive Pot: has to reach minData
                        KARABO_LOG_FRAMEWORK_TRACE << traceId << "Too early to process inactive Pot: has to reach minData.";
                    } else {
                            std::swap(m_activeChunk, m_inactiveChunk);

                            // After swapping the pots, the new active one is ready...
                            m_strand->post(util::bind_weak(&InputChannel::triggerIOEvent, this));
                            // ...and the other one can be filled
                            twoPotsLock.unlock(); // before synchronous write to Tcp
                            notifyOutputChannelsForPossibleRead();
                    }
                } catch (const std::exception& ex) {
                    KARABO_LOG_FRAMEWORK_ERROR << "InputChannel::update exception -- " << ex.what();
                }
            }

            // endOfStream handling if required - but after requesting more data and releasing m_twoPotsMutex
            if (treatEndOfStream && m_endOfStreamHandler && m_respondToEndOfStream) {
                try {
                    m_endOfStreamHandler(shared_from_this());
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Exception from endOfStream handler for instance '"
                            << m_instanceId << "': " << e.what();
                }
            }
        }


        void InputChannel::deferredNotificationOfOutputChannelForPossibleRead(const karabo::net::Channel::WeakPointer& channelW) {
            const net::Channel::Pointer channel = channelW.lock();
            if (channel && channel->isOpen()) {
                const std::string traceId("(" + boost::lexical_cast<std::string>(boost::this_thread::get_id()) + ": deferredNotificationOfOutputChannel...) ");
                KARABO_LOG_FRAMEWORK_TRACE << traceId << "INPUT Notifying output channel that " << this->getInstanceId() << " is ready for next read.";
                // write can fail if disconnected in wrong moment - but then channel should be closed afterwards:
                try {
                    channel->write(karabo::util::Hash("reason", "update", "instanceId", this->getInstanceId()));
                } catch (const std::exception& e) {
                    if (channel->isOpen()) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Channel still open!"));
                    } else {
                        karabo::util::Exception::clearTrace();
                    }
                }
            }
        }


        void InputChannel::notifyOutputChannelForPossibleRead(const karabo::net::Channel::WeakPointer& channel) {
            if (m_delayOnInput <= 0) { // no delay
                deferredNotificationOfOutputChannelForPossibleRead(channel);
            } else {
                m_deadline.expires_from_now(boost::posix_time::milliseconds(m_delayOnInput));
                m_deadline.async_wait(util::bind_weak(&InputChannel::deferredNotificationOfOutputChannelForPossibleRead, this, channel));
            }
        }


        void InputChannel::deferredNotificationsOfOutputChannelsForPossibleRead() {
            std::string problems;
            for (OpenConnections::const_iterator it = m_openConnections.begin(); it != m_openConnections.end(); ++it) {
                const karabo::net::Channel::Pointer& channel = it->second.second;
                if (!channel->isOpen()) continue;

                // write can fail if disconnected in wrong moment - but then channel should be closed afterwards:
                try {
                    channel->write(karabo::util::Hash("reason", "update", "instanceId", this->getInstanceId()));
                } catch (const std::exception& e) {
                    if (channel->isOpen()) {
                        // collect information propagate exception only after notifying all others
                        if (!problems.empty()) {
                            problems += "\n--- next bad channel: ---\n";
                        }
                        problems += e.what();
                    } else {
                        karabo::util::Exception::clearTrace();
                    }
                }
            }
            if (!problems.empty()) {
                throw KARABO_PROPAGATED_EXCEPTION("Channel(s) still open after write failed: "
                                                  + problems);
            }
        }


        void InputChannel::notifyOutputChannelsForPossibleRead() {
            if (m_delayOnInput <= 0) // no delay
                deferredNotificationsOfOutputChannelsForPossibleRead();
            else { // wait "asynchronously"
                m_deadline.expires_from_now(boost::posix_time::milliseconds(m_delayOnInput));
                m_deadline.async_wait(util::bind_weak(&InputChannel::deferredNotificationsOfOutputChannelsForPossibleRead, this));
            }
        }


        bool InputChannel::respondsToEndOfStream() {
            return m_respondToEndOfStream;
        }


        void InputChannel::parseOutputChannelConfiguration(const karabo::util::Hash& config) {
            if (config.has("connectedOutputChannels")) {
                std::vector<std::string> connectedOutputChannels;
                config.get("connectedOutputChannels", connectedOutputChannels);
                boost::mutex::scoped_lock lock(m_outputChannelsMutex);
                m_connectedOutputChannels.clear();
                for (size_t i = 0; i < connectedOutputChannels.size(); ++i) {
                    std::vector<std::string> tmp;
                    boost::split(tmp, connectedOutputChannels[i], boost::is_any_of("@:"));
                    if (tmp.size() == 2) {
                        m_connectedOutputChannels[connectedOutputChannels[i]] = Hash();
                    } else {
                        throw KARABO_PARAMETER_EXCEPTION("Illegal format for connected output channel '"
                                                         + toString(connectedOutputChannels[i]) += "', expecting <deviceId>:<channelName>");
                    }
                }
                for (auto it = m_openConnections.begin(); it != m_openConnections.end();) {
                    const std::string& outputChannelString = it->first;
                    if (std::find(connectedOutputChannels.begin(), connectedOutputChannels.end(), outputChannelString)
                        == connectedOutputChannels.end()) {
                        // not anymore configured, so erase and thus stop
                        KARABO_LOG_FRAMEWORK_DEBUG << "Disconnecting '" << outputChannelString << "' in reconfigure";
                        it = m_openConnections.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }


        void InputChannel::updateOutputChannelConfiguration(const std::string& outputChannelString, const karabo::util::Hash& config) {
            boost::mutex::scoped_lock lock(m_outputChannelsMutex);
            m_connectedOutputChannels[outputChannelString] = config;
        }

    }
}
