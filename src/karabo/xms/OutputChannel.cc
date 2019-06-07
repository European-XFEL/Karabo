/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <exception>
#include <boost/pointer_cast.hpp>
#include "OutputChannel.hh"
#include "karabo/net/TcpChannel.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/net/EventLoop.hh"

namespace bs = boost::system;
using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::net;


namespace karabo {
    namespace xms {

        KARABO_REGISTER_FOR_CONFIGURATION(OutputChannel);

        void OutputChannel::expectedParameters(karabo::util::Schema& expected) {
            using namespace karabo::util;


            STRING_ELEMENT(expected).key("distributionMode")
                    .displayedName("Distribution Mode")
                    .description("Describes the policy of how to fan-out data to multiple (shared) input channels")
                    .options("load-balanced,round-robin")
                    .assignmentOptional().defaultValue("load-balanced")
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("noInputShared")
                    .displayedName("No Input (Shared)")
                    .description("What to do if currently no share-input channel is available for writing to")
                    .options("drop,queue,throw,wait")
                    .assignmentOptional().defaultValue("wait")
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("hostname")
                    .displayedName("Hostname")
                    .description("The hostname to which connecting clients will be routed to")
                    .assignmentOptional().defaultValue("default")
                    .commit();


            INT32_ELEMENT(expected).key("compression")
                    .displayedName("Compression")
                    .description("Configures when the data is compressed (-1 = off, 0 = always, >0 = threshold in MB")
                    .expertAccess()
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .assignmentOptional().defaultValue(-1)
                    .commit();

            UINT32_ELEMENT(expected).key("port")
                    .displayedName("Port")
                    .description("Port number for TCP connection")
                    .expertAccess()
                    .assignmentOptional().defaultValue(0)
                    .init()
                    .commit();

            Schema columns;

            STRING_ELEMENT(columns).key("remoteId")
                    .displayedName("Remote ID")
                    .description("Id of remote input channel")
                    .readOnly()
                    .commit();

            STRING_ELEMENT(columns).key("dataDistribution")
                    .displayedName("Distribution")
                    .description("Data distribution behavior by input channel: shared or copy")
                    .readOnly()
                    .commit();

            STRING_ELEMENT(columns).key("onSlowness")
                    .displayedName("On slowness")
                    .description("Data handling policy in case of slowness if data Distribution is copy: drop, wait, queue, throw")
                    .readOnly()
                    .commit();

            STRING_ELEMENT(columns).key("memoryLocation")
                    .displayedName("MemoryLocation")
                    .description("Cache Memory class location: can be remote or local")
                    .readOnly()
                    .commit();

            STRING_ELEMENT(columns).key("remoteAddress")
                    .displayedName("Remote IP")
                    .description("Remote TCP address of active connection")
                    .readOnly()
                    .commit();

            UINT16_ELEMENT(columns).key("remotePort")
                    .displayedName("Remote port")
                    .description("Remote TCP port of active connection")
                    .readOnly()
                    .commit();

            STRING_ELEMENT(columns).key("localAddress")
                    .displayedName("Local IP")
                    .description("Local TCP address of active connection")
                    .readOnly()
                    .commit();

            UINT16_ELEMENT(columns).key("localPort")
                    .displayedName("Local port")
                    .description("Local TCP port of active connection")
                    .readOnly()
                    .commit();

            TABLE_ELEMENT(expected).key("connections")
                    .displayedName("Connections")
                    .description("Table of active connections")
                    .setColumns(columns)
                    .assignmentOptional().defaultValue(std::vector<util::Hash>())
                    .expertAccess()
                    .commit();

            INT32_ELEMENT(expected).key("updatePeriod")
                    .displayedName("Update period")
                    .description("Time period for updating network statistics of OutputChannel")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(10)
                    .expertAccess()
                    .commit();

            VECTOR_UINT64_ELEMENT(expected).key("bytesRead")
                    .displayedName("Read bytes")
                    .description("Vector of bytes read so far per connection taken from 'connections' table.")
                    .readOnly()
                    .expertAccess()
                    .archivePolicy(Schema::NO_ARCHIVING)
                    .commit();

            VECTOR_UINT64_ELEMENT(expected).key("bytesWritten")
                    .displayedName("Written bytes")
                    .description("Vector of bytes written so far per connection taken from 'connections' table.")
                    .readOnly()
                    .expertAccess()
                    .archivePolicy(Schema::NO_ARCHIVING)
                    .commit();

        }


        OutputChannel::OutputChannel(const karabo::util::Hash& config)
                : m_port(0)
                , m_sharedInputIndex(0)
                , m_toUnregisterSharedInput(false)
                , m_showConnectionsHandler([](const std::vector<Hash>&) {})
                , m_showStatisticsHandler([](const std::vector<unsigned long long>&, const std::vector<unsigned long long>&) {})
                , m_connections()
                , m_updateDeadline(karabo::net::EventLoop::getIOService()) {
            //KARABO_LOG_FRAMEWORK_DEBUG << "*** OutputChannel::OutputChannel CTOR ***";
            config.get("distributionMode", m_distributionMode);
            config.get("noInputShared", m_onNoSharedInputChannelAvailable);
            config.get("hostname", m_hostname);
            config.get("port", m_port);
            if (m_hostname == "default") m_hostname = boost::asio::ip::host_name();
            config.get("compression", m_compression);
            config.get("updatePeriod", m_period);

            KARABO_LOG_FRAMEWORK_DEBUG << "NoInputShared: " << m_onNoSharedInputChannelAvailable;

            // Memory related
            try {
                m_channelId = Memory::registerChannel();
                m_chunkId = Memory::registerChunk(m_channelId);
            } catch (...) {
                KARABO_RETHROW
            }

            KARABO_LOG_FRAMEWORK_DEBUG << "Outputting data on channel " << m_channelId << " and chunk " << m_chunkId;

            // Initialize server connectivity:
            // Cannot use bind_weak in constructor but... usually it is safe to use here boost::bind.
            // And in this way we ensure that onTcpConnect is properly bound with bind_weak.
            // But see the HACK in initializeServerConnection if even that is called too early.
            // Once (only) seen it fail > 500 times (with 128 instantiateNoWait...), so put 1500 here.
            karabo::net::EventLoop::getIOService().post(boost::bind(&OutputChannel::initializeServerConnection, this, 1500));
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
            const Self::Pointer sharedSelf(weak_from_this().lock()); // Promote to shared_ptr. and keep alive for bind_weak
            if (!sharedSelf) {
                if (countdown > 0) {
                    KARABO_LOG_FRAMEWORK_DEBUG << "initializeServerConnection: no shared_ptr yet, try again up to "
                            << countdown << " more times"; // Unfortunately, m_instanceId cannot be filled yet.
                    // Let other threads potentially take over to increase the chance that shared_ptr is there next time:
                    boost::this_thread::yield();
                    // Bare boost::bind with this as in constructor.
                    karabo::net::EventLoop::getIOService().post(boost::bind(&OutputChannel::initializeServerConnection,
                                                                            this, --countdown));
                    return;
                } else {
                    const std::string msg("Give up to initialize server connection! Better recreate channel, e.g. by "
                                          "re-instantiating device.");
                    KARABO_LOG_FRAMEWORK_ERROR << msg;
                    throw KARABO_NETWORK_EXCEPTION(msg);
                }
            }
            // HACK ends

            karabo::util::Hash h("type", "server", "port", m_port, "compressionUsageThreshold", m_compression * 1E6);
            Connection::Pointer connection = Connection::create("Tcp", h);
            // The following call can throw in case you provide with configuration's Hash the none-zero port number
            // and this port number is already used in the system, for example, by another application.
            // Or when bind_weak tries to create a shared_ptr, but fails - which we try to prevent above...
            try {
                m_port = connection->startAsync(bind_weak(&karabo::xms::OutputChannel::onTcpConnect, this, _1, _2));
            } catch (const std::exception& ex) {
                std::ostringstream oss;
                oss << "Could not start TcpServer for output channel (\"" << m_channelId
                        <<  "\", port = " << m_port << ") : " << ex.what();
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


        bool OutputChannel::hasRegisteredCopyInputChannel(const std::string& instanceId) const {
            boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
            return (m_registeredCopyInputs.find(instanceId) != m_registeredCopyInputs.end());
        }


        bool OutputChannel::hasRegisteredSharedInputChannel(const std::string& instanceId) const {
            boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
            return (m_registeredSharedInputs.find(instanceId) != m_registeredSharedInputs.end());
        }


        void OutputChannel::registerIOEventHandler(const boost::function<void (const OutputChannel::Pointer&)>& ioEventHandler) {
            m_ioEventHandler = ioEventHandler;
        }


        karabo::util::Hash OutputChannel::getInformation() const {
            return karabo::util::Hash("connectionType", "tcp", "hostname", m_hostname, "port", m_port);
        }


        void OutputChannel::onTcpConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
            using namespace karabo::net;

            switch (ec.value()) {
                    // Expected when io_service is stopped ... normal shutdown
                case bs::errc::no_such_file_or_directory: // End-of-file, code 2
                case bs::errc::operation_canceled: // code 125 because of io_service was stopped?
                    return;
                    // Accepting the new connection ...
                case bs::errc::success:
                    break;
                    // "Retry" behavior because of following reasons
                case bs::errc::resource_unavailable_try_again: // temporary(?) problems with some resources   - retry
                case bs::errc::interrupted: // The system call was interrupted by a signal  - retry
                case bs::errc::protocol_error:
                case bs::errc::host_unreachable:
                case bs::errc::network_unreachable:
                case bs::errc::network_down:
                {
                    // The server should always wait for other connection attempts ...
                    if (m_dataConnection) {
                        m_dataConnection->startAsync(bind_weak(&karabo::xms::OutputChannel::onTcpConnect, this, _1, _2));
                        KARABO_LOG_FRAMEWORK_WARN << "onTcpConnect received error code " << ec.value() << " (i.e. '"
                                << ec.message() << "'). Wait for new connections ...";
                    }
                    return;
                }
                    // These error resulting in "dead" server.  They should be considered by developer.
                default:
                {
                    KARABO_LOG_FRAMEWORK_ERROR << "onTcpConnect received error code " << ec.value() << " (i.e. '"
                            << ec.message() << "'). Clients cannot connect anymore to this server! Developer's intervention is required!";
                    return;
                }
            }
            // Prepare to accept more connections
            if (m_dataConnection) m_dataConnection->startAsync(bind_weak(&karabo::xms::OutputChannel::onTcpConnect, this, _1, _2));
            KARABO_LOG_FRAMEWORK_DEBUG << "***** Connection established *****";
            {
                // Move responsibility to keep channel alive to one place
                boost::mutex::scoped_lock lock(m_inputNetChannelsMutex);
                m_inputNetChannels.insert(channel);
            }
            channel->readAsyncHash(bind_weak(&karabo::xms::OutputChannel::onTcpChannelRead, this,
                                             _1, Channel::WeakPointer(channel), _2));
        }


        void OutputChannel::onTcpChannelError(const karabo::net::ErrorCode& error, const karabo::net::Channel::Pointer& channel) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Tcp channel error on \"" << m_instanceId << "\", code #" << error.value() << " -- \""
                    << error.message() << "\".  Close channel at address " << channel.get();

            // Unregister channel
            onInputGone(channel, error);
        }


        void OutputChannel::onTcpChannelRead(const karabo::net::ErrorCode& ec, const karabo::net::Channel::WeakPointer& weakChannel, const karabo::util::Hash& message) {
            Channel::Pointer channel = weakChannel.lock();
            if (ec || !channel) {
                onTcpChannelError(ec, channel);
                return;
            }

            std::string reason;
            if (message.has("reason")) message.get<std::string > ("reason", reason);

            if (reason == "hello") {

                /* The hello message is expected to have:
                 *     instanceId (std::string)
                 *     memoryLocation (std::string) [local/remote]
                 *     dataDistribution (std::string) [shared/copy]
                 *     onSlowness (std::string) [queue/drop/wait/throw]
                 */

                const std::string& instanceId = message.get<std::string > ("instanceId");
                const std::string& memoryLocation = message.get<std::string > ("memoryLocation");
                const std::string& dataDistribution = message.get<std::string > ("dataDistribution");
                const std::string& onSlowness = message.get<std::string > ("onSlowness");

                karabo::util::Hash info;
                info.set("instanceId", instanceId);
                info.set("memoryLocation", memoryLocation);
                info.set("tcpChannel", weakChannel);
                info.set("onSlowness", onSlowness);
                info.set("queuedChunks", std::deque<int>());
                info.set("bytesRead", 0ull);
                info.set("bytesWritten", 0ull);

                {
                    // Need to lock both mutexes since eraseOldChannel has to look into both m_registered*Inputs
                    // since a "shared" input could become a "copy" one and vice versa. And removal and adding
                    // registered inputs has to be done under the same mutex lock to ensure consistency.
                    boost::mutex::scoped_lock lockShared(m_registeredSharedInputsMutex);
                    boost::mutex::scoped_lock lockCopy(m_registeredCopyInputsMutex);
                    eraseOldChannel(m_registeredSharedInputs, instanceId, channel);
                    eraseOldChannel(m_registeredCopyInputs, instanceId, channel);

                    if (dataDistribution == "shared") {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Registering shared-input channel of instance: " << instanceId;
                        m_registeredSharedInputs[instanceId] = info;
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Registering copy-input channel of instance: " << instanceId;
                        m_registeredCopyInputs[instanceId] = info;
                    }
                }
                onInputAvailable(instanceId); // Immediately register for reading
                updateConnectionTable();
                KARABO_LOG_FRAMEWORK_INFO << "OutputChannel handshake (hello)... from InputChannel : \"" << instanceId
                        << "\", \"" << dataDistribution << "\", \"" << onSlowness << "\"";
            } else if (reason == "update") {

                if (message.has("instanceId")) {
                    const std::string& instanceId = message.get<std::string > ("instanceId");
                    KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT of '" << this->getInstanceId() << "': instanceId "
                            << instanceId << " has updated...";
                    onInputAvailable(instanceId);
                }

            }
            if (channel->isOpen()) {
                channel->readAsyncHash(bind_weak(&karabo::xms::OutputChannel::onTcpChannelRead, this, _1, weakChannel, _2));
            } else {
                onInputGone(channel, karabo::net::ErrorCode());
            }
        }


        void OutputChannel::eraseOldChannel(OutputChannel::InputChannels& channelContainer,
                                            const std::string& instanceId, const karabo::net::Channel::Pointer& newChannel) const {
            auto it = channelContainer.find(instanceId);
            if (it != channelContainer.end()) {
                const Hash& channelInfo = it->second;
                Channel::Pointer oldChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();
                if (oldChannel) {
                    if (oldChannel == newChannel) {
                        // Ever reached? Let's not close, but try to go on...
                        KARABO_LOG_FRAMEWORK_WARN << "Existing channel '" << instanceId << "' sent hello message again.";
                    } else {
                        const TcpChannel::Pointer oldTcpChannel = boost::static_pointer_cast<TcpChannel>(oldChannel);
                        const Hash oldTcpInfo(TcpChannel::getChannelInfo(oldTcpChannel));
                        KARABO_LOG_FRAMEWORK_INFO << "New channel says hello with existing id '" << instanceId << "'. "
                                << "Close old one to " << oldTcpInfo.get<std::string>("remoteAddress") << ":"
                                << oldTcpInfo.get<unsigned short>("remotePort") << ".";
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
                    const Hash &channelInfo = idInfoPair.second;
                    Channel::WeakPointer wptr = channelInfo.get<Channel::WeakPointer>("tcpChannel");
                    Channel::Pointer channel = wptr.lock();
                    net::TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(channel);
                    Hash row = TcpChannel::getChannelInfo(tcpChannel);
                    row.set("remoteId", channelInfo.get<std::string>("instanceId"));
                    row.set("memoryLocation", channelInfo.get<std::string>("memoryLocation"));
                    row.set("dataDistribution","shared");
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
                    const Hash &channelInfo = idInfoPair.second;
                    boost::weak_ptr<Channel> wptr = channelInfo.get<boost::weak_ptr<Channel> >("tcpChannel");
                    boost::shared_ptr<Channel> channel = wptr.lock();
                    boost::shared_ptr<TcpChannel> tcpChannel = boost::static_pointer_cast<TcpChannel>(channel);
                    Hash row = TcpChannel::getChannelInfo(tcpChannel);
                    row.set("remoteId", channelInfo.get<std::string>("instanceId"));
                    row.set("memoryLocation", channelInfo.get<std::string>("memoryLocation"));
                    row.set("dataDistribution","copy");
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
                m_updateDeadline.async_wait(bind_weak(&OutputChannel::updateNetworkStatistics, this, boost::asio::placeholders::error));
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
                boost::weak_ptr<Channel> wptr = h.get<boost::weak_ptr<Channel> >("weakChannel");
                boost::shared_ptr<Channel> channel = wptr.lock();
                vBytesRead[i] =  h.get<unsigned long long>("bytesRead");
                vBytesWritten[i] =  h.get<unsigned long long>("bytesWritten");
                if (!channel) continue;
                vBytesRead[i] +=  channel->dataQuantityRead();
                vBytesWritten[i] +=  channel->dataQuantityWritten();
                h.set<unsigned long long>("bytesRead", vBytesRead[i]);
                h.set<unsigned long long>("bytesWritten", vBytesWritten[i]);
            }
            m_showStatisticsHandler(vBytesRead, vBytesWritten);

            m_updateDeadline.expires_from_now(boost::posix_time::seconds(m_period));
            m_updateDeadline.async_wait(bind_weak(&OutputChannel::updateNetworkStatistics, this, boost::asio::placeholders::error));
        }


        void OutputChannel::onInputAvailable(const std::string& instanceId) {

            {
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                auto itIdChannelInfo = m_registeredSharedInputs.find(instanceId);
                if (itIdChannelInfo != m_registeredSharedInputs.end()) {
                    Hash &channelInfo = itIdChannelInfo->second;
                    if (m_distributionMode == "load-balanced" && !m_sharedLoadBalancedQueuedChunks.empty()) {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId()
                                << " Writing single-queued (shared) data to instance " << instanceId;
                        distributeQueue(channelInfo, m_sharedLoadBalancedQueuedChunks);
                        return;
                    } else if (!channelInfo.get<std::deque<int> >("queuedChunks").empty()) {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId()
                                << " Writing queued (shared) data to instance " << instanceId;
                        distributeQueue(channelInfo, channelInfo.get<std::deque<int>>("queuedChunks"));
                        return;
                    }
                    lock.unlock();
                    pushShareNext(instanceId);
                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " New (shared) input on instance " << instanceId << " available for writing ";
                    this->triggerIOEvent();
                    return;
                }
            }

            boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
            auto itIdChannelInfo = m_registeredCopyInputs.find(instanceId);
            if (itIdChannelInfo != m_registeredCopyInputs.end()) {
                Hash &channelInfo = itIdChannelInfo->second;
                if (!channelInfo.get<std::deque<int>>("queuedChunks").empty()) {
                    KARABO_LOG_FRAMEWORK_TRACE << debugId() << " Writing queued (copied) data to instance " << instanceId;
                    copyQueue(channelInfo);
                    return;
                }
                // Be safe and unlock before pushCopyNext locks another mutex.
                // One also never knows what handlers are registered for io event...
                lock.unlock();
                pushCopyNext(instanceId);
                KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " New (copied) input on instance " << instanceId << " available for writing ";
                this->triggerIOEvent();
                return;
            }
            KARABO_LOG_FRAMEWORK_WARN << this->debugId() << " An input channel (" << instanceId << ") updated, but is not registered.";
        }


        void OutputChannel::onInputGone(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& error) {
            using namespace karabo::net;
            const Hash tcpInfo(TcpChannel::getChannelInfo(boost::static_pointer_cast<TcpChannel>(channel)));
            const std::string tcpAddress(tcpInfo.get<std::string>("remoteAddress") + ':'
                                         + toString(tcpInfo.get<unsigned short>("remotePort")));

            // Clean specific channel from bookkeeping structures and ... 
            // ... clean expired entries as well (we are not expecting them but we want to be on the safe side!)
            unsigned int inputsLeft = 0;
            {
                // SHARED Inputs
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                for (InputChannels::iterator it = m_registeredSharedInputs.begin(); it != m_registeredSharedInputs.end();) {
                    const Hash& channelInfo = it->second;
                    auto tcpChannel = channelInfo.get<Channel::WeakPointer>("tcpChannel").lock();

                    // Cleaning expired or specific channels only
                    if (!tcpChannel || tcpChannel == channel) {
                        const std::string& instanceId = it->first;
                        KARABO_LOG_FRAMEWORK_INFO << m_instanceId << " : Shared input channel '" << instanceId
                                << "' (ip/port " << (tcpChannel ? tcpAddress : "?") << ") disconnected since '"
                                << error.message() << "' (#" << error.value() << ").";


                        // Transfer queued chunks or release them
                        const std::deque<int>& queuedChunks = channelInfo.get<std::deque<int> >("queuedChunks");
                        if (m_registeredSharedInputs.size() == 1u) { // i.e. will erase the last element below
                            // Nothing left to transfer, so
                            // * round-robin case: release chunks in 'queuedChunks'
                            // * load balanced case: release chunks in common queue and clear it
                            for (const int chunkId : queuedChunks) {
                                unregisterWriterFromChunk(chunkId);
                            }
                            for (const int chunkId : m_sharedLoadBalancedQueuedChunks) {
                                unregisterWriterFromChunk(chunkId);
                            }
                            m_sharedLoadBalancedQueuedChunks.clear();
                        } else if (m_distributionMode == "round-robin" && !queuedChunks.empty()) {
                            // Append queued chunks to other shared input
                            // Note: if load-balanced, queuedChunks is empty anyway...
                            // TODO: GF thinks the correct logic is to just clean the queue, as in copy case...
                            InputChannels::iterator itIdChannelInfo = getNextRoundRobinChannel();
                            Hash& nextChannelInfo = itIdChannelInfo->second;
                            std::deque<int>& src = nextChannelInfo.get<std::deque<int> >("queuedChunks");
                            src.insert(src.end(), queuedChunks.begin(), queuedChunks.end());
                            undoGetNextRoundRobinChannel();
                        }

                        // Delete from input queue
                        eraseSharedInput(instanceId);
                        it = m_registeredSharedInputs.erase(it); // after last use of instanceId and queuedChunks which get invalidated here
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

                        KARABO_LOG_FRAMEWORK_INFO << m_instanceId << " : Copy input channel '" << instanceId
                                << "' (ip/port " << (tcpChannel ? tcpAddress : "?") << ") disconnected since '"
                                << error.message() << "' (#" << error.value() << ").";
                        // Release any queued chunks:
                        for (const int chunkId : channelInfo.get<std::deque<int> >("queuedChunks")) {
                            unregisterWriterFromChunk(chunkId);
                        }
                        // Delete from input queue
                        eraseCopyInput(instanceId);
                        it = m_registeredCopyInputs.erase(it); // after last use of instanceId which gets invalidated here
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
                    KARABO_LOG_FRAMEWORK_WARN << m_instanceId << " : Failed to remove channel with address " << channel.get();
                }
                if (m_inputNetChannels.size() != inputsLeft) {
                    KARABO_LOG_FRAMEWORK_WARN << m_instanceId << " : Inconsistent number of channels left: "
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
                KARABO_LOG_FRAMEWORK_ERROR << "\"triggerIOEvent\" Exception code #" << e.userFriendlyMsg() << " -- " << e.detailedMsg();
                KARABO_RETHROW;
            } catch (const boost::bad_weak_ptr& e) {
                KARABO_LOG_FRAMEWORK_INFO << "\"triggerIOEvent\" call is too late: OutputChannel destroyed already -- " << e.what();
            } catch (const std::exception& ex) {
                KARABO_LOG_FRAMEWORK_ERROR << "\"triggerIOEvent\" exception -- " << ex.what();
                //throw KARABO_SYSTEM_EXCEPTION(string("\"triggerIOEvent\" exception -- ") + ex.what());
            }
        }


        void OutputChannel::distributeQueue(karabo::util::Hash& channelInfo, std::deque<int>& chunkIds) {
            int chunkId = chunkIds.front();
            chunkIds.pop_front();
            KARABO_LOG_FRAMEWORK_DEBUG << "Distributing from queue: " << chunkId;
            if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                distributeLocal(chunkId, channelInfo);
            } else {
                distributeRemote(chunkId, channelInfo);
            }
        }

        void OutputChannel::copyQueue(karabo::util::Hash& channelInfo) {
            std::deque<int>& chunkIds = channelInfo.get<std::deque<int> >("queuedChunks");
            int chunkId = chunkIds.front();
            chunkIds.pop_front();
            KARABO_LOG_FRAMEWORK_DEBUG << "Copying chunk " << chunkId << " from queue, "
                    << chunkIds.size() << " queue items left ";
            if (channelInfo.get<std::string > ("memoryLocation") == "local") copyLocal(chunkId, channelInfo);
            else copyRemote(chunkId, channelInfo);
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


        bool OutputChannel::hasCopyInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            return (m_copyNext.count(instanceId) > 0);
        }


        void OutputChannel::eraseCopyInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            m_copyNext.erase(instanceId);
        }


        bool OutputChannel::canCompute() const {
            return true;
        }


        void OutputChannel::update() {

            // m_channelId is unique per _process_...
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT " << m_channelId << " of '" << this->getInstanceId() << "' update()";

            // If no data was written return
            if (Memory::size(m_channelId, m_chunkId) == 0) return;

            // Take current chunkId for sending
            unsigned int chunkId = m_chunkId;

            // This will increase the usage counts for this chunkId
            // by the number of all interested connected inputs
            // and set m_toUnregisterSharedInput/m_toUnregisterCopyInputs to check later for whom we registered
            registerWritersOnChunk(chunkId);

            // Distribute chunk(s)
            distribute(chunkId);

            // Copy chunk(s)
            copy(chunkId);

            // Clean-up chunk registration
            unsigned int numUnregister = 1; // That is the usage of the OutputChannel itself!
            if (m_toUnregisterSharedInput) { // The last shared input disconnected while updating...
                ++numUnregister;
            }
            numUnregister += m_toUnregisterCopyInputs.size(); // All these copy inputs disconnected while updating
            // We are done with this chunkId, it may stay alive until local receivers are done as well
            for (size_t i = 0; i < numUnregister; ++i) {
                unregisterWriterFromChunk(chunkId);
            }

            // What if this throws, e.g. if configured to queue, but receiver is permanently too slow?
            // Catch and go on? Block in a loop until it does not throw?
            // Register new chunkId for writing to
            m_chunkId = Memory::registerChunk(m_channelId);
        }


        void OutputChannel::signalEndOfStream() {
            using namespace karabo::net;

            // If there is still some data in the pipe, put it out
            if (Memory::size(m_channelId, m_chunkId) > 0) update();

            // Wait until all queued data is fetched
            bool doWait = true;
            while (doWait) {
                doWait = false;
                {
                    boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                    for (const auto& idChannelInfo : m_registeredSharedInputs) {
                        if (!idChannelInfo.second.get<std::deque<int> >("queuedChunks").empty()) {
                            doWait = true;
                        }
                    }
                    
                    if (!m_sharedLoadBalancedQueuedChunks.empty()) {
                        doWait = true;
                    }
                }
                {
                    boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                    for (const auto& idChannelInfo : m_registeredCopyInputs) {
                        if (!idChannelInfo.second.get<std::deque<int> >("queuedChunks").empty()) {
                            doWait = true;
                            break;
                        }
                    }
                }
                if (!doWait) break;
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            }

            {
                // Need to lock - even around synchronous tcp write... :-(
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                for (const auto& idChannelInfo : m_registeredSharedInputs) {
                    Channel::Pointer tcpChannel = idChannelInfo.second.get<Channel::WeakPointer > ("tcpChannel").lock();
                    if (!tcpChannel) continue;

                    try {
                        if (tcpChannel->isOpen()) {
                            tcpChannel->write(karabo::util::Hash("endOfStream", true), std::vector<BufferSet::Pointer>());
                        }
                    } catch (const std::exception& e) {
                        KARABO_LOG_FRAMEWORK_ERROR << "OutputChannel::signalEndOfStream (shared) :  " << e.what();
                    }
                }
            }
            boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
            for (const auto& idChannelInfo : m_registeredCopyInputs) {
                Channel::Pointer tcpChannel = idChannelInfo.second.get<Channel::WeakPointer> ("tcpChannel").lock();
                if (!tcpChannel) continue;

                try {
                    if (tcpChannel->isOpen()) {
                        tcpChannel->write(karabo::util::Hash("endOfStream", true), std::vector<BufferSet::Pointer>());
                    }
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "OutputChannel::signalEndOfStream (copy) :  " << e.what();
                }
            }
        }


        void OutputChannel::registerShowConnectionsHandler(const ShowConnectionsHandler& handler) {
            boost::mutex::scoped_lock lock(m_showConnectionsHandlerMutex);
            if (!handler) {
                m_showConnectionsHandler = [](const std::vector<karabo::util::Hash>&){};
            } else {
                m_showConnectionsHandler = handler;
            }
        }


        void OutputChannel::registerShowStatisticsHandler(const ShowStatisticsHandler& handler) {
            boost::mutex::scoped_lock lock(m_showConnectionsHandlerMutex);
            if (!handler) {
                m_showStatisticsHandler = [](const std::vector<unsigned long long>&, const std::vector<unsigned long long>&){};
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
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT Registered " << Memory::getChunkStatus(m_channelId, chunkId) << " uses for [" << m_channelId << "][" << chunkId << "]";
        }


        void OutputChannel::unregisterWriterFromChunk(int chunkId) {
            Memory::decrementChunkUsage(m_channelId, chunkId);
            KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT " << Memory::getChunkStatus(m_channelId, chunkId) << " uses left for [" << m_channelId << "][" << chunkId << "]";
        }


        void OutputChannel::distribute(unsigned int chunkId) {

            boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
            
            // If no shared input channels are registered at all, we do not go on
            if (m_registeredSharedInputs.empty()) return;
            if (!m_toUnregisterSharedInput) {
                // Increment chunk usage since a first shared input just connected while we update
                Memory::incrementChunkUsage(m_channelId, chunkId);
            }
            m_toUnregisterSharedInput = false; // We care for it!

            if (m_distributionMode == "round-robin") {
                distributeRoundRobin(chunkId, lock);
            } else if (m_distributionMode == "load-balanced") {
                distributeLoadBalanced(chunkId, lock);
            } else { // We should never be here!!
                throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured: " + m_distributionMode);
            }
        }


        void OutputChannel::distributeRoundRobin(unsigned int chunkId, boost::mutex::scoped_lock& lock) {
            // Next input
            InputChannels::iterator itIdChannelInfo = getNextRoundRobinChannel();
            karabo::util::Hash& channelInfo = itIdChannelInfo->second;
            const std::string& instanceId = itIdChannelInfo->first; //channelInfo.get<std::string>("instanceId");


            if (hasSharedInput(instanceId)) { // Found
                // Note: If now, before we can actually distribute, instanceId disconnects, the data that should go
                //       there is lost, also no other shared input will receive it. But that should be acceptable
                //       in a dynamic and distributed system like Karabo.
                eraseSharedInput(instanceId);
                if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Now distributing data (local)";
                    distributeLocal(chunkId, channelInfo);
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Now distributing data (remote)";
                    distributeRemote(chunkId, channelInfo);
                }
            } else { // Not found
                bool haveToWait = false;
                if (m_onNoSharedInputChannelAvailable == "drop") {
                    // Drop data and try same destination again next time
                    undoGetNextRoundRobinChannel(); // lock must not yet be unlocked() after getNextSharedInputIdx() above!
                    unregisterWriterFromChunk(chunkId);
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Dropping (shared) data package with chunkId: " << chunkId;

                } else if (m_onNoSharedInputChannelAvailable == "throw") {
                    unregisterWriterFromChunk(chunkId);
                    throw KARABO_IO_EXCEPTION("Can not write data because no (shared) input is available");

                } else if (m_onNoSharedInputChannelAvailable == "queue") {
                    // Since distributing round-robin, it is really this instance's turn.
                    // So we queue for exactly this one.
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing (shared) data package with chunkId: " << chunkId;
                    Memory::assureAllDataIsCopied(m_channelId, chunkId);
                    channelInfo.get<std::deque<int> >("queuedChunks").push_back(chunkId);
                } else if (m_onNoSharedInputChannelAvailable == "wait") {
                    // Blocking actions must not happen under the mutex that is also needed to unblock (in onInputAvailable)
                    haveToWait = true;
                } else {
                    // We should never be here!!
                    throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured: " + m_onNoSharedInputChannelAvailable);
                }

                if (haveToWait) {
                    // Make copy of references which might become dangling when unlocking mutex lock
                    const karabo::util::Hash channelInfoCopy = channelInfo;
                    const std::string& instanceIdCopy = channelInfoCopy.get<std::string>("instanceId");
                    lock.unlock(); // Otherwise hasSharedInput will never become true (and deadlock with hasRegisteredSharedInputChannel)!
                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Waiting for available (shared) input channel...";

                    while (!hasSharedInput(instanceIdCopy)) {
                        boost::this_thread::sleep(boost::posix_time::millisec(1));
                        if (!hasRegisteredSharedInputChannel(instanceIdCopy)) { // might have disconnected meanwhile...
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " input channel (shared) of " << instanceIdCopy
                                                       << " disconnected while waiting for it";
                            lock.lock();
                            if (m_registeredSharedInputs.empty()) {  // nothing left, so release chunk
                                unregisterWriterFromChunk(chunkId);
                            } else { // recurse to find next available shared input
                                distributeRoundRobin(chunkId, lock);
                            }
                            return;
                        }
                    }
                    // lock.lock() not really needed...
                    // Note: if 'instanceIdCopy' is now gone, chunkId will not delivered to anybody else
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " found (shared) input channel after waiting, distributing now";
                    eraseSharedInput(instanceIdCopy);
                    if (channelInfoCopy.get<std::string > ("memoryLocation") == "local") {
                        distributeLocal(chunkId, channelInfoCopy);
                    } else {
                        distributeRemote(chunkId, channelInfoCopy);
                    }
                } // end have to wait
            } // end else - not found
        }

        void OutputChannel::distributeLoadBalanced(unsigned int chunkId, boost::mutex::scoped_lock& lock) {

            if (!isShareNextEmpty()) { // Found
                const std::string instanceId = popShareNext();
                auto it = m_registeredSharedInputs.find(instanceId);
                if (it == m_registeredSharedInputs.end()) { // paranoia check
                    // Should never come here...
                    KARABO_LOG_FRAMEWORK_ERROR << "Next load balanced input '" << instanceId << "' does not exist anymore!";
                    return;
                }
                const karabo::util::Hash& channelInfo = it->second;
                if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Distributing data (local)";
                    distributeLocal(chunkId, channelInfo);
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Distributing data (remote)";
                    distributeRemote(chunkId, channelInfo);
                }
            } else { // Not found
                bool haveToWait = false;
                if (m_onNoSharedInputChannelAvailable == "drop") {
                    unregisterWriterFromChunk(chunkId);
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                            << " Dropping (shared) data package with chunkId: " << chunkId;
                } else if (m_onNoSharedInputChannelAvailable == "throw") {
                    unregisterWriterFromChunk(chunkId);
                    throw KARABO_IO_EXCEPTION("Can not write data because no (shared) input is available");
                } else if (m_onNoSharedInputChannelAvailable == "queue") {
                    // For load-balanced mode the chunks should be put on a single queue.
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                            << "Placing chunk in single queue (load-balanced distribution mode): " << chunkId;
                    Memory::assureAllDataIsCopied(m_channelId, chunkId);
                    m_sharedLoadBalancedQueuedChunks.push_back(chunkId);
                } else if (m_onNoSharedInputChannelAvailable == "wait") {
                    // Blocking actions must not happen under the mutex that is also needed to unblock (in onInputAvailable)
                    haveToWait = true;
                } else {
                    // We should never be here!!
                    throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured: " + m_onNoSharedInputChannelAvailable);
                }
                if (haveToWait) {
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Waiting for available (shared) input channel...";
                    // while loop such that the following popShareNext() is called under the same lock.lock() under
                    // which isShareNextEmpty() became false - otherwise there might not be anything left to pop...
                    do {
                        lock.unlock(); // Otherwise isShareNextEmpty() will never become false
                        boost::this_thread::sleep(boost::posix_time::millisec(1));
                        lock.lock();
                        if (m_registeredSharedInputs.empty()) {
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " found all (shared) input channels gone while waiting";
                            unregisterWriterFromChunk(chunkId);
                            return; // Nothing to distribute anymore: no shared channels left
                        }
                    } while (isShareNextEmpty());
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " found (shared) input channel after waiting, distributing now";
                    const std::string instanceId = popShareNext();
                    auto it = m_registeredSharedInputs.find(instanceId);
                    if (it == m_registeredSharedInputs.end()) { // paranoia check
                        // Should never come here...
                        KARABO_LOG_FRAMEWORK_ERROR << "Next load balanced input '" << instanceId << "' does not exist anymore (after wait)!";
                        return;
                    }
                    const karabo::util::Hash& channelInfo = it->second;
                    if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now distributing data (local)";
                        distributeLocal(chunkId, channelInfo);
                    } else {
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now distributing data (remote)";
                        distributeRemote(chunkId, channelInfo);
                    }
                } // end have to wait
            } // end else - not found
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
                m_sharedInputIndex = m_registeredSharedInputs.size() - 1u; // size() == 0 excluded by documented requirements
            } else {
                --m_sharedInputIndex;
            }
        }


        void OutputChannel::distributeLocal(unsigned int chunkId, const InputChannelInfo& channelInfo) {
            using namespace karabo::net;

            Channel::Pointer tcpChannel = channelInfo.get<Channel::WeakPointer > ("tcpChannel").lock();

            bool notSent = true;
            if (tcpChannel) {

                // Synchronous write as it takes no time here
                KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT Now distributing (local memory)";
                try {
                    if (tcpChannel->isOpen()) {
                        using namespace karabo::io;
                        // in case of short-cutting the receiver may async. work on data the sender is already altering again.
                        // we assure that the contents in the chunk the receiver gets sent have been copied once
                        Memory::assureAllDataIsCopied(m_channelId, chunkId);
                        tcpChannel->write(karabo::util::Hash("channelId", m_channelId, "chunkId", chunkId),
                                          // To allow old versions <= 2.2.4.4 to read our data, send vector with one
                                          // empty BufferSet instead of an empty vector:
                                          std::vector<BufferSet::Pointer>(1, BufferSet::Pointer(new BufferSet)));
                        notSent = false;
                    }
                } catch (const std::exception& e) {
                    if (tcpChannel->isOpen()) {
                        KARABO_LOG_FRAMEWORK_WARN << "OutputChannel::distributeLocal - channel still open :  " << e.what();
                    } else {
                        karabo::util::Exception::clearTrace();
                    }
                }
            }
            // NOTE: Here the same note concerning e.g. a chunk leak applies as at the end of copyLocal(..)
            if (notSent) unregisterWriterFromChunk(chunkId);
        }


        void OutputChannel::distributeRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {
            using namespace karabo::net;

            Channel::Pointer tcpChannel = channelInfo.get<Channel::WeakPointer > ("tcpChannel").lock();

            if (tcpChannel) {
                karabo::util::Hash header;
                std::vector<karabo::io::BufferSet::Pointer> data;
                Memory::readAsContiguousBlock(data, header, m_channelId, chunkId);

                try {
                    if (tcpChannel->isOpen()) {
                        tcpChannel->write(header, data); // Blocks whilst writing
                    }
                } catch (const std::exception& e) {
                    if (tcpChannel->isOpen()) {
                        KARABO_LOG_FRAMEWORK_ERROR << "OutputChannel::distributeRemote - channel still open :  " << e.what();
                    } else {
                        karabo::util::Exception::clearTrace();
                    }
                }
                data.clear();
            }

            unregisterWriterFromChunk(chunkId);
            //Memory::decrementChannelUsage(m_channelId, chunkId); // Later use this one!
        }


        void OutputChannel::copy(unsigned int chunkId) {

            InputChannels waitingInstances;
            {
                boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                // If no copied input channels are registered at all, we do not go on
                if (m_registeredCopyInputs.empty()) return;

                for (auto& idChannelInfo : m_registeredCopyInputs) { // not const since queue might be extended

                    InputChannelInfo& channelInfo = idChannelInfo.second;
                    const std::string& instanceId = idChannelInfo.first;
                    const std::string& onSlowness = channelInfo.get<std::string>("onSlowness");

                    const auto unregisterIter = m_toUnregisterCopyInputs.find(instanceId);
                    if (unregisterIter == m_toUnregisterCopyInputs.end()) {
                        // Increment chunk usage since this copy input just connected while we update
                        Memory::incrementChunkUsage(m_channelId, chunkId);
                    } else {
                        m_toUnregisterCopyInputs.erase(unregisterIter); // We care about it!
                    }
                    if (hasCopyInput(instanceId)) {
                        eraseCopyInput(instanceId);
                        if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Now copying data (local)";
                            copyLocal(chunkId, channelInfo);
                        } else {
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Now copying data (remote)";
                            copyRemote(chunkId, channelInfo);
                        }
                    } else if (onSlowness == "drop") {
                        unregisterWriterFromChunk(chunkId);
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Dropping (copied) data package for " << instanceId;
                    } else if (onSlowness == "throw") {
                        unregisterWriterFromChunk(chunkId);
                        throw KARABO_IO_EXCEPTION("Can not write (copied) data because input channel of " + instanceId + " was too late");
                    } else if (onSlowness == "queue") {
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing (copied) data package for "
                                << instanceId << ", chunk " << chunkId;
                        Memory::assureAllDataIsCopied(m_channelId, chunkId);
                        channelInfo.get<std::deque<int> >("queuedChunks").push_back(chunkId);
                    } else if (onSlowness == "wait") {
                        // Blocking actions must not happen under the mutex that is also needed to unblock (in onInputAvailable)
                        waitingInstances.insert(idChannelInfo);
                    }
                }
            } // end of mutex lock

            for (const InputChannels::value_type& idChannelInfo : waitingInstances) {
                const std::string& instanceId = idChannelInfo.first;
                KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Data (copied) is waiting for input channel of "
                                           << instanceId << " to be available";
                bool instanceDisconnected = false;
                while (!hasCopyInput(instanceId)) {
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
                KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " found (copied) input channel after waiting, copying now";
                eraseCopyInput(instanceId);
                const Hash& channelInfo = idChannelInfo.second;
                if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now copying data (local)";
                    copyLocal(chunkId, channelInfo);
                } else {
                    KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now copying data (remote)";
                    copyRemote(chunkId, channelInfo);
                }
            }
        }


        void OutputChannel::copyLocal(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {
            using namespace karabo::net;
            Channel::Pointer tcpChannel = channelInfo.get<Channel::WeakPointer > ("tcpChannel").lock();

            bool notSent = true;
            if (tcpChannel) {
                // Synchronous write as it takes no time here
                try {
                    if (tcpChannel->isOpen()) {
                        using namespace karabo::io;
                        // in case of short-cutting the receiver may async. work on data the sender is already altering again.
                        // we assure that the contents in the chunk the receiver gets sent have been copied once
                        Memory::assureAllDataIsCopied(m_channelId, chunkId);
                        tcpChannel->write(karabo::util::Hash("channelId", m_channelId, "chunkId", chunkId),
                                          // To allow old versions <= 2.2.4.4 to read our data, send vector with one
                                          // empty BufferSet instead of an empty vector:
                                          std::vector<BufferSet::Pointer>(1, BufferSet::Pointer(new BufferSet)));
                        notSent = false;
                    }
                } catch (const std::exception& e) {
                    if (tcpChannel->isOpen()) {
                        KARABO_LOG_FRAMEWORK_WARN << "OutputChannel::copyLocal - channel still open :  " << e.what();
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


        void OutputChannel::copyRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {
            using namespace karabo::net;

            Channel::Pointer tcpChannel = channelInfo.get<Channel::WeakPointer > ("tcpChannel").lock();

            if (tcpChannel) {
                karabo::util::Hash header;
                std::vector<karabo::io::BufferSet::Pointer> data;
                Memory::readAsContiguousBlock(data, header, m_channelId, chunkId);

                try {
                    if (tcpChannel->isOpen()) {
                        tcpChannel->write(header, data);
                    }
                } catch (const std::exception& e) {
                    if (tcpChannel->isOpen()) {
                        KARABO_LOG_FRAMEWORK_WARN << "OutputChannel::copyRemote - channel still open :  " << e.what();
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
            return std::string((("OUTPUT " + util::toString(m_channelId) += " of '") += this->getInstanceId()) += "'");
        }


        void OutputChannel::write(const karabo::util::Hash& data, const OutputChannel::MetaData& metaData, bool copyAllData) {
            Memory::write(data, m_channelId, m_chunkId, metaData, copyAllData);
        }


        void OutputChannel::write(const karabo::util::Hash& data, bool copyAllData) {
            OutputChannel::MetaData meta(/*source*/ m_instanceId + ":" + m_channelName, /*timestamp*/ karabo::util::Timestamp());
            Memory::write(data, m_channelId, m_chunkId, meta, copyAllData);
        }


        void OutputChannel::write(const karabo::util::Hash::Pointer& data, const OutputChannel::MetaData& metaData) {
            write(*data, metaData, true);
        }


        void OutputChannel::write(const karabo::util::Hash::Pointer& data) {
            write(*data, true);
        }
    }
}
