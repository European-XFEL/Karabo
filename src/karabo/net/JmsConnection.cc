/* 
 * File:   JmsConnection.cc
 * Author: heisenb
 * 
 * Created on July 15, 2016, 4:08 PM
 */


#include <karabo/util.hpp>
#include <karabo/log.hpp>

#include "utils.hh"
#include "JmsConnection.hh"
#include "JmsChannel.hh"
#include "JmsBrokerConnection.hh"
#include "EventLoop.hh"

namespace karabo {
    namespace net {

        using namespace karabo::util;
        using namespace boost;


        JmsConnection::JmsConnection(const std::string& brokerUrls, const int nThreads)
            : m_availableBrokerUrls(brokerUrls),
            m_reconnectStrand(*EventLoop::getIOService()) {

            this->setFlagDisconnected();

            // Give precedence to the environment variable (if defined)
            char* env = 0;
            env = getenv("KARABO_BROKER");
            if (env != 0) m_availableBrokerUrls = string(env);
            parseBrokerUrl();

            // Add one event-loop thread for handling broker disconnects
            EventLoop::addThread();
        }


        JmsConnection::~JmsConnection() {
            EventLoop::removeThread();
        }       

        void JmsConnection::parseBrokerUrl() {

            // This splits by ","
            const vector<string> urls = fromString<string, vector>(m_availableBrokerUrls);


            BOOST_FOREACH(string url, urls) {
                const tuple<string, string, string, string, string> urlParts = karabo::net::parseUrl(url);
                m_brokerAddresses.push_back(make_tuple(urlParts.get<0>(), urlParts.get<1>(), urlParts.get<2>()));
            }
        }


        void JmsConnection::connect() {

            MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;
            while (true) {


                BOOST_FOREACH(BrokerAddress adr, m_brokerAddresses) {
                    const std::string scheme = to_upper_copy(adr.get<0>());
                    const std::string host = adr.get<1>();
                    const int port = fromString<int>(adr.get<2>());
                    // Retrieve property handle
                    MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));
                    // Set all properties
                    setConnectionProperties(scheme, host, port, propertiesHandle);
                    MQStatus status;
                    status = MQCreateConnection(propertiesHandle, "guest", "guest", NULL /*clientID*/, &onException, this, &m_connectionHandle);
                    if (MQStatusIsError(status) == MQ_TRUE) {
                        MQString tmp = MQGetStatusString(status);
                        KARABO_LOG_FRAMEWORK_WARN << tmp;
                        std::cout << tmp; // TODO Remove
                        MQFreeString(tmp);
                    } else { // Connection established
                        m_connectedBrokerUrl = adr.get<0>() + "://" + adr.get<1>() + ":" + adr.get<2>();
                        MQFreeProperties(propertiesHandle);
                        this->setFlagConnected();
                        // Immediately enable message consumption
                        MQ_SAFE_CALL(MQStartConnection(m_connectionHandle));
                        return;
                    }
                }
                boost::this_thread::sleep(boost::posix_time::seconds(10));
            }
        }


        void JmsConnection::onException(const MQConnectionHandle connectionHandle, MQStatus status, void* callbackData) {
            JmsConnection* that = reinterpret_cast<JmsConnection*> (callbackData);
            // Cleanly disconnect
            that->m_reconnectStrand.post(boost::bind(&karabo::net::JmsConnection::disconnect, that));
            // Try to reconnect
            that->m_reconnectStrand.post(boost::bind(&karabo::net::JmsConnection::connect, that));
        }


        void JmsConnection::setConnectionProperties(const std::string& scheme, const std::string& host,
                                                    const int port,
                                                    const MQPropertiesHandle& propertiesHandle) const {

            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle, MQ_CONNECTION_TYPE_PROPERTY, scheme.c_str()));
            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle, MQ_BROKER_HOST_PROPERTY, host.c_str()));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_BROKER_PORT_PROPERTY, port));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_PING_INTERVAL_PROPERTY, ping));
            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle, MQ_SSL_BROKER_IS_TRUSTED, trustBroker));
            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle, MQ_ACK_ON_PRODUCE_PROPERTY, blockUntilAcknowledge));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_ACK_TIMEOUT_PROPERTY, acknowledgeTimeout));
        }


        void JmsConnection::disconnect() {

            MQ_SAFE_CALL(MQStopConnection(m_connectionHandle));

            MQ_SAFE_CALL(MQCloseConnection(m_connectionHandle));

            // Close all session, consumers and producers

            MQ_SAFE_CALL(MQFreeConnection(m_connectionHandle));

            // Unfortunately, openMQ does not do this
            this->setFlagDisconnected();

            // Invalidate the connectedBrokerUrl
            m_connectedBrokerUrl = "";

        }


        bool JmsConnection::isConnected() const {
            boost::lock_guard<boost::mutex> lock(m_mut);
            return m_isConnected;
        }


        MQConnectionHandle JmsConnection::getConnection() const {
            return m_connectionHandle;
        }


        std::string JmsConnection::getBrokerUrl() const {
            return m_connectedBrokerUrl;
        }


        void JmsConnection::waitForConnectionAvailable() {
            boost::unique_lock<boost::mutex> lock(m_mut);
            while (!m_isConnected) {
                m_cond.wait(lock);
            }
        }


        void JmsConnection::setFlagConnected() {
            {
                boost::lock_guard<boost::mutex> lock(m_mut);
                m_isConnected = true;
            }
            m_cond.notify_all();
        }


        void JmsConnection::setFlagDisconnected() {
            boost::lock_guard<boost::mutex> lock(m_mut);
            m_isConnected = false;
            m_connectionHandle.handle = HANDLED_OBJECT_INVALID_HANDLE;
        }


        boost::shared_ptr<JmsChannel> JmsConnection::createChannel() {
            boost::shared_ptr<JmsChannel> channel(new JmsChannel(shared_from_this()));
            m_channels.insert(channel);
            return channel;
        }






    } // namespace net
} // namespace karabo

