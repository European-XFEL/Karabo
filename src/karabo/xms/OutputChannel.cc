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

        typedef boost::weak_ptr<karabo::net::Channel> ChannelWeakPointer;

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
        }


        OutputChannel::OutputChannel(const karabo::util::Hash& config) : m_port(0), m_sharedInputIndex(0) {
            //KARABO_LOG_FRAMEWORK_DEBUG << "*** OutputChannel::OutputChannel CTOR ***";
            config.get("distributionMode", m_distributionMode);
            config.get("noInputShared", m_onNoSharedInputChannelAvailable);
            config.get("hostname", m_hostname);
            config.get("port", m_port);
            if (m_hostname == "default") m_hostname = boost::asio::ip::host_name();
            config.get("compression", m_compression);

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
            if (!weak_from_this().lock()) { // Try to promote to shared_ptr.
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
            for (const InputChannelInfo& channelInfo : m_registeredCopyInputs) {
                if (channelInfo.get<std::string>("instanceId") == instanceId) {
                    return true;
                }
            }
            return false;
        }


        bool OutputChannel::hasRegisteredSharedInputChannel(const std::string& instanceId) const {
            boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
            for (const InputChannelInfo& channelInfo : m_registeredSharedInputs) {
                if (channelInfo.get<std::string>("instanceId") == instanceId) {
                    return true;
                }
            }
            return false;
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
            channel->readAsyncHash(bind_weak(&karabo::xms::OutputChannel::onTcpChannelRead, this, _1, channel, _2));
        }


        void OutputChannel::onTcpChannelError(const karabo::net::ErrorCode& error, const karabo::net::Channel::Pointer& channel) {
            KARABO_LOG_FRAMEWORK_INFO << "Tcp channel error on \"" << m_instanceId << "\", code #" << error.value() << " -- \"" 
                    << error.message() << "\".  Channel closed.";

            // Unregister channel
            onInputGone(channel);
        }


        void OutputChannel::onTcpChannelRead(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, const karabo::util::Hash& message) {
            if (ec) {
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
                info.set("tcpChannel", boost::weak_ptr<Channel>(channel));
                info.set("onSlowness", onSlowness);
                info.set("queuedChunks", std::deque<int>());

                if (dataDistribution == "shared") {
                    KARABO_LOG_FRAMEWORK_DEBUG << "Registering shared-input channel of instance: " << instanceId;
                    boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                    m_registeredSharedInputs.push_back(info);
                } else {
                    bool isNew = true;
                    boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                    for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {
                        karabo::util::Hash& channelInfo = m_registeredCopyInputs[i];
                        if (channelInfo.get<std::string>("instanceId") == instanceId) {
                            KARABO_LOG_FRAMEWORK_DEBUG << "Registering copy-input channel of (used before) instance: " << instanceId;
                            channelInfo.set("memoryLocation", memoryLocation);
                            channelInfo.set("tcpChannel", boost::weak_ptr<Channel>(channel));
                            channelInfo.set("onSlowness", onSlowness);
                            isNew = false;
                            break;
                        }
                    }
                    if (isNew) {
                        KARABO_LOG_FRAMEWORK_DEBUG << "Registering copy-input channel of (new) instance: " << instanceId;
                        m_registeredCopyInputs.push_back(info);
                    }
                }
                onInputAvailable(instanceId); // Immediately register for reading

            } else if (reason == "update") {

                if (message.has("instanceId")) {
                    const std::string& instanceId = message.get<std::string > ("instanceId");
                    KARABO_LOG_FRAMEWORK_TRACE << "OUTPUT of '" << this->getInstanceId() << "': instanceId "
                            << instanceId << " has updated...";
                    onInputAvailable(instanceId);
                }

            }
            if (channel->isOpen()) {
                channel->readAsyncHash(bind_weak(&karabo::xms::OutputChannel::onTcpChannelRead, this, _1, channel, _2));
            } else {
                onInputGone(channel);
            }
        }


        void OutputChannel::onInputAvailable(const std::string& instanceId) {

            {
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                    karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                    if (channelInfo.get<std::string>("instanceId") == instanceId) {
                        if (m_distributionMode == "load-balanced" && !m_sharedLoadBalancedQueuedChunks.empty()) {
                            KARABO_LOG_FRAMEWORK_TRACE << this->debugId()
                                    << " Writing single-queued (shared) data to instance " << instanceId;
                            distributeQueueSharedLoadBalanced(channelInfo);
                            return;
                        } else if (!channelInfo.get<std::deque<int> >("queuedChunks").empty()) {
                            KARABO_LOG_FRAMEWORK_TRACE << this->debugId()
                                    << " Writing queued (shared) data to instance " << instanceId;
                            distributeQueue(channelInfo);
                            return;
                        }
                        // Be safe and unlock before pushShareNext locks another mutex.
                        // One also never knows what handlers are registered for io event...
                        lock.unlock();
                        pushShareNext(instanceId);
                        KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " New (shared) input on instance " << instanceId << " available for writing ";
                        this->triggerIOEvent();
                        return;
                    }
                }
            }
            boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
            for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {
                karabo::util::Hash& channelInfo = m_registeredCopyInputs[i];
                if (channelInfo.get<std::string>("instanceId") == instanceId) {
                    if (!channelInfo.get<std::deque<int> >("queuedChunks").empty()) {
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
            }
            KARABO_LOG_FRAMEWORK_WARN << this->debugId() << " An input channel wants to connect (" << instanceId << ") that was not registered before.";
        }


        void OutputChannel::onInputGone(const karabo::net::Channel::Pointer& channel) {
            using namespace karabo::net;
            KARABO_LOG_FRAMEWORK_DEBUG << "*** OutputChannel::onInputGone ***";

            // Clean specific channel from bookkeeping structures and ... 
            // ... clean expired entries as well (we are not expecting them but we want to be on the safe side!)

            {
                // SHARED Inputs
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                for (InputChannels::iterator it = m_registeredSharedInputs.begin(); it != m_registeredSharedInputs.end();) {
                    auto tcpChannel = it->get<ChannelWeakPointer>("tcpChannel").lock();

                    // Cleaning expired or specific channels only
                    if (!tcpChannel || tcpChannel == channel) {

                        std::string instanceId = it->get<std::string>("instanceId");

                        KARABO_LOG_FRAMEWORK_DEBUG << "Connected (shared) input on instanceId " << instanceId << " disconnected";
                        std::deque<int> tmp = it->get<std::deque<int> >("queuedChunks");
                        // Delete from registry
                        it = m_registeredSharedInputs.erase(it);

                        // Note: if distribution mode is set to "load-balanced" the chunks will be stored in a single
                        //       queue and the per input channel should be empty. Nothing has to be done in that mode.

                        if (!m_registeredSharedInputs.empty()) { // There are other shared input channels available
                            // Append queued chunks to other shared input
                            unsigned int idx = getNextSharedInputIdx();
                            std::deque<int>& src = m_registeredSharedInputs[idx].get<std::deque<int> >("queuedChunks");
                            src.insert(src.end(), tmp.begin(), tmp.end());
                        }

                        // Delete from input queue
                        eraseSharedInput(instanceId);
                    } else {
                        ++it;
                    }
                }
            }

            // COPY Inputs
            boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
            for (InputChannels::iterator it = m_registeredCopyInputs.begin(); it != m_registeredCopyInputs.end();) {
                auto tcpChannel = it->get<ChannelWeakPointer>("tcpChannel").lock();
                if (!tcpChannel || tcpChannel == channel) {

                    std::string instanceId = it->get<std::string>("instanceId");

                    KARABO_LOG_FRAMEWORK_DEBUG << "Connected (copy) input on instanceId " << instanceId << " disconnected";
                    it = m_registeredCopyInputs.erase(it);

                    // Delete from input queue
                    eraseCopyInput(instanceId);
                } else {
                    ++it;
                }
            }
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


        void OutputChannel::distributeQueue(karabo::util::Hash& channelInfo) {
            std::deque<int>& chunkIds = channelInfo.get<std::deque<int> >("queuedChunks");
            int chunkId = chunkIds.front();
            chunkIds.pop_front();
            KARABO_LOG_FRAMEWORK_DEBUG << "Distributing from queue: " << chunkId;
            if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                distributeLocal(chunkId, channelInfo);
            } else {
                distributeRemote(chunkId, channelInfo);
            }
        }


        void OutputChannel::distributeQueueSharedLoadBalanced(karabo::util::Hash& channelInfo) {
            int chunkId = m_sharedLoadBalancedQueuedChunks.front();
            m_sharedLoadBalancedQueuedChunks.pop_front();
            KARABO_LOG_FRAMEWORK_DEBUG << "Distributing from single queue (Load balanced mode): " << chunkId;
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
            KARABO_LOG_FRAMEWORK_DEBUG << "Copying from queue: " << chunkId;
            if (channelInfo.get<std::string > ("memoryLocation") == "local") copyLocal(chunkId, channelInfo);
            else copyRemote(chunkId, channelInfo);
        }


        void OutputChannel::pushShareNext(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            m_shareNext.push_back(instanceId);
        }


        std::string OutputChannel::popShareNext() {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
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
            InputChannelQueue::iterator it = std::find(m_shareNext.begin(), m_shareNext.end(), instanceId);
            if (it != m_shareNext.end()) m_shareNext.erase(it);
        }


        void OutputChannel::pushCopyNext(const std::string& info) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            m_copyNext.push_back(info);
        }


        std::string OutputChannel::popCopyNext() {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            std::string info = m_copyNext.front();
            m_copyNext.pop_front();
            return info;
        }


        bool OutputChannel::hasCopyInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            return std::find(m_copyNext.begin(), m_copyNext.end(), instanceId) != m_copyNext.end();
        }


        void OutputChannel::eraseCopyInput(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            InputChannelQueue::iterator it = std::find(m_copyNext.begin(), m_copyNext.end(), instanceId);
            if (it != m_copyNext.end()) m_copyNext.erase(it);
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
            registerWritersOnChunk(chunkId);

            // We are done with this chunkId, it will stay alive only until
            // all inputs are served (see above)
            Memory::unregisterChunk(m_channelId, chunkId);

            // Register new chunkId for writing to
            m_chunkId = Memory::registerChunk(m_channelId);

            // Distribute chunk(s)
            distribute(chunkId);

            // Copy chunk(s)
            copy(chunkId);

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
                    for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                        if (!m_registeredSharedInputs[i].get<std::deque<int> >("queuedChunks").empty()) {
                            doWait = true;
                        }
                    }

                    if (!m_sharedLoadBalancedQueuedChunks.empty()) {
                        doWait = true;
                    }
                }
                {
                    boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                    for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {
                        if (!m_registeredCopyInputs[i].get<std::deque<int> >("queuedChunks").empty()) {
                            doWait = true;
                        }
                    }
                }
                if (!doWait) break;
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            }

            {
                // Need to lock - even around synchronous tcp write... :-(
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                    const karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                    Channel::Pointer tcpChannel = channelInfo.get<ChannelWeakPointer > ("tcpChannel").lock();
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
            for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {
                const karabo::util::Hash& channelInfo = m_registeredCopyInputs[i];
                Channel::Pointer tcpChannel = channelInfo.get<ChannelWeakPointer > ("tcpChannel").lock();
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


        void OutputChannel::registerWritersOnChunk(unsigned int chunkId) {
            {
                boost::mutex::scoped_lock lock(m_registeredSharedInputsMutex);
                // Only one of the shared inputs will be provided with data
                if (!m_registeredSharedInputs.empty()) Memory::incrementChunkUsage(m_channelId, chunkId);
            }
            {
                boost::mutex::scoped_lock lock(m_registeredCopyInputsMutex);
                for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) Memory::incrementChunkUsage(m_channelId, chunkId);
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
            const unsigned int sharedInputIdx = getNextSharedInputIdx();
            const karabo::util::Hash& channelInfo = m_registeredSharedInputs[sharedInputIdx];
            const std::string& instanceId = channelInfo.get<std::string>("instanceId");


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
                    undoGetNextSharedInputIdx(); // lock must not yet be unlocked() after getNextSharedInputIdx() above!
                    unregisterWriterFromChunk(chunkId);
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Dropping (shared) data package with chunkId: " << chunkId;

                } else if (m_onNoSharedInputChannelAvailable == "throw") {
                    unregisterWriterFromChunk(chunkId);
                    throw KARABO_IO_EXCEPTION("Can not write data because no (shared) input is available");

                } else if (m_onNoSharedInputChannelAvailable == "queue") {
                    // Since distributing round-robin, it is really this instance's turn.
                    // So we queue for exactly this one.
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing (shared) data package with chunkId: " << chunkId;
                    m_registeredSharedInputs[sharedInputIdx].get<std::deque<int> >("queuedChunks").push_back(chunkId); // channelInfo is const...
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
                            // recurse to find next available shared input
                            distribute(chunkId); // or lock.lock(), check !m_registeredSharedInputs.empty() and distributeRoundRobin(chunkId, lock)
                            return;
                        }
                    }
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
                for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                    const karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                    if (instanceId == channelInfo.get<std::string>("instanceId")) {
                        if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Distributing data (local)";
                            distributeLocal(chunkId, channelInfo);
                        } else {
                            KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Distributing data (remote)";
                            distributeRemote(chunkId, channelInfo);
                        }
                        break;
                    }
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
                    // When distributing load-balanced, there is a common queue, not individual ones!
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                            << " Queuing (shared) data package with chunkId: " << chunkId;
                    // For load-balanced mode the chunks should be put on a single queue.
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId()
                            << "Placing chunk in single queue (load-balanced distribution mode)";
                        m_sharedLoadBalancedQueuedChunks.push_back(chunkId);
                } else if (m_onNoSharedInputChannelAvailable == "wait") {
                    // Blocking actions must not happen under the mutex that is also needed to unblock (in onInputAvailable)
                    haveToWait = true;
                } else {
                    // We should never be here!!
                    throw KARABO_LOGIC_EXCEPTION("Output channel case internally misconfigured: " + m_onNoSharedInputChannelAvailable);
                }
                if (haveToWait) {
                    lock.unlock(); // Otherwise isShareNextEmpty() will never become false
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Waiting for available (shared) input channel...";
                    while (isShareNextEmpty()) {
                        boost::this_thread::sleep(boost::posix_time::millisec(1));
                        lock.lock();
                        if (m_registeredSharedInputs.empty()) {
                            return; // Nothing to distribute anymore: no shared channels left
                        }
                        lock.unlock();
                    }
                    KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " found (shared) input channel after waiting, distributing now";
                    std::string instanceId = popShareNext();
                    lock.lock();
                    for (size_t i = 0; i < m_registeredSharedInputs.size(); ++i) {
                        const karabo::util::Hash& channelInfo = m_registeredSharedInputs[i];
                        if (instanceId == channelInfo.get<std::string>("instanceId")) {
                            if (channelInfo.get<std::string > ("memoryLocation") == "local") {
                                KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now distributing data (local)";
                                distributeLocal(chunkId, channelInfo);
                            } else {
                                KARABO_LOG_FRAMEWORK_TRACE << this->debugId() << " Now distributing data (remote)";
                                distributeRemote(chunkId, channelInfo);
                            }
                            break;
                        }
                    }
                } // end have to wait
            } // end else - not found
        }


        unsigned int OutputChannel::getNextSharedInputIdx() {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            ++m_sharedInputIndex;
            m_sharedInputIndex %= m_registeredSharedInputs.size(); // size() == 0 excluded by documented requirements
            return m_sharedInputIndex;
        }


        void OutputChannel::undoGetNextSharedInputIdx() {
            boost::mutex::scoped_lock lock(m_nextInputMutex);
            if (0 == m_sharedInputIndex) {
                m_sharedInputIndex = m_registeredSharedInputs.size() - 1u; // size() == 0 excluded by documented requirements
            } else {
                --m_sharedInputIndex;
            }
        }


        void OutputChannel::distributeLocal(unsigned int chunkId, const InputChannelInfo& channelInfo) {
            using namespace karabo::net;

            Channel::Pointer tcpChannel = channelInfo.get<ChannelWeakPointer > ("tcpChannel").lock();
            if (!tcpChannel) return;
          
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
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "OutputChannel::distributeLocal  :  " << e.what();
            }
            // The input channel will decrement the chunkId usage, as he uses the same memory location
            // unregisterWriterFromChunk(chunkId);
        }


        void OutputChannel::distributeRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {
            using namespace karabo::net;

            Channel::Pointer tcpChannel = channelInfo.get<ChannelWeakPointer > ("tcpChannel").lock();

            if (tcpChannel) {
                karabo::util::Hash header;
                std::vector<karabo::io::BufferSet::Pointer> data;
                Memory::readAsContiguousBlock(data, header, m_channelId, chunkId);

                try {
                    if (tcpChannel->isOpen()) {
                        tcpChannel->write(header, data); // Blocks whilst writing
                    }
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "OutputChannel::distributeRemote  :  " << e.what();
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

                for (size_t i = 0; i < m_registeredCopyInputs.size(); ++i) {

                    const InputChannelInfo& channelInfo = m_registeredCopyInputs[i];
                    const std::string& instanceId = channelInfo.get<std::string>("instanceId");
                    const std::string& onSlowness = channelInfo.get<std::string>("onSlowness");

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
                        KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " Queuing (copied) data package for " << instanceId;
                        m_registeredCopyInputs[i].get<std::deque<int> >("queuedChunks").push_back(chunkId);
                    } else if (onSlowness == "wait") {
                        // Blocking actions must not happen under the mutex that is also needed to unblock (in onInputAvailable)
                        waitingInstances.push_back(channelInfo);
                    }
                }
            } // end of mutex lock

            for (const InputChannelInfo& channelInfo : waitingInstances) {
                const std::string& instanceId = channelInfo.get<std::string>("instanceId");
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
                    continue;
                }
                KARABO_LOG_FRAMEWORK_DEBUG << this->debugId() << " found (copied) input channel after waiting, copying now";
                eraseCopyInput(instanceId);
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

            Channel::Pointer tcpChannel = channelInfo.get<ChannelWeakPointer > ("tcpChannel").lock();
            if (!tcpChannel) return;
            
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
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "OutputChannel::copyLocal  :  " << e.what();
            }

            // NOTE: The input channel will decrement the chunkId usage, as he uses the same memory location
            // Having the next line commented is thus correct!!!
            //unregisterWriterFromChunk(chunkId);
        }


        void OutputChannel::copyRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo) {
            using namespace karabo::net;

            Channel::Pointer tcpChannel = channelInfo.get<ChannelWeakPointer > ("tcpChannel").lock();

            if (tcpChannel) {
                karabo::util::Hash header;
                std::vector<karabo::io::BufferSet::Pointer> data;
                Memory::readAsContiguousBlock(data, header, m_channelId, chunkId);

                try {
                    if (tcpChannel->isOpen()) {
                        tcpChannel->write(header, data);
                    }
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "OutputChannel::copyRemote  :  " << e.what();
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
