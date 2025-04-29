/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Modified to new concepts: April 17, 2015
 */

#include "InputChannel.hh"

#include <boost/system/error_code.hpp>
#include <chrono>

#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/MetaTools.hh"

using namespace std::chrono;
using namespace karabo::data;
using namespace karabo::data;
using namespace karabo::net;

using std::string;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


KARABO_REGISTER_FOR_CONFIGURATION(karabo::xms::InputChannel)

namespace karabo {
    namespace xms {


        const unsigned int InputChannel::DEFAULT_MAX_QUEUE_LENGTH = 2u;

        void InputChannel::expectedParameters(karabo::data::Schema& expected) {
            using namespace karabo::data;

            VECTOR_STRING_ELEMENT(expected)
                  .key("missingConnections")
                  .displayedName("Missing Connections")
                  .description("Output channels from 'Configured Connections' that are not connected")
                  .readOnly()
                  // no initial value: Cannot be set in static method here, Device will do so
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("connectedOutputChannels")
                  .displayedName("Configured Connections")
                  .description("Defines which output channels to connect to (use format: <instanceId>:<channelName>)")
                  .assignmentOptional()
                  .defaultValue(std::vector<std::string>())
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("dataDistribution")
                  .displayedName("Data Distribution")
                  .description("The way data is fetched from the connected output channels (shared/copy)")
                  .options(std::vector<std::string>({"copy", "shared"}))
                  .assignmentOptional()
                  .defaultValue("copy")
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("onSlowness")
                  .displayedName("On Slowness")
                  .description(
                        "Policy for what to do if this input is too slow for the fed data rate (only used in copy "
                        "mode), 'queue' means 'queueDrop'")
                  .options({"drop", "wait", "queue", "queueDrop"})
                  .assignmentOptional()
                  .defaultValue("drop")
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("maxQueueLength")
                  .displayedName("Max. Queue Length Output Channels")
                  .description(
                        "Maximum number of data items to be queued by connected Output Channels (only in copy mode and "
                        "for queue and queueDrop policies)")
                  .assignmentOptional()
                  .defaultValue(InputChannel::DEFAULT_MAX_QUEUE_LENGTH)
                  .minInc(1u)
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("minData")
                  .displayedName("Minimum number input packets")
                  .description(
                        "The number of elements to be read before any computation is started - not respected "
                        "for last data before endOfStream is received (0 = all until endOfStream)")
                  .assignmentOptional()
                  .defaultValue(1)
                  .init()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("respondToEndOfStream")
                  .displayedName("Respond to end-of-stream")
                  .description(
                        "Determines whether this input should forward a end-of-stream event to its parent device.")
                  .assignmentOptional()
                  .defaultValue(true)
                  .init()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("delayOnInput")
                  .displayedName("Delay on Input channel")
                  .description("Some delay before informing output channel about readiness for next data.")
                  .assignmentOptional()
                  .defaultValue(0)
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .init()
                  .commit();
        }


        InputChannel::InputChannel(const karabo::data::Hash& config)
            : m_strand(std::make_shared<Strand>(karabo::net::EventLoop::getIOService())),
              m_deadline(karabo::net::EventLoop::getIOService()),
              // "guaranteeToRun" = true to ensure handlers are all called under all circumstances
              m_connectStrand(Configurator<Strand>::create("Strand", Hash("guaranteeToRun", true))),
              m_respondToEndOfStream(true) {
            reconfigure(config, false);

            m_channelId = Memory::registerChannel();
            m_inactiveChunk = Memory::registerChunk(m_channelId);
            m_activeChunk = Memory::registerChunk(m_channelId);

            KARABO_LOG_FRAMEWORK_DEBUG << "Inputting on channel " << m_channelId << " (active) chunkId "
                                       << m_activeChunk << " and (inactive) chunkId " << m_inactiveChunk;
        }


        InputChannel::~InputChannel() {
            // Actively disonnect (i.e. close) all TcpChannels - as safety in case a shared_ptr is dangling somewhere
            disconnectAll();
            Memory::unregisterChannel(m_channelId);
            KARABO_LOG_FRAMEWORK_DEBUG << "*** InputChannel::~InputChannel DTOR for channelId = " << m_channelId;
        }


        void InputChannel::reconfigure(const karabo::data::Hash& config, bool allowMissing) {
            parseOutputChannelConfiguration(config);
            // If allowMissing is false, the lack of a key will throw an exception - that is on purpose!
            if (!allowMissing || config.has("dataDistribution")) config.get("dataDistribution", m_dataDistribution);
            if (!allowMissing || config.has("minData")) config.get("minData", m_minData);
            if (!allowMissing || config.has("onSlowness")) {
                config.get("onSlowness", m_onSlowness);
                if (m_onSlowness == "queue") m_onSlowness += "Drop";
            }
            if (!allowMissing || config.has("respondToEndOfStream"))
                config.get("respondToEndOfStream", m_respondToEndOfStream);
            if (!allowMissing || config.has("delayOnInput")) config.get("delayOnInput", m_delayOnInput);
            if (!allowMissing || config.has("maxQueueLength")) config.get("maxQueueLength", m_maxQueueLength);
        }


        void InputChannel::setInstanceId(const std::string& instanceId) {
            m_instanceId = instanceId;
        }


        const std::string& InputChannel::getInstanceId() const {
            return m_instanceId;
        }


        void InputChannel::registerInputHandler(const InputHandler& ioInputHandler) {
            m_inputHandler = ioInputHandler;
        }


        void InputChannel::registerDataHandler(const DataHandler& ioDataHandler) {
            m_dataHandler = ioDataHandler;
        }


        void InputChannel::registerEndOfStreamEventHandler(const InputHandler& endOfStreamEventHandler) {
            m_endOfStreamHandler = endOfStreamEventHandler;
        }


        void InputChannel::registerConnectionTracker(const ConnectionTracker& tracker) {
            m_connectionTracker = tracker;
        }


        InputChannel::Handlers InputChannel::getRegisteredHandlers() const {
            Handlers handlers(m_dataHandler, m_endOfStreamHandler);
            handlers.inputHandler = m_inputHandler;
            return handlers;
        }


        size_t InputChannel::dataQuantityRead() {
            std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
            size_t bytesRead = 0;
            for (auto it = m_openConnections.begin(); it != m_openConnections.end(); ++it) {
                bytesRead += it->second.second->dataQuantityRead();
            }
            return bytesRead;
        }

        size_t InputChannel::dataQuantityWritten() {
            std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
            size_t bytesWritten = 0;
            for (auto it = m_openConnections.begin(); it != m_openConnections.end(); ++it) {
                bytesWritten += it->second.second->dataQuantityWritten();
            }
            return bytesWritten;
        }

        std::map<std::string, karabo::data::Hash> InputChannel::getConnectedOutputChannels() {
            std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
            return m_configuredOutputChannels;
        }

        std::unordered_map<std::string, karabo::net::ConnectionStatus> InputChannel::getConnectionStatus() {
            std::unordered_map<std::string, karabo::net::ConnectionStatus> result;

            std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
            for (auto itChannel = m_configuredOutputChannels.begin(); itChannel != m_configuredOutputChannels.end();
                 ++itChannel) {
                const std::string& outputChannel = itChannel->first;
                auto itOpenConn = m_openConnections.find(outputChannel);
                if (itOpenConn != m_openConnections.end()) {
                    if (itOpenConn->second.second->isOpen()) {
                        result[outputChannel] = net::ConnectionStatus::CONNECTED;
                    } else { // How happen?
                        KARABO_LOG_FRAMEWORK_WARN << getInstanceId() << " - getConnectionStatus() finds connection to '"
                                                  << outputChannel << "' having its channel closed - erase.";
                        m_openConnections.erase(itOpenConn);
                        postConnectionTracker(outputChannel, net::ConnectionStatus::DISCONNECTED);
                        result[outputChannel] = net::ConnectionStatus::DISCONNECTED;
                    }
                } else if (m_connectionsBeingSetup.find(outputChannel) != m_connectionsBeingSetup.end()) {
                    result[outputChannel] = net::ConnectionStatus::CONNECTING;
                } else {
                    result[outputChannel] = net::ConnectionStatus::DISCONNECTED;
                }
            }

            return result;
        }


        const InputChannel::MetaData& InputChannel::read(karabo::data::Hash& data, size_t idx) {
            data = *(m_dataList[idx]); // This is a copy (except for NDArray bulk data)!
            return m_metaDataList[idx];
        }


        karabo::data::Hash::Pointer InputChannel::read(size_t idx) {
            return m_dataList[idx];
        }


        karabo::data::Hash::Pointer InputChannel::read(size_t idx, InputChannel::MetaData& metaData) {
            metaData = m_metaDataList[idx];
            return m_dataList[idx];
        }


        size_t InputChannel::size() {
            return m_dataList.size();
        }


        unsigned int InputChannel::getMinimumNumberOfData() const {
            return m_minData;
        }


        void InputChannel::connect(const karabo::data::Hash& outputChannelInfo,
                                   const std::function<void(const karabo::net::ErrorCode&)>& handler) {
            namespace bse = boost::system::errc;
            karabo::net::ErrorCode ec;
            const std::string& connectionType = outputChannelInfo.get<std::string>("connectionType");
            if (connectionType != "tcp") {
                KARABO_LOG_FRAMEWORK_ERROR << getInstanceId() << " does not support connection type '" << connectionType
                                           << "'";
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
                std::unique_lock<std::mutex> lock(m_outputChannelsMutex);
                if (m_configuredOutputChannels.find(outputChannelString) == m_configuredOutputChannels.end()) {
                    KARABO_LOG_FRAMEWORK_WARN << "InputChannel with id " << getInstanceId()
                                              << " not configured to connect to " << outputChannelString;
                    ec = bse::make_error_code(bse::argument_out_of_domain);
                } else {
                    auto openConnIt = m_openConnections.find(outputChannelString);
                    if (openConnIt != m_openConnections.end()) {
                        // Failure of https://git.xfel.eu/Karabo/Framework/-/jobs/865769
                        // maybe due to this?
                        const karabo::net::Channel::Pointer& channel = openConnIt->second.second;
                        // I see this happening also for channels that claim isOpen() where it is probably correct.
                        // But we know it is unreliable. So better try to connect as we were told.
                        KARABO_LOG_FRAMEWORK_WARN << "Connection attempt from '" << getInstanceId() << "' to '"
                                                  << outputChannelString << "' finds existing channel that claims to "
                                                  << "be " << (channel->isOpen() ? "" : "not ")
                                                  << "open - erase and retry anyway.";
                        m_openConnections.erase(openConnIt);
                        postConnectionTracker(outputChannelString, net::ConnectionStatus::DISCONNECTED);
                    }
                    if (m_connectionsBeingSetup.find(outputChannelString) != m_connectionsBeingSetup.end()) {
                        KARABO_LOG_FRAMEWORK_INFO << "InputChannel with id " << getInstanceId()
                                                  << " already in connection process to " << outputChannelString;
                        ec = bse::make_error_code(bse::connection_already_in_progress);
                    } else {
                        // Store a random number together with handler - this helps to distinguish whether a call to
                        // onConnect(..) was triggered by a connection attempt that was already cancelled or a following
                        // new connection attempt.
                        const unsigned int id = m_random(); // concurrency protection by lock(m_outputChannelsMutex)
                        m_connectionsBeingSetup[outputChannelString] = std::make_pair(id, handler);
                        KARABO_LOG_FRAMEWORK_DEBUG << "connect  on \"" << m_instanceId << "\" - create connection!";
                        postConnectionTracker(outputChannelString, net::ConnectionStatus::CONNECTING);
                        lock.unlock();
                        // Prepare connection configuration given output channel information
                        karabo::data::Hash config = prepareConnectionConfiguration(outputChannelInfo);
                        // Instantiate connection object
                        karabo::net::Connection::Pointer connection = karabo::net::Connection::create(config);

                        // Establish connection (and define sub-type of server)
                        // Notes:
                        // 1) To guarantee that handler is called even if 'this' is quickly destructed after the
                        //    call to connect(..), do not bind_weak(onConnect, this, ...), but implement the protection
                        //    that bind_weak offers in a wrapper that then calls the handler.
                        // 2) It may look fishy to bind 'connection' to a handler passed to 'connection' itself. Indeed,
                        //    this creates circular shared pointers, but we expect the handler to be called by boost
                        //    (be it with success or failure) and then to be dropped which solves the circularity again.
                        connection->startAsync(std::bind(&InputChannel::onConnectWrap, weak_from_this(), _1, connection,
                                                         outputChannelInfo, _2, id, handler));
                        return; // Do not call handler below
                    }
                }
            }

            // We are left here with a detected failure:
            if (handler) {
                m_connectStrand->post(std::bind(handler, ec));
            }
        }


        void InputChannel::disconnect(const karabo::data::Hash& outputChannelInfo) {
            if (outputChannelInfo.has("outputChannelString")) {
                disconnect(outputChannelInfo.get<std::string>("outputChannelString"));
                return;
            } else {
                // Not directly configured, so try host/port
                const std::string& hostname = outputChannelInfo.get<std::string>("hostname");
                unsigned int port = outputChannelInfo.get<unsigned int>("port");
                std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
                for (ConfiguredOutputChannels::const_iterator it = m_configuredOutputChannels.begin();
                     it != m_configuredOutputChannels.end(); ++it) {
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
            std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
            disconnectImpl(outputChannelString);
        }


        void InputChannel::disconnectImpl(const std::string& outputChannelString) {
            // If someone is still waiting for connection, tell that this was cancelled
            auto itBeingSetup = m_connectionsBeingSetup.find(outputChannelString);
            if (itBeingSetup != m_connectionsBeingSetup.end()) {
                const auto handler = std::move(itBeingSetup->second.second);
                m_connectionsBeingSetup.erase(itBeingSetup);
                if (handler) {
                    auto ec = boost::system::errc::make_error_code(boost::system::errc::operation_canceled);
                    m_connectStrand->post(std::bind(handler, ec));
                }
                postConnectionTracker(outputChannelString, net::ConnectionStatus::DISCONNECTED);
                return;
            }

            OpenConnections::iterator it = m_openConnections.find(outputChannelString);
            if (it == m_openConnections.end()) return; // see below

            KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << ": Disconnecting " << outputChannelString;
            it->second.second->close(); // Closes channel
            it->second.first->stop();
            m_openConnections.erase(it);
            postConnectionTracker(outputChannelString, net::ConnectionStatus::DISCONNECTED);
        }


        void InputChannel::disconnectAll() {
            std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
            std::vector<std::string> openConnections;
            for (OpenConnections::iterator it = m_openConnections.begin(); it != m_openConnections.end(); ++it) {
                openConnections.push_back(it->first);
            }
            // Cannot directly call disconnectImpl in above loop since it manipulates the map which is looped over
            for (const std::string& connection : openConnections) {
                disconnectImpl(connection);
            }
        }


        karabo::data::Hash InputChannel::prepareConnectionConfiguration(
              const karabo::data::Hash& outputChannelInfo) const {
            const std::string& hostname = outputChannelInfo.get<std::string>("hostname");
            const unsigned int& port = outputChannelInfo.get<unsigned int>("port");
            karabo::data::Hash h("Tcp",
                                 Hash("type", "client", "hostname", hostname, "port", port, "keepalive.enabled", true));
            return h;
        }


        void InputChannel::onConnectWrap(InputChannel::WeakPointer weakSelf, karabo::net::ErrorCode ec,
                                         karabo::net::Connection::Pointer connection,
                                         const karabo::data::Hash& outputChannelInfo,
                                         karabo::net::Channel::Pointer channel, unsigned int connectId,
                                         const std::function<void(const karabo::net::ErrorCode&)>& handler) {
            Pointer self = weakSelf.lock();
            if (self) {
                self->onConnect(ec, connection, outputChannelInfo, channel, connectId, handler);
            } else { // Already dead - but do not forget to call handler
                if (handler) {
                    handler(boost::system::errc::make_error_code(boost::system::errc::operation_canceled));
                }
            }
        }


        void InputChannel::onConnect(karabo::net::ErrorCode ec, karabo::net::Connection::Pointer& connection,
                                     const karabo::data::Hash& outputChannelInfo,
                                     karabo::net::Channel::Pointer& channel, unsigned int connectId,
                                     const std::function<void(const karabo::net::ErrorCode&)>& handler) {
            KARABO_LOG_FRAMEWORK_DEBUG << "onConnect  :  outputChannelInfo is ...\n" << outputChannelInfo;
            // NOTE: Locking this mutex here before a synchronous write is not ideal.
            //       However, we subscribe a read operation before a write to make sure the connection status
            //       is properly caught and there is no race condition. In case the handler is called asynchronously
            //       with an error code while we register the channel, connection pair in the `m_openConnections`
            //       set, it will wait for the mutex to be unlocked and properly deregister the channels.
            std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
            if (!ec) { // succeeded so far
                try {
                    channel->readAsyncHashVectorBufferSetPointer(
                          util::bind_weak(&karabo::xms::InputChannel::onTcpChannelRead, this, _1,
                                          net::Channel::WeakPointer(channel), _2, _3));
                    // synchronous write could throw if connection already broken again
                    channel->write(karabo::data::Hash(
                          "reason", "hello", "instanceId", this->getInstanceId(), "memoryLocation",
                          outputChannelInfo.get<std::string>("memoryLocation"), "dataDistribution", m_dataDistribution,
                          "onSlowness", m_onSlowness, "maxQueueLength", m_maxQueueLength)); // Say hello!
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_WARN << getInstanceId()
                                              << ": connecting failed while writing hello: " << e.what();
                    ec = boost::system::errc::make_error_code(boost::system::errc::operation_canceled);
                }
            }

            const string& outputChannelString = outputChannelInfo.get<string>("outputChannelString");
            auto itSetup = m_connectionsBeingSetup.find(outputChannelString);
            if (itSetup == m_connectionsBeingSetup.end()) {
                // TODO: In case of success so far, we should not have tried to write hello above!
                KARABO_LOG_FRAMEWORK_INFO << "onConnect for " << outputChannelString << ": No preparation found, "
                                          << "likely disconnected before, so cut connection.";
                // handler already posted with failure from disconnectImpl(..)
                return;
            } else if (itSetup->second.first != connectId) {
                KARABO_LOG_FRAMEWORK_INFO
                      << "onConnect for " << outputChannelString << ": Preparation found with "
                      << "wrong id, likely disconnected directly after connecting and then reconnected - reaching "
                      << "here from disconnected first attempt";
                // handler already posted with failure from disconnectImpl(..)
                return; // connection and channel will 'fade out'
            } else {
                m_connectionsBeingSetup.erase(itSetup);
            }

            if (!ec) {
                KARABO_LOG_FRAMEWORK_INFO << "'" << m_instanceId << "' connected to '" << outputChannelString << "'";
                m_openConnections[outputChannelString] = std::make_pair(connection, channel);
            }

            // Post handlers - with some bad luck, first data arrived before they are executed...
            // With some other bad luck, the output channel may not yet have processed (though received) our "hello"
            // and will thus not yet serve us immediately if one of the handlers instructs the output to send data.
            // Which order of handlers? InputOutputChannel_Test::testConnectDisconnect assumes first connection tracker
            auto status = (ec ? net::ConnectionStatus::DISCONNECTED : net::ConnectionStatus::CONNECTED);
            postConnectionTracker(outputChannelString, status);
            if (handler) {
                m_connectStrand->post(std::bind(handler, ec));
            }
        }


        void InputChannel::onTcpChannelError(const karabo::net::ErrorCode& error,
                                             const karabo::net::Channel::Pointer& channel) {
            bool runEndOfStream = false;
            {
                std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
                for (auto it = m_eosChannels.begin(); it != m_eosChannels.end();) {
                    net::Channel::Pointer ptr = it->lock();
                    if (!ptr // should not happen, but if it does clean up nevertheless
                        || ptr == channel) {
                        it = m_eosChannels.erase(it);
                    } else {
                        ++it;
                    }
                }
                // If the only output (of several) that did not yet provide endOfStream disconnects, trigger eos
                // handling
                runEndOfStream = (!m_openConnections.empty() && m_eosChannels.size() == m_openConnections.size());
            }

            if (runEndOfStream) {
                std::lock_guard<std::mutex> twoPotsLock(m_twoPotsMutex);
                // FIXME: When fixing the handling of multiple output channels, check that eos is really called
                //        (might not be the case when no more data arrives - as expected after EOS...)
                Memory::setEndOfStream(m_channelId, m_inactiveChunk, true);
            }

            if (!channel) {
                // Should never come here...
                KARABO_LOG_FRAMEWORK_WARN << "onTcpChannelError on '" << m_instanceId << "' called for empty channel "
                                          << "together with error code #" << error.value() << " -- '" << error.message()
                                          << "'.";
                return;
            }

            std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
            for (OpenConnections::iterator ii = m_openConnections.begin(); ii != m_openConnections.end(); ++ii) {
                if (ii->second.second == channel) {
                    const std::string outputChannelString = std::move(ii->first); // move: erase(ii) will invalidate
                    KARABO_LOG_FRAMEWORK_INFO << "onTcpChannelError on \"" << m_instanceId << "\"  connected to \""
                                              << outputChannelString << "\"  :  code #" << error.value() << " -- \""
                                              << error.message() << "\". Erase connection...";
                    m_openConnections.erase(ii);
                    // Better call m_connectionTracker last (which needs outputChannelString to be a copy, see above).
                    postConnectionTracker(outputChannelString, net::ConnectionStatus::DISCONNECTED);
                    return;
                }
            }

            // Should come here only if the hello message failed before the connection is registered in
            // `m_openConnections` on connect.
            auto connectionPtr = channel->getConnection();
            KARABO_LOG_FRAMEWORK_ERROR << "onTcpChannelError on \"" << m_instanceId
                                       << "\"  for untracked connection: " << "code #" << error.value() << " -- \""
                                       << error.message() << "\"" << ((connectionPtr) ? "." : ". Stop connection.");
            if (connectionPtr) connectionPtr->stop();
        }


        void InputChannel::onTcpChannelRead(const karabo::net::ErrorCode& ec, karabo::net::Channel::WeakPointer channel,
                                            const karabo::data::Hash& header,
                                            const std::vector<karabo::data::BufferSet::Pointer>& data) {
            net::Channel::Pointer channelPtr = channel.lock();
            if (ec || !channelPtr) {
                onTcpChannelError(ec, channelPtr);
                return;
            }

            // Trace helper (m_channelId is unique per process...):
            const std::string debugId((("INPUT " + data::toString(m_channelId) += " of '") += this->getInstanceId()) +=
                                      "' ");
            KARABO_LOG_FRAMEWORK_DEBUG << debugId << "ENTRY onTcpChannelRead  header is ...\n"
                                       << header << "\nand data.size=" << data.size();

            const std::string traceId("(" + boost::lexical_cast<std::string>(std::this_thread::get_id()) +
                                      ": onTcpChannelRead) ");

            try {
                // The twoPotsLock has to be here before potentially assigning treatEndOfStream = true although it
                // protects usually only m_[in]activeChunk: Otherwise, if receiving data from several output channels,
                // their 'onTcpChannelRead' can run in parallel and block at the lock. If these calls to
                // 'onTcpChannelRead' are the endOfStream messages, a problem would arise if then the endOfStream call
                // of the last output that sets treatEndOfStream = true overtakes  one of the earlier endOfStream calls.
                // Then the overtaken one will set Memory::setEndOfStream(.., false) again and the endOfStream handler
                // is not called. Unfortunately, that means we lock sometimes both, m_twoPotsMutex and
                // m_outputChannelsMutex, but that fortunately does not harm. Instead of the m_twoPotsMutex, one
                // might consider to post onTcpChannelRead on the same strand as triggerIOEvent - anyway almost all of
                // these functions is protected by that mutex...
                std::unique_lock<std::mutex> twoPotsLock(m_twoPotsMutex);
                bool treatEndOfStream = false;
                if (header.has("endOfStream")) {
                    std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
                    m_eosChannels.insert(channel);
                    if (m_eosChannels.size() < m_openConnections.size()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << debugId << "Received EOS #" << m_eosChannels.size() << ", await "
                                                   << m_openConnections.size() - m_eosChannels.size() << " more.";
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << debugId << "Received EOS #" << m_eosChannels.size()
                                                   << ", i.e. the last one.";
                        treatEndOfStream = true;
                    }
                }

                if (header.has("channelId") && header.has("chunkId")) {
                    // Local memory
                    unsigned int channelId = header.get<unsigned int>("channelId");
                    unsigned int chunkId = header.get<unsigned int>("chunkId");
                    KARABO_LOG_FRAMEWORK_TRACE << traceId << "Reading from local memory [" << channelId << "]["
                                               << chunkId << "]";
                    Memory::writeChunk(Memory::readChunk(channelId, chunkId), m_channelId, m_inactiveChunk,
                                       Memory::getMetaData(channelId, chunkId));
                    Memory::decrementChunkUsage(channelId, chunkId);
                } else { // TCP data
                    KARABO_LOG_FRAMEWORK_TRACE << traceId << "Reading from remote memory (over tcp)";
                    Memory::writeFromBuffers(data, header, m_channelId, m_inactiveChunk);
                }
                // Due to minData needs or multi-input, we may have a chunk marked as endOfStream that also contains
                // data!
                Memory::setEndOfStream(m_channelId, m_inactiveChunk, treatEndOfStream);

                if (this->getMinimumNumberOfData() == 0 &&
                    !treatEndOfStream) { // should keep reading until EOS (minData == 0 means that)
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
                        if (nActiveData == 0 &&
                            !Memory::isEndOfStream(m_channelId, m_activeChunk)) { // ...second pot still empty,...
                            KARABO_LOG_FRAMEWORK_TRACE
                                  << traceId << "At nActiveData == 0; will swap buffers for active and inactive data.";
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
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Problem in onTcpChannelRead (std::exception) : " << e.what();
            }
            // Continue reading, whatever happened...
            channelPtr->readAsyncHashVectorBufferSetPointer(
                  util::bind_weak(&karabo::xms::InputChannel::onTcpChannelRead, this, _1, channel, _2, _3));
        }


        /**
         * Prepares data and metadata from the active chunk.
         *
         * @details prepareData assumes that it has exclusive access to the m_activeChunk member variable when it's
         * called. Is up to the caller to guarantee that assumption.
         */
        void InputChannel::prepareData() {
            m_metaDataList = Memory::getMetaData(m_channelId, m_activeChunk);
            m_dataList.resize(m_metaDataList.size());

            m_sourceMap.clear();
            m_trainIdMap.clear();
            m_reverseMetaDataMap.clear();
            unsigned int i = 0;
            for (auto it = m_metaDataList.cbegin(); it != m_metaDataList.cend();) {
                m_dataList[i] = std::make_shared<Hash>();
                try {
                    Memory::read(*(m_dataList[i]), i, m_channelId, m_activeChunk);
                } catch (const karabo::data::Exception& e) {
                    // Simply log and skip bad data. Likely corrupt data that cannot be deserialised (How that?)
                    KARABO_LOG_FRAMEWORK_ERROR << "Failed to read (deserialize) a data item from " << it->getSource()
                                               << ", so skip it: " << e.userFriendlyMsg();
                    m_dataList[i].reset();
                    m_dataList.resize(m_dataList.size() - 1);
                    it = m_metaDataList.erase(it); // Not efficient on vector, but happens so rarely...
                    continue;
                }
                m_sourceMap.emplace(it->getSource(), i);
                m_trainIdMap.emplace(it->getTimestamp().getTrainId(), i);
                m_reverseMetaDataMap.emplace(i, *it);
                ++i;
                ++it;
            }
        }


        const std::vector<InputChannel::MetaData>& InputChannel::getMetaData() const {
            return m_metaDataList;
        }


        std::vector<unsigned int> InputChannel::sourceToIndices(const std::string& source) const {
            const std::pair<std::multimap<std::string, unsigned int>::const_iterator,
                            std::multimap<std::string, unsigned int>::const_iterator>
                  indices = m_sourceMap.equal_range(source);
            std::vector<unsigned int> ret;
            for (auto it = std::get<0>(indices); it != std::get<1>(indices); ++it) {
                ret.push_back(it->second);
            }
            return ret;
        }


        std::vector<unsigned int> InputChannel::trainIdToIndices(unsigned long long trainId) const {
            const std::pair<std::multimap<unsigned long long, unsigned int>::const_iterator,
                            std::multimap<unsigned long long, unsigned int>::const_iterator>
                  indices = m_trainIdMap.equal_range(trainId);
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
            bool notifyForNextRead = false;
            {
                std::lock_guard<std::mutex> twoPotsLock(m_twoPotsMutex);

                // Cache need for endOfStream handling since cleared in clearChunkData
                treatEndOfStream = Memory::isEndOfStream(m_channelId, m_activeChunk);

                // Prepare data and meta data structures from data in the active chunk
                prepareData();

                // Data is prepared, so prepare chunks for next round
                Memory::clearChunkData(m_channelId, m_activeChunk);

                // Swap buffers if so far inactive chunk is ready
                KARABO_LOG_FRAMEWORK_TRACE << getInstanceId() << " Will call swapBuffers after processing input";
                size_t nInactiveData = Memory::size(m_channelId, m_inactiveChunk);
                if (nInactiveData < this->getMinimumNumberOfData() &&
                    !Memory::isEndOfStream(m_channelId, m_inactiveChunk)) {
                    // Too early to process inactive Pot: has to reach minData
                    KARABO_LOG_FRAMEWORK_TRACE << getInstanceId()
                                               << " Too early to process inactive Pot: has to reach minData.";
                } else {
                    std::swap(m_activeChunk, m_inactiveChunk);
                    notifyForNextRead = true;
                }
            }

            // Run handlers outside lock of m_twoPotsMutex, but be prepared for exceptions from external code
            try {
                if (m_inputHandler && m_dataList.size() > 0) { // size could be zero if only endOfStream
                    KARABO_LOG_FRAMEWORK_TRACE << getInstanceId() << " Calling inputHandler";
                    m_inputHandler(shared_from_this());
                }
                if (m_dataHandler) {
                    KARABO_LOG_FRAMEWORK_TRACE << getInstanceId() << " Calling dataHandler: " << m_dataList.size()
                                               << " items";
                    for (size_t i = 0; i < m_dataList.size(); ++i) {
                        // Note: Optimisation in GuiServerDevice::onNetworkData will cast const away from its 'data'
                        // argument, apply std::move and (move-)assign it to another Hash to avoid copies of big
                        // vectors (the data of an NDArray is anyway not copied, but shared...).
                        // That is safe as long as after this loop nothing is done anymore with m_dataList.
                        m_dataHandler(*(m_dataList[i]), m_metaDataList[i]);
                    }
                }
                // endOfStream handling if required
                if (treatEndOfStream && m_endOfStreamHandler && m_respondToEndOfStream) {
                    m_endOfStreamHandler(shared_from_this());
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Exception from input/data/endOfStream handler for instance '"
                                           << m_instanceId << "': " << e.what();
            }
            // Clear cached data - though m_inputhandler might have stored one Hash::Pointer somewhere.
            m_dataList.clear();

            if (notifyForNextRead) {
                // After swapping the pots, the new active one is ready...
                m_strand->post(util::bind_weak(&InputChannel::triggerIOEvent, this));
                // ...and the other one can be filled
                notifyOutputChannelsForPossibleRead();
            }
        }


        void InputChannel::deferredNotificationOfOutputChannelForPossibleRead(
              const karabo::net::Channel::WeakPointer& channelW) {
            const net::Channel::Pointer channel = channelW.lock();
            if (channel && channel->isOpen()) {
                const std::string traceId("(" + boost::lexical_cast<std::string>(std::this_thread::get_id()) +
                                          ": deferredNotificationOfOutputChannel...) ");
                KARABO_LOG_FRAMEWORK_TRACE << traceId << "INPUT Notifying output channel that " << this->getInstanceId()
                                           << " is ready for next read.";
                // write can fail if disconnected in wrong moment - but then channel should be closed afterwards:
                try {
                    channel->write(karabo::data::Hash("reason", "update", "instanceId", this->getInstanceId()));
                } catch (const std::exception& e) {
                    if (channel->isOpen()) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Channel still open!"));
                    } else {
                        karabo::data::Exception::clearTrace();
                    }
                }
            }
        }


        void InputChannel::notifyOutputChannelForPossibleRead(const karabo::net::Channel::WeakPointer& channel) {
            if (m_delayOnInput <= 0) { // no delay
                deferredNotificationOfOutputChannelForPossibleRead(channel);
            } else {
                m_deadline.expires_after(milliseconds(m_delayOnInput));
                m_deadline.async_wait(util::bind_weak(&InputChannel::deferredNotificationOfOutputChannelForPossibleRead,
                                                      this, channel));
            }
        }


        void InputChannel::deferredNotificationsOfOutputChannelsForPossibleRead() {
            // Get channels to update under mutex lock - weak pointers ensure that channels are not kept alive by this
            std::vector<karabo::net::Channel::WeakPointer> channelsToUpdate;
            {
                std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
                channelsToUpdate.reserve(m_openConnections.size());
                for (OpenConnections::const_iterator it = m_openConnections.begin(); it != m_openConnections.end();
                     ++it) {
                    const karabo::net::Channel::Pointer& channel = it->second.second;
                    if (channel->isOpen()) {
                        channelsToUpdate.push_back(karabo::net::Channel::WeakPointer(channel));
                    }
                }
            }

            // Synchronous write without mutex lock
            std::string problems;
            for (const karabo::net::Channel::WeakPointer& wChannel : channelsToUpdate) {
                karabo::net::Channel::Pointer channel(wChannel.lock());
                if (!channel) continue;
                // write can fail if disconnected in wrong moment - but then channel should be closed afterwards:
                try {
                    channel->write(karabo::data::Hash("reason", "update", "instanceId", this->getInstanceId()));
                } catch (const std::exception& e) {
                    if (channel->isOpen()) {
                        // collect information propagate exception only after notifying all others
                        if (!problems.empty()) {
                            problems += "\n--- next bad channel: ---\n";
                        }
                        problems += e.what();
                    } else {
                        karabo::data::Exception::clearTrace();
                    }
                }
            }
            if (!problems.empty()) {
                throw KARABO_PROPAGATED_EXCEPTION("Channel(s) still open after write failed: " + problems);
            }
        }


        void InputChannel::notifyOutputChannelsForPossibleRead() {
            if (m_delayOnInput <= 0) // no delay
                deferredNotificationsOfOutputChannelsForPossibleRead();
            else { // wait "asynchronously"
                m_deadline.expires_after(milliseconds(m_delayOnInput));
                m_deadline.async_wait(
                      util::bind_weak(&InputChannel::deferredNotificationsOfOutputChannelsForPossibleRead, this));
            }
        }


        bool InputChannel::respondsToEndOfStream() {
            return m_respondToEndOfStream;
        }


        void InputChannel::parseOutputChannelConfiguration(const karabo::data::Hash& config) {
            if (config.has("connectedOutputChannels")) {
                std::vector<std::string> connectedOutputChannels;
                config.get("connectedOutputChannels", connectedOutputChannels);
                std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
                m_configuredOutputChannels.clear();
                for (size_t i = 0; i < connectedOutputChannels.size(); ++i) {
                    std::vector<std::string> tmp;
                    boost::split(tmp, connectedOutputChannels[i], boost::is_any_of(":"));
                    if (tmp.size() == 2) {
                        m_configuredOutputChannels[connectedOutputChannels[i]] = Hash();
                    } else {
                        throw KARABO_PARAMETER_EXCEPTION("Illegal format for connected output channel '" +
                                                         connectedOutputChannels[i] +
                                                         "', expecting <deviceId>:<channelName>");
                    }
                }
                for (auto it = m_openConnections.begin(); it != m_openConnections.end();) {
                    const std::string& outputChannelString = it->first;
                    if (std::find(connectedOutputChannels.begin(), connectedOutputChannels.end(),
                                  outputChannelString) == connectedOutputChannels.end()) {
                        // not anymore configured, so erase and thus stop
                        KARABO_LOG_FRAMEWORK_DEBUG << "Disconnecting '" << outputChannelString << "' in reconfigure";
                        it = m_openConnections.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }


        void InputChannel::postConnectionTracker(const std::string& outputChannel,
                                                 karabo::net::ConnectionStatus status) {
            if (m_connectionTracker) {
                m_connectStrand->post(std::bind(m_connectionTracker, outputChannel, status));
            }
        }


        void InputChannel::updateOutputChannelConfiguration(const std::string& outputChannelString,
                                                            const karabo::data::Hash& config) {
            std::lock_guard<std::mutex> lock(m_outputChannelsMutex);
            m_configuredOutputChannels[outputChannelString] = config;
        }


        void InputChannelElement::commit() {
            m_inputChannel.commit();
        }

    } // namespace xms
} // namespace karabo
