#include <sstream>
#include <boost/asio.hpp>
#include "utils.hh"
#include "PointToPoint.hh"

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace net {


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


        PointToPoint::Consumer::Consumer() {
        }


        PointToPoint::Consumer::~Consumer() {
        }


        void PointToPoint::Consumer::connectionErrorHandler(const std::string& signalInstanceId,
                                                            const std::string& signalConnectionString,
                                                            const karabo::net::Connection::Pointer& connection,
                                                            const karabo::net::ErrorCode& ec) {
            if (ec.value() != 2 && ec.value() != 125) {
                std::cout << "ERROR  :  Point-to-point connection to \"" << signalInstanceId << "\" using \"" << signalConnectionString
                        << "\" failed.  Code " << ec.value() << " -- " << ec.message() << std::endl;
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
                std::cout << "ERROR  :  karabo::net::Channel to \"" << signalInstanceId << "\" using \"" << signalConnectionString
                        << "\" failed.  Code " << ec.value() << " -- " << ec.message() << std::endl;
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


        PointToPoint::PointToPoint() : m_producer(), m_consumer() {
            m_svc = m_producer.m_connection->getIOService();
            m_thread = boost::thread(boost::bind(&IOService::work, m_svc));
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }


        PointToPoint::~PointToPoint() {
            m_producer.stop();
            m_svc->stop();
            m_thread.join();
        }


        std::string PointToPoint::getConnectionString() const {
            ostringstream oss;
            oss << "tcp://" << bareHostName() << ":" << m_producer.m_port;
            return oss.str();
        }


        bool PointToPoint::publish(const std::string& slotInstanceId, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body, int prio) {
            boost::mutex::scoped_lock lock(m_producer.m_registeredChannelsMutex);
            for (map<Channel::Pointer, std::set<string> >::iterator it = m_producer.m_registeredChannels.begin();
                    it != m_producer.m_registeredChannels.end(); ++it) {
                const Channel::Pointer& channel = it->first;
                const std::set<std::string>& slotInstanceIds = it->second;
                std::set<string>::iterator ii = slotInstanceIds.find(slotInstanceId);
                if (ii == slotInstanceIds.end()) continue;
                channel->writeAsync(*header, *body, prio);
                return true;
            }
            return false;
        }


        void PointToPoint::connect(const std::string& signalInstanceId, const std::string& slotInstanceId,
                                   const std::string& signalConnectionString, const karabo::net::ConsumeHandler& handler) {

            boost::mutex::scoped_lock lock(m_consumer.m_connectedInstancesMutex);

            // Are we not connected yet to the "signalInstanceId" producer...
            if (m_consumer.m_connectedInstances.find(signalInstanceId) == m_consumer.m_connectedInstances.end()) {
                // Check if the TCP connection does not exist yet ...
                if (m_consumer.m_openConnections.find(signalConnectionString) == m_consumer.m_openConnections.end()) {
                    Hash params("type", "client");
                    {
                        vector<string> v;

                        string hostport = signalConnectionString.substr(6); // tcp://host:port
                        boost::split(v, hostport, boost::is_any_of(":"));
                        params.set("hostname", v[0]);
                        params.set("port", fromString<unsigned int>(v[1]));
                    }

                    Connection::Pointer connection = Connection::create(Hash("Tcp", params));
                    connection->setIOService(m_svc);

                    connection->setErrorHandler(boost::bind(&Consumer::connectionErrorHandler, &m_consumer,
                                                            signalInstanceId, signalConnectionString, connection, _1));
                    connection->startAsync(boost::bind(&PointToPoint::consumerConnectHandler, this, slotInstanceId,
                                                       signalInstanceId, signalConnectionString, handler, connection, _1));

                    return;
                }

                //KARABO_LOG_FRAMEWORK_DEBUG << "Use existing point-to-point connection to \"" << signalConnectionString
                //        << "\" between signal instance \"" << signalInstanceId << "\" and slot instance \"" << slotInstanceId << "\"";

                if (m_consumer.m_connectedInstances.find(signalInstanceId) == m_consumer.m_connectedInstances.end())
                    m_consumer.m_connectedInstances[signalInstanceId] = std::make_pair(signalConnectionString, Consumer::SlotInstanceIds());
                Consumer::SlotInstanceIds& slotInstanceIds = m_consumer.m_connectedInstances[signalInstanceId].second;
                if (slotInstanceIds.find(slotInstanceId) == slotInstanceIds.end()) slotInstanceIds[slotInstanceId] = handler;

                // Subscribe to producer with slotInstanceId
                m_consumer.m_openConnections[signalConnectionString].second->write(slotInstanceId + " SUBSCRIBE");
            }
            // Connected!
        }


        void PointToPoint::consumerConnectHandler(const std::string& slotInstanceId,
                                                  const std::string& signalInstanceId,
                                                  const std::string& signalConnectionString,
                                                  const ConsumeHandler& handler,
                                                  const karabo::net::Connection::Pointer& connection,
                                                  const karabo::net::Channel::Pointer& channel) {
            channel->setErrorHandler(boost::bind(&Consumer::channelErrorHandler, &m_consumer,
                                                 signalInstanceId, signalConnectionString, connection, channel, _1));
            {
                boost::mutex::scoped_lock lock(m_consumer.m_connectedInstancesMutex);
                // store TCP connection info
                if (m_consumer.m_openConnections.find(signalConnectionString) == m_consumer.m_openConnections.end())
                    m_consumer.m_openConnections[signalConnectionString] = std::make_pair(connection, channel);
                // store logical signal slot connection info
                if (m_consumer.m_connectedInstances.find(signalInstanceId) == m_consumer.m_connectedInstances.end())
                    m_consumer.m_connectedInstances[signalInstanceId] = std::make_pair(signalConnectionString, Consumer::SlotInstanceIds());
                Consumer::SlotInstanceIds& slotInstanceIds = m_consumer.m_connectedInstances[signalInstanceId].second;
                if (slotInstanceIds.find(slotInstanceId) == slotInstanceIds.end()) slotInstanceIds[slotInstanceId] = handler;
            }
            // Subscribe to producer with our own instanceId
            channel->write(slotInstanceId + " SUBSCRIBE");
            // ... and, finally, wait for publications ...
            channel->readAsyncHashPointerHashPointer(boost::bind(&PointToPoint::consume, this, channel, _1, _2));

            //KARABO_LOG_FRAMEWORK_DEBUG << "New point-to-point connection established to \"" << signalConnectionString
            //        << "\" between signal instance \"" << signalInstanceId << "\" and slot instance \"" << slotInstanceId << "\"";
        }


        void PointToPoint::disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId) {

            boost::mutex::scoped_lock lock(m_consumer.m_connectedInstancesMutex);

            Consumer::ConnectedInstances::iterator it = m_consumer.m_connectedInstances.find(signalInstanceId);
            if (it == m_consumer.m_connectedInstances.end()) return; // Done! ... nothing to disconnect

            string signalConnectionString = it->second.first;

            // un-subscribe from producer
            Connection::Pointer connection = m_consumer.m_openConnections[signalConnectionString].first;
            Channel::Pointer channel = m_consumer.m_openConnections[signalConnectionString].second;
            channel->write(slotInstanceId + " UNSUBSCRIBE");

            Consumer::SlotInstanceIds& slotInstanceIds = it->second.second;
            Consumer::SlotInstanceIds::iterator i = slotInstanceIds.find(slotInstanceId);
            if (i != slotInstanceIds.end()) slotInstanceIds.erase(i);

            // It may happen that signalInstanceId entry will be erased...
            if (slotInstanceIds.empty()) m_consumer.m_connectedInstances.erase(it);

            //KARABO_LOG_FRAMEWORK_DEBUG << "Point-to-point connection between \"" << signalInstanceId << "\" and \""
            //        << slotInstanceId << "\"  closed.";

            if (m_consumer.m_openConnections.find(signalConnectionString) == m_consumer.m_openConnections.end()) return;

            // Check if TCP connection should be closed .... try to find signalConnectionString in connectedInstances.
            bool found = false;
            for (Consumer::ConnectedInstances::iterator i = m_consumer.m_connectedInstances.begin(); i != m_consumer.m_connectedInstances.end(); ++i) {
                if (i->second.first == signalConnectionString) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                channel->close();
                connection->stop();
                m_consumer.m_openConnections.erase(signalConnectionString);
            }

        }


        void PointToPoint::consume(const karabo::net::Channel::Pointer& channel,
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
                    boost::mutex::scoped_lock lock(m_consumer.m_connectedInstancesMutex);

                    Consumer::ConnectedInstances::iterator ii = m_consumer.m_connectedInstances.find(signalInstanceId);
                    if (ii == m_consumer.m_connectedInstances.end()) return;
                    Consumer::SlotInstanceIds& slotInstanceIds = ii->second.second;

                    Consumer::SlotInstanceIds::iterator iii = slotInstanceIds.find(slotInstanceId);
                    if (iii == slotInstanceIds.end()) continue;
                    handler = iii->second;
                }
                //                std::cout << "PointToPoint::consume  CALL HANDLER for slotInstanceId=\"" << slotInstanceId
                //                        << "\" and header is ...\n" << *header << std::endl;
                handler(slotInstanceId, header, body); // call user callback of type "ConsumeHandler"
            }
            // Re-register itself
            channel->readAsyncHashPointerHashPointer(boost::bind(&PointToPoint::consume, this, channel, _1, _2));
        }


        void PointToPoint::publishIfConnected(std::map<std::string, std::set<std::string> >& registeredSlots,
                                              const karabo::util::Hash::Pointer& header,
                                              const karabo::util::Hash::Pointer& message, int prio) {
            if (registeredSlots.empty()) return;
            
            boost::mutex::scoped_lock lock(m_producer.m_registeredChannelsMutex);

            for (Producer::ChannelToSlotInstanceIds::const_iterator ii = m_producer.m_registeredChannels.begin();
                    ii != m_producer.m_registeredChannels.end(); ++ii) {

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


        void PointToPoint::updateHeader(const karabo::util::Hash::Pointer& header, const std::map<std::string, std::set<std::string> >& registeredSlots) const {
            string registeredSlotsString;
            string registeredSlotInstanceIdsString;

            for (std::map<std::string, std::set<std::string> >::const_iterator it = registeredSlots.begin(); it != registeredSlots.end(); ++it) {
                registeredSlotInstanceIdsString += "|" + it->first + "|";
                registeredSlotsString += "|" + it->first + ":" + karabo::util::toString(it->second) + "|";
            }

            header->set("slotInstanceIds", registeredSlotInstanceIdsString);
            header->set("slotFunctions", registeredSlotsString);
        }
    }
}
