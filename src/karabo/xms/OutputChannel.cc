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

#include <chrono>
#include <exception>

#include "InputChannel.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/TableElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/NetworkInterface.hh"
#include "karabo/net/TcpChannel.hh"
#include "karabo/util/MetaTools.hh"

using namespace std::chrono;
namespace bs = boost::system;
using namespace karabo::data;
using namespace karabo::util;
using namespace karabo::data;
using namespace karabo::net;
using namespace std::string_literals; // For '"abc"s'
using std::placeholders::_1;
using std::placeholders::_2;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::xms::OutputChannel)
// Register also the constructor with an extra int flag:
KARABO_REGISTER_FOR_CONFIGURATION_ADDON(int, karabo::xms::OutputChannel);

namespace karabo {
    namespace xms {


        // Number of attempts for delayed 2nd construction phase, i.e. initializeServerConnection().
        // Seen it fail 1500 times (in a very busy host and C++ server) before adding the sleep in
        // initializeServerConnection(), so put 2000 here.
        const int kMaxServerInitializationAttempts = 2000;

        void OutputChannel::expectedParameters(karabo::data::Schema& expected) {
            using namespace karabo::data;

            STRING_ELEMENT(expected)
                  .key("noInputShared")
                  .displayedName("No Input (Shared)")
                  .description(
                        "What to do if currently none of the share-input channels is available for writing to ('queue' "
                        "is same as 'queueDrop')")
                  .options({"drop", "queue", "queueDrop", "wait"})
                  .assignmentOptional()
                  .defaultValue("drop")
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("hostname")
                  .displayedName("Hostname")
                  .description(
                        "The requested hostname to which connecting clients shall be routed to. The field can be "
                        "the string 'default' (in which case the default host name will be chosen), the name of a "
                        "host interface -such as eth0 or enp4s0, wildcards can be used- (in which case the "
                        "first interface that matches the provided string will be chosen), one of the IP addresses "
                        "of the host, or an IP range, specified in CIDR-like notation, where '123.234.0.0/24' "
                        "represents the IP range between '123.234.0.0' and '123.234.0.255' (in this case, the first IP "
                        "found that falls within the specified range will be chosen).")
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
                        "drop, wait, queueDrop")
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
                  .initialValue(std::vector<data::Hash>())
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

            STRING_ELEMENT(expected)
                  .key("validateSchema")
                  .displayedName("Validate schema")
                  .description("How often per data stream should write(..) validata data against a given schema")
                  .assignmentOptional()
                  .defaultValue("once")
                  .options({std::string("once"), std::string("always")})
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


        OutputChannel::OutputChannel(const karabo::data::Hash& config) : OutputChannel(config, 1) {}

        OutputChannel::OutputChannel(const karabo::data::Hash& config, int autoInit)
            : m_dataSchemaValidated(false),
              m_validateAlways(config.get<std::string>("validateSchema") == "always"),
              m_port(0),
              m_showConnectionsHandler([](const std::vector<Hash>&) {}),
              m_showStatisticsHandler(
                    [](const std::vector<unsigned long long>&, const std::vector<unsigned long long>&) {}),
              m_connections(),
              m_updateDeadline(karabo::net::EventLoop::getIOService()),
              m_addedThreads(0) {
            config.get("noInputShared", m_onNoSharedInputChannelAvailable);
            if (m_onNoSharedInputChannelAvailable == "queue") m_onNoSharedInputChannelAvailable += "Drop";
            config.get("port", m_port);
            config.get("updatePeriod", m_period);

            const std::string& hostname = config.get<std::string>("hostname");
            // resolve the hostname if needed
            if (hostname == "default") {
                m_hostname = boost::asio::ip::host_name();
            } else {
                m_hostname = NetworkInterface{hostname}.presentationIP();
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
                // Cannot use bind_weak in constructor but... usually it is safe to use here std::bind.
                // And in this way we ensure that onTcpConnect is properly bound with bind_weak.
                // But see the HACK in initializeServerConnection if even that is called too early.
                karabo::net::EventLoop::post(
                      std::bind(&OutputChannel::initializeServerConnection, this, kMaxServerInitializationAttempts));
            }
        }


        OutputChannel::~OutputChannel() {
            disable(); // explicitely close channels and stop connection
            Memory::unregisterChannel(m_channelId);
            EventLoop::removeThread(m_addedThreads);
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
                        std::this_thread::sleep_for(1ms);
                    }
                    std::this_thread::yield();
                    // Bare std::bind with this as in constructor.
                    karabo::net::EventLoop::post(
                          std::bind(&OutputChannel::initializeServerConnection, this, --countdown));
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

            initialize(karabo::data::Schema());
        }


        void OutputChannel::initialize(const karabo::data::Schema& dataSchema) {
            m_dataSchema = dataSchema;
            karabo::data::Hash h("type", "server", "port", m_port);
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

        karabo::data::Hash OutputChannel::getInitialConfiguration() const {
            return Hash("address", m_hostname);
        }

        std::string OutputChannel::getInstanceIdName() const {
            return (m_instanceId + ":") += m_channelName;
        }

        bool OutputChannel::hasRegisteredCopyInputChannel(const std::string& instanceId) const {
            std::lock_guard<std::mutex> lock(m_registeredInputsMutex);
            return (m_registeredCopyInputs.find(instanceId) != m_registeredCopyInputs.end());
        }


        bool OutputChannel::hasRegisteredSharedInputChannel(const std::string& instanceId) const {
            std::lock_guard<std::mutex> lock(m_registeredInputsMutex);
            return (m_registeredSharedInputs.find(instanceId) != m_registeredSharedInputs.end());
        }


        karabo::data::Hash OutputChannel::getInformation() const {
            return karabo::data::Hash("connectionType", "tcp", "hostname", m_hostname, "port", m_port);
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
                std::lock_guard<std::mutex> lock(m_inputNetChannelsMutex);
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
                                             const karabo::data::Hash& message) {
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
                 *     onSlowness (std::string) [drop/queueDrop/wait]
                 *     maxQueueLength (unsigned int; when onSlowness is queue or queueDrop)
                 */

                const std::string& instanceId = message.get<std::string>("instanceId");
                const std::string& memoryLocation = message.get<std::string>("memoryLocation");
                const std::string& dataDistribution = message.get<std::string>("dataDistribution");
                const std::string& onSlowness = message.get<std::string>("onSlowness");

                const unsigned int maxQueueLength =
                      (message.has("maxQueueLength") ? message.get<unsigned int>("maxQueueLength")
                                                     : InputChannel::DEFAULT_MAX_QUEUE_LENGTH);

                karabo::data::Hash info;
                info.set("instanceId", instanceId);
                info.set("memoryLocation", memoryLocation);
                info.set("tcpChannel", weakChannel);
                if (onSlowness == "queue") { // pre 2.19.0 version is connecting...
                    KARABO_LOG_FRAMEWORK_WARN << "For input channel " << instanceId << " overwrite outdated "
                                              << "'onSlowness' value \"queue\" by \"queueDrop\"";
                    info.set("onSlowness", "queueDrop"s);
                } else if (onSlowness == "throw") { // pre 2.19.0 MDL (or any pre 2.10.0) is connecting...
                    KARABO_LOG_FRAMEWORK_WARN << "For input channel " << instanceId << " overwrite outdated "
                                              << "'onSlowness' value \"throw\" by \"drop\"";
                    info.set("onSlowness", "drop"s);
                } else {
                    info.set("onSlowness", onSlowness);
                }
                info.set("maxQueueLength", maxQueueLength);
                info.set("queuedChunks", std::deque<int>());
                info.set("bytesRead", 0ull);
                info.set("bytesWritten", 0ull);
                info.set("sendOngoing", false);

                std::string finalSlownessForLog = info.get<std::string>("onSlowness");
                {
                    std::lock_guard<std::mutex> lockShared(m_registeredInputsMutex);
                    eraseOldChannel(m_registeredSharedInputs, instanceId, channel);
                    eraseOldChannel(m_registeredCopyInputs, instanceId, channel);

                    if (dataDistribution == "shared") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Registering shared-input channel '" << instanceId << "'";
                        m_registeredSharedInputs[instanceId] = info;
                    } else {
                        if (finalSlownessForLog == "queueDrop") {
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
                        const TcpChannel::Pointer oldTcpChannel = std::static_pointer_cast<TcpChannel>(oldChannel);
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
            std::lock_guard<std::mutex> lock(m_showConnectionsHandlerMutex);
            m_updateDeadline.cancel();
            m_connections.clear();
            {
                std::lock_guard<std::mutex> lock(m_registeredInputsMutex);
                for (const auto& idInfoPair : m_registeredSharedInputs) {
                    const Hash& channelInfo = idInfoPair.second;
                    Channel::WeakPointer wptr = channelInfo.get<Channel::WeakPointer>("tcpChannel");
                    Channel::Pointer channel = wptr.lock();
                    net::TcpChannel::Pointer tcpChannel = std::static_pointer_cast<TcpChannel>(channel);
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
                std::lock_guard<std::mutex> lock(m_registeredInputsMutex);
                for (const auto& idInfoPair : m_registeredCopyInputs) {
                    const Hash& channelInfo = idInfoPair.second;
                    Channel::WeakPointer wptr = channelInfo.get<Channel::WeakPointer>("tcpChannel");
                    Channel::Pointer channel = wptr.lock();
                    TcpChannel::Pointer tcpChannel = std::static_pointer_cast<TcpChannel>(channel);
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
                m_updateDeadline.expires_after(seconds(m_period));
                m_updateDeadline.async_wait(
                      bind_weak(&OutputChannel::updateNetworkStatistics, this, boost::asio::placeholders::error));
            }
        }


        void OutputChannel::updateNetworkStatistics(const boost::system::error_code& e) {
            if (e) return;
            if (m_period <= 0) return;

            std::lock_guard<std::mutex> lock(m_showConnectionsHandlerMutex);
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

            m_updateDeadline.expires_after(seconds(m_period));
            m_updateDeadline.async_wait(
                  bind_weak(&OutputChannel::updateNetworkStatistics, this, boost::asio::placeholders::error));
        }


        void OutputChannel::onInputAvailable(const std::string& instanceId) {
            std::unique_lock<std::mutex> lock(m_registeredInputsMutex);

            auto itIdChannelInfo = m_registeredSharedInputs.find(instanceId);
            if (itIdChannelInfo != m_registeredSharedInputs.end()) {
                Hash& channelInfo = itIdChannelInfo->second;
                if (channelInfo.get<bool>("sendOngoing")) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Early onInputAvailable for (shared) input " << instanceId
                                               << ": Still sending => postpone.";
                    EventLoop::post(bind_weak(&OutputChannel::onInputAvailable, this, instanceId));
                    return;
                }
                // First check whether instanceId unblocks something
                if (m_unblockSharedHandler) {
                    m_unblockSharedHandler(&channelInfo);
                    m_unblockSharedHandler = nullptr;
                    return;
                }
                auto itUnblockHandler = m_unblockHandlers.find(instanceId);
                if (itUnblockHandler != m_unblockHandlers.end()) {
                    itUnblockHandler->second(&channelInfo);
                    m_unblockHandlers.erase(itUnblockHandler);
                    return;
                }

                // Now check individual queue, even for load-balanced - might be endOfStream
                std::deque<int>& individualQueue = channelInfo.get<std::deque<int>>("queuedChunks");
                if (!individualQueue.empty()) {
                    asyncSendOne(individualQueue.front(), channelInfo, [debugId(this->debugId()), instanceId]() {
                        KARABO_LOG_FRAMEWORK_DEBUG_C("OutputChannel")
                              << debugId << " Written individually queued data to (shared) instance " << instanceId;
                    });
                    individualQueue.pop_front();
                    return;
                }
                if (!m_sharedLoadBalancedQueuedChunks.empty()) {
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
                    asyncSendOne(m_sharedLoadBalancedQueuedChunks.front(), channelInfo,
                                 [debugId(this->debugId()), instanceId]() {
                                     KARABO_LOG_FRAMEWORK_DEBUG_C("OutputChannel")
                                           << debugId << " Written queued data to (shared) instance " << instanceId;
                                 });
                    m_sharedLoadBalancedQueuedChunks.pop_front();
                    return;
                }
                pushShareNext(instanceId);
                KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " New (shared) input on instance " << instanceId
                                           << " available for writing ";
                return;
            }

            itIdChannelInfo = m_registeredCopyInputs.find(instanceId);
            if (itIdChannelInfo != m_registeredCopyInputs.end()) {
                Hash& channelInfo = itIdChannelInfo->second;
                if (channelInfo.get<bool>("sendOngoing")) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Early onInputAvailable for (copy) input " << instanceId
                                               << ": Still sending => postpone.";
                    EventLoop::post(bind_weak(&OutputChannel::onInputAvailable, this, instanceId));
                    return;
                }

                // Check whether instanceId unblocks a waiting copy input
                auto itunblockHandler = m_unblockHandlers.find(instanceId);
                if (itunblockHandler != m_unblockHandlers.end()) {
                    itunblockHandler->second(&channelInfo);
                    m_unblockHandlers.erase(itunblockHandler);
                    return;
                }

                std::deque<int>& queue = channelInfo.get<std::deque<int>>("queuedChunks");
                if (!queue.empty()) {
                    asyncSendOne(queue.front(), channelInfo, [debugId(this->debugId()), instanceId]() {
                        KARABO_LOG_FRAMEWORK_DEBUG_C("OutputChannel")
                              << debugId << " Written queued data to (copy) instance " << instanceId;
                    });
                    queue.pop_front();
                    return;
                }
                pushCopyNext(instanceId);
                KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " New (copied) input on instance " << instanceId
                                           << " available for writing ";
                return;
            }
            KARABO_LOG_FRAMEWORK_WARN << this->debugId() << " An input channel (" << instanceId
                                      << ") updated, but is not registered.";
        }


        void OutputChannel::onInputGone(const karabo::net::Channel::Pointer& channel,
                                        const karabo::net::ErrorCode& error) {
            using namespace karabo::net;
            const Hash tcpInfo(TcpChannel::getChannelInfo(std::static_pointer_cast<TcpChannel>(channel)));
            const std::string tcpAddress(tcpInfo.get<std::string>("remoteAddress") + ':' +
                                         toString(tcpInfo.get<unsigned short>("remotePort")));

            // Clean specific channel from bookkeeping structures and ...
            // ... clean expired entries as well (we are not expecting them but we want to be on the safe side!)
            unsigned int inputsLeft = 0;
            {
                std::lock_guard<std::mutex> lock(m_registeredInputsMutex);
                // SHARED Inputs
                for (InputChannels::iterator it = m_registeredSharedInputs.begin();
                     it != m_registeredSharedInputs.end();) {
                    Hash& channelInfo = it->second;
                    auto tcpChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();

                    // Cleaning expired for specific channels only
                    if (!tcpChannel || tcpChannel == channel) {
                        const std::string& instanceId = it->first;
                        KARABO_LOG_FRAMEWORK_INFO << getInstanceIdName() << " : Shared input channel '" << instanceId
                                                  << "' (ip/port " << (tcpChannel ? tcpAddress : "?")
                                                  << ") disconnected since '" << error.message() << "' (#"
                                                  << error.value() << ").";

                        // If the instanceId blocks something, release that
                        auto itBlockHandler = m_unblockHandlers.find(instanceId);
                        if (itBlockHandler != m_unblockHandlers.end()) {
                            // Call handler although that will try to send (and fail).
                            // That does not harm, but it unblocks and decrements chunk usage
                            itBlockHandler->second(&channelInfo);
                            m_unblockHandlers.erase(itBlockHandler);
                        }

                        // Release queued chunks
                        // shared selector case: release dedicated chunks (should be empty [or EOS] otherwise)
                        for (const int chunkId : channelInfo.get<std::deque<int>>("queuedChunks")) {
                            unregisterWriterFromChunk(chunkId);
                        }
                        // normal load-balanced case:
                        // release chunks in common queue if nothing left to send to (and also unblock)
                        if (m_registeredSharedInputs.size() == 1u) { // i.e. will erase the last element below
                            for (const int chunkId : m_sharedLoadBalancedQueuedChunks) {
                                unregisterWriterFromChunk(chunkId);
                            }
                            m_sharedLoadBalancedQueuedChunks.clear();
                            if (m_unblockSharedHandler) {
                                m_unblockSharedHandler(&channelInfo); // tries to send (and fails), but no harm
                                m_unblockSharedHandler = nullptr;
                            }
                        }

                        // Delete from input queue
                        eraseSharedInput(instanceId);
                        // Erase after last use of instanceId and queuedChunks which get invalidated here
                        it = m_registeredSharedInputs.erase(it);
                    } else {
                        ++it;
                    }
                }
                inputsLeft += m_registeredSharedInputs.size();

                // COPY Inputs
                for (InputChannels::iterator it = m_registeredCopyInputs.begin(); it != m_registeredCopyInputs.end();) {
                    Hash& channelInfo = it->second;
                    auto tcpChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();
                    if (!tcpChannel || tcpChannel == channel) {
                        const std::string& instanceId = it->first;

                        KARABO_LOG_FRAMEWORK_INFO << getInstanceIdName() << " : Copy input channel '" << instanceId
                                                  << "' (ip/port " << (tcpChannel ? tcpAddress : "?")
                                                  << ") disconnected since '" << error.message() << "' (#"
                                                  << error.value() << ").";
                        // If the instanceId blocks something, release that
                        auto itBlockHandler = m_unblockHandlers.find(instanceId);
                        if (itBlockHandler != m_unblockHandlers.end()) { // See comment above in SHARED part
                            itBlockHandler->second(&channelInfo);
                            m_unblockHandlers.erase(itBlockHandler);
                        }

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
            } // end lock of m_registeredInputsMutex
            {
                // Erase from container that keeps channel alive - and warn about inconsistencies:
                std::lock_guard<std::mutex> lock(m_inputNetChannelsMutex);
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


        void OutputChannel::pushShareNext(const std::string& instanceId) {
            if (std::find(m_shareNext.begin(), m_shareNext.end(), instanceId) == m_shareNext.end()) {
                m_shareNext.push_back(instanceId);
            }
        }


        std::string OutputChannel::popShareNext() {
            if (m_shareNext.empty()) {
                throw KARABO_LOGIC_EXCEPTION("No shared input ready to pop its id.");
            }
            std::string info = std::move(m_shareNext.front());
            m_shareNext.pop_front();
            return info;
        }

        bool OutputChannel::isShareNextEmpty() const {
            return m_shareNext.empty();
        }

        bool OutputChannel::hasSharedInput(const std::string& instanceId) {
            return (std::find(m_shareNext.begin(), m_shareNext.end(), instanceId) != m_shareNext.end());
        }


        void OutputChannel::eraseSharedInput(const std::string& instanceId) {
            auto it = std::find(m_shareNext.begin(), m_shareNext.end(), instanceId);
            if (it != m_shareNext.end()) m_shareNext.erase(it);
        }


        void OutputChannel::pushCopyNext(const std::string& instanceId) {
            m_copyNext.insert(instanceId);
        }


        bool OutputChannel::eraseCopyInput(const std::string& instanceId) {
            return (m_copyNext.erase(instanceId) > 0);
        }


        bool OutputChannel::updateChunkId() {
            try {
                m_chunkId = Memory::registerChunk(m_channelId);
                return true;
            } catch (const karabo::data::MemoryInitException&) {
                karabo::data::Exception::clearTrace();
                m_chunkId = m_invalidChunkId;
            } catch (const std::exception&) {
                KARABO_RETHROW;
            }
            return false;
        }


        void OutputChannel::awaitUpdateFuture(std::future<void>& fut, const char* which) {
            int overallSeconds = 0;

            // The caller of this method runs on any thread, likely in the common event loop, and the 'handler' to
            // unblock it (by set_value() on the promise that belongs to 'fut') will run in the same event loop. Now
            // what if all other threads are blocked on some mutex that the code that runs this method has already
            // acquired?
            // ==> We will stay blocked here because the unblock handler cannot run.
            //     As a work around we add a thread here.
            while (fut.wait_for(1s) != std::future_status::ready) {
                std::stringstream sstr;
                sstr << getInstanceIdName() << " awaiting future for '" << which << "' takes more than "
                     << ++overallSeconds << " seconds. Likely, the code calling '" << which << "' has locked a mutex "
                     << "that also all other threads in the EventLoop want to lock, but of course cannot. So the "
                     << "handler that should resolve the blocking in 'awaitUpdateFuture' cannot run.\n"
                     << "Please fix the device code, e.g. by avoding to call '" << which << "' with a mutex locked.";
                if (m_addedThreads < 10) {
                    sstr << "\n => As a rescue here, a thread is added to unblock.";
                    EventLoop::addThread();
                    ++m_addedThreads;
                } else {
                    sstr << "\n => Do not add more threads since added already " << m_addedThreads << ".";
                    KARABO_LOG_FRAMEWORK_WARN << sstr.str();
                    break;
                }
                KARABO_LOG_FRAMEWORK_WARN << sstr.str();
            }
            fut.get();
        }

        void OutputChannel::update(bool safeNDArray) {
            auto prom = std::make_shared<std::promise<void>>();
            auto fut = prom->get_future();
            auto handler = [prom{std::move(prom)}]() { prom->set_value(); };

            asyncUpdateNoWait([]() {}, handler, safeNDArray);
            awaitUpdateFuture(fut, "update");
        }

        void OutputChannel::asyncUpdate(bool safeNDArray, std::function<void()>&& writeDoneHandler) {
            auto prom = std::make_shared<std::promise<void>>();
            auto fut = prom->get_future();
            auto readyForNextHandler = [prom{std::move(prom)}]() { prom->set_value(); };

            asyncUpdateNoWait(readyForNextHandler, std::move(writeDoneHandler), safeNDArray);
            awaitUpdateFuture(fut, "asyncUpdate");
        }

        void OutputChannel::asyncUpdateNoWait(std::function<void()>&& readyForNextHandler,
                                              std::function<void()>&& writeDoneHandler, bool safeNDArray) {
            // If no data was written and not endOfStream: nothing to do
            if (Memory::size(m_channelId, m_chunkId) == 0 && !Memory::isEndOfStream(m_channelId, m_chunkId)) {
                karabo::net::EventLoop::post(std::move(readyForNextHandler));
                karabo::net::EventLoop::post(std::move(writeDoneHandler));
                return;
            }

            // Take current chunkId for sending and get already next one
            unsigned int chunkId = m_chunkId;
            updateChunkId(); // if this fails, m_chunkId is set to m_invalidChunkId (i.e. queues are full)

            std::vector<Hash*> toSendImmediately, toQueue, toBlock;
            bool blockForShared = false, queueForShared = false;

            std::unique_lock<std::mutex> lock(m_registeredInputsMutex);
            // Need to keep this mutex lock as long as we deal with anything filled into the above containers
            asyncPrepareCopy(chunkId, toSendImmediately, toQueue, toBlock);
            asyncPrepareDistribute(chunkId, toSendImmediately, toQueue, toBlock, queueForShared, blockForShared);

            // Check whether safety copy is needed and if so do that
            if (!safeNDArray) {
                auto hasLocal = [](const std::vector<Hash*>& channelInfos) {
                    for (const Hash* channelInfo : channelInfos) {
                        if (channelInfo->get<std::string>("memoryLocation") == "local") return true;
                    }
                    return false;
                };
                // blockForShared case taken care of later - we cannot yet know whether input channel is local
                if (!toQueue.empty() || queueForShared || hasLocal(toSendImmediately) || hasLocal(toBlock)) {
                    Memory::assureAllDataIsCopied(m_channelId, chunkId);
                }
            }

            // First queue where needed
            for (Hash* channelInfo : toQueue) {
                Memory::incrementChunkUsage(m_channelId, chunkId);
                channelInfo->get<std::deque<int>>("queuedChunks").push_back(chunkId);
            }
            if (queueForShared) {
                Memory::incrementChunkUsage(m_channelId, chunkId);
                m_sharedLoadBalancedQueuedChunks.push_back(chunkId);
            }
            // If all connected want to drop or queue: nothing to do
            if (toSendImmediately.empty() && toBlock.empty() && !blockForShared) {
                karabo::net::EventLoop::post(std::move(readyForNextHandler));
                karabo::net::EventLoop::post(std::move(writeDoneHandler));
            } else {
                // Prepare handler
                const size_t numBlock = toBlock.size() + blockForShared;
                const size_t numToWrite = toSendImmediately.size() + numBlock;
                auto doneFlags = std::make_shared<std::vector<bool>>(numToWrite, false);
                auto doneFlagMutex = std::make_shared<std::mutex>();
                std::function<void(size_t)> singleWriteDone =
                      [doneFlags{std::move(doneFlags)}, doneFlagMutex{std::move(doneFlagMutex)},
                       writeDoneHandler{std::move(writeDoneHandler)}](size_t n) mutable {
                          std::lock_guard<std::mutex> lock(*doneFlagMutex);
                          (*doneFlags)[n] = true;
                          for (const bool ready : *doneFlags) {
                              if (!ready) return;
                          }
                          // lock.unlock(); irrelevant since it is anyway the last time this mutex is used
                          writeDoneHandler();
                      };
                // Now send data to those that are ready immediately
                size_t counter = 0;
                for (Hash* channelInfo : toSendImmediately) {
                    Memory::incrementChunkUsage(m_channelId, chunkId);
                    asyncSendOne(chunkId, *channelInfo, std::bind(singleWriteDone, counter++)); // post-fix increment!
                }

                if (numBlock) {
                    // Finally register handlers for blocking receivers
                    auto doneBlockFlags = std::make_shared<std::vector<bool>>(numBlock, false);
                    auto doneBlockFlagMutex = std::make_shared<std::mutex>();
                    // Technical comment: I failed to compile it with 'Hash&', maybe (!) related to
                    // https://www.boost.org/doc/libs/1_82_0/libs/bind/doc/html/bind.html#bind.limitations
                    // Works with 'const Hash&' and casting away the const downstream where needed...
                    std::function<void(Hash*, size_t, size_t, bool)> singleUnblockTrigger =
                          [this, chunkId, singleWriteDone{std::move(singleWriteDone)},
                           doneBlockFlags{std::move(doneBlockFlags)}, doneBlockFlagMutex{std::move(doneBlockFlagMutex)},
                           readyForNext{std::move(readyForNextHandler)}](Hash* channelInfo, size_t blockCounter,
                                                                         size_t allCounter, bool copyIfLocal) {
                              // Capturing 'this' OK: lambda will be directly called (not posted) by a valid 'this'
                              if (copyIfLocal && channelInfo->get<std::string>("memoryLocation") == "local") {
                                  Memory::assureAllDataIsCopied(m_channelId, chunkId);
                              }
                              this->asyncSendOne(chunkId, *channelInfo, std::bind(singleWriteDone, allCounter));
                              // Check whether all blockings are resolved and if so call handler that waits for that
                              std::lock_guard<std::mutex> lock(*doneBlockFlagMutex);
                              (*doneBlockFlags)[blockCounter] = true;
                              for (const bool ready : *doneBlockFlags) {
                                  if (!ready) return;
                              }
                              readyForNext(); // Last sending launched
                          };
                    size_t blockCounter = 0;
                    if (blockForShared) {
                        Memory::incrementChunkUsage(m_channelId, chunkId);
                        // If NDArray not safe, need to request copy if input channel will be local
                        m_unblockSharedHandler =
                              std::bind(singleUnblockTrigger, _1, blockCounter++, counter++, !safeNDArray);
                    }

                    for (const Hash* channelInfo : toBlock) {
                        Memory::incrementChunkUsage(m_channelId, chunkId);
                        const std::string& instanceId = channelInfo->get<std::string>("instanceId");
                        // Last argument false since any needed copy was done already before
                        m_unblockHandlers[instanceId] =
                              std::bind(singleUnblockTrigger, _1, blockCounter++, counter++, false);
                    }
                } else {
                    // no blocking, so ready immediately
                    EventLoop::post(std::move(readyForNextHandler));
                }
            }
            // Finally unregister ourselves and ensure for next round
            unregisterWriterFromChunk(chunkId);
            ensureValidChunkId(lock);
        }

        void OutputChannel::asyncPrepareCopy(unsigned int chunkId, std::vector<Hash*>& toSendImmediately,
                                             std::vector<Hash*>& toQueue, std::vector<Hash*>& toBlock) {
            toSendImmediately.reserve(m_registeredCopyInputs.size() + 1); // +1 for potential distribute case?
            toQueue.reserve(toSendImmediately.size());
            toBlock.reserve(toQueue.size());
            for (auto& idChannelInfo : m_registeredCopyInputs) {
                InputChannelInfo& channelInfo = idChannelInfo.second;
                const std::string& instanceId = idChannelInfo.first;
                std::string onSlowness(channelInfo.get<std::string>("onSlowness")); // by value on purpose

                if (eraseCopyInput(instanceId)) { // Ready for data
                    toSendImmediately.push_back(&channelInfo);
                    continue;
                }
                const bool isEos = Memory::isEndOfStream(m_channelId, chunkId);
                if (isEos && onSlowness == "drop") { // EOS must never get lost
                    onSlowness = "queueDrop";
                }

                if (onSlowness == "drop") {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Dropping (copied) data package for "
                                               << instanceId;
                } else if (onSlowness == "queueDrop") {
                    if (isEos || // Really never drop EOS!
                        (m_chunkId != m_invalidChunkId &&
                         static_cast<unsigned int>(channelInfo.get<std::deque<int>>("queuedChunks").size()) <
                               channelInfo.get<unsigned int>("maxQueueLength"))) {
                        // i.e. EOS or all fine with queue length
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing (copied) "
                                                   << (isEos ? "EOS" : "data") << " package for " << instanceId
                                                   << ", chunk " << chunkId;
                        toQueue.push_back(&channelInfo);
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queue-dropping (copied) data package for "
                                                   << instanceId;
                    }
                } else if (onSlowness == "wait") {
                    toBlock.push_back(&channelInfo);
                } else { // We should never be here!!
                    throw KARABO_LOGIC_EXCEPTION("Output channel copy case internally misconfigured: " + onSlowness);
                }
            }
        }

        void OutputChannel::asyncPrepareDistribute(unsigned int chunkId, std::vector<Hash*>& toSendImmediately,
                                                   std::vector<Hash*>& toQueue, std::vector<Hash*>& toBlock,
                                                   bool& queue, bool& block) {
            queue = block = false;
            if (!m_registeredSharedInputs.empty()) {
                if (Memory::isEndOfStream(m_channelId, chunkId)) {
                    queue = asyncPrepareDistributeEos(chunkId, toSendImmediately, toQueue, toBlock);
                } else if (m_sharedInputSelector) {
                    asyncPrepareDistributeSelected(chunkId, toSendImmediately, toQueue, toBlock);
                } else {
                    asyncPrepareDistributeLoadBal(chunkId, toSendImmediately, toQueue, toBlock, queue, block);
                }
            }
        }

        bool OutputChannel::asyncPrepareDistributeEos(unsigned int chunkId, std::vector<Hash*>& toSendImmediately,
                                                      std::vector<Hash*>& toQueue, std::vector<Hash*>& toBlock) {
            // Despite sharing, each input should receive endOfStream

            // But for load-balanced, the shared queue has to be worked on before
            if (!m_sharedLoadBalancedQueuedChunks.empty()) {
                return true;
            }

            for (auto& idChannelInfo : m_registeredSharedInputs) { // not const since queue might be extended

                InputChannelInfo& channelInfo = idChannelInfo.second;
                const std::string& instanceId = idChannelInfo.first;

                if (hasSharedInput(instanceId)) {
                    eraseSharedInput(instanceId);
                    toSendImmediately.push_back(&channelInfo);
                } else {
                    // We queue the EOS so it is sent immediately when the input is ready to receive data.
                    //
                    // No need to care about full queue - ensureValidChunkId() at the end of update() will care.
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing endOfStream for shared input "
                                               << instanceId;
                    if (m_onNoSharedInputChannelAvailable == "wait") {
                        toBlock.push_back(&channelInfo);
                    } else { // also for "drop" - EOS must never get lost
                        toQueue.push_back(&channelInfo);
                    }
                }
            }
            return false;
        }

        void OutputChannel::asyncPrepareDistributeSelected(unsigned int chunkId, std::vector<Hash*>& toSendImmediately,
                                                           std::vector<Hash*>& toQueue, std::vector<Hash*>& toBlock) {
            // Prepare to call selector
            std::vector<std::string> allInputIds;
            allInputIds.reserve(m_registeredSharedInputs.size());
            for (auto& keyValue : m_registeredSharedInputs) {
                allInputIds.push_back(keyValue.first);
            }
            const std::string nextInputId = m_sharedInputSelector(allInputIds);

            InputChannels::iterator itIdChannelInfo = m_registeredSharedInputs.find(nextInputId);
            if (itIdChannelInfo == m_registeredSharedInputs.end()) {
                KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Dropping (shared) data package with chunkId "
                                           << chunkId << " since selected input '" << nextInputId
                                           << "' is not among shared inputs";
                return;
            }
            karabo::data::Hash& channelInfo = itIdChannelInfo->second;
            const std::string& instanceId = itIdChannelInfo->first; // channelInfo.get<std::string>("instanceId");

            if (hasSharedInput(instanceId)) { // Found
                eraseSharedInput(instanceId);
                toSendImmediately.push_back(&channelInfo);
            } else { // Not found
                if (m_onNoSharedInputChannelAvailable == "drop") {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                               << " Dropping (shared) data package with chunkId: " << chunkId;
                } else if (m_onNoSharedInputChannelAvailable == "queueDrop") {
                    if (m_chunkId != m_invalidChunkId) { // i.e. all fine with queue length
                        // TODO: We could also consider the maxQueueLength of instanceId.
                        // We queue for exactly this one.
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                                   << " Queuing (shared) data package with chunkId: " << chunkId;
                        toQueue.push_back(&channelInfo);
                    } else { // queue full, so drop
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queue-dropping (shared) data package for "
                                                   << instanceId;
                    }
                } else if (m_onNoSharedInputChannelAvailable == "wait") {
                    toBlock.push_back(&channelInfo);
                } else { // We should never be here!!
                    throw KARABO_LOGIC_EXCEPTION("Output channel shared case internally misconfigured: " +
                                                 m_onNoSharedInputChannelAvailable);
                }

            } // end else - not found
        }


        void OutputChannel::asyncPrepareDistributeLoadBal(unsigned int chunkId, std::vector<Hash*>& toSendImmediately,
                                                          std::vector<Hash*>& toQueue, std::vector<Hash*>& toBlock,
                                                          bool& queue, bool& block) {
            if (!isShareNextEmpty()) { // Found
                const std::string instanceId = popShareNext();
                auto it = m_registeredSharedInputs.find(instanceId);
                if (it == m_registeredSharedInputs.end()) { // paranoia check
                    // Should never come here...
                    KARABO_LOG_FRAMEWORK_ERROR << "Next load balanced input '" << instanceId
                                               << "' does not exist anymore!";
                } else {
                    karabo::data::Hash& channelInfo = it->second;
                    toSendImmediately.push_back(&channelInfo);
                }
            } else { // Not found
                if (m_onNoSharedInputChannelAvailable == "drop") {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                                               << " Dropping (shared) data package with chunkId: " << chunkId;
                } else if (m_onNoSharedInputChannelAvailable == "queueDrop") {
                    if (m_chunkId != m_invalidChunkId) { // i.e. all fine with queue length
                        KARABO_LOG_FRAMEWORK_DEBUG
                              << this->debugId()
                              << " Placing chunk in single queue (load-balanced distribution mode): " << chunkId;
                        queue = true;
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queue-dropping (shared) data package with "
                                                   << "chunkId " << chunkId << " (single queue full)";
                    }
                } else if (m_onNoSharedInputChannelAvailable == "wait") {
                    block = true;
                } else {
                    // We should never be here!!
                    throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured: " +
                                                 m_onNoSharedInputChannelAvailable);
                }
            } // end else - not found
        }

        void OutputChannel::ensureValidChunkId(std::unique_lock<std::mutex>& lockOfRegisteredInputsMutex) {
            // If still invalid, drop from queueDrop channels until resources freed and valid chunkId available.

            // Loop until dropping from "queueDrop" queues makes a valid chunkId available.
            while (m_chunkId == m_invalidChunkId) {
                if (updateChunkId()) {
                    return;
                }
                // free resources: loop copy channels and drop the oldest chunk (if channel is "queueDrop")
                auto dropIndividualQueues = [this](InputChannels& channels) {
                    for (auto& idChannelInfo : channels) {
                        Hash& channelInfo = idChannelInfo.second;
                        auto& queuedChunks = channelInfo.get<std::deque<int>>("queuedChunks");
                        for (auto it = queuedChunks.begin(); it != queuedChunks.end(); ++it) {
                            const int chunkId = *it;
                            if (!Memory::isEndOfStream(this->m_channelId, chunkId)) { // Never drop EOS
                                unregisterWriterFromChunk(chunkId);
                                queuedChunks.erase(it); // invalidates 'it', but we dreturn immediately below anyway
                                KARABO_LOG_FRAMEWORK_DEBUG << "Drop from queue for channel '" << idChannelInfo.first
                                                           << "', queue's new size is " << queuedChunks.size();
                                return true; // only drop one from this queue
                            }
                        }
                    }
                    return false; // nothing dropped
                };
                if (dropIndividualQueues(m_registeredCopyInputs)) continue;
                // free resources: loop shared channels queues and their common queue
                if (dropIndividualQueues(m_registeredSharedInputs)) continue;
                bool dropped = false;
                for (auto it = m_sharedLoadBalancedQueuedChunks.begin(); it != m_sharedLoadBalancedQueuedChunks.end();
                     ++it) {
                    const int chunkId = *it;
                    if (!Memory::isEndOfStream(m_channelId, chunkId)) { // never drop EOS
                        unregisterWriterFromChunk(chunkId);
                        m_sharedLoadBalancedQueuedChunks.erase(it);
                        KARABO_LOG_FRAMEWORK_DEBUG << "Drop from load-balanced queue, queue's new size is "
                                                   << m_sharedLoadBalancedQueuedChunks.size();
                        dropped = true;
                        break; // for loop on shared load balanced queue
                    }
                }
                if (dropped) continue; // while

                // Give queues a chance to shrink via onInputAvailable
                lockOfRegisteredInputsMutex.unlock();
                std::this_thread::sleep_for(1ms);
                lockOfRegisteredInputsMutex.lock();
            }
        }

        void OutputChannel::signalEndOfStream() {
            auto prom = std::make_shared<std::promise<void>>();
            auto fut = prom->get_future();
            auto handler = [prom{std::move(prom)}]() { prom->set_value(); };

            asyncSignalEndOfStream(handler);

            awaitUpdateFuture(fut, "signalEndOfStream");
        }


        void OutputChannel::asyncSignalEndOfStream(std::function<void()>&& readyForNextHandler) {
            m_dataSchemaValidated = false; // Reset and thus validate next stream
            // If there is still some data in the pipe, put it out
            if (Memory::size(m_channelId, m_chunkId) > 0) {
                // Lambda for what to do when data is out
                auto finalSignal = [weakThis{weak_from_this()},
                                    doneHandler = std::move(readyForNextHandler)]() mutable {
                    auto self(weakThis.lock());
                    if (self) {
                        // Mark next chunk as EOS and send it out
                        Memory::setEndOfStream(self->m_channelId, self->m_chunkId);
                        self->asyncUpdate(true, std::move(doneHandler)); // move requires the above mutable
                    } else {
                        doneHandler();
                    }
                };
                asyncUpdate(false, std::move(finalSignal)); // safeNDArray false since we do not know what is in
            } else {
                Memory::setEndOfStream(m_channelId, m_chunkId);
                asyncUpdate(true, std::move(readyForNextHandler)); // safeNDArray true - there is only EOS
            }
        }


        void OutputChannel::registerShowConnectionsHandler(const ShowConnectionsHandler& handler) {
            std::lock_guard<std::mutex> lock(m_showConnectionsHandlerMutex);
            if (!handler) {
                m_showConnectionsHandler = [](const std::vector<karabo::data::Hash>&) {};
            } else {
                m_showConnectionsHandler = handler;
            }
        }


        void OutputChannel::registerShowStatisticsHandler(const ShowStatisticsHandler& handler) {
            std::lock_guard<std::mutex> lock(m_showConnectionsHandlerMutex);
            if (!handler) {
                m_showStatisticsHandler = [](const std::vector<unsigned long long>&,
                                             const std::vector<unsigned long long>&) {};
            } else {
                m_showStatisticsHandler = handler;
            }
        }
        void OutputChannel::registerSharedInputSelector(SharedInputSelector&& selector) {
            std::lock_guard<std::mutex> lock(m_registeredInputsMutex);
            m_sharedInputSelector = std::move(selector);
        }


        void OutputChannel::unregisterWriterFromChunk(int chunkId) {
            Memory::decrementChunkUsage(m_channelId, chunkId);
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT " << Memory::getChunkStatus(m_channelId, chunkId)
                                       << " uses left for [" << m_channelId << "][" << chunkId << "]";
        }

        void OutputChannel::resetSendOngoing(const std::string& instanceId) {
            std::lock_guard<std::mutex> lock(m_registeredInputsMutex);
            auto it = m_registeredCopyInputs.find(instanceId);
            if (it != m_registeredCopyInputs.end()) {
                it->second.set("sendOngoing", false);
            } else {
                it = m_registeredSharedInputs.find(instanceId);
                if (it != m_registeredSharedInputs.end()) {
                    it->second.set("sendOngoing", false);
                } // else: has disconnected!
            }
        }

        void OutputChannel::asyncSendOne(unsigned int chunkId, InputChannelInfo& channelInfo,
                                         std::function<void()>&& doneHandler) {
            const bool local = (channelInfo.get<std::string>("memoryLocation") == "local"); // else 'remote'
            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << "Async send chunk "
                                       << chunkId // << (isEos ? " (EOS)" : "")
                                       << " to " << (local ? "local" : "remote") << " input "
                                       << channelInfo.get<std::string>("instanceId");

            Channel::Pointer tcpChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();
            if (tcpChannel && tcpChannel->isOpen()) { // isOpen needed? Would it fail?
                karabo::data::Hash header;
                if (local) {
                    header.set("channelId", m_channelId);
                    header.set("chunkId", chunkId);
                }
                const bool isEos = Memory::isEndOfStream(m_channelId, chunkId);
                if (isEos) {
                    header.set("endOfStream", true);
                }
                std::vector<karabo::data::BufferSet::Pointer> data;
                if (!isEos && !local) {
                    Memory::readIntoBuffers(data, header, m_channelId, chunkId); // Note: clears 'header'
                }
                Channel::WriteCompleteHandler handler = [weakThis{weak_from_this()},
                                                         instanceId{channelInfo.get<std::string>("instanceId")},
                                                         channelId{m_channelId}, chunkId, local,
                                                         doneHandler{std::move(doneHandler)}](
                                                              const boost::system::error_code& ec) {
                    if (auto self = weakThis.lock()) {
                        self->resetSendOngoing(instanceId);
                    }
                    if (!local || ec) {
                        // local InputChannel will take over responsibility of chunk
                        // CAVEAT: If the other end disconnects after receiving, but before processing our message,
                        //         the chunk is leaked!
                        //         But it is an unlikely scenario that a local receiver disconnects often - usually
                        //         the full process including the sender (i.e. we here) is shutdown.
                        Memory::decrementChunkUsage(channelId, chunkId); // i.e. unregisterWriterFromChunk(chunkId);
                    }
                    doneHandler();
                };
                channelInfo.set("sendOngoing", true);
                // 1) The memory behind 'data' is kept alive until 'handler' is called by keeping chunkId available and
                //    only decrement its usage in the 'handler'.
                // 2) Marking an input as available for more data is postponed in onInputAvailable for the case that
                //    "sendOngoing" is still true. That ensures that writeAsyncHashVectorBufferSetPointer is not called
                //    before a previous call for the same tcpChannel has completed.
                tcpChannel->writeAsyncHashVectorBufferSetPointer(header, data, handler);
            } else {
                Memory::decrementChunkUsage(m_channelId, chunkId); // i.e. unregisterWriterFromChunk(chunkId);
                KARABO_LOG_FRAMEWORK_WARN << "asyncSendOne failed - channel " << (tcpChannel ? "not open" : "gone");
                EventLoop::post(std::move(doneHandler));
            }
        }


        std::string OutputChannel::debugId() const {
            // m_channelId is unique per process and not per instance
            return std::string((("OUTPUT " + data::toString(m_channelId) += " of '") += this->getInstanceIdName()) +=
                               "'");
        }


        void OutputChannel::write(const karabo::data::Hash& data, const OutputChannel::MetaData& metaData) {
            if (!m_dataSchema.empty() && (!m_dataSchemaValidated || m_validateAlways)) {
                // These rules are tested in Validator_Test::testNDArray().
                karabo::data::Validator::ValidationRules rules;
                rules.allowAdditionalKeys = false;
                rules.allowMissingKeys = false;
                rules.allowUnrootedConfiguration = true;
                rules.injectDefaults = false;
                rules.injectTimestamps = false;
                rules.strict = true; // to validate readOnly
                karabo::data::Validator val(rules);

                karabo::data::Hash dummy; // strict validation: stays empty
                const std::pair<bool, std::string> res = val.validate(m_dataSchema, data, dummy);
                if (!res.first) {
                    KARABO_LOGGING_ERROR("{} - write(..) failed validating data paths '{}' vs schema paths '{}': {}",
                                         getInstanceIdName(), toString(data.getPaths()),
                                         toString(m_dataSchema.getPaths()), res.second);
                    throw KARABO_PARAMETER_EXCEPTION(m_channelName + " - data/schema mismatch: " + res.second);
                }
                if (!m_dataSchemaValidated) { // Not for every write if m_validateAlways
                    KARABO_LOGGING_INFO("{} - write(..) validated data paths '{}' vs schema paths '{}'", m_channelName,
                                        toString(data.getPaths()), toString(m_dataSchema.getPaths()));
                }
                m_dataSchemaValidated = true; // after successfull validation
            }
            Memory::write(data, m_channelId, m_chunkId, metaData, false);
        }


        void OutputChannel::write(const karabo::data::Hash& data) {
            OutputChannel::MetaData meta(/*source*/ getInstanceIdName(), /*timestamp*/ karabo::data::Timestamp());
            write(data, meta);
        }


        void OutputChannel::disable() {
            {
                std::lock_guard<std::mutex> lock(m_inputNetChannelsMutex);
                for (const Channel::Pointer& channel : m_inputNetChannels) {
                    // Be sure to close, even if shared_ptr still around somewhere and thus destructor won't be
                    // called by clear() below.
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
