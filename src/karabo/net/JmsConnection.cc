/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   JmsConnection.cc
 * Author: heisenb
 *
 * Created on July 15, 2016, 4:08 PM
 */

#include "JmsConnection.hh"

#include <openmqc/mqcrt.h>

#include <karabo/log/Logger.hh>

#include "EventLoop.hh"
#include "JmsConsumer.hh"
#include "JmsProducer.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/VectorElement.hh"
#include "utils.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::JmsConnection)

namespace karabo {
    namespace net {

        using namespace karabo::util;
        using namespace boost;

        void JmsConnection::expectedParameters(Schema& s) {
            VECTOR_STRING_ELEMENT(s)
                  .key("brokers")
                  .displayedName("Brokers")
                  .description(
                        "Brokers must be provided as URLs of format: tcp://<host>:<port>. Extra URLs serve as "
                        "fallback.")
                  .assignmentOptional()
                  .defaultValueFromString("tcp://exfl-broker.desy.de:7777")
                  .minSize(1ul)
                  .commit();
        }


        JmsConnection::JmsConnection(const karabo::util::Hash& config)
            : JmsConnection(config.get<std::vector<std::string>>("brokers")) {}


        JmsConnection::JmsConnection(const std::string& brokerUrls)
            : JmsConnection(fromString<std::string, std::vector>(brokerUrls)) {}


        JmsConnection::JmsConnection(const std::vector<std::string>& brokerUrls)
            : m_availableBrokerUrls(brokerUrls), m_reconnectStrand(EventLoop::getIOService()) {
            this->setFlagDisconnected();

            parseBrokerUrl();

            // Add one event-loop thread for handling automatic reconnection
            // Specifially, the thread is needed because connect() is posted and is of blocking nature
            // thanks to openMQ :-(
            EventLoop::addThread();

            MQSetLoggingFunc(&karabo::net::JmsConnection::onOpenMqLog, 0);
            MQSetStdErrLogLevel(MQ_LOG_OFF);
        }


        JmsConnection::~JmsConnection() {
            EventLoop::removeThread();
        }


        void JmsConnection::parseBrokerUrl() {
            using std::string;


            for (const string& url : m_availableBrokerUrls) {
                const boost::tuple<string, string, string, string, string> urlParts = karabo::net::parseUrl(url);
                m_brokerAddresses.push_back(make_tuple(urlParts.get<0>(), urlParts.get<1>(), urlParts.get<2>()));
            }
        }


        void JmsConnection::connect() {
            MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;
            while (true) {
                if (m_brokerAddresses.empty()) {
                    throw KARABO_NETWORK_EXCEPTION("No JMS broker address given.");
                }
                for (const BrokerAddress& adr : m_brokerAddresses) {
                    const std::string scheme = to_upper_copy(adr.get<0>());
                    const std::string host = adr.get<1>();
                    const int port = fromString<int>(adr.get<2>());
                    const std::string url = adr.get<0>() + "://" + adr.get<1>() + ":" + adr.get<2>();
                    // Retrieve property handle
                    MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));
                    // Set all properties
                    setConnectionProperties(scheme, host, port, propertiesHandle);
                    MQStatus status;
                    status = MQCreateConnection(propertiesHandle, "guest", "guest", NULL /*clientID*/, &onException,
                                                this, &m_connectionHandle);
                    if (MQStatusIsError(status) == MQ_TRUE) {
                        KARABO_LOG_FRAMEWORK_WARN << "Failed to open TCP connection to broker " << url;
                        MQFreeProperties(propertiesHandle);
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


        void JmsConnection::setConnectionProperties(const std::string& scheme, const std::string& host, const int port,
                                                    const MQPropertiesHandle& propertiesHandle) const {
            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle, MQ_CONNECTION_TYPE_PROPERTY, scheme.c_str()));
            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle, MQ_BROKER_HOST_PROPERTY, host.c_str()));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_BROKER_PORT_PROPERTY, port));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_PING_INTERVAL_PROPERTY, ping));
            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle, MQ_SSL_BROKER_IS_TRUSTED, trustBroker));
            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle, MQ_ACK_ON_PRODUCE_PROPERTY, blockUntilAcknowledge));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_ACK_TIMEOUT_PROPERTY, acknowledgeTimeout));
            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle, MQ_ACK_ON_ACKNOWLEDGE_PROPERTY, false));
        }


        void JmsConnection::onException(const MQConnectionHandle connectionHandle, MQStatus status,
                                        void* callbackData) {
            JmsConnection* that = reinterpret_cast<JmsConnection*>(callbackData);
            KARABO_LOG_FRAMEWORK_ERROR << "Lost TCP connection to broker " << that->m_connectedBrokerUrl;
            that->setFlagDisconnected();
            // Try to reconnect
            that->m_reconnectStrand.post(bind_weak(&karabo::net::JmsConnection::connect, that));
        }


        void JmsConnection::setFlagDisconnected() {
            boost::lock_guard<boost::mutex> lock(m_isConnectedMutex);
            m_isConnected = false;
            m_connectionHandle.handle = HANDLED_OBJECT_INVALID_HANDLE;
            // Invalidate the connectedBrokerUrl
            m_connectedBrokerUrl = "";
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

            KARABO_LOG_FRAMEWORK_INFO << "Closed TCP connection to broker" << m_connectedBrokerUrl;

            this->setFlagDisconnected();
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


        boost::shared_ptr<JmsConsumer> JmsConnection::createConsumer(const std::string& topic,
                                                                     const std::string& selector,
                                                                     bool skipSerialisation) {
            return boost::shared_ptr<JmsConsumer>(
                  new JmsConsumer(shared_from_this(), topic, selector, skipSerialisation));
        }


        boost::shared_ptr<JmsProducer> JmsConnection::createProducer() {
            return boost::shared_ptr<JmsProducer>(new JmsProducer(shared_from_this()));
        }


        void JmsConnection::onOpenMqLog(const MQLoggingLevel severity, const MQInt32 logCode, ConstMQString logMessage,
                                        const MQInt64 timeOfMessage, const MQInt64 connectionID, ConstMQString filename,
                                        const MQInt32 fileLineNumber, void* callbackData) {
            switch (severity) {
                case MQ_LOG_SEVERE:
                    karabo::log::Logger::logError("openMq") << logMessage;
                    break;
                case MQ_LOG_WARNING:
                    karabo::log::Logger::logWarn("openMq") << logMessage;
                    break;
                case MQ_LOG_INFO:
                    karabo::log::Logger::logInfo("openMq") << logMessage;
                    break;
                default:
                    break; // do nothing
            }
        }
    } // namespace net
} // namespace karabo
