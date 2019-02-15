#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <list>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "karabo/log.hpp"
#include "utils.hh"
#include "PointToPoint.hh"
#include "EventLoop.hh"
#include "karabo/util/MetaTools.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::log;

namespace karabo {
    namespace net {


        class PointToPoint::Producer : public boost::enable_shared_from_this<PointToPoint::Producer> {

            typedef std::map<karabo::net::Channel::Pointer, std::unordered_set<std::string> > ChannelToSlotInstanceIds;

        public:

            KARABO_CLASSINFO(Producer, "PointToPointProducer", "1.0")

            Producer();

            virtual ~Producer();


            boost::asio::io_service& getIOService() {
                return EventLoop::getIOService();
            }


            unsigned int getPort() const {
                return m_port;
            }

            bool publish(const std::string& slotInstanceId,
                         const karabo::util::Hash::Pointer& header,
                         const karabo::util::Hash::Pointer& body,
                         int prio);

            void publishIfConnected(std::map<std::string, std::set<std::string> >& registeredSlots,
                                    const karabo::util::Hash::Pointer& header,
                                    const karabo::util::Hash::Pointer& message, int prio);

            void connectHandler(const karabo::net::ErrorCode& e, const karabo::net::Channel::Pointer& channel);

            void onSubscribe(const ErrorCode& e, const karabo::net::Channel::Pointer& channel, const std::string& subscription);

            void stop();

        private:

            void updateHeader(const karabo::util::Hash::Pointer& header,
                              const std::map<std::string, std::set<std::string> >& registeredSlots) const;

            void channelErrorHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

        private:

            unsigned int m_port;
            Connection::Pointer m_connection;
            ChannelToSlotInstanceIds m_registeredChannels;
            boost::shared_mutex m_registeredChannelsMutex;

        };


        class PointToPoint::Consumer : public boost::enable_shared_from_this<PointToPoint::Consumer> {

            // SlotInstanceIds :  slotInstanceId  --> ConsumeHandler ( function<Hash::Pointer,Hash::Pointer> )
            typedef std::map<std::string, ConsumeHandler> SlotInstanceIds;

            // ConnectedInstances : signalInstanceId --> pair of (signalConnectionString, slotInstanceIds)
            typedef std::map<std::string, std::pair<std::string, SlotInstanceIds> > ConnectedInstances;

            // OpenConnections : connectionString (like "tcp://host:port") -->  pair of (Connection::Pointer, Channel::Pointer)
            typedef std::map<std::string, std::pair<karabo::net::Connection::Pointer, karabo::net::Channel::Pointer> > OpenConnections;

        public:

            KARABO_CLASSINFO(Consumer, "PointToPointConsumer", "1.0")

            Consumer();

            virtual ~Consumer();

            /**
             * Connect slotInstanceId to signalInstanceId using signalConnectionString (tcp://host:port) and process
             * incoming messages in ConsumeHandler (read handler)
             * @param signalInstanceId
             * @param slotInstanceId
             * @param signalConnectionString
             * @param handler
             */
            void connect(const std::string& signalInstanceId,
                         const std::string& slotInstanceId,
                         const std::string& signalConnectionString,
                         const karabo::net::ConsumeHandler& handler);

            void disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId);

            void consume(const karabo::net::ErrorCode& ec,
                         const std::string& signalConnectionString,
                         const karabo::net::Connection::Pointer& connection,
                         const karabo::net::Channel::Pointer& channel,
                         karabo::util::Hash::Pointer& header,
                         karabo::util::Hash::Pointer& body);

        private:


            void connectionErrorHandler(const karabo::net::ErrorCode& ec,
                                        const std::string& signalInstanceId,
                                        const std::string& signalConnectionString,
                                        const karabo::net::Connection::Pointer& connection);

            void channelErrorHandler(const karabo::net::ErrorCode& ec,
                                     const std::string& signalConnectionString,
                                     const karabo::net::Connection::Pointer& connection,
                                     const karabo::net::Channel::Pointer& channel);


            void storeTcpConnectionInfo(const std::string& signalConnectionString,
                                        const karabo::net::Connection::Pointer& connection,
                                        const karabo::net::Channel::Pointer& channel) {
                m_openConnections[signalConnectionString] = std::make_pair(connection, channel);
            }

            /** To be called under protection of 'boost::mutex::scoped_lock lock(m_connectedInstancesMutex);'.
             */
            void storeSignalSlotConnectionInfo(const std::string& slotInstanceId,
                                               const std::string& signalInstanceId,
                                               const std::string& signalConnectionString,
                                               const ConsumeHandler& handler);

            void connectHandler(const karabo::net::ErrorCode& ec,
                                const std::string& subscription,
                                const std::string& instanceId,
                                const std::string& connectionString,
                                const karabo::net::ConsumeHandler& handler,
                                const karabo::net::Connection::Pointer& connection,
                                const karabo::net::Channel::Pointer& channel);

        private:

            OpenConnections m_openConnections; // key: connection_string 
            ConnectedInstances m_connectedInstances;
            boost::mutex m_connectedInstancesMutex;
            // key is signalConnectionString, value is list of tuple(slotInstanceId, signalInstanceId, handler)
            // Before C++14 not an unordered_map since we want to erase an iterator while looping on it.
            typedef std::map<std::string, std::list<std::tuple<std::string, std::string, karabo::net::ConsumeHandler> > > PendingSubscriptionsMap;
            PendingSubscriptionsMap m_pendingSubscriptions; // to be protected by m_connectedInstancesMutex

        };


        PointToPoint::Producer::Producer()
            : m_port(0)
            , m_connection()
            , m_registeredChannels() {
            m_connection = Connection::create(Hash("Tcp.port", 0, "Tcp.type", "server"));
            // No bind_weak in constructor possible yet...
            m_port = m_connection->startAsync(boost::bind(&PointToPoint::Producer::connectHandler, this, _1, _2));
        }


        PointToPoint::Producer::~Producer() {
        }


        void PointToPoint::Producer::channelErrorHandler(const ErrorCode& ec, const Channel::Pointer& channel) {
            {
                // Could use shared_lock here and promote to unique_lock before erase...
                boost::unique_lock<boost::shared_mutex> lock(m_registeredChannelsMutex);
                for (ChannelToSlotInstanceIds::const_iterator it = m_registeredChannels.begin(); it != m_registeredChannels.end(); ++it) {
                    if (it->first.get() == channel.get()) {
                        m_registeredChannels.erase(it);
                        break;
                    }
                }
            }
            if (channel) channel->close();
        }


        void PointToPoint::Producer::connectHandler(const ErrorCode& e, const Channel::Pointer& channel) {
            if (e) {
                return;
            }
            m_connection->startAsync(bind_weak(&PointToPoint::Producer::connectHandler, this, _1, _2));
            channel->setAsyncChannelPolicy(3, "REMOVE_OLDEST");
            channel->setAsyncChannelPolicy(4, "LOSSLESS");
            channel->readAsyncString(bind_weak(&PointToPoint::Producer::onSubscribe, this, _1, channel, _2));
            boost::unique_lock<boost::shared_mutex> lock(m_registeredChannelsMutex);
            m_registeredChannels[channel] = std::unordered_set<std::string>();
        }


        void PointToPoint::Producer::onSubscribe(const ErrorCode& e, const karabo::net::Channel::Pointer& channel, const std::string& subscription) {
            if (e) {
                channelErrorHandler(e, channel);
                return;
            }

            vector<string> v;
            string s = subscription;
            boost::split(v, s, boost::is_any_of(" "));
            if (v.size() == 2) {
                const string& slotInstanceId = v[0];
                const string& command = v[1];

                boost::unique_lock<boost::shared_mutex> lock(m_registeredChannelsMutex);
                if (command == "SUBSCRIBE")
                    m_registeredChannels[channel].insert(slotInstanceId);
                else if (command == "UNSUBSCRIBE")
                    m_registeredChannels[channel].erase(slotInstanceId);
            } else {
                // Likely new version of p2p:
                KARABO_LOG_FRAMEWORK_WARN << "'onSubscribe' received incompatible subscription message: "
                        << subscription;
                channel->close();
                return;
            }
            // wait for next command
            channel->readAsyncString(bind_weak(&PointToPoint::Producer::onSubscribe, this, _1, channel, _2));
        }


        void PointToPoint::Producer::stop() {
            boost::unique_lock<boost::shared_mutex> lock(m_registeredChannelsMutex);
            for (ChannelToSlotInstanceIds::const_iterator it = m_registeredChannels.begin(); it != m_registeredChannels.end(); ++it) {
                const Channel::Pointer& channel = it->first;
                channel->close();
            }
            m_registeredChannels.clear();
            m_connection->stop();
            m_port = 0;
        }


        bool PointToPoint::Producer::publish(const std::string& slotInstanceId,
                                             const karabo::util::Hash::Pointer& header,
                                             const karabo::util::Hash::Pointer& body,
                                             int prio) {
            // A read-only lock since content of m_registeredChannels is not changed:
            boost::shared_lock<boost::shared_mutex> lock(m_registeredChannelsMutex);
            for (ChannelToSlotInstanceIds::const_iterator it = m_registeredChannels.begin(); it != m_registeredChannels.end(); ++it) {
                const Channel::Pointer& channel = it->first;
                const std::unordered_set<std::string>& slotInstanceIds = it->second;
                auto ii = slotInstanceIds.find(slotInstanceId);
                if (ii == slotInstanceIds.end()) continue;
                try {
                    // Concurrent writeAsync allowed since TcpChannel protects its queues itself.
                    // ==> No need to promote lock to unique_lock.
                    channel->writeAsync(*header, *body, prio);
                } catch (const karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_WARN << e;
                }
                return true;
            }
            return false;
        }


        void PointToPoint::Producer::publishIfConnected(std::map<std::string, std::set<std::string> >& registeredSlots,
                                                        const karabo::util::Hash::Pointer& header,
                                                        const karabo::util::Hash::Pointer& message, int prio) {
            if (registeredSlots.empty()) return;

            // A read-only lock since content of m_registeredChannels is not changed:
            boost::shared_lock<boost::shared_mutex> lock(m_registeredChannelsMutex);

            for (ChannelToSlotInstanceIds::const_iterator mapItChannelSlotInstIds = m_registeredChannels.cbegin();
                 mapItChannelSlotInstIds != m_registeredChannels.cend(); ++mapItChannelSlotInstIds) {

                const Channel::Pointer& channel = mapItChannelSlotInstIds->first;
                const std::unordered_set<string>& slotInstanceIds = mapItChannelSlotInstIds->second;
                std::map<std::string, std::set<std::string> > slotsToUse;

                for (auto iSlotInstId = slotInstanceIds.cbegin(); iSlotInstId != slotInstanceIds.cend(); ++iSlotInstId) {
                    const string& slotInstanceId = *iSlotInstId;
                    std::map<std::string, std::set<std::string> >::iterator it = registeredSlots.find(slotInstanceId);
                    if (it == registeredSlots.end()) continue;
                    slotsToUse[slotInstanceId] = it->second;
                    registeredSlots.erase(it); // filter out
                }

                if (slotsToUse.empty()) continue;

                updateHeader(header, slotsToUse);
                try {
                    channel->writeAsync(*header, *message, prio);
                } catch (const karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_WARN << e;
                }
            }

            updateHeader(header, registeredSlots);
        }


        void PointToPoint::Producer::updateHeader(const karabo::util::Hash::Pointer& header, const std::map<std::string, std::set<std::string> >& registeredSlots) const {
            string registeredSlotsString;
            string registeredSlotInstanceIdsString;

            for (std::map<std::string, std::set<std::string> >::const_iterator it = registeredSlots.begin(); it != registeredSlots.end(); ++it) {
                registeredSlotInstanceIdsString += "|" + it->first + "|";
                registeredSlotsString += "|" + it->first + ":" + karabo::util::toString(it->second) + "|";
            }

            header->set("slotInstanceIds", registeredSlotInstanceIdsString);
            header->set("slotFunctions", registeredSlotsString);
        }


        PointToPoint::Consumer::Consumer() {
        }


        PointToPoint::Consumer::~Consumer() {
        }


        void PointToPoint::Consumer::connectionErrorHandler(const karabo::net::ErrorCode& ec,
                                                            const std::string& signalInstanceId,
                                                            const std::string& signalConnectionString,
                                                            const karabo::net::Connection::Pointer& connection) {

            KARABO_LOG_FRAMEWORK_WARN << "Point-to-point connection to \"" << signalInstanceId << "\" using \"" << signalConnectionString
                    << "\" failed.  Code " << ec.value() << " -- \"" << ec.message() << "\"";

            karabo::net::Channel::Pointer channel;
            {
                boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

                for (ConnectedInstances::iterator i = m_connectedInstances.begin(); i != m_connectedInstances.end(); ) {
                    if (i->second.first == signalConnectionString) {
                        i = m_connectedInstances.erase(i);
                    } else {
                        ++i;
                    }
                }

                OpenConnections::iterator ii = m_openConnections.find(signalConnectionString);
                if (ii != m_openConnections.end()) {
                    channel = ii->second.second;
                    m_openConnections.erase(ii);
                }
            }
            if (channel) channel->close();
            if (connection) connection->stop();
        }


        void PointToPoint::Consumer::channelErrorHandler(const karabo::net::ErrorCode& ec,
                                                         const std::string& signalConnectionString,
                                                         const karabo::net::Connection::Pointer& connection,
                                                         const karabo::net::Channel::Pointer& channel) {
            
            KARABO_LOG_FRAMEWORK_WARN << "karabo::net::Channel to \"" << signalConnectionString
                    << "\" failed.  Code " << ec.value() << " -- \"" << ec.message() << "\"";

            {
                boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

                for (ConnectedInstances::iterator i = m_connectedInstances.begin(); i != m_connectedInstances.end(); ) {
                    if (i->second.first == signalConnectionString) {
                        i = m_connectedInstances.erase(i);
                    } else {
                        ++i;
                    }
                }

                OpenConnections::iterator ii = m_openConnections.find(signalConnectionString);
                if (ii != m_openConnections.end()) m_openConnections.erase(ii);
            }
            if (channel) channel->close();
        }


        void PointToPoint::Consumer::storeSignalSlotConnectionInfo(const std::string& slotInstanceId,
                                                                   const std::string& signalInstanceId,
                                                                   const std::string& signalConnectionString,
                                                                   const ConsumeHandler& handler) {
            if (m_connectedInstances.find(signalInstanceId) == m_connectedInstances.end()) {
                m_connectedInstances[signalInstanceId] = std::make_pair(signalConnectionString, Consumer::SlotInstanceIds());
            }
            SlotInstanceIds& slotInstanceIds = m_connectedInstances[signalInstanceId].second;
            if (slotInstanceIds.find(slotInstanceId) == slotInstanceIds.end()) {
                slotInstanceIds[slotInstanceId] = handler;
            }
        }


        void PointToPoint::Consumer::connectHandler(const karabo::net::ErrorCode& ec,
                                                    const std::string& slotInstanceId,
                                                    const std::string& signalInstanceId,
                                                    const std::string& signalConnectionString,
                                                    const ConsumeHandler& handler,
                                                    const karabo::net::Connection::Pointer& connection,
                                                    const karabo::net::Channel::Pointer& channel) {

            if (ec) {
                channelErrorHandler(ec, signalConnectionString, connection, channel);
                return;
            }
            // bookkeeping ...
            std::vector<std::string> pendingInstanceIds;
            {
                boost::mutex::scoped_lock lock(m_connectedInstancesMutex);
                storeTcpConnectionInfo(signalConnectionString, connection, channel);
                storeSignalSlotConnectionInfo(slotInstanceId, signalInstanceId, signalConnectionString, handler);
                // Also for pending stuff:
                auto iter = m_pendingSubscriptions.find(signalConnectionString);
                if (iter != m_pendingSubscriptions.end()) {
                    for (const auto& tup : iter->second) {
                        // tup contains slotInstanceId, signalInstanceId, handler for 0, 1, 2
                        storeSignalSlotConnectionInfo(std::get<0>(tup), std::get<1>(tup), signalConnectionString, std::get<2>(tup));
                        pendingInstanceIds.push_back(std::get<1>(tup));
                    }
                    m_pendingSubscriptions.erase(iter);
                }
            }

            // Subscribe to producer with our own instanceId
            KARABO_LOG_FRAMEWORK_DEBUG << "SUBSCRIBE " << slotInstanceId << " to new connection to " << signalConnectionString;
            channel->write(slotInstanceId + " SUBSCRIBE");
            for (const std::string& slotInstance : pendingInstanceIds) {
                KARABO_LOG_FRAMEWORK_DEBUG << "SUBSCRIBE pending " << slotInstance << " to " << signalConnectionString;
                channel->write(slotInstance + " SUBSCRIBE");
            }
            // ... and, finally, wait for publications ...
            channel->readAsyncHashPointerHashPointer(bind_weak(&Consumer::consume, this, _1,
                                                               signalConnectionString, connection, channel, _2, _3));
        }


        void PointToPoint::Consumer::connect(const std::string& signalInstanceId,
                                             const std::string& slotInstanceId,
                                             const std::string& signalConnectionString,
                                             const karabo::net::ConsumeHandler& handler) {

            boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

            // Are we not connected yet to the "signalInstanceId" producer...
            if (m_connectedInstances.find(signalInstanceId) == m_connectedInstances.end()) {
                // Check if the TCP connection does not exist yet ...
                auto connectionsIter = m_openConnections.find(signalConnectionString);
                if (connectionsIter == m_openConnections.end()) {
                    Hash params("type", "client");
                    {
                        vector<string> v;

                        string hostport = signalConnectionString.substr(6); // tcp://host:port
                        boost::split(v, hostport, boost::is_any_of(":"));
                        params.set("hostname", v[0]);
                        params.set("port", fromString<unsigned int>(v[1]));
                    }

                    // Store empty connection/channel pointers to mark that we are preparing them.
                    storeTcpConnectionInfo(signalConnectionString, Connection::Pointer(), Channel::Pointer());

                    Connection::Pointer connection = Connection::create(Hash("Tcp", params));
                    connection->startAsync(bind_weak(&Consumer::connectHandler, this, _1, slotInstanceId,
                                                     signalInstanceId, signalConnectionString, handler, connection, _2));

                } else if (connectionsIter->second.first) { // connection already there
                    // bookkeeping ...
                    storeSignalSlotConnectionInfo(slotInstanceId, signalInstanceId, signalConnectionString, handler);

                    // Subscribe to producer with slotInstanceId
                    auto& channelPtr = connectionsIter->second.second;
                    KARABO_LOG_FRAMEWORK_DEBUG << "SUBSCRIBE " << slotInstanceId << " to established connection to " << signalConnectionString;
                    channelPtr->write(slotInstanceId + " SUBSCRIBE");
                } else { // connection is being established
                    // store what later has to be done for subscription: storeSignalSlotConnectionInfo, write
                    auto& allTuples = m_pendingSubscriptions[signalConnectionString];
                    const auto& tup = std::make_tuple(slotInstanceId, signalInstanceId, handler);
                    allTuples.push_back(tup);
                }

            }
            // Connected!
        }


        void PointToPoint::Consumer::disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId) {

            boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

            // An iterator pointing to a pair of a connection string and SlotInstanceIds
            ConnectedInstances::iterator itConnectStringSlotIds = m_connectedInstances.find(signalInstanceId);
            if (itConnectStringSlotIds == m_connectedInstances.end()) {
                // Instance not yet connected, but check also pending stuff:
                for (PendingSubscriptionsMap::iterator itPending = m_pendingSubscriptions.begin(),
                     itEnd = m_pendingSubscriptions.end(); itPending != itEnd;) {
                    // key is signalConnectionString, value is list of tuple(slotInstanceId, signalInstanceId, handler)
                    auto& allTuples = itPending->second;
                    for (auto itTuple = allTuples.begin(), itEnd = allTuples.end(); itTuple != itEnd;) {
                        if (std::get<1>(*itTuple) == signalInstanceId) {
                            KARABO_LOG_FRAMEWORK_DEBUG << "Disconnect pending " << signalInstanceId;
                            itTuple = allTuples.erase(itTuple);
                        } else {
                            ++itTuple;
                        }
                    }
                    if (allTuples.empty()) {
                        itPending = m_pendingSubscriptions.erase(itPending);
                    } else {
                        ++itPending;
                    }
                }
                return;
            }

            KARABO_LOG_FRAMEWORK_INFO << "Disconnect signalId '" << signalInstanceId
                    << "' from slotId '" << slotInstanceId << "'.";

            const string& signalConnectionString = itConnectStringSlotIds->second.first;

            // un-subscribe from producer
            auto& connectionChannelPair = m_openConnections[signalConnectionString];
            auto& channel = connectionChannelPair.second;
            if (channel && channel->isOpen()) {
                // Safety check - connection should always exist if signalInstanceId in m_connectedInstances
                channel->write(slotInstanceId + " UNSUBSCRIBE");
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Channel to unsubscribe not open (connection string is '"
                        << signalConnectionString << "').";
            }

            // Remove handler for the slotInstanceId
            SlotInstanceIds& slotInstanceIds = itConnectStringSlotIds->second.second;
            slotInstanceIds.erase(slotInstanceId);

            // Check if TCP connection should be closed:
            // Try to find signalConnectionString among _other_ connectedInstances.
            bool found = false;
            for (auto i = m_connectedInstances.begin(); i != m_connectedInstances.end(); ++i) {
                if (i != itConnectStringSlotIds && i->second.first == signalConnectionString) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                KARABO_LOG_FRAMEWORK_INFO << "Close TCP channel/connection to: '" << signalConnectionString << "'.";
                if (channel) channel->close();
                if (connectionChannelPair.first) {
                    connectionChannelPair.first->stop();
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "No connection to '" << signalConnectionString << "' to close!";
                }
                m_openConnections.erase(signalConnectionString);
            }

            // If no slotInstanceIds left for that signalConnectionString, we erase the complete entry.
            // (Do that at the very end since there are references to things belonging to 'itConnectStringSlotIds'.)
            if (slotInstanceIds.empty()) m_connectedInstances.erase(itConnectStringSlotIds);
        }


        void PointToPoint::Consumer::consume(const karabo::net::ErrorCode& ec,
                                             const std::string& signalConnectionString,
                                             const karabo::net::Connection::Pointer& connection,
                                             const karabo::net::Channel::Pointer& channel,
                                             karabo::util::Hash::Pointer& header,
                                             karabo::util::Hash::Pointer& body) {

            if (ec) {
                channelErrorHandler(ec, signalConnectionString, connection, channel);
                return;
            }
            
            // Re-register itself
            channel->readAsyncHashPointerHashPointer(bind_weak(&Consumer::consume, this, _1, signalConnectionString,
                                                               connection, channel, _2, _3));
            // Get signalInstancdId and slotInstanceIds from header
            const string& signalInstanceId = header->get<string>("signalInstanceId");
            const string& slotInstanceIdsString = header->get<string>("slotInstanceIds");

            // Split into vector of "slotInstanceId"
            vector<string> ids;
            boost::split(ids, slotInstanceIdsString, boost::is_any_of("|"), boost::token_compress_on);

            // Try to call all slot handlers
            for (vector<string>::iterator i = ids.begin(); i != ids.end(); ++i) {
                if (i->empty())
                    continue;
                const string& slotInstanceId = *i;

                ConsumeHandler handler;
                {
                    boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

                    ConnectedInstances::iterator ii = m_connectedInstances.find(signalInstanceId);
                    if (ii == m_connectedInstances.end()) return;
                    SlotInstanceIds& slotInstanceIds = ii->second.second;

                    SlotInstanceIds::iterator iii = slotInstanceIds.find(slotInstanceId);
                    if (iii == slotInstanceIds.end()) continue;
                    handler = iii->second;
                }
                // call user callback of type "ConsumeHandler"
                handler(header, body);
            }
        }


        PointToPoint::PointToPoint() : m_producer(new Producer), m_consumer(new Consumer) {
            EventLoop::addThread();
        }


        PointToPoint::~PointToPoint() {
            m_producer->stop();
            EventLoop::removeThread();
        }


        void PointToPoint::connect(const std::string& signalInstanceId, const std::string& slotInstanceId,
                                   const std::string& signalConnectionString, const karabo::net::ConsumeHandler& handler) {
            m_consumer->connect(signalInstanceId, slotInstanceId, signalConnectionString, handler);
        }


        void PointToPoint::disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId) {
            m_consumer->disconnect(signalInstanceId, slotInstanceId);
        }


        bool PointToPoint::publish(const std::string& slotInstanceId,
                                   const karabo::util::Hash::Pointer& header,
                                   const karabo::util::Hash::Pointer& body,
                                   int prio) {
            return m_producer->publish(slotInstanceId, header, body, prio);
        }


        void PointToPoint::publishIfConnected(std::map<std::string, std::set<std::string> >& registeredSlots,
                                              const karabo::util::Hash::Pointer& header,
                                              const karabo::util::Hash::Pointer& message, int prio) {
            m_producer->publishIfConnected(registeredSlots, header, message, prio);
        }


        std::string PointToPoint::getConnectionString() const {
            ostringstream oss;
            oss << "tcp://" << boost::asio::ip::host_name() << ":" << m_producer->getPort();
            return oss.str();
        }
    }
}
