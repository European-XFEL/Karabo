/* 
 * File:   JmsConnection.cc
 * Author: heisenb
 * 
 * Created on July 15, 2016, 4:08 PM
 */

#include <openmqc/mqlogutil-priv.h>

#include <karabo/util.hpp>
#include <karabo/log.hpp>

#include "utils.hh"
#include "JmsConnection.hh"
#include "JmsConsumer.hh"
#include "JmsProducer.hh"
#include "JmsBrokerConnection.hh"
#include "EventLoop.hh"

namespace karabo {
    namespace net {

        using namespace karabo::util;
        using namespace boost;


        JmsConnection::JmsConnection(const std::string& brokerUrls)
            : m_availableBrokerUrls(brokerUrls),
            m_reconnectStrand(*EventLoop::getIOService()) {

            this->setFlagDisconnected();

            // Give precedence to the environment variable (if defined)
            char* env = 0;
            env = getenv("KARABO_BROKER");
            if (env != 0) m_availableBrokerUrls = string(env);
            parseBrokerUrl();

            // Add one event-loop thread for handling automatic reconnection
            // Specifially, the thread is needed because connect() is posted and is of blocking nature
            // thanks to openMQ :-(
            EventLoop::addThread();

            // Set logging function
            // TODO: Decide what should be done here
            MQSetLogFileName("broker.log");

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
                    const string url = adr.get<0>() + "://" + adr.get<1>() + ":" + adr.get<2>();
                    // Retrieve property handle
                    MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));
                    // Set all properties
                    setConnectionProperties(scheme, host, port, propertiesHandle);
                    MQStatus status;
                    status = MQCreateConnection(propertiesHandle, "guest", "guest", NULL /*clientID*/, &onException, this, &m_connectionHandle);
                    if (MQStatusIsError(status) == MQ_TRUE) {
                        KARABO_LOG_FRAMEWORK_WARN << "Failed to open TCP connection to broker " << url;
                    } else { // Connection established
                        m_connectedBrokerUrl = url;
                        MQFreeProperties(propertiesHandle);
                        this->setFlagConnected();
                        // Immediately enable message consumption
                        MQ_SAFE_CALL(MQStartConnection(m_connectionHandle));
                        KARABO_LOG_FRAMEWORK_INFO << "Opened TCP connection to broker " << m_connectedBrokerUrl;
                        return;
                    }
                }
                boost::this_thread::sleep(boost::posix_time::seconds(10));
            }
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


        void JmsConnection::onException(const MQConnectionHandle connectionHandle, MQStatus status, void* callbackData) {
            JmsConnection* that = reinterpret_cast<JmsConnection*> (callbackData);
            KARABO_LOG_FRAMEWORK_ERROR << "Lost TCP connection to broker " << that->m_connectedBrokerUrl;
            that->setFlagDisconnected();
            // Try to reconnect
            that->m_reconnectStrand.post(boost::bind(&karabo::net::JmsConnection::connect, that));
        }


        void JmsConnection::setFlagDisconnected() {
            boost::lock_guard<boost::mutex> lock(m_isConnectedMutex);
            m_isConnected = false;
            m_connectionHandle.handle = HANDLED_OBJECT_INVALID_HANDLE;
        }


        void JmsConnection::setFlagConnected() {
            {
                boost::lock_guard<boost::mutex> lock(m_isConnectedMutex);
                m_isConnected = true;
            }
            m_isConnectedCond.notify_all();
        }


        void JmsConnection::disconnect() {

            MQ_SAFE_CALL(MQStopConnection(m_connectionHandle));

            MQ_SAFE_CALL(MQCloseConnection(m_connectionHandle));

            // Close all session, consumers and producers

            MQ_SAFE_CALL(MQFreeConnection(m_connectionHandle));

            // Unfortunately, openMQ does not do this
            this->setFlagDisconnected();

            KARABO_LOG_FRAMEWORK_INFO << "Closed TCP connection to broker" << m_connectedBrokerUrl;

            // Invalidate the connectedBrokerUrl
            m_connectedBrokerUrl = "";
        }


        bool JmsConnection::isConnected() const {
            boost::lock_guard<boost::mutex> lock(m_isConnectedMutex);
            return m_isConnected;
        }


        MQConnectionHandle JmsConnection::getConnection() const {
            return m_connectionHandle;
        }


        std::string JmsConnection::getBrokerUrl() const {
            return m_connectedBrokerUrl;
        }


        void JmsConnection::waitForConnectionAvailable() {
            boost::unique_lock<boost::mutex> lock(m_isConnectedMutex);
            while (!m_isConnected) {
                m_isConnectedCond.wait(lock);
            }
        }


        boost::shared_ptr<JmsConsumer> JmsConnection::createConsumer() {
            return boost::shared_ptr<JmsConsumer>(new JmsConsumer(shared_from_this()));
        }


        boost::shared_ptr<JmsProducer> JmsConnection::createProducer() {
            return boost::shared_ptr<JmsProducer>(new JmsProducer(shared_from_this()));
        }
    }
}

