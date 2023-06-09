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
 */

#include "OutputChannel.hh"

#include <boost/pointer_cast.hpp>
#include <exception>

#include "InputChannel.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/TcpChannel.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/TableElement.hh"
#include "karabo/util/VectorElement.hh"

namespace bs = boost::system;
using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::net;
using boost::placeholders::_1;
using boost::placeholders::_2;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::xms::OutputChannel)
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, karabo::xms::OutputChannel);

namespace karabo {
    namespace xms {


        // Number of attempts for delayed 2nd construction phase, i.e. initializeServerConnection().
        // Seen it fail 1500 times (in a very busy host and C++ server) before adding the sleep in
        // initializeServerConnection(), so put 2000 here.
        const int kMaxServerInitializationAttempts = 2000;

        void OutputChannel::expectedParameters(karabo::util::Schema& expected) {
            using namespace karabo::util;

            STRING_ELEMENT(expected)
                  .key("distributionMode")
                  .displayedName("Distribution Mode")
                  .description("Describes the policy of how to fan-out data to multiple (shared) input channels")
                  .options(std::vector<std::string>({"load-balanced", "round-robin"}))
                  .assignmentOptional()
                  .defaultValue("load-balanced")
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("noInputShared")
                  .displayedName("No Input (Shared)")
                  .description("What to do if currently none of the share-input channels is available for writing to")
                  .options({"drop", "queue", "queueDrop", "wait"})
                  .assignmentOptional()
                  .defaultValue("drop")
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("hostname")
                  .displayedName("Hostname")
                  .description(
                        "The requested hostname to which connecting clients will be "
                        "routed to. Classless Inter-Domain Routing specification and "
                        "a default string are accepted as well.")
                  .assignmentOptional()
                  .defaultValue("default")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("address")
                  .displayedName("Address")
                  .description("The hostname to which connecting clients will be routed to.")
                  .readOnly()
                  .initialValue(std::string())
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("port")
                  .displayedName("Port")
                  .description("Port number for TCP connection")
                  .expertAccess()
                  .assignmentOptional()
                  .defaultValue(0)
                  .maxInc(65535) // ports are 16-bit
                  .init()
                  .commit();

            Schema columns;

            STRING_ELEMENT(columns)
                  .key("remoteId")
                  .displayedName("Remote ID")
                  .description("Id of remote input channel")
                  .readOnly()
                  .commit();

            STRING_ELEMENT(columns)
                  .key("dataDistribution")
                  .displayedName("Distribution")
                  .description("Data distribution behavior by input channel: shared or copy")
                  .readOnly()
                  .commit();

            STRING_ELEMENT(columns)
                  .key("onSlowness")
                  .displayedName("On slowness")
                  .description(
                        "Data handling policy in case of slowness if data Distribution is copy: "
                        "drop, wait, queue, queueDrop")
                  .readOnly()
                  .commit();

            STRING_ELEMENT(columns)
                  .key("memoryLocation")
                  .displayedName("MemoryLocation")
                  .description("Cache Memory class location: can be remote or local")
                  .readOnly()
                  .commit();

            STRING_ELEMENT(columns)
                  .key("remoteAddress")
                  .displayedName("Remote IP")
                  .description("Remote TCP address of active connection")
                  .readOnly()
                  .commit();

            UINT16_ELEMENT(columns)
                  .key("remotePort")
                  .displayedName("Remote port")
                  .description("Remote TCP port of active connection")
                  .readOnly()
                  .commit();

            STRING_ELEMENT(columns)
                  .key("localAddress")
                  .displayedName("Local IP")
                  .description("Local TCP address of active connection")
                  .readOnly()
                  .commit();

            UINT16_ELEMENT(columns)
                  .key("localPort")
                  .displayedName("Local port")
                  .description("Local TCP port of active connection")
                  .readOnly()
                  .commit();

            TABLE_ELEMENT(expected)
                  .key("connections")
                  .displayedName("Connections")
                  .description("Table of active connections")
                  .setColumns(columns)
                  .readOnly()
                  .initialValue(std::vector<util::Hash>())
                  .expertAccess()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("updatePeriod")
                  .displayedName("Update period")
                  .description("Time period for updating network statistics of OutputChannel")
                  .unit(Unit::SECOND)
                  .assignmentOptional()
                  .defaultValue(10)
                  .expertAccess()
                  .commit();

            VECTOR_UINT64_ELEMENT(expected)
                  .key("bytesRead")
                  .displayedName("Read bytes")
                  .description("Vector of bytes read so far per connection taken from 'connections' table.")
                  .readOnly()
                  .expertAccess()
                  .archivePolicy(Schema::NO_ARCHIVING)
                  .commit();

            VECTOR_UINT64_ELEMENT(expected)
                  .key("bytesWritten")
                  .displayedName("Written bytes")
                  .description("Vector of bytes written so far per connection taken from 'connections' table.")
                  .readOnly()
                  .expertAccess()
                  .archivePolicy(Schema::NO_ARCHIVING)
                  .commit();
        }


        OutputChannel::OutputChannel(const karabo::util::Hash& config) : OutputChannel(config, 1) {}

        OutputChannel::OutputChannel(const karabo::util::Hash& config, int autoInit)
            : m_port(0),
              m_sharedInputIndex(0),
              m_toUnregisterSharedInput(false),
              m_showConnectionsHandler([](const std::vector<Hash>&) {}),
              m_showStatisticsHandler(
                    [](const std::vector<unsigned long long>&, const std::vector<unsigned long long>&) {}),
              m_connections(),
              m_updateDeadline(karabo::net::EventLoop::getIOService()) {
            config.get("distributionMode", m_distributionMode);
            config.get("noInputShared", m_onNoSharedInputChannelAvailable);
            config.get("port", m_port);
            config.get("updatePeriod", m_period);

            const std::string& hostname = config.get<std::string>("hostname");
            // resolve the hostname if needed
            if (hostname == "default") {
                m_hostname = boost::asio::ip::host_name();
            } else {
                m_hostname = getIpFromCIDRNotation(hostname);
            }


            KARABO_LOG_FRAMEWORK_DEBUG << "NoInputShared: " << m_onNoSharedInputChannelAvailable;

            // Memory related
            try {
                m_channelId = Memory::registerChannel();
                m_chunkId = Memory::registerChunk(m_channelId);
            } catch (...) {
                KARABO_RETHROW
            }
            assert(m_invalidChunkId > Memory::MAX_N_CHUNKS);

            KARABO_LOG_FRAMEWORK_DEBUG << "Outputting data on channel " << m_channelId << " and chunk " << m_chunkId;

            if (autoInit != 0) {
                // Initialize server connectivity:
                // Cannot use bind_weak in constructor but... usually it is safe to use here boost::bind.
                // And in this way we ensure that onTcpConnect is properly bound with bind_weak.
                // But see the HACK in initializeServerConnection if even that is called too early.
                karabo::net::EventLoop::post(
                      boost::bind(&OutputChannel::initializeServerConnection, this, kMaxServerInitializationAttempts));
            }
        }


        OutputChannel::~OutputChannel() {
            if (m_dataConnection) m_dataConnection->stop();
            Memory::unregisterChannel(m_channelId);
        }


        void OutputChannel::initializeServerConnection(int countdown) {
            using namespace karabo::net;

            // HACK starts
            // Treat situations when this method already runs although there is no shared_ptr for this object yet!
            // Seen e.g. if many devices with OutputChannels are instantiated simultaneously, i.e. in a busy process.
            const Self::Pointer sharedSelf(
                  weak_from_this().lock()); // Promote to shared_ptr. and keep alive for bind_weak
            if (!sharedSelf) {
                if (countdown > 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "initializeServerConnection: no shared_ptr yet, try again up to "
                                               << countdown
                                               << " more times"; // Unfortunately, m_instanceId cannot be filled yet.
                    // Let other threads potentially take over to increase the chance that shared_ptr is there next
                    // time: First rely on yield() to give constructor time to finish, but if that is not enough, sleep
                    // a bit.
                    if (2 * countdown < kMaxServerInitializationAttempts) {
                        boost::this_thread::sleep(boost::posix_time::milliseconds(1));
                    }
                    boost::this_thread::yield();
                    // Bare boost::bind with this as in constructor.
                    karabo::net::EventLoop::post(
                          boost::bind(&OutputChannel::initializeServerConnection, this, --countdown));
                    return;
                } else {
                    // m_instanceId not known yet...
                    const std::string msg(
                          "Give up to initialize server connection! Better recreate channel, e.g. by "
                          "re-instantiating device.");
                    KARABO_LOG_FRAMEWORK_ERROR << msg;
                    throw KARABO_NETWORK_EXCEPTION(msg);
                }
            }
            // HACK ends

            initialize();
        }


        void OutputChannel::initialize() {
            karabo::util::Hash h("type", "server", "port", m_port);
            Connection::Pointer connection = Connection::create("Tcp", h);
            // The following call can throw in case you provide with configuration's Hash the none-zero port number
            // and this port number is already used in the system, for example, by another application.
            // Or when bind_weak tries to create a shared_ptr, but fails - which initializeServerConnection above
            // prevents
            try {
                m_port = connection->startAsync(bind_weak(&karabo::xms::OutputChannel::onTcpConnect, this, _1, _2));
            } catch (const std::exception& ex) {
                std::ostringstream oss;
                oss << "Could not start TcpServer for output channel (\"" << m_channelId << "\", port = " << m_port
                    << ") : " << ex.what();
                KARABO_LOG_FRAMEWORK_ERROR << oss.str();
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION(oss.str()));
            }
            m_dataConnection = connection;
            KARABO_LOG_FRAMEWORK_DEBUG << "Started DeviceOutput-Server listening on port: " << m_port;
        }

        void OutputChannel::setInstanceIdAndName(const std::string& instanceId, const std::string& name) {
            m_instanceId = instanceId;
            m_channelName = name;
        }


        const std::string& OutputChannel::getInstanceId() const {
            return m_instanceId;
        }

        karabo::util::Hash OutputChannel::getInitialConfiguration() const {
            return Hash("address", m_hostname);
        }

        std::string OutputChannel::getInstanceIdName() const {
            return (m_instanceId + ":") += m_channelName;
        }

        bool OutputChannel::hasRegisteredCopyInputChannel(const std::string& instanceId) const {
            boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
            return (m_registeredCopyInputs.find(instanceId) != m_registeredCopyInputs.end());
        }


        bool OutputChannel::hasRegisteredSharedInputChannel(const std::string& instanceId) const {
            boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
            return (m_registeredSharedInputs.find(instanceId) != m_registeredSharedInputs.end());
        }


        void OutputChannel::registerIOEventHandler(
              const boost::function<void(const OutputChannel::Pointer&)>& ioEventHandler) {
            m_ioEventHandler = ioEventHandler;
        }


        karabo::util::Hash OutputChannel::getInformation() const {
            return karabo::util::Hash("connectionType", "tcp", "hostname", m_hostname, "port", m_port);
        }


        void OutputChannel::onTcpConnect(const karabo::net::ErrorCode& ec,
                                         const karabo::net::Channel::Pointer& channel) {
            using namespace karabo::net;

            switch (ec.value()) {
                    // Expected when io_service is stopped ... normal shutdown
                case bs::errc::no_such_file_or_directory: // End-of-file, code 2
                case bs::errc::operation_canceled:        // code 125 because of io_service was stopped?
                    return;
                    // Accepting the new connection ...
                case bs::errc::success:
                    break;
                    // "Retry" behavior because of following reasons
                case bs::errc::resource_unavailable_try_again: // temporary(?) problems with some resources   - retry
                case bs::errc::interrupted:                    // The system call was interrupted by a signal  - retry
                case bs::errc::protocol_error:
                case bs::errc::host_unreachable:
                case bs::errc::network_unreachable:
                case bs::errc::network_down: {
                    // The server should always wait for other connection attempts ...
                    if (m_dataConnection) {
                        m_dataConnection->startAsync(
                              bind_weak(&karabo::xms::OutputChannel::onTcpConnect, this, _1, _2));
                        KARABO_LOG_FRAMEWORK_WARN << "onTcpConnect received error code " << ec.value() << " (i.e. '"
                                                  << ec.message() << "'). Wait for new connections ...";
                    }
                    return;
                }
                    // These error resulting in "dead" server.  They should be considered by developer.
                default: {
                    KARABO_LOG_FRAMEWORK_ERROR
                          << "onTcpConnect received error code " << ec.value() << " (i.e. '" << ec.message()
                          << "'). Clients cannot connect anymore to this server! Developer's intervention is required!";
                    return;
                }
            }
            // Prepare to accept more connections
            if (m_dataConnection)
                m_dataConnection->startAsync(bind_weak(&karabo::xms::OutputChannel::onTcpConnect, this, _1, _2));
            KARABO_LOG_FRAMEWORK_DEBUG << "***** Connection established *****";
            {
                // Move responsibility to keep channel alive to one place
                boost::mutex::scoped_lock lock(m_inputNetChannelsMutex);
                m_inputNetChannels.insert(channel);
            }
            channel->readAsyncHash(bind_weak(&karabo::xms::OutputChannel::onTcpChannelRead, this, _1,
                                             Channel::WeakPointer(channel), _2));
        }


        void OutputChannel::onTcpChannelError(const karabo::net::ErrorCode& error,
                                              const karabo::net::Channel::Pointer& channel) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Tcp channel error on \"" << m_instanceId << "\", code #" << error.value()
                                       << " -- \"" << error.message() << "\".  Close channel at address "
                                       << channel.get();

            // Unregister channel
            onInputGone(channel, error);
        }


        void OutputChannel::onTcpChannelRead(const karabo::net::ErrorCode& ec,
                                             const karabo::net::Channel::WeakPointer& weakChannel,
                                             const karabo::util::Hash& message) {
            Channel::Pointer channel = weakChannel.lock();
            if (ec || !channel) {
                onTcpChannelError(ec, channel);
                return;
            }

            std::string reason;
            if (message.has("reason")) message.get<std::string>("reason", reason);

            if (reason == "hello") {
                /* The hello message is expected to have:
                 *     instanceId (std::string)
                 *     memoryLocation (std::string) [local/remote]
                 *     dataDistribution (std::string) [shared/copy]
                 *     onSlowness (std::string) [queue/drop/queueDrop/wait]
                 *     maxQueueLength (unsigned int; when onSlowness is queue or queueDrop)
                 */

                const std::string& instanceId = message.get<std::string>("instanceId");
                const std::string& memoryLocation = message.get<std::string>("memoryLocation");
                const std::string& dataDistribution = message.get<std::string>("dataDistribution");
                const std::string& onSlowness = message.get<std::string>("onSlowness");

                const unsigned int maxQueueLength =
                      (message.has("maxQueueLength") ? message.get<unsigned int>("maxQueueLength")
                                                     : InputChannel::DEFAULT_MAX_QUEUE_LENGTH);

                karabo::util::Hash info;
                info.set("instanceId", instanceId);
                info.set("memoryLocation", memoryLocation);
                info.set("tcpChannel", weakChannel);
                if (onSlowness == "throw") { // old version is connecting...
                    KARABO_LOG_FRAMEWORK_WARN << "For input channel " << instanceId << " overwrite outdated "
                                              << "'onSlowness' value \"throw\" by \"drop\"";
                    info.set("onSlowness", "drop");
                } else {
                    info.set("onSlowness", onSlowness);
                }
                info.set("maxQueueLength", maxQueueLength);
                info.set("queuedChunks", std::deque<int>());
                info.set("bytesRead", 0ull);
                info.set("bytesWritten", 0ull);

                std::string finalSlownessForLog = info.get<std::string>("onSlowness");
                {
                    // Need to lock both mutexes since eraseOldChannel has to look into both m_registered*Inputs
                    // since a "shared" input could become a "copy" one and vice versa. And removal and adding
                    // registered inputs has to be done under the same mutex lock to ensure consistency.
                    boost::mutex::scoped_lock lockShared(m_registeredSharedInputsMutex);
                    boost::mutex::scoped_lock lockCopy(m_registeredCopyInputsMutex);
                    eraseOldChannel(m_registeredSharedInputs, instanceId, channel);
                    eraseOldChannel(m_registeredCopyInputs, instanceId, channel);

                    if (dataDistribution == "shared") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Registering shared-input channel '" << instanceId << "'";
                        m_registeredSharedInputs[instanceId] = info;
                    } else {
                        if (boost::algorithm::starts_with(finalSlownessForLog, "queue")) {
                            (finalSlownessForLog += ", max. length ") += info.getAs<std::string>("maxQueueLength");
                        }
                        KARABO_LOG_FRAMEWORK_DEBUG << "Registering copy-input channel '" << instanceId << "'";
                        m_registeredCopyInputs[instanceId] = info;
                    }
                }
                onInputAvailable(instanceId); // Immediately register for reading
                updateConnectionTable();
                KARABO_LOG_FRAMEWORK_INFO << getInstanceIdName() << ": handshake (hello)... from InputChannel : \""
                                          << instanceId << "\", \"" << dataDistribution << "\", \""
                                          << finalSlownessForLog << "\"";
            } else if (reason == "update") {
                if (message.has("instanceId")) {
                    const std::string& instanceId = message.get<std::string>("instanceId");
                    KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT of '" << this->getInstanceIdName() << "': instanceId "
                                               << instanceId << " has updated...";
                    onInputAvailable(instanceId);
                }
            }
            if (channel->isOpen()) {
                channel->readAsyncHash(
                      bind_weak(&karabo::xms::OutputChannel::onTcpChannelRead, this, _1, weakChannel, _2));
            } else {
                onInputGone(channel, karabo::net::ErrorCode());
            }
        }


        void OutputChannel::eraseOldChannel(OutputChannel::InputChannels& channelContainer,
                                            const std::string& instanceId,
                                            const karabo::net::Channel::Pointer& newChannel) const {
            auto it = channelContainer.find(instanceId);
            if (it != channelContainer.end()) {
                const Hash& channelInfo = it->second;
                Channel::Pointer oldChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();
                if (oldChannel) {
                    if (oldChannel == newChannel) {
                        // Ever reached? Let's not close, but try to go on...
                        KARABO_LOG_FRAMEWORK_WARN << "Existing channel '" << instanceId
                                                  << "' sent hello message again.";
                    } else {
                        const TcpChannel::Pointer oldTcpChannel = boost::static_pointer_cast<TcpChannel>(oldChannel);
                        const Hash oldTcpInfo(TcpChannel::getChannelInfo(oldTcpChannel));
                        KARABO_LOG_FRAMEWORK_INFO << "New channel says hello with existing id '" << instanceId << "'. "
                                                  << "Close old one to " << oldTcpInfo.get<std::string>("remoteAddress")
                                                  << ":" << oldTcpInfo.get<unsigned short>("remotePort") << ".";
                        oldChannel->close();
                    }
                }
                // Remove channel entry now - it is
                // - either superseeded by a new connection with same id,
                // - or a misbehaving connection that 'hellos' again - it will be added again
                // - or some dangling weak pointer which can safely be removed
                channelContainer.erase(it);
            }
        }


        void OutputChannel::updateConnectionTable() {
            boost::mutex::scoped_lock lock(m_showConnectionsHandlerMutex);
            m_updateDeadline.cancel();
            m_connections.clear();
            {
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                for (const auto& idInfoPair : m_registeredSharedInputs) {
                    const Hash& channelInfo = idInfoPair.second;
                    Channel::WeakPointer wptr = channelInfo.get<Channel::WeakPointer>("tcpChannel");
                    Channel::Pointer channel = wptr.lock();
                    net::TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(channel);
                    Hash row = TcpChannel::getChannelInfo(tcpChannel);
                    row.set("remoteId", channelInfo.get<std::string>("instanceId"));
                    row.set("memoryLocation", channelInfo.get<std::string>("memoryLocation"));
                    row.set("dataDistribution", "shared");
                    row.set("onSlowness", channelInfo.get<std::string>("onSlowness"));
                    row.set("bytesRead", 0ull);
                    row.set("bytesWritten", 0ull);
                    row.set("weakChannel", wptr);
                    m_connections.push_back(std::move(row));
                }
            }
            {
                boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                for (const auto& idInfoPair : m_registeredCopyInputs) {
                    const Hash& channelInfo = idInfoPair.second;
                    Channel::WeakPointer wptr = channelInfo.get<Channel::WeakPointer>("tcpChannel");
                    Channel::Pointer channel = wptr.lock();
                    TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(channel);
                    Hash row = TcpChannel::getChannelInfo(tcpChannel);
                    row.set("remoteId", channelInfo.get<std::string>("instanceId"));
                    row.set("memoryLocation", channelInfo.get<std::string>("memoryLocation"));
                    row.set("dataDistribution", "copy");
                    row.set("onSlowness", channelInfo.get<std::string>("onSlowness"));
                    row.set("bytesRead", 0ull);
                    row.set("bytesWritten", 0ull);
                    row.set("weakChannel", wptr);
                    m_connections.push_back(std::move(row));
                }
            }
            // Copy and remove "weakChannel" column.  Otherwise the validator is getting upset
            auto connections = m_connections;
            for (Hash& h : connections) {
                h.erase("weakChannel");
                h.erase("bytesRead");
                h.erase("bytesWritten");
            }
            // Send filtered out table
            m_showConnectionsHandler(connections);
            // Check if we have to update this table periodically ...
            if (!m_connections.empty() && m_period > 0) {
                m_updateDeadline.expires_from_now(boost::posix_time::seconds(m_period));
                m_updateDeadline.async_wait(
                      bind_weak(&OutputChannel::updateNetworkStatistics, this, boost::asio::placeholders::error));
            }
        }


        void OutputChannel::updateNetworkStatistics(const boost::system::error_code& e) {
            if (e) return;
            if (m_period <= 0) return;

            boost::mutex::scoped_lock lock(m_showConnectionsHandlerMutex);
            size_t length = m_connections.size();
            std::vector<unsigned long long> vBytesRead(length);
            std::vector<unsigned long long> vBytesWritten(length);
            for (size_t i = 0; i < length; ++i) {
                Hash& h = m_connections[i];
                Channel::WeakPointer wptr = h.get<Channel::WeakPointer>("weakChannel");
                Channel::Pointer channel = wptr.lock();
                vBytesRead[i] = h.get<unsigned long long>("bytesRead");
                vBytesWritten[i] = h.get<unsigned long long>("bytesWritten");
                if (!channel) continue;
                vBytesRead[i] += channel->dataQuantityRead();
                vBytesWritten[i] += channel->dataQuantityWritten();
                h.set<unsigned long long>("bytesRead", vBytesRead[i]);
                h.set<unsigned long long>("bytesWritten", vBytesWritten[i]);
            }
            m_showStatisticsHandler(vBytesRead, vBytesWritten);

            m_updateDeadline.expires_from_now(boost::posix_time::seconds(m_period));
            m_updateDeadline.async_wait(
                  bind_weak(&OutputChannel::updateNetworkStatistics, this, boost::asio::placeholders::error));
        }


        void OutputChannel::onInputAvailable(const std::string& instanceId) {
            {
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                auto itIdChannelInfo = m_registeredSharedInputs.find(instanceId);
                if (itIdChannelInfo != m_registeredSharedInputs.end()) {
                    Hash& channelInfo = itIdChannelInfo->second;
                    // First check individual queue, even for load-balanced - might be endOfStream
                    std::deque<int>& individualQueue = channelInfo.get<std::deque<int>>("queuedChunks");
                    if (!individualQueue.empty()) {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Writing queued (shared) data to instance "
                                                   << instanceId;
                        sendFromQueue(channelInfo, individualQueue, lock); // unlocks lock!
                        return;
                    }
                    if (m_distributionMode == "load-balanced" && !m_sharedLoadBalancedQueuedChunks.empty()) {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId()
                                                   << " Writing single-queued (shared) data to instance " << instanceId;
                        const int chunkId = m_sharedLoadBalancedQueuedChunks.front();
                        if (Memory::isEndOfStream(m_channelId, chunkId)) {
                            // endOfStream in common queue: copy to all other's individual queues
                            for (auto& idChannelInfo2 : m_registeredSharedInputs) {
                                InputChannelInfo& otherChannelInfo = idChannelInfo2.second;
                                const std::string& otherInstanceId = idChannelInfo2.first;
                                if (otherInstanceId != instanceId) {
                                    otherChannelInfo.get<std::deque<int>>("queuedChunks").push_back(chunkId);
                                    Memory::incrementChunkUsage(m_channelId, chunkId);
                                }
                            }
                        }
                        sendFromQueue(channelInfo, m_sharedLoadBalancedQueuedChunks, lock); // unlocks lock!
                        return;
                    }
                    lock.unlock();
                    pushShareNext(instanceId);
                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " New (shared) input on instance " << instanceId
                                               << " available for writing ";
                    this->triggerIOEvent(); // now could also come for EOS...
                    return;
                }
            }

            boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
            auto itIdChannelInfo = m_registeredCopyInputs.find(instanceId);
            if (itIdChannelInfo != m_registeredCopyInputs.end()) {
                Hash& channelInfo = itIdChannelInfo->second;
                std::deque<int>& queue = channelInfo.get<std::deque<int>>("queuedChunks");
                if (!queue.empty()) {
                    KARABO_LOG_FRAMEWORK_TRACE << debugId() << " Writing queued (copied) data to instance "
                                               << instanceId;
                    sendFromQueue(channelInfo, queue, lock); // unlocks lock!
                    return;
                }
                // Be safe and unlock before pushCopyNext locks another mutex.
                // One also never knows what handlers are registered for io event...
                lock.unlock();
                pushCopyNext(instanceId);
                KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " New (copied) input on instance " << instanceId
                                           << " available for writing ";
                this->triggerIOEvent();
                return;
            }
            KARABO_LOG_FRAMEWORK_WARN << this->debugId() << " An input channel (" << instanceId
                                      << ") updated, but is not registered.";
        }


        void OutputChannel::onInputGone(const karabo::net::Channel::Pointer& channel,
                                        const karabo::net::ErrorCode& error) {
            using namespace karabo::net;
            const Hash tcpInfo(TcpChannel::getChannelInfo(boost::static_pointer_cast<TcpChannel>(channel)));
            const std::string tcpAddress(tcpInfo.get<std::string>("remoteAddress") + ':' +
                                         toString(tcpInfo.get<unsigned short>("remotePort")));

            // Clean specific channel from bookkeeping structures and ...
            // ... clean expired entries as well (we are not expecting them but we want to be on the safe side!)
            unsigned int inputsLeft = 0;
            {
                // SHARED Inputs
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                for (InputChannels::iterator it = m_registeredSharedInputs.begin();
                     it != m_registeredSharedInputs.end();) {
                    const Hash& channelInfo = it->second;
                    auto tcpChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();

                    // Cleaning expired or specific channels only
                    if (!tcpChannel || tcpChannel == channel) {
                        const std::string& instanceId = it->first;
                        KARABO_LOG_FRAMEWORK_INFO << getInstanceIdName() << " : Shared input channel '" << instanceId
                                                  << "' (ip/port " << (tcpChannel ? tcpAddress : "?")
                                                  << ") disconnected since '" << error.message() << "' (#"
                                                  << error.value() << ").";


                        // Release queued chunks
                        // round-robin case: release dedicated chunks (empty for "load-balanced")
                        for (const int chunkId : channelInfo.get<std::deque<int>>("queuedChunks")) {
                            unregisterWriterFromChunk(chunkId);
                        }
                        // load-balanced case: release chunks in common queue and clear it if nothing left to transfer
                        if (m_registeredSharedInputs.size() == 1u) { // i.e. will erase the last element below
                            for (const int chunkId : m_sharedLoadBalancedQueuedChunks) {
                                unregisterWriterFromChunk(chunkId);
                            }
                            m_sharedLoadBalancedQueuedChunks.clear();
                        }

                        // Delete from input queue
                        eraseSharedInput(instanceId);
                        it = m_registeredSharedInputs.erase(
                              it); // after last use of instanceId and queuedChunks which get invalidated here
                    } else {
                        ++it;
                    }
                }
                inputsLeft += m_registeredSharedInputs.size();
            }

            {
                // COPY Inputs
                boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                for (InputChannels::iterator it = m_registeredCopyInputs.begin(); it != m_registeredCopyInputs.end();) {
                    const Hash& channelInfo = it->second;
                    auto tcpChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();
                    if (!tcpChannel || tcpChannel == channel) {
                        const std::string& instanceId = it->first;

                        KARABO_LOG_FRAMEWORK_INFO << getInstanceIdName() << " : Copy input channel '" << instanceId
                                                  << "' (ip/port " << (tcpChannel ? tcpAddress : "?")
                                                  << ") disconnected since '" << error.message() << "' (#"
                                                  << error.value() << ").";
                        // Release any queued chunks:
                        for (const int chunkId : channelInfo.get<std::deque<int>>("queuedChunks")) {
                            unregisterWriterFromChunk(chunkId);
                        }
                        // Delete from input queue
                        eraseCopyInput(instanceId);
                        it = m_registeredCopyInputs.erase(
                              it); // after last use of instanceId which gets invalidated here
                    } else {
                        ++it;
                    }
                }
                inputsLeft += m_registeredCopyInputs.size();
            }
            {
                // Erase from container that keeps channel alive - and warn about inconsistencies:
                boost::mutex::scoped_lock lock(m_inputNetChannelsMutex);
                if (m_inputNetChannels.erase(channel) < 1u) {
                    KARABO_LOG_FRAMEWORK_WARN << getInstanceIdName() << " : Failed to remove channel with address "
                                              << channel.get();
                }
                if (m_inputNetChannels.size() != inputsLeft) {
                    KARABO_LOG_FRAMEWORK_WARN << getInstanceIdName() << " : Inconsistent number of channels left: "
                                              << m_inputNetChannels.size() << " / " << inputsLeft;
                }
            }
            updateConnectionTable();
        }


        void OutputChannel::triggerIOEvent() {
            using namespace karabo::net;
            try {
                OutputChannel::Pointer self = shared_from_this();
                if (m_ioEventHandler) m_ioEventHandler(self);
            } catch (karabo::util::Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "\"triggerIOEvent\" Exception code #" << e.detailedMsg();
                KARABO_RETHROW;
            } catch (const boost::bad_weak_ptr& e) {
                KARABO_LOG_FRAMEWORK_INFO << "\"triggerIOEvent\" call is too late: OutputChannel destroyed already -- "
                                          << e.what();
            } catch (const std::exception& ex) {
                KARABO_LOG_FRAMEWORK_ERROR << "\"triggerIOEvent\" exception -- " << ex.what();
                // throw KARABO_SYSTEM_EXCEPTION(string("\"triggerIOEvent\" exception -- ") + ex.what());
            }
        }


        void OutputChannel::sendFromQueue(karabo::util::Hash& channelInfo, std::deque<int>& chunkIds,
                                          boost::mutex::scoped_lock& lock) {
            int chunkId = chunkIds.front();
            chunkIds.pop_front();
            lock.unlock();
            KARABO_LOG_FRAMEWORK_DEBUG << "Sending chunk " << chunkId << " from queue, " << chunkIds.size()
                                       << " queue items left ";
            sendOne(chunkId, channelInfo, true); // chunk is safe - if copy was needed, that was done when queuing
        }

        void OutputChannel::pushShareNext(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            if (std::find(m_shareNext.begin(), m_shareNext.end(), instanceId) == m_shareNext.end()) {
                m_shareNext.push_back(instanceId);
            }
        }


        std::string OutputChannel::popShareNext() {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            if (m_shareNext.empty()) {
                throw KARABO_LOGIC_EXCEPTION("No shared input ready to pop its id.");
            }
            std::string info = m_shareNext.front();
            m_shareNext.pop_front();
            return info;
        }

        bool OutputChannel::isShareNextEmpty() const {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            return m_shareNext.empty();
        }

        bool OutputChannel::hasSharedInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            return (std::find(m_shareNext.begin(), m_shareNext.end(), instanceId) != m_shareNext.end());
        }


        void OutputChannel::eraseSharedInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            auto it = std::find(m_shareNext.begin(), m_shareNext.end(), instanceId);
            if (it != m_shareNext.end()) m_shareNext.erase(it);
        }


        void OutputChannel::pushCopyNext(const std::string& info) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            m_copyNext.insert(info);
        }


        bool OutputChannel::eraseCopyInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            return (m_copyNext.erase(instanceId) > 0);
        }


        bool OutputChannel::updateChunkId() {
            try {
                m_chunkId = Memory::registerChunk(m_channelId);
                return true;
            } catch (const karabo::util::MemoryInitException&) {
                karabo::util::Exception::clearTrace();
                m_chunkId = m_invalidChunkId;
            } catch (const std::exception&) {
                KARABO_RETHROW;
            }
            return false;
        }


        void OutputChannel::update(bool safeNDArray) {
            // m_channelId is unique per _process_...
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT " << m_channelId << " of '" << this->getInstanceIdName()
                                       << "' update()";

            // If no data was written and not endOfStream: nothing to do
            if (Memory::size(m_channelId, m_chunkId) == 0 && !Memory::isEndOfStream(m_channelId, m_chunkId)) return;

            // Take current chunkId for sending and get already next one
            unsigned int chunkId = m_chunkId;
            updateChunkId(); // if this fails, m_chunkId is set to m_invalidChunkId

            // This will increase the usage counts for this chunkId
            // by the number of all interested connected inputs
            // and set m_toUnregisterSharedInput/m_toUnregisterCopyInputs to check later for whom we registered
            registerWritersOnChunk(chunkId);

            // Distribute chunk(s)
            distribute(chunkId, safeNDArray);

            // Copy chunk(s)
            copy(chunkId, safeNDArray);

            // Clean-up chunk registration
            unsigned int numUnregister = 1;  // That is the usage of the OutputChannel itself!
            if (m_toUnregisterSharedInput) { // The last shared input disconnected while updating...
                ++numUnregister;
            }
            numUnregister += m_toUnregisterCopyInputs.size(); // All these copy inputs disconnected while updating
            // We are done with this chunkId, it may stay alive until local receivers are done as well
            for (size_t i = 0; i < numUnregister; ++i) {
                unregisterWriterFromChunk(chunkId);
            }

            // By all means, try to get a valid m_chunkId for the next round. If not available yet
            // * drop from queueDrop queues
            // * if that does not help, block until a queue(-Wait) queue shrinks
            ensureValidChunkId();
        }


        void OutputChannel::ensureValidChunkId() {
            // If still invalid, drop from queueDrop channels until resources freed and valid chunkId available.
            // If nothing to drop left, check whether anything is waiting due to "queue", block as requested
            // until valid chunkId available.
            // If nothing is queuing anymore at all, but still no valid chunkId available, throw a logic exception
            // (should never happen!).

            // Note by GF: In praxis, even in the tests that run into full queues, I never saw this method to go
            //             beyond the first 'break;' in the first while loop. Nevertheless, I'd prefer to keep the
            //             rest of the code as a "last rescue attempt".

            // 1) Loop until dropping from "queueDrop" queues makes a valid chunkId available.
            while (m_chunkId == m_invalidChunkId) {
                if (updateChunkId()) {
                    break;
                }
                // free resources: loop copy channels and drop the oldest chunk (if channel is "queueDrop")
                long long nTotalQueueDropLeft = 0;
                long long minTotalEndOfStream = 0;
                auto dropIndividualQueues = [&nTotalQueueDropLeft, &minTotalEndOfStream, this](
                                                  boost::mutex& mut, InputChannels& channels) {
                    boost::mutex::scoped_lock lock(mut);
                    for (auto& idChannelInfo : channels) {
                        Hash& channelInfo = idChannelInfo.second;
                        if (channelInfo.get<std::string>("onSlowness") == "queueDrop") {
                            auto& queuedChunks = channelInfo.get<std::deque<int>>("queuedChunks");
                            for (auto it = queuedChunks.begin(); it != queuedChunks.end(); ++it) {
                                const int chunkId = *it;
                                if (Memory::isEndOfStream(m_channelId, chunkId)) {
                                    ++minTotalEndOfStream;
                                } else {
                                    unregisterWriterFromChunk(chunkId);
                                    queuedChunks.erase(it); // invalidates 'it', but we do 'break;' below anyway
                                    KARABO_LOG_FRAMEWORK_INFO << "Drop from queue for 'queueDrop' channel '"
                                                              << idChannelInfo.first << "', queue's new size is "
                                                              << queuedChunks.size();
                                    break; // only drop one from this queue
                                }
                            }
                            nTotalQueueDropLeft += queuedChunks.size();
                        }
                    }
                };
                dropIndividualQueues(m_registeredCopyInputsMutex, m_registeredCopyInputs);
                // free resources: loop shared channels queues or their common queue
                if (m_onNoSharedInputChannelAvailable == "queueDrop") {
                    // for shared channels it depends on the distribution mode where queues are
                    if (m_distributionMode == "round-robin") {
                        dropIndividualQueues(m_registeredSharedInputsMutex, m_registeredSharedInputs);
                    } else if (m_distributionMode == "load-balanced") {
                        boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                        if (!m_sharedLoadBalancedQueuedChunks.empty()) {
                            // No need to care for endOfStream in common queue - would be in individual queues
                            for (auto it = m_sharedLoadBalancedQueuedChunks.begin();
                                 it != m_sharedLoadBalancedQueuedChunks.end(); ++it) {
                                const int chunkId = *it;
                                if (Memory::isEndOfStream(m_channelId, chunkId)) {
                                    ++minTotalEndOfStream;
                                } else {
                                    unregisterWriterFromChunk(chunkId);
                                    m_sharedLoadBalancedQueuedChunks.erase(it);
                                    KARABO_LOG_FRAMEWORK_INFO << "Drop from load-balanced queue, queue's new size is "
                                                              << m_sharedLoadBalancedQueuedChunks.size();
                                    break;
                                }
                            }
                            nTotalQueueDropLeft += m_sharedLoadBalancedQueuedChunks.size();
                        }
                    }
                }
                if (nTotalQueueDropLeft <= minTotalEndOfStream) { // should not be possible to be '<', just '=='
                    // Nothing left to drop
                    break;
                }
            }

            // 2) Block a while as long there is any "queue(Wait)" queue around, as long as m_chunkid is invalid.
            while (m_chunkId == m_invalidChunkId) {
                if (updateChunkId()) {
                    break;
                }
                // If there is any "queue(-Wait)" queue left, block
                auto anyNonEmptyQueue = [](boost::mutex& mut, InputChannels& channels) {
                    boost::mutex::scoped_lock lock(mut);
                    for (auto& idChannelInfo : channels) {
                        Hash& channelInfo = idChannelInfo.second;
                        if (channelInfo.get<std::string>("onSlowness") == "queue") { // i.e. queueWait
                            auto& queuedChunks = channelInfo.get<std::deque<int>>("queuedChunks");
                            if (!queuedChunks.empty()) {
                                KARABO_LOG_FRAMEWORK_INFO << "Block queue for 'queue(-Wait)' channel '"
                                                          << idChannelInfo.first << "', queue's size is "
                                                          << queuedChunks.size();
                                return true;
                            }
                        }
                    }
                    return false;
                };
                // Check the copy queues
                bool block = false;
                if (anyNonEmptyQueue(m_registeredCopyInputsMutex, m_registeredCopyInputs)) {
                    block = true;
                } else if (m_onNoSharedInputChannelAvailable == "queue") { // i.e. queueWait
                    // Check the shared queue(s) - it depends on the distribution mode where queues are
                    if (m_distributionMode == "round-robin") {
                        if (anyNonEmptyQueue(m_registeredSharedInputsMutex, m_registeredSharedInputs)) {
                            block = true;
                        }
                    } else if (m_distributionMode == "load-balanced") {
                        boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                        if (!m_sharedLoadBalancedQueuedChunks.empty()) {
                            KARABO_LOG_FRAMEWORK_INFO << "Block for load-balanced queue of size "
                                                      << m_sharedLoadBalancedQueuedChunks.size();
                            block = true;
                        } else if (!updateChunkId()) {
                            // Should never come here! In principle, local input channels could use chunks of this
                            // output channel. But while there is space in Memory for > 2000 (Memory::MAX_N_CHUNKS)
                            // chunks, there are less than 200 (Memory::MAX_N_CHANNELS) Input-/OutputChannels and the
                            // local input channels cannot use more than two chunks (for their two pots) from here.
                            throw KARABO_LOGIC_EXCEPTION(
                                  "No new chunk available, but no queues that could occupy them!");
                        }
                    }
                }
                if (block) boost::this_thread::sleep(boost::posix_time::milliseconds(2));
            } // while treatment for queue(-Wait))
        }

        void OutputChannel::signalEndOfStream() {
            // If there is still some data in the pipe, put it out
            if (Memory::size(m_channelId, m_chunkId) > 0) update();

            // Mark next chunk as EOS and send it out as well
            Memory::setEndOfStream(m_channelId, m_chunkId);
            update();
        }

        void OutputChannel::registerShowConnectionsHandler(const ShowConnectionsHandler& handler) {
            boost::mutex::scoped_lock lock(m_showConnectionsHandlerMutex);
            if (!handler) {
                m_showConnectionsHandler = [](const std::vector<karabo::util::Hash>&) {};
            } else {
                m_showConnectionsHandler = handler;
            }
        }


        void OutputChannel::registerShowStatisticsHandler(const ShowStatisticsHandler& handler) {
            boost::mutex::scoped_lock lock(m_showConnectionsHandlerMutex);
            if (!handler) {
                m_showStatisticsHandler = [](const std::vector<unsigned long long>&,
                                             const std::vector<unsigned long long>&) {};
            } else {
                m_showStatisticsHandler = handler;
            }
        }

        void OutputChannel::registerWritersOnChunk(unsigned int chunkId) {
            {
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                if (m_registeredSharedInputs.empty()) {
                    m_toUnregisterSharedInput = false;
                } else {
                    // Only one of the shared inputs will be provided with data
                    Memory::incrementChunkUsage(m_channelId, chunkId);
                    m_toUnregisterSharedInput = true;
                }
            }
            {
                m_toUnregisterCopyInputs.clear();
                boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                for (const auto& idChannelInfo : m_registeredCopyInputs) {
                    Memory::incrementChunkUsage(m_channelId, chunkId);
                    m_toUnregisterCopyInputs.insert(idChannelInfo.first);
                }
            }
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT Registered " << Memory::getChunkStatus(m_channelId, chunkId)
                                       << " uses for [" << m_channelId << "][" << chunkId << "]";
        }


        void OutputChannel::unregisterWriterFromChunk(int chunkId) {
            Memory::decrementChunkUsage(m_channelId, chunkId);
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT " << Memory::getChunkStatus(m_channelId, chunkId)
                                       << " uses left for [" << m_channelId << "][" << chunkId << "]";
        }


        void OutputChannel::distribute(unsigned int chunkId, bool safeNDArray) {
            boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);

            // If no shared input channels are registered at all, we do not go on
            if (m_registeredSharedInputs.empty()) return;
            if (!m_toUnregisterSharedInput) {
                // Increment chunk usage since a first shared input just connected while we update
                Memory::incrementChunkUsage(m_channelId, chunkId);
            }
            m_toUnregisterSharedInput = false; // We care for it!

            if (Memory::isEndOfStream(m_channelId, chunkId)) {
                distributeEndOfStream(chunkId);
            } else if (m_distributionMode == "round-robin") {
                distributeRoundRobin(chunkId, lock, safeNDArray);
            } else if (m_distributionMode == "load-balanced") {
                distributeLoadBalanced(chunkId, lock, safeNDArray);
            } else { // We should never be here!!
                throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured: " + m_distributionMode);
            }
        }


        void OutputChannel::distributeEndOfStream(unsigned int chunkId) {
            // Despite sharing, each input should receive endOfStream

            // But for load-balanced, the shared queue has to be worked on before
            if (!m_sharedLoadBalancedQueuedChunks.empty()) {
                // Put in here once and "multiplex" when taken out
                m_sharedLoadBalancedQueuedChunks.push_back(chunkId);
                return;
            }

            for (auto& idChannelInfo : m_registeredSharedInputs) { // not const since queue might be extended

                // Non-eos logic so far foresaw only one chunk usage - here add for each (and subtract the one below)
                Memory::incrementChunkUsage(m_channelId, chunkId);

                InputChannelInfo& channelInfo = idChannelInfo.second;
                const std::string& instanceId = idChannelInfo.first;

                if (hasSharedInput(instanceId)) {
                    eraseSharedInput(instanceId);
                    sendOne(chunkId, channelInfo, true); // safeNDArray does not matter for EOS
                } else {
                    // We queue the EOS so it is sent immediately when the input is ready to receive data.
                    // No need to care about full queue - ensureValidChunkId() add the end of update() will care.
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing endOfStream for shared input "
                                               << instanceId;
                    channelInfo.get<std::deque<int>>("queuedChunks").push_back(chunkId);
                }
            }
            // Remove the one usage count too much (m_registeredSharedInputs cannot be empty here!)
            unregisterWriterFromChunk(chunkId);
        }


        void OutputChannel::distributeRoundRobin(unsigned int chunkId, boost::mutex::scoped_lock& lock,
                                                 bool safeNDArray) {
            // Next input
            InputChannels::iterator itIdChannelInfo = getNextRoundRobinChannel();
            karabo::util::Hash& channelInfo = itIdChannelInfo->second;
            const std::string& instanceId = itIdChannelInfo->first; // channelInfo.get<std::string>("instanceId");


            if (hasSharedInput(instanceId)) { // Found
                // Note: If now, before we can actually distribute, instanceId disconnects, the data that should go
                //       there is lost, also no other shared input will receive it. But that should be acceptable
                //       in a dynamic and distributed system like Karabo.
                eraseSharedInput(instanceId);
                sendOne(chunkId, channelInfo, safeNDArray);
            } else { // Not found
                bool haveToWait = false;
                bool haveToWaitForQueue = false;
                if (m_onNoSharedInputChannelAvailable == "drop") {
                    // Drop data and try same destination again next time
                    undoGetNextRoundRobinChannel(); // lock must not yet be unlocked() after getNextSharedInputIdx()
                                                    // above!
                    unregisterWriterFromChunk(chunkId);
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                               << " Dropping (shared) data package with chunkId: " << chunkId;

                } else if (boost::algorithm::starts_with(m_onNoSharedInputChannelAvailable,
                                                         "queue")) { // i.e. queue(-Wait) or queueDrop
                    if (m_chunkId != m_invalidChunkId) {             // i.e. all fine with queue length
                        // Since distributing round-robin, it is really this instance's turn.
                        // So we queue for exactly this one.
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                                   << " Queuing (shared) data package with chunkId: " << chunkId;
                        if (!safeNDArray) Memory::assureAllDataIsCopied(m_channelId, chunkId);
                        channelInfo.get<std::deque<int>>("queuedChunks").push_back(chunkId);
                    } else if (m_onNoSharedInputChannelAvailable == "queueDrop") {
                        unregisterWriterFromChunk(chunkId);
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queue-dropping (shared) data package for "
                                                   << instanceId;
                    } else { // i.e. make wait
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queue (shared) data package means wait for "
                                                   << instanceId;
                        // Blocking actions must not happen under the mutex that is also needed to unblock (in
                        // onInputAvailable)
                        haveToWaitForQueue = true;
                    }
                } else if (m_onNoSharedInputChannelAvailable == "wait") {
                    // Blocking actions must not happen under the mutex that is also needed to unblock (in
                    // onInputAvailable)
                    haveToWait = true;
                } else {
                    // We should never be here!!
                    throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured: " +
                                                 m_onNoSharedInputChannelAvailable);
                }

                if (haveToWait || haveToWaitForQueue) {
                    // The input channel  whose turn it is, is not ready and we
                    // either have to wait or we have to queue, but queue is full

                    // Make copy of references which might become dangling when unlocking mutex lock
                    const karabo::util::Hash channelInfoCopy = channelInfo;
                    const std::string& instanceIdCopy = channelInfoCopy.get<std::string>("instanceId");
                    lock.unlock(); // Otherwise hasSharedInput will never become true (and deadlock with
                                   // hasRegisteredSharedInputChannel)!
                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Waiting for available (shared) input channel...";
                    while (true) {
                        if (haveToWaitForQueue) {
                            if (m_invalidChunkId != m_chunkId || updateChunkId()) break;
                        } else { // i.e. wait
                            if (hasSharedInput(instanceIdCopy)) break;
                        }
                        boost::this_thread::sleep(boost::posix_time::millisec(1));
                        if (!hasRegisteredSharedInputChannel(instanceIdCopy)) { // might have disconnected meanwhile...
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " input channel (shared) of "
                                                       << instanceIdCopy << " disconnected while waiting for it";
                            lock.lock();
                            if (m_registeredSharedInputs.empty()) { // nothing left, so release chunk
                                unregisterWriterFromChunk(chunkId);
                            } else { // recurse to find next available shared input
                                distributeRoundRobin(chunkId, lock, safeNDArray);
                            }
                            return;
                        }
                    }
                    if (haveToWaitForQueue) {
                        // Can neither use 'channelInfo' (its InputChannel could have disconnected meanwhile)
                        // nor 'channelInfoCopy' (its queue is a copy of the original one),
                        lock.lock();
                        auto it = m_registeredSharedInputs.find(instanceIdCopy);
                        if (it != m_registeredSharedInputs.end()) {
                            // XXX: We ignore the case that 'instanceIdCopy' dis- and reconnected. In that case it
                            //      might not be its turn and it could even be ready to directly receive data...
                            it->second.get<std::deque<int>>("queuedChunks").push_back(chunkId);
                            KARABO_LOG_FRAMEWORK_DEBUG
                                  << this->debugId() << " queuing data package for input channel (shared) "
                                  << instanceId << " after waiting for full queue, chunk " << chunkId;
                        } else if (m_registeredSharedInputs.empty()) { // nothing left, so release chunk
                            unregisterWriterFromChunk(chunkId);
                        } else { // recurse to find next available shared input
                            distributeRoundRobin(chunkId, lock, safeNDArray);
                            // return; // redundant here
                        }
                    } else if (haveToWait) {
                        // Note: if 'instanceIdCopy' is now gone, chunkId will not be delivered to anybody else
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                                   << " found (shared) input channel after waiting, distributing now";
                        eraseSharedInput(instanceIdCopy);
                        sendOne(chunkId, channelInfoCopy, safeNDArray);
                    }
                } // end have to wait or queue
            }     // end else - not found
        }

        void OutputChannel::distributeLoadBalanced(unsigned int chunkId, boost::mutex::scoped_lock& lock,
                                                   bool safeNDArray) {
            if (!isShareNextEmpty()) { // Found
                const std::string instanceId = popShareNext();
                auto it = m_registeredSharedInputs.find(instanceId);
                if (it == m_registeredSharedInputs.end()) { // paranoia check
                    // Should never come here...
                    KARABO_LOG_FRAMEWORK_ERROR << "Next load balanced input '" << instanceId
                                               << "' does not exist anymore!";
                    return;
                }
                const karabo::util::Hash& channelInfo = it->second;
                sendOne(chunkId, channelInfo, safeNDArray);
            } else { // Not found
                bool haveToWait = false;
                bool haveToWaitForQueue = false;
                if (m_onNoSharedInputChannelAvailable == "drop") {
                    unregisterWriterFromChunk(chunkId);
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                               << " Dropping (shared) data package with chunkId: " << chunkId;
                } else if (boost::algorithm::starts_with(m_onNoSharedInputChannelAvailable,
                                                         "queue")) { // i.e. queue(-Wait) or queueDrop
                    if (m_chunkId != m_invalidChunkId) {             // i.e. all fine with queue length
                        // For load-balanced mode the chunks should be put on a single queue.
                        KARABO_LOG_FRAMEWORK_DEBUG
                              << this->debugId()
                              << " Placing chunk in single queue (load-balanced distribution mode): " << chunkId;
                        if (!safeNDArray) Memory::assureAllDataIsCopied(m_channelId, chunkId);
                        m_sharedLoadBalancedQueuedChunks.push_back(chunkId);
                    } else if (m_onNoSharedInputChannelAvailable == "queueDrop") {
                        unregisterWriterFromChunk(chunkId);
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queue-dropping (shared) data package with "
                                                   << "chunkId " << chunkId << " (single queue full)";
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                                   << " Queue (shared) data package means wait since single queue full";
                        haveToWaitForQueue = true;
                    }
                } else if (m_onNoSharedInputChannelAvailable == "wait") {
                    // Blocking actions must not happen under the mutex that is also needed to unblock (in
                    // onInputAvailable)
                    haveToWait = true;
                } else {
                    // We should never be here!!
                    throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured: " +
                                                 m_onNoSharedInputChannelAvailable);
                }
                if (haveToWait || haveToWaitForQueue) {
                    // None of the connected inputs is ready for more data and we are configured to wait for them
                    // or to queue, but queue is full.

                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Waiting for available (shared) input channel...";
                    // while loop such that the following popShareNext() is called under the same lock.lock() under
                    // which isShareNextEmpty() became false - otherwise there might not be anything left to pop...
                    while (true) {
                        lock.unlock(); // Otherwise isShareNextEmpty() will never become false
                        boost::this_thread::sleep(boost::posix_time::millisec(1));
                        lock.lock();
                        if (m_registeredSharedInputs.empty()) {
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                                       << " found all (shared) input channels gone while waiting";
                            unregisterWriterFromChunk(chunkId);
                            return; // Nothing to distribute anymore: no shared channels left
                        }
                        if (haveToWait) {
                            if (!isShareNextEmpty()) break;
                        } else if (haveToWaitForQueue) {
                            if (m_invalidChunkId != m_chunkId || updateChunkId()) break;
                        }
                    }
                    if (haveToWait) {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                                   << " found (shared) input channel after waiting, distributing now";
                        const std::string instanceId = popShareNext();
                        auto it = m_registeredSharedInputs.find(instanceId);
                        if (it == m_registeredSharedInputs.end()) { // paranoia check
                            // Should never come here...
                            KARABO_LOG_FRAMEWORK_ERROR << "Next load balanced input '" << instanceId
                                                       << "' does not exist anymore (after wait)!";
                            return;
                        }
                        const karabo::util::Hash& channelInfo = it->second;
                        sendOne(chunkId, channelInfo, safeNDArray);
                    } else if (haveToWaitForQueue) {
                        // XXX: Do not treat the unlikely case that the queue could be suddenly empty now
                        //      and even one receiver ready to get the data.
                        //      (E.g. when chunks were exhausted by a too slow copy input that now disconnected...?)
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                                   << " Placing chunk in single queue after wait: " << chunkId;
                        if (!safeNDArray) Memory::assureAllDataIsCopied(m_channelId, chunkId);
                        m_sharedLoadBalancedQueuedChunks.push_back(chunkId);
                    }
                } // haveToWait or -Queue
            }     // end else - not found
        }


        OutputChannel::InputChannels::iterator OutputChannel::getNextRoundRobinChannel() {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            ++m_sharedInputIndex;
            m_sharedInputIndex %= m_registeredSharedInputs.size(); // size() == 0 excluded by documented requirements

            // Increment begin() as many times as needed to match index (there is not map::iterator::operator +=...).
            // One could store the iterator directly instead of m_sharedInputIndex, but then it has to be taken care of
            // that the iterator stays valid whenever m_registeredSharedInputs is touched.
            InputChannels::iterator result = m_registeredSharedInputs.begin();
            unsigned int loop = m_sharedInputIndex;
            while (loop > 0) {
                ++result;
                --loop;
            }
            return result;
        }


        void OutputChannel::undoGetNextRoundRobinChannel() {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            if (0 == m_sharedInputIndex) {
                m_sharedInputIndex =
                      m_registeredSharedInputs.size() - 1u; // size() == 0 excluded by documented requirements
            } else {
                --m_sharedInputIndex;
            }
        }


        void OutputChannel::copy(unsigned int chunkId, bool safeNDArray) {
            InputChannels waitingInstances;
            std::vector<Hash> toSendImmediately;
            {
                boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                toSendImmediately.reserve(m_registeredCopyInputs.size());
                for (auto& idChannelInfo : m_registeredCopyInputs) { // not const since queue might be extended

                    InputChannelInfo& channelInfo = idChannelInfo.second;
                    const std::string& instanceId = idChannelInfo.first;
                    std::string onSlowness(channelInfo.get<std::string>("onSlowness")); // by value on purpose

                    const auto unregisterIter = m_toUnregisterCopyInputs.find(instanceId);
                    if (unregisterIter == m_toUnregisterCopyInputs.end()) {
                        // Increment chunk usage since this copy input just connected while we update
                        Memory::incrementChunkUsage(m_channelId, chunkId);
                    } else {
                        m_toUnregisterCopyInputs.erase(unregisterIter); // We care about it!
                    }
                    if (eraseCopyInput(instanceId)) {
                        // Ready for data - can call sendOne, but better do that without mutex lock, especially since
                        // m_registeredCopyInputsMutex is locked also when readiness is stored in onInputAvailable.
                        toSendImmediately.push_back(channelInfo);
                        continue;
                    }
                    if (Memory::isEndOfStream(m_channelId, chunkId)) {
                        // EOS must never get lost, but we do not want to block here either (except if queue is full)
                        onSlowness = "queue";
                    }
                    if (onSlowness == "drop") {
                        unregisterWriterFromChunk(chunkId);
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Dropping (copied) data package for "
                                                   << instanceId;
                    } else if (boost::algorithm::starts_with(onSlowness, "queue")) { // i.e. queue(-Wait) or queueDrop
                        if (m_chunkId != m_invalidChunkId &&
                            static_cast<unsigned int>(channelInfo.get<std::deque<int>>("queuedChunks").size()) <
                                  channelInfo.get<unsigned int>("maxQueueLength")) {
                            // i.e. all fine with queue length
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing (copied) data package for "
                                                       << instanceId << ", chunk " << chunkId;
                            if (!safeNDArray) Memory::assureAllDataIsCopied(m_channelId, chunkId);
                            channelInfo.get<std::deque<int>>("queuedChunks").push_back(chunkId);
                        } else if (onSlowness == "queueDrop") {
                            unregisterWriterFromChunk(chunkId);
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                                       << " Queue-dropping (copied) data package for " << instanceId;
                        } else { // i.e. make wait
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                                       << " Queue (copied) data package means wait for " << instanceId;
                            waitingInstances.insert(idChannelInfo);
                        }
                    } else if (onSlowness == "wait") {
                        // Blocking actions must not happen under the mutex that is also needed to unblock (in
                        // onInputAvailable)
                        waitingInstances.insert(idChannelInfo);
                    }
                }
            } // end of mutex lock

            // Now send data to those that are ready immediately
            for (const Hash& channelInfo : toSendImmediately) {
                sendOne(chunkId, channelInfo, safeNDArray);
            }

            // Finally care for those not ready yet, but so eager that we must wait:
            // onSlowness "wait" and onSlowness "queue" with maximum queue size reached.
            for (const InputChannels::value_type& idChannelInfo : waitingInstances) {
                const std::string& instanceId = idChannelInfo.first;
                const Hash& channelInfo = idChannelInfo.second;
                const std::string& onSlowness = channelInfo.get<std::string>("onSlowness");
                const bool isQueue = ("wait" != onSlowness);
                KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Data (copied) is waiting for input channel of "
                                           << instanceId << " (" << onSlowness << ") to be available";
                bool instanceDisconnected = false;
                while (true) {
                    if (isQueue) {
                        if (m_invalidChunkId != m_chunkId || updateChunkId()) {
                            // Still need to check queue length - but note that we only have a copy at
                            // channelInfo.get<std::deque<int> >("queuedChunks") - need mutex lock for the real one:
                            boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                            auto it = m_registeredCopyInputs.find(instanceId);
                            if (it != m_registeredCopyInputs.end() && // other, disconnected case treated below
                                it->second.get<std::deque<int>>("queuedChunks").size() <
                                      it->second.get<unsigned int>("maxQueueLength")) {
                                break;
                            }
                        }
                    } else { // !isQueue (i.e. wait)
                        if (eraseCopyInput(instanceId)) break;
                    }
                    boost::this_thread::sleep(boost::posix_time::millisec(1));
                    if (!hasRegisteredCopyInputChannel(instanceId)) { // might have disconnected meanwhile...
                        instanceDisconnected = true;
                        break;
                    }
                }
                if (instanceDisconnected) {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " input channel (copy) of " << instanceId
                                               << " disconnected while waiting for it";
                    unregisterWriterFromChunk(chunkId);
                    continue;
                }
                if (!isQueue) {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                               << " found (copied) input channel after waiting, copying now";
                    const Hash& channelInfo = idChannelInfo.second;
                    sendOne(chunkId, channelInfo, safeNDArray);
                } else {
                    boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                    auto it = m_registeredCopyInputs.find(instanceId);
                    if (it == m_registeredCopyInputs.end()) {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " input channel (copy) of " << instanceId
                                                   << " disconnected while waiting for it due to full queue";
                        unregisterWriterFromChunk(chunkId);
                        continue;
                    }
                    Hash& channelInfo = it->second;
                    std::deque<int>& queue = channelInfo.get<std::deque<int>>("queuedChunks");
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " queuing data package for input channel (copy) "
                                               << instanceId << " after waiting for full queue, chunk " << chunkId;
                    if (!safeNDArray) Memory::assureAllDataIsCopied(m_channelId, chunkId);
                    queue.push_back(chunkId);
                }
            } // loop on waitingInstances
        }


        void OutputChannel::sendOne(const unsigned int& chunkId, const InputChannelInfo& channelInfo,
                                    bool safeNDArray) {
            const bool isEos = Memory::isEndOfStream(m_channelId, chunkId);
            const std::string& destination = channelInfo.get<std::string>("memoryLocation"); // 'local' or 'remote'
            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << "Sent chunk " << chunkId << (isEos ? " (EOS)" : "")
                                       << " to " << destination << " input "
                                       << channelInfo.get<std::string>("instanceId");
            if (destination == "local") {
                sendLocal(chunkId, channelInfo, isEos, safeNDArray);
            } else {
                sendRemote(chunkId, channelInfo, isEos);
            }
        }


        void OutputChannel::sendLocal(const unsigned int& chunkId, const InputChannelInfo& channelInfo, bool eos,
                                      bool safeNDArray) {
            using namespace karabo::net;
            Channel::Pointer tcpChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();

            bool notSent = true;
            if (tcpChannel && tcpChannel->isOpen()) {
                // Synchronous write as it takes no time here
                try {
                    karabo::util::Hash header("channelId", m_channelId, "chunkId", chunkId);
                    if (eos) {
                        header.set("endOfStream", true);
                    } else if (!safeNDArray) {
                        // in case of short-cutting the receiver may async. work on data the sender is already altering
                        // again. we assure that the contents in the chunk the receiver gets sent have been copied once
                        Memory::assureAllDataIsCopied(m_channelId, chunkId);
                    }
                    tcpChannel->write(header, std::vector<karabo::io::BufferSet::Pointer>());
                    notSent = false;
                } catch (const std::exception& e) {
                    if (tcpChannel->isOpen()) {
                        KARABO_LOG_FRAMEWORK_WARN << "OutputChannel::sentLocal - channel still open :  " << e.what();
                    } else {
                        karabo::util::Exception::clearTrace();
                    }
                }
            }
            // NOTE: The input channel will decrement the chunkId usage, as it uses the same memory location
            //       Having the next line only if not sent is thus correct.
            // NOTE II: If the other end disconnects before processing our message, the chunk is leaked!
            //          But it is an unlikely scenario that a local receiver disconnects often - usually the full
            //          process including the sender (i.e. we here) is shutdown.
            if (notSent) unregisterWriterFromChunk(chunkId);
        }


        void OutputChannel::sendRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo, bool eos) {
            using namespace karabo::net;

            Channel::Pointer tcpChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();

            if (tcpChannel && tcpChannel->isOpen()) {
                karabo::util::Hash header;
                std::vector<karabo::io::BufferSet::Pointer> data;
                if (eos) {
                    header.set("endOfStream", true);
                } else {
                    Memory::readIntoBuffers(data, header, m_channelId, chunkId);
                }
                try {
                    tcpChannel->write(header, data);
                } catch (const std::exception& e) {
                    if (tcpChannel->isOpen()) {
                        KARABO_LOG_FRAMEWORK_WARN << "OutputChannel::sendRemote - channel still open :  " << e.what();
                    } else {
                        karabo::util::Exception::clearTrace();
                    }
                }
                data.clear();
            }

            unregisterWriterFromChunk(chunkId);
        }


        std::string OutputChannel::debugId() const {
            // m_channelId is unique per process and not per instance
            return std::string((("OUTPUT " + util::toString(m_channelId) += " of '") += this->getInstanceIdName()) +=
                               "'");
        }


        void OutputChannel::write(const karabo::util::Hash& data, const OutputChannel::MetaData& metaData,
                                  bool copyAllData) {
            Memory::write(data, m_channelId, m_chunkId, metaData, copyAllData);
        }


        void OutputChannel::write(const karabo::util::Hash& data, bool copyAllData) {
            OutputChannel::MetaData meta(/*source*/ getInstanceIdName(), /*timestamp*/ karabo::util::Timestamp());
            Memory::write(data, m_channelId, m_chunkId, meta, copyAllData);
        }


        void OutputChannel::write(const karabo::util::Hash::Pointer& data, const OutputChannel::MetaData& metaData) {
            write(*data, metaData, false);
        }


        void OutputChannel::write(const karabo::util::Hash::Pointer& data) {
            write(*data, false);
        }


        void OutputChannel::disable() {
            {
                boost::mutex::scoped_lock lock(m_inputNetChannelsMutex);
                for (const Channel::Pointer& channel : m_inputNetChannels) {
                    // Be sure to close, even if shared_ptr still around somewhere and thus destructor won't be called
                    // by clear() below.
                    if (channel) channel->close();
                }
                m_inputNetChannels.clear();
            }
            // Also here ensure it is stopped in case clearing the shared_ptr does not call the destructor.
            if (m_dataConnection) m_dataConnection->stop();
            m_dataConnection.reset();
        }
    } // namespace xms
} // namespace karabo
