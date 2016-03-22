#include <sstream>
#include <boost/asio.hpp>
#include <karabo/log.hpp>
#include "utils.hh"
#include "PointToPoint.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::log;

namespace karabo {
    namespace net {


        class PointToPoint::Producer {
            typedef std::map<Channel::Pointer, std::set<std::string> > ChannelToSlotInstanceIds;

        public:

            KARABO_CLASSINFO(Producer, "PointToPointProducer", "1.0")

            Producer();

            virtual ~Producer();


            IOService::Pointer getIOService() const {
                return m_connection->getIOService();
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

            void connectHandler(const karabo::net::Channel::Pointer& channel);

            void onSubscribe(const karabo::net::Channel::Pointer& channel, const std::string& subscription);

            void channelErrorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec);

            void stop();

        private:

            void updateHeader(const karabo::util::Hash::Pointer& header,
                              const std::map<std::string, std::set<std::string> >& registeredSlots) const;

        private:

            unsigned int m_port;
            Connection::Pointer m_connection;
            ChannelToSlotInstanceIds m_registeredChannels;
            boost::mutex m_registeredChannelsMutex;

        };


        class PointToPoint::Consumer {
            typedef std::map<std::string, ConsumeHandler> SlotInstanceIds;
            // Mapping signalInstanceId into signalConnectionString like "tcp://host:port" and set of pairs of slotInstanceId and ConsumeHandler
            typedef std::map<std::string, std::pair<std::string, SlotInstanceIds> > ConnectedInstances;
            // Mapping connectionString into (Connection, Channel) pointers
            typedef std::map<std::string, std::pair<karabo::net::Connection::Pointer, karabo::net::Channel::Pointer> > OpenConnections;

        public:

            KARABO_CLASSINFO(Consumer, "PointToPointConsumer", "1.0")

            Consumer();

            virtual ~Consumer();

            void connectionErrorHandler(const std::string& signalInstanceId,
                                        const std::string& signalConnectionString,
                                        const karabo::net::Connection::Pointer& connection,
                                        const karabo::net::ErrorCode& ec);

            void channelErrorHandler(const std::string& signalInstanceId,
                                     const std::string& signalConnectionString,
                                     const karabo::net::Connection::Pointer& connection,
                                     const karabo::net::Channel::Pointer& channel,
                                     const karabo::net::ErrorCode& ec);

            void connect(const karabo::net::IOService::Pointer& svc,
                         const std::string& signalInstanceId,
                         const std::string& slotInstanceId,
                         const std::string& signalConnectionString,
                         const karabo::net::ConsumeHandler& handler);

            void disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId);

            void consume(const karabo::net::Channel::Pointer& channel,
                         karabo::util::Hash::Pointer& header,
                         karabo::util::Hash::Pointer& body);

        private:


            void storeTcpConnectionInfo(const std::string& signalConnectionString,
                                        const karabo::net::Connection::Pointer& connection,
                                        const karabo::net::Channel::Pointer& channel) {
                if (m_openConnections.find(signalConnectionString) == m_openConnections.end())
                    m_openConnections[signalConnectionString] = std::make_pair(connection, channel);
            }

            void storeSignalSlotConnectionInfo(const std::string& slotInstanceId,
                                               const std::string& signalInstanceId,
                                               const std::string& signalConnectionString,
                                               const ConsumeHandler& handler);

            void connectHandler(const std::string& subscription,
                                const std::string& instanceId,
                                const std::string& connectionString,
                                const karabo::net::ConsumeHandler& handler,
                                const karabo::net::Connection::Pointer& connection,
                                const karabo::net::Channel::Pointer& channel);

        private:

            OpenConnections m_openConnections; // key: connection_string 
            ConnectedInstances m_connectedInstances;
            boost::mutex m_connectedInstancesMutex;

        };


        PointToPoint::Producer::Producer()
        : m_port(0)
        , m_connection()
        , m_registeredChannels() {
            m_connection = Connection::create(Hash("Tcp.port", 0, "Tcp.type", "server"));
            m_port = m_connection->startAsync(boost::bind(&PointToPoint::Producer::connectHandler, this, _1));
        }


        PointToPoint::Producer::~Producer() {
        }


        void PointToPoint::Producer::channelErrorHandler(const Channel::Pointer& channel, const ErrorCode& ec) {
            {
                boost::mutex::scoped_lock lock(m_registeredChannelsMutex);
                for (map<Channel::Pointer, std::set<string> >::iterator it = m_registeredChannels.begin(); it != m_registeredChannels.end(); ++it) {
                    if (it->first.get() == channel.get()) {
                        m_registeredChannels.erase(it);
                        break;
                    }
                }
            }
            channel->close();
        }


        void PointToPoint::Producer::connectHandler(const Channel::Pointer& channel) {
            m_connection->startAsync(boost::bind(&PointToPoint::Producer::connectHandler, this, _1));
            channel->setErrorHandler(boost::bind(&PointToPoint::Producer::channelErrorHandler, this, channel, _1));
            channel->setAsyncChannelPolicy(3, "REMOVE_OLDEST");
            channel->setAsyncChannelPolicy(4, "LOSSLESS");
            channel->readAsyncString(boost::bind(&PointToPoint::Producer::onSubscribe, this, channel, _1));
            boost::mutex::scoped_lock lock(m_registeredChannelsMutex);
            m_registeredChannels[channel] = std::set<std::string>();
        }


        void PointToPoint::Producer::onSubscribe(const karabo::net::Channel::Pointer& channel, const std::string& subscription) {
            vector<string> v;
            string s = subscription;
            boost::split(v, s, boost::is_any_of(" "));
            const string& slotInstanceId = v[0];
            const string& command = v[1];

            //KARABO_LOG_FRAMEWORK_DEBUG << "Consumer registration message : \"" << subscription << "\".";

            boost::mutex::scoped_lock lock(m_registeredChannelsMutex);
            if (command == "SUBSCRIBE")
                m_registeredChannels[channel].insert(slotInstanceId);
            else if (command == "UNSUBSCRIBE")
                m_registeredChannels[channel].erase(slotInstanceId);
            // wait for next command
            channel->readAsyncString(boost::bind(&PointToPoint::Producer::onSubscribe, this, channel, _1));
        }


        void PointToPoint::Producer::stop() {
            boost::mutex::scoped_lock lock(m_registeredChannelsMutex);
            for (map<Channel::Pointer, std::set<string> >::iterator it = m_registeredChannels.begin(); it != m_registeredChannels.end(); ++it) {
                const Channel::Pointer& channel = it->first;
                channel->close();
            }
            m_registeredChannels.clear();
            m_connection->stop();
        }


        bool PointToPoint::Producer::publish(const std::string& slotInstanceId,
                                             const karabo::util::Hash::Pointer& header,
                                             const karabo::util::Hash::Pointer& body,
                                             int prio) {
            boost::mutex::scoped_lock lock(m_registeredChannelsMutex);
            for (map<Channel::Pointer, std::set<string> >::iterator it = m_registeredChannels.begin(); it != m_registeredChannels.end(); ++it) {
                const Channel::Pointer& channel = it->first;
                const std::set<std::string>& slotInstanceIds = it->second;
                std::set<string>::iterator ii = slotInstanceIds.find(slotInstanceId);
                if (ii == slotInstanceIds.end()) continue;
                channel->writeAsync(*header, *body, prio);
                return true;
            }
            return false;
        }


        void PointToPoint::Producer::publishIfConnected(std::map<std::string, std::set<std::string> >& registeredSlots,
                                                        const karabo::util::Hash::Pointer& header,
                                                        const karabo::util::Hash::Pointer& message, int prio) {
            if (registeredSlots.empty()) return;

            boost::mutex::scoped_lock lock(m_registeredChannelsMutex);

            for (Producer::ChannelToSlotInstanceIds::const_iterator ii = m_registeredChannels.begin();
                    ii != m_registeredChannels.end(); ++ii) {

                const Channel::Pointer& channel = ii->first;
                const std::set<string>& slotInstanceIds = ii->second;
                std::map<std::string, std::set<std::string> > slotsToUse;

                for (std::set<std::string>::const_iterator iii = slotInstanceIds.begin(); iii != slotInstanceIds.end(); ++iii) {
                    const string& slotInstanceId = *iii;
                    std::map<std::string, std::set<std::string> >::iterator it = registeredSlots.find(slotInstanceId);
                    if (it == registeredSlots.end()) continue;
                    slotsToUse[slotInstanceId] = it->second;
                    registeredSlots.erase(it); // filter out
                }

                if (slotsToUse.empty()) continue;

                updateHeader(header, slotsToUse);
                channel->writeAsync(*header, *message, prio);
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


        void PointToPoint::Consumer::connectionErrorHandler(const std::string& signalInstanceId,
                                                            const std::string& signalConnectionString,
                                                            const karabo::net::Connection::Pointer& connection,
                                                            const karabo::net::ErrorCode& ec) {
            if (ec.value() != 2 && ec.value() != 125) {
                KARABO_LOG_FRAMEWORK_WARN << "Point-to-point connection to \"" << signalInstanceId << "\" using \"" << signalConnectionString
                        << "\" failed.  Code " << ec.value() << " -- \"" << ec.message() << "\"";
            }

            karabo::net::Channel::Pointer channel;
            {
                boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

                for (ConnectedInstances::iterator i = m_connectedInstances.begin(); i != m_connectedInstances.end(); ++i) {
                    if (i->second.first == signalConnectionString) {
                        m_connectedInstances.erase(i);
                    }
                }

                OpenConnections::iterator ii = m_openConnections.find(signalConnectionString);
                if (ii != m_openConnections.end()) {
                    channel = ii->second.second;
                    m_openConnections.erase(ii);
                }
            }
            if (channel) channel->close();
            connection->stop();
        }


        void PointToPoint::Consumer::channelErrorHandler(const std::string& signalInstanceId,
                                                         const std::string& signalConnectionString,
                                                         const karabo::net::Connection::Pointer& connection,
                                                         const karabo::net::Channel::Pointer& channel,
                                                         const karabo::net::ErrorCode& ec) {
            if (ec.value() != 2 && ec.value() != 125) {
                KARABO_LOG_FRAMEWORK_WARN << "karabo::net::Channel to \"" << signalInstanceId
                        << "\" using \"" << signalConnectionString << "\" failed.  Code " << ec.value() << " -- \"" << ec.message() << "\"";
            }
            {
                boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

                for (ConnectedInstances::iterator i = m_connectedInstances.begin(); i != m_connectedInstances.end(); ++i) {
                    if (i->second.first == signalConnectionString) {
                        m_connectedInstances.erase(i);
                    }
                }

                OpenConnections::iterator ii = m_openConnections.find(signalConnectionString);
                if (ii != m_openConnections.end()) m_openConnections.erase(ii);
            }
            channel->close();
            connection->stop();
        }


        void PointToPoint::Consumer::storeSignalSlotConnectionInfo(const std::string& slotInstanceId,
                                                                   const std::string& signalInstanceId,
                                                                   const std::string& signalConnectionString,
                                                                   const ConsumeHandler& handler) {
            if (m_connectedInstances.find(signalInstanceId) == m_connectedInstances.end())
                m_connectedInstances[signalInstanceId] = std::make_pair(signalConnectionString, Consumer::SlotInstanceIds());
            SlotInstanceIds& slotInstanceIds = m_connectedInstances[signalInstanceId].second;
            if (slotInstanceIds.find(slotInstanceId) == slotInstanceIds.end()) slotInstanceIds[slotInstanceId] = handler;
        }


        void PointToPoint::Consumer::connectHandler(const std::string& slotInstanceId,
                                                    const std::string& signalInstanceId,
                                                    const std::string& signalConnectionString,
                                                    const ConsumeHandler& handler,
                                                    const karabo::net::Connection::Pointer& connection,
                                                    const karabo::net::Channel::Pointer& channel) {

            channel->setErrorHandler(boost::bind(&Consumer::channelErrorHandler, this,
                                                 signalInstanceId, signalConnectionString, connection, channel, _1));
            // bookkeeping ...
            {
                boost::mutex::scoped_lock lock(m_connectedInstancesMutex);
                storeTcpConnectionInfo(signalConnectionString, connection, channel);
                storeSignalSlotConnectionInfo(slotInstanceId, signalInstanceId, signalConnectionString, handler);
            }

            // Subscribe to producer with our own instanceId
            channel->write(slotInstanceId + " SUBSCRIBE");
            // ... and, finally, wait for publications ...
            channel->readAsyncHashPointerHashPointer(boost::bind(&Consumer::consume, this, channel, _1, _2));
        }


        void PointToPoint::Consumer::connect(const karabo::net::IOService::Pointer& svc,
                                             const std::string& signalInstanceId,
                                             const std::string& slotInstanceId,
                                             const std::string& signalConnectionString,
                                             const karabo::net::ConsumeHandler& handler) {

            boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

            // Are we not connected yet to the "signalInstanceId" producer...
            if (m_connectedInstances.find(signalInstanceId) == m_connectedInstances.end()) {
                // Check if the TCP connection does not exist yet ...
                if (m_openConnections.find(signalConnectionString) == m_openConnections.end()) {
                    Hash params("type", "client");
                    {
                        vector<string> v;

                        string hostport = signalConnectionString.substr(6); // tcp://host:port
                        boost::split(v, hostport, boost::is_any_of(":"));
                        params.set("hostname", v[0]);
                        params.set("port", fromString<unsigned int>(v[1]));
                    }

                    Connection::Pointer connection = Connection::create(Hash("Tcp", params));
                    connection->setIOService(svc);

                    connection->setErrorHandler(boost::bind(&Consumer::connectionErrorHandler, this,
                                                            signalInstanceId, signalConnectionString, connection, _1));
                    connection->startAsync(boost::bind(&Consumer::connectHandler, this, slotInstanceId,
                                                       signalInstanceId, signalConnectionString, handler, connection, _1));

                    return;
                }

                // bookkeeping ...
                storeSignalSlotConnectionInfo(slotInstanceId, signalInstanceId, signalConnectionString, handler);

                // Subscribe to producer with slotInstanceId
                m_openConnections[signalConnectionString].second->write(slotInstanceId + " SUBSCRIBE");
            }
            // Connected!
        }


        void PointToPoint::Consumer::disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId) {

            boost::mutex::scoped_lock lock(m_connectedInstancesMutex);

            ConnectedInstances::iterator it = m_connectedInstances.find(signalInstanceId);
            if (it == m_connectedInstances.end()) return; // Done! ... nothing to disconnect

            string signalConnectionString = it->second.first;

            // un-subscribe from producer
            Connection::Pointer connection = m_openConnections[signalConnectionString].first;
            Channel::Pointer channel = m_openConnections[signalConnectionString].second;
            channel->write(slotInstanceId + " UNSUBSCRIBE");

            SlotInstanceIds& slotInstanceIds = it->second.second;
            SlotInstanceIds::iterator i = slotInstanceIds.find(slotInstanceId);
            if (i != slotInstanceIds.end()) slotInstanceIds.erase(i);

            // It may happen that signalInstanceId entry will be erased...
            if (slotInstanceIds.empty()) m_connectedInstances.erase(it);

            //KARABO_LOG_FRAMEWORK_DEBUG << "Point-to-point connection between \"" << signalInstanceId << "\" and \""
            //        << slotInstanceId << "\"  closed.";

            if (m_openConnections.find(signalConnectionString) == m_openConnections.end()) return;

            // Check if TCP connection should be closed .... try to find signalConnectionString in connectedInstances.
            bool found = false;
            for (ConnectedInstances::iterator i = m_connectedInstances.begin(); i != m_connectedInstances.end(); ++i) {
                if (i->second.first == signalConnectionString) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                channel->close();
                connection->stop();
                m_openConnections.erase(signalConnectionString);
            }

        }


        void PointToPoint::Consumer::consume(const karabo::net::Channel::Pointer& channel,
                                             karabo::util::Hash::Pointer& header,
                                             karabo::util::Hash::Pointer& body) {

            // Get from header...
            // ... signalInstanceId
            string signalInstanceId = header->get<string>("signalInstanceId");

            // ... slotInstanceIdsString
            string slotInstanceIdsString = header->get<string>("slotInstanceIds");

            // Strip it from "|"
            slotInstanceIdsString.substr(1, slotInstanceIdsString.length() - 2);

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
                handler(slotInstanceId, header, body);
            }
            // Re-register itself
            channel->readAsyncHashPointerHashPointer(boost::bind(&Consumer::consume, this, channel, _1, _2));
        }


        PointToPoint::PointToPoint() : m_producer(new Producer), m_consumer(new Consumer) {
            m_svc = m_producer->getIOService();
            m_thread = boost::thread(boost::bind(&IOService::work, m_svc));
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }


        PointToPoint::~PointToPoint() {
            m_producer->stop();
            m_svc->stop();
            m_thread.join();
        }


        void PointToPoint::connect(const std::string& signalInstanceId, const std::string& slotInstanceId,
                                   const std::string& signalConnectionString, const karabo::net::ConsumeHandler& handler) {
            m_consumer->connect(m_svc, signalInstanceId, slotInstanceId, signalConnectionString, handler);
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


        void PointToPoint::consume(const karabo::net::Channel::Pointer& channel, karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body) {
            m_consumer->consume(channel, header, body);
        }


        std::string PointToPoint::getConnectionString() const {
            ostringstream oss;
            oss << "tcp://" << boost::asio::ip::host_name() << ":" << m_producer->getPort();
            return oss.str();
        }
    }
}
