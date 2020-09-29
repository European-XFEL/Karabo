#include "karabo/net/OpenMQBroker.hh"
#include "karabo/net/JmsConnection.hh"
#include "karabo/net/utils.hh"
#include "karabo/tests/util/Factory_Test.hh"
#include <karabo/log.hpp>


using namespace karabo::util;


namespace karabo {
    namespace net {
        

        KARABO_REGISTER_FOR_CONFIGURATION(Broker, OpenMQBroker)

        void OpenMQBroker::expectedParameters(karabo::util::Schema& s) {
        }


        OpenMQBroker::OpenMQBroker(const karabo::util::Hash& config)
                : Broker(config)
                , m_connection()
                , m_producerChannel()
                , m_consumerChannel()
                , m_heartbeatProducerChannel()
                , m_heartbeatConsumerChannel()
                , m_logProducerChannel()
                , m_logConsumerChannel()
                , m_guiDebugProducerChannel() {
            Hash jmsConfig("brokers", m_availableBrokerUrls);
            m_connection = Configurator<JmsConnection>::create("JmsConnection", jmsConfig);
        }


        OpenMQBroker::~OpenMQBroker() {
        }


        OpenMQBroker::OpenMQBroker(const OpenMQBroker& o) : Broker(o) {
            m_connection = o.m_connection;
            m_producerChannel.reset();
            m_consumerChannel.reset();
            m_heartbeatProducerChannel.reset();
            m_heartbeatConsumerChannel.reset();
            m_logProducerChannel.reset();
            m_logConsumerChannel.reset();
            m_guiDebugProducerChannel.reset();
        }


        Broker::Pointer OpenMQBroker::clone(const std::string& instanceId) {
            auto o = boost::make_shared<OpenMQBroker>(*this);
            o->m_instanceId = instanceId;
            return boost::static_pointer_cast<Broker>(o);
        }


        void OpenMQBroker::connect() {
            if (!m_connection) {
                std::ostringstream oss;
                oss << "Broker::connect : JMS connection pointer is not initialized";
                throw KARABO_OPENMQ_EXCEPTION(oss.str());
            }
            if (!m_connection->isConnected()) m_connection->connect();
        }


        void OpenMQBroker::disconnect() {
        }


        bool OpenMQBroker::isConnected() const {
            return (m_connection && m_connection->isConnected());
        }


        std::string OpenMQBroker::getBrokerUrl() const {
            return m_connection->getBrokerUrl();
        }


        boost::system::error_code OpenMQBroker::subscribeToRemoteSignal(
                const std::string& signalInstanceId,
                const std::string& signalFunction,
                const consumer::MessageHandler& handler,
                const consumer::ErrorNotifier& errorNotifier) {
            return boost::system::errc::make_error_code(boost::system::errc::success);
        }


        boost::system::error_code OpenMQBroker::unsubscribeFromRemoteSignal(
                const std::string& signalInstanceId,
                const std::string& signalFunction) {
            return boost::system::errc::make_error_code(boost::system::errc::success);
        }


        void OpenMQBroker::write(const std::string& target,
                                 const karabo::util::Hash::Pointer& header,
                                 const karabo::util::Hash::Pointer& body,
                                 const int priority, const int timeToLive) {

            KARABO_LOG_FRAMEWORK_TRACE << "*** write TARGET = \"" << target
                    << "\"...\n... and HEADER is \n" << *header;
            if (m_topic.empty()) return;
            if (target == m_topic) {
                if (!m_producerChannel) m_producerChannel = m_connection->createProducer();
                m_producerChannel->write(target, *header, *body, priority, timeToLive);
            } else if (target == m_topic + "_beats") {
                if (!m_heartbeatProducerChannel) m_heartbeatProducerChannel = m_connection->createProducer();
                m_heartbeatProducerChannel->write(target, *header, *body, priority, timeToLive);
            } else if (target == "karaboGuiDebug") {
                if (!m_guiDebugProducerChannel) m_guiDebugProducerChannel = m_connection->createProducer();
                m_guiDebugProducerChannel->write(target, *header, *body, priority, timeToLive);
            } else { // target = 'log'
                if (!m_logProducerChannel) m_logProducerChannel = m_connection->createProducer();
                m_logProducerChannel->write(target, *header, *body, priority, timeToLive);
            }
        }


        /**
         * JMS subscription:
         * 'selector' is SQL-like expression on properties...
         *   "slotInstanceIds LIKE '%|" + m_instanceId + "|%' OR slotInstanceIds LIKE '%|*|%'"
         *                             specific subscription            global subscription
         *
         * @param handler       - success handler
         * @param errorNotifier - error handler
         */
        void OpenMQBroker::startReading(const consumer::MessageHandler& handler,
                                        const consumer::ErrorNotifier& errorNotifier) {
            if (!m_consumerChannel) {
                std::string selector("slotInstanceIds LIKE '%|"
                                     + m_instanceId + "|%'");
                if (m_consumeBroadcasts) {
                    selector += " OR slotInstanceIds LIKE '%|*|%'";
                }
                m_consumerChannel = m_connection->createConsumer(m_topic, selector);
            }
            m_messageHandler = handler;
            m_errorNotifier = errorNotifier;
            m_consumerChannel->startReading(m_messageHandler, m_errorNotifier);            
        }


        void OpenMQBroker::stopReading() {
            if (m_consumerChannel) m_consumerChannel->stopReading();
        }


        void OpenMQBroker::writeLocal(const consumer::MessageHandler& handler,
                                const karabo::util::Hash::Pointer& header,
                                const karabo::util::Hash::Pointer& body) {
            handler(header, body);
        }


        /**
         * Heartbeat is used for tracking instances (tracking all instances or no tracking at all)
         *
         * JMS subscription
         * 'selector' is SQL-like logical expression on properties...
         *   "signalFunction = 'signalHeartbeat'"
         *
         * @param handler       - success handler
         * @param errorNotifier - error handler
         */
        void OpenMQBroker::startReadingHeartbeats(const consumer::MessageHandler& handler,
                                          const consumer::ErrorNotifier& errorNotifier) {
            if (!m_heartbeatConsumerChannel) {
                std::string selector = "signalFunction = 'signalHeartbeat'";
                m_heartbeatConsumerChannel = m_connection->createConsumer(m_topic + "_beats", selector);
            }
            m_heartbeatConsumerChannel->startReading(handler, errorNotifier);
        }


        void OpenMQBroker::stopReadingHeartbeats() {
            if (m_heartbeatConsumerChannel) m_heartbeatConsumerChannel->stopReading();
        }


        /**
         * JMS subscription.
         * 'selector' is SQL-like expression on properties (in header)
         *   "target = 'log'"
         *
         * @param handler       - success handler
         * @param errorNotifier - error handler
         */
        void OpenMQBroker::startReadingLogs(const consumer::MessageHandler& handler, const consumer::ErrorNotifier& errorNotifier) {
            if (!m_logConsumerChannel) {
                m_logConsumerChannel = m_connection->createConsumer(m_topic, "target = 'log'");
            }
            m_logConsumerChannel->startReading(handler, errorNotifier);
        }


        void OpenMQBroker::stopReadingLogs() {
            if (m_logConsumerChannel) m_logConsumerChannel->stopReading();
        }

    }
}
