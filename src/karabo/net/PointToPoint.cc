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

            /**
             * Create Producer object that is internal to PointToPoint.
             *
             * Note that full initialisation requires to call the start() method after the constructor
             *
             */
            Producer();

            virtual ~Producer();

            /**
             * Second constructor to be called to finally initialise a Producer object.
             *
             * Will make use of karabo::util::bind_weak and thus cannot be used in constructor directly.
             */
            void start();


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

        private:

            void consume(const karabo::net::ErrorCode& ec,
                         const std::string& signalConnectionString,
                         const karabo::net::Connection::Pointer& connection,
                         const karabo::net::Channel::Pointer& channel,
                         karabo::util::Hash::Pointer& header,
                         karabo::util::Hash::Pointer& body);

            void channelErrorHandler(const karabo::net::ErrorCode& ec,
                                     const std::string& signalConnectionString,
                                     const karabo::net::Connection::Pointer& connection,
                                     const karabo::net::Channel::Pointer& channel);


            /** To be called under protection of 'boost::mutex::scoped_lock lock(m_connectedInstancesMutex);'.
             */
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
        }


        void PointToPoint::Producer::start() {
            if (m_port != 0) {
                KARABO_LOG_FRAMEWORK_ERROR << "start() called although port number already initialised to " << m_port;
            }
            // No bind_weak in constructor possible yet - but now:
            m_port = m_connection->startAsync(bind_weak(&PointToPoint::Producer::connectHandler, this, _1, _2));
        }


        PointToPoint::Producer::~Producer() {
        }


        void PointToPoint::Producer::channelErrorHandler(const ErrorCode& ec, const Channel::Pointer& channel) {

            boost::unique_lock<boost::shared_mutex> lock(m_registeredChannelsMutex);
            ChannelToSlotInstanceIds::const_iterator it = m_registeredChannels.find(channel);
            if (it != m_registeredChannels.end()) {
                // it->second is an std::unordered_set - an std::set could be directly passed to toString
                const std::string slotIds(toString(std::vector<std::string>(it->second.begin(), it->second.end())));
                KARABO_LOG_FRAMEWORK_WARN << "Channel to slotInstanceIds '" << slotIds << "' received error. Code "
                        << ec.value() << ", i.e. '" << ec.message() << "'. Erase channel!";
                m_registeredChannels.erase(it);
                // channel will be destructed (and thus closed) since not in any container anymore
            } else {
                // Output also raw pointer to check whether it is null:
                KARABO_LOG_FRAMEWORK_ERROR << "channelErrorHandler called for unknown channel " << channel.get();
                // Better close this zombie guy:
                if (channel) channel->close();
            }
        }


        void PointToPoint::Producer::connectHandler(const ErrorCode& e, const Channel::Pointer& channel) {
            if (e) {
                return;
            }
            m_connection->startAsync(bind_weak(&PointToPoint::Producer::connectHandler, this, _1, _2));
            channel->setAsyncChannelPolicy(3, "REMOVE_OLDEST");
            channel->setAsyncChannelPolicy(4, "LOSSLESS");
            channel->readAsyncString(bind_weak(&PointToPoint::Producer::onSubscribe, this, _1, channel, _2));
            // No need to put already something into m_registeredChannels - will happen in onSubscribe.
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
                auto& slotInstanceIds = m_registeredChannels[channel]; // construct if not yet there
                if (command == "SUBSCRIBE") {
                    slotInstanceIds.insert(slotInstanceId);
                } else if (command == "UNSUBSCRIBE") {
                    const bool erased = (slotInstanceIds.erase(slotInstanceId) > 0);
                    if (slotInstanceIds.empty()) {
                        KARABO_LOG_FRAMEWORK_INFO << "Disconnect channel after erasing slotInstanceId '"
                                << slotInstanceId << "'. " << erased;
                        m_registeredChannels.erase(channel); // invalidates slotInstanceIds!
                        return; // channel will be destructed (and thus closed) since not in any container anymore
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "'onSubscribe' received bad subscription message: " << subscription;
                    // But just go on...
                }
            } else {
                // Likely new version of p2p:
                KARABO_LOG_FRAMEWORK_WARN << "'onSubscribe' received incompatible subscription message: "
                        << subscription;
                m_registeredChannels.erase(channel);
                return; // channel will be destructed (and thus closed) since not in any container anymore
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
                if (slotInstanceIds.find(slotInstanceId) == slotInstanceIds.end()) {
                    continue;
                }
                try {
                    // Concurrent writeAsync allowed since TcpChannel protects its queues itself.
                    // ==> No need to promote lock to unique_lock.
                    channel->writeAsync(*header, *body, prio);
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_WARN << "publish failed: " << e.what();
                    return false;
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
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_WARN << "publishIfConnected failed: " << e.what();
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

            std::vector<std::string> pendingInstanceIds(1, slotInstanceId);
            std::vector<std::string> connectionsText(1, signalInstanceId + " -> " + slotInstanceId);
            {
                boost::mutex::scoped_lock lock(m_connectedInstancesMutex);
                auto iterPending = m_pendingSubscriptions.find(signalConnectionString);
                if (ec) {
                    // remove placeholder of empty connection/channel pointers
                    m_openConnections.erase(signalConnectionString);

                    // Collect all failed connections, including pending ones:
                    if (iterPending != m_pendingSubscriptions.end()) {
                        for (const auto& tup : iterPending->second) {
                            // tup contains slotInstanceId and signalInstanceId for indices 0 and 1, respectively.
                            connectionsText.push_back(std::get<1>(tup) + " -> " + std::get<0>(tup));
                        }
                        m_pendingSubscriptions.erase(iterPending);
                    }
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to establish Tcp connection to '" << signalConnectionString
                            << "' for following connections: " << toString(connectionsText)
                            << ". Code " << ec.value() << ", i.e. '" << ec.message() << "'";
                    return;
                }

                // Now overwrite place holders of empty pointers
                storeTcpConnectionInfo(signalConnectionString, connection, channel);
                // Store connection info - also for pending stuff:
                storeSignalSlotConnectionInfo(slotInstanceId, signalInstanceId, signalConnectionString, handler);

                if (iterPending != m_pendingSubscriptions.end()) {
                    for (const auto& tup : iterPending->second) {
                        // tup contains slotInstanceId, signalInstanceId, handler for 0, 1, 2
                        storeSignalSlotConnectionInfo(std::get<0>(tup), std::get<1>(tup), signalConnectionString, std::get<2>(tup));
                        pendingInstanceIds.push_back(std::get<0>(tup)); // we subscribe slotInstanceIds!
                        connectionsText.push_back(std::get<1>(tup) + " -> " + std::get<0>(tup));
                    }
                    m_pendingSubscriptions.erase(iterPending);
                }
            }

            // Subscribe to producer with all instance ids.
            KARABO_LOG_FRAMEWORK_INFO << "Subscribe to new connection to '" << signalConnectionString << "' for '"
                    << toString(connectionsText) << "'";
            for (const std::string& slotInstance : pendingInstanceIds) {
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

            // Check if the TCP connection does not exist yet ...
            auto connectionsIter = m_openConnections.find(signalConnectionString);
            if (connectionsIter == m_openConnections.end()) {
                Hash params("type", "client");
                {
                    vector<string> v;

                    string hostport = signalConnectionString.substr(6); // tcp://host:port
                    boost::split(v, hostport, boost::is_any_of(":"));
                    if (v.size() != 2) {
                        std::string msg("Invalid connection string not matching 'tcp://host:port': ");
                        throw KARABO_PARAMETER_EXCEPTION(msg += signalConnectionString);
                    }
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
                KARABO_LOG_FRAMEWORK_INFO << "Subscribe to established connection to '" << signalConnectionString
                        << "' for '" << signalInstanceId << " --> " << slotInstanceId << "'";
                channelPtr->write(slotInstanceId + " SUBSCRIBE");
            } else { // connection is being established
                // store what later has to be done for subscription: storeSignalSlotConnectionInfo, write
                auto& allTuples = m_pendingSubscriptions[signalConnectionString];
                const auto& tup = std::make_tuple(slotInstanceId, signalInstanceId, handler);
                allTuples.push_back(tup);
            }
            // Connected!
        }


        void PointToPoint::Consumer::disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId) {

            boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

            // An iterator pointing to a pair of a connection string and SlotInstanceIds
            ConnectedInstances::iterator itConnectStringSlotIds = m_connectedInstances.find(signalInstanceId);
            if (itConnectStringSlotIds == m_connectedInstances.end()) {
                // Instance not yet connected, but check also pending stuff (which exists only when no connection):
                for (PendingSubscriptionsMap::iterator itPending = m_pendingSubscriptions.begin(),
                     itEnd = m_pendingSubscriptions.end(); itPending != itEnd;) {
                    // key is signalConnectionString, value is list of tuple(slotInstanceId, signalInstanceId, handler)
                    auto& allTuples = itPending->second;
                    for (auto itTuple = allTuples.begin(), itEnd = allTuples.end(); itTuple != itEnd;) {
                        if (std::get<1>(*itTuple) == signalInstanceId && std::get<0>(*itTuple) == slotInstanceId) {
                            KARABO_LOG_FRAMEWORK_INFO << "Disconnect pending signalInstId '" << signalInstanceId
                                    << "' from slotInstId '" << slotInstanceId << "'.";
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

            const string signalConnectionString = itConnectStringSlotIds->second.first; // by value: could get dangling

            // Remove handler for the slotInstanceId
            SlotInstanceIds& slotInstanceIds = itConnectStringSlotIds->second.second;
            slotInstanceIds.erase(slotInstanceId);
            // If no slotInstanceIds left for that signalConnectionString, we erase the complete entry.
            if (slotInstanceIds.empty()) {
                m_connectedInstances.erase(itConnectStringSlotIds);
            }

            // Loop on what is left for same signalConnectionString:
            bool foundConnection = false;
            bool foundSlotInstanceId = false;
            for (auto it = m_connectedInstances.begin(); it != m_connectedInstances.end(); ++it) {
                if (it->second.first != signalConnectionString) {
                    continue; // unrelated
                }
                foundConnection = true; // Connection still needed for others
                SlotInstanceIds& slotInstanceIds = it->second.second;
                if (slotInstanceIds.find(slotInstanceId) != slotInstanceIds.end()) {
                    // The given slotInstanceId should still receives p2p data from that connection
                    foundSlotInstanceId = true;
                    break; // no need to loop further
                }
            }

            // un-subscribe if and potentially
            auto itConnectionChannelPair = m_openConnections.find(signalConnectionString);
            if (foundConnection) {
                if (!foundSlotInstanceId) {
                    KARABO_LOG_FRAMEWORK_INFO << "Channel to '" << signalConnectionString << "' unsubscribes for '"
                            << slotInstanceId << "'";
                    auto& channel = itConnectionChannelPair->second.second;
                    if (channel && channel->isOpen()) {
                        // Safety check - connection should always exist if signalInstanceId in m_connectedInstances
                        channel->write(slotInstanceId + " UNSUBSCRIBE");
                    }
                } else {
                    KARABO_LOG_FRAMEWORK_INFO << "Channel to '" << signalConnectionString << "' does not unsubscribe "
                            << "for '" << slotInstanceId << "' since other signal ids than '" << signalInstanceId
                            << "' shall still send to it.";
                }
            } else {
                KARABO_LOG_FRAMEWORK_INFO << "Close connection to '" << signalConnectionString << "' since no need "
                        << "after disconnecting '" << signalInstanceId << " --> " << slotInstanceId << "'.";
                auto& channel = itConnectionChannelPair->second.second;
                if (channel) {
                    channel->close();
                }
                auto& connection = itConnectionChannelPair->second.first;
                if (connection) {
                    connection->stop();
                }
                m_openConnections.erase(itConnectionChannelPair);
            }
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
            
            // Get signalInstancdId and slotInstanceIds from header
            const string& signalInstanceId = header->get<string>("signalInstanceId");
            const string& slotInstanceIdsString = header->get<string>("slotInstanceIds");

            // Split into vector of "slotInstanceId"
            vector<string> ids;
            boost::split(ids, slotInstanceIdsString, boost::is_any_of("|"), boost::token_compress_on);

            // Try to call all slot handlers
            for (const string& slotInstanceId : ids) {
                if (slotInstanceId.empty()) continue;

                ConsumeHandler handler;
                {
                    boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

                    ConnectedInstances::iterator ii = m_connectedInstances.find(signalInstanceId);
                    if (ii == m_connectedInstances.end()) {
                        // FIXME on Producer side, but for a clean fix that needs a new SUBSCRIBE protocol!
                        // This is the fundamental failure of this PointToPoint business: The producer process sends
                        // all data to the slotInstanceId, i.e. from all signalers in its process, not only for the
                        // signalInstanceId provided in Consumer::connect(..). But on the Consumer side, the handler is
                        // registered specifically per signalInstanceId!
                        // So either:
                        // - Hack SignalSlotable and bind a handler that works with the static map of all SignalSlotables
                        //   (e.g. wrapping tryToCallDirectly by getting instanceId out of the header),
                        // - change the p2p protocol and SUBSCRIBE by also sending the signalInstanceId,
                        // - get rid of this p2p business at all :-|.
                        KARABO_LOG_FRAMEWORK_WARN << "Received message from '" << signalInstanceId << "' (to '"
                                << slotInstanceId << "'), but no connection known for that.";
                        channel->readAsyncHashPointerHashPointer(bind_weak(&Consumer::consume, this, _1, signalConnectionString,
                                                                           connection, channel, _2, _3));
                        return;
                    }
                    SlotInstanceIds& slotInstanceIds = ii->second.second;

                    SlotInstanceIds::iterator iii = slotInstanceIds.find(slotInstanceId);
                    if (iii == slotInstanceIds.end()) {
                        KARABO_LOG_FRAMEWORK_WARN << "Received message from '" << signalInstanceId << "' to '"
                                << slotInstanceId << "', but receiver not known.";
                        continue;
                    }
                    handler = iii->second;
                }
                // call user callback of type "ConsumeHandler"
                handler(header, body);
            }
            // Re-register itself - do at the very end to guarantee correct order of handler execution.
            channel->readAsyncHashPointerHashPointer(bind_weak(&Consumer::consume, this, _1, signalConnectionString,
                                                               connection, channel, _2, _3));
        }


        PointToPoint::PointToPoint() : m_producer(new Producer), m_consumer(new Consumer) {
            EventLoop::addThread();
            m_producer->start();
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
