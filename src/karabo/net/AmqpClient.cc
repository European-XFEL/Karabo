#include "AmqpClient.hh"

#include <boost/algorithm/string.hpp>
#include <boost/core/null_deleter.hpp>

#include "karabo/log/Logger.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/VectorElement.hh"


using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::log;


KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::AmqpClient)


namespace karabo {
    namespace net {


        static std::mutex amqpMutex;
        static std::mutex connectorMutex;
        std::weak_ptr<TcpConnector> TcpConnector::m_weakConnector;
        std::unordered_set<std::string> AmqpClient::m_registeredExchanges;

        TcpConnector::Pointer TcpConnector::create(const std::vector<std::string>& urls) {
            // Apply DCLP ("Double-Check Locking Pattern")
            // https://www.aristeia.com/Papers/DDJ_Jul_Aug_2004_revised.pdf
            static std::weak_ptr<TcpConnector> wptr;
            Pointer ptr = m_weakConnector.lock();
            // Check
            if (ptr) return ptr;
            // Locking
            std::lock_guard<std::mutex> lk(connectorMutex);
            // Check
            if (!ptr) {
                ptr = std::make_shared<TcpConnector>(urls);
                m_weakConnector = ptr;
            }
            return ptr;
        }


        TcpConnector::TcpConnector(const std::vector<std::string>& urls)
            : m_thread(),
              m_loop(std::make_shared<boost::asio::io_context>()),
              m_handler(*m_loop),
              m_connection(),
              m_url("") {
            for (const auto& url : urls) {
                auto prom = std::make_shared<std::promise<bool> >();
                auto fut = prom->get_future();
                m_handler.setPromise(prom);
                m_connection.reset(new AMQP::TcpConnection(&m_handler, url));
                if (!m_thread) run();
                if (fut.get()) {
                    m_url = url;
                    break;
                }
                m_connection.reset();
            }
        }


        TcpConnector::~TcpConnector() {
            // Stop event loop to return run() in thread
            m_loop->stop();
            if (m_thread) {
                if (m_thread->joinable()) m_thread->join();
                m_thread.reset();
            }
        }


        void TcpConnector::run() {
            m_thread = std::make_shared<std::thread>([this]() {
                if (m_connection) {
                    boost::asio::io_context::work work(*m_loop);
                    m_loop->run();
                    m_connection->close();
                    m_connection.reset();
                }
            });
        }


        void AmqpClient::expectedParameters(Schema& expected) {
            VECTOR_STRING_ELEMENT(expected)
                  .key("brokers")
                  .displayedName("Broker URLs")
                  .description("Vector of URLs {\"amqp://user:pass@hostname:port\",...}")
                  .assignmentMandatory()
                  .minSize(1)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("instanceId")
                  .displayedName("Instance ID")
                  .description("Instance ID")
                  .assignmentOptional()
                  .defaultValue("none")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("domain")
                  .displayedName("Domain")
                  .description("Domain is root topic (former JMS topic)")
                  .assignmentMandatory()
                  .commit();

            unsigned int defTimeout = 10;
            const char* env = getenv("KARABO_AMQP_TIMEOUT");
            if (env) {
                const unsigned int envInt = util::fromString<unsigned int>(env);
                defTimeout = (envInt > 0 ? envInt : defTimeout);
                KARABO_LOG_FRAMEWORK_INFO << "AMQP timeout from environment: " << defTimeout;
            }

            UINT32_ELEMENT(expected)
                  .key("amqpRequestTimeout")
                  .displayedName("AMQP request timeout")
                  .description("AMQP request timeout in seconds")
                  .assignmentOptional()
                  .defaultValue(defTimeout)
                  .unit(Unit::SECOND)
                  .commit();
        }


        AmqpClient::AmqpClient(const karabo::util::Hash& input)
            : m_brokerUrls(input.get<std::vector<std::string> >("brokers")),
              m_binarySerializer(karabo::io::BinarySerializer<karabo::util::Hash>::create("Bin")),
              m_domain(input.get<std::string>("domain")),
              m_instanceId(input.get<std::string>("instanceId")),
              m_amqpRequestTimeout(input.get<unsigned int>("amqpRequestTimeout")),
              m_subscriptions(),
              m_subscriptionsMutex(),
              m_connector(),
              m_channel(),
              m_reliable(),
              m_queue(""),
              m_consumerTag(""),
              m_onRead() {
        }


        AmqpClient::~AmqpClient() {
            {
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                m_subscriptions.clear();
            }
            {
                std::lock_guard<std::mutex> lock(amqpMutex);
                m_connector.reset();
            }
        }


        boost::system::error_code AmqpClient::connect() {
            boost::system::error_code ec = KARABO_ERROR_CODE_CONNECT_REFUSED;
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            // Calls connectAsycn passing as argument a lambda that sets the promise value
            connectAsync([prom](const boost::system::error_code& ec) { prom->set_value(ec); });

            // Wait on the future for the operation completion or a specified timeout
            auto status = fut.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            ec = fut.get();
            return ec;
        }


        std::shared_ptr<AMQP::TcpConnection> AmqpClient::createClientForUrls(const std::vector<std::string>& urls) {
            KARABO_LOG_FRAMEWORK_INFO << "Attempt to connect to RabbitMQ broker : \"" << toString(urls) << "\"";

            // Create connector (singleton) that carries a connection
            std::shared_ptr<AMQP::TcpConnection> connection;
            {
                std::lock_guard<std::mutex> lock(amqpMutex);
                m_connector = TcpConnector::create(urls);
                connection = m_connector->native();
                if (!connection) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Connection to KARABO_BROKER list fails: "
                                               << m_connector->getStatus();
                    m_connector.reset();
                }
            }
            return connection;
        }


        void AmqpClient::createChannel(std::shared_ptr<AMQP::TcpConnection> connection, const AsyncHandler& onConnect) {
            dispatch([this, connection, onConnect]() {
                // make a channel
                {
                    std::lock_guard<std::mutex> lock(amqpMutex);
                    m_channel = std::make_shared<AMQP::TcpChannel>(&(*connection));
                }
                // Do this in case of errors ...
                m_channel->onError(bind_weak(&AmqpClient::onConnectError, this, _1, onConnect));
                // Do this in success ...
                m_channel->onReady(bind_weak(&AmqpClient::onConnectReady, this, onConnect));
            });
        }


        void AmqpClient::onConnectError(const char* message, const AsyncHandler& onConnect) {
            m_connector.reset();
            KARABO_LOG_FRAMEWORK_ERROR << "The whole KARABO_BROKER list failed"
                                       << " : " << message;
            onConnect(KARABO_ERROR_CODE_CONNECT_REFUSED);
        }


        void AmqpClient::onConnectReady(const AsyncHandler& onConnect) {
            std::lock_guard<std::mutex> lock(amqpMutex);
            m_reliable.reset(new AMQP::Reliable<>(*m_channel));
            m_channel->declareQueue(AMQP::exclusive)
                  .onSuccess(
                        // TODO by Gero: consider to use `bind_weak' here
                        [this, wptr{weak_from_this()}, onConnect](const std::string& name, std::uint32_t messageCount,
                                                                  std::uint32_t consumerCount) {
                            auto guard = wptr.lock();
                            if (!guard) return;
                            onDeclareQueueSuccess(name, messageCount, consumerCount, onConnect);
                        })
                  .onError(bind_weak(&AmqpClient::onDeclareQueueError, this, _1, onConnect));
        }


        void AmqpClient::onDeclareQueueError(const char* message, const AsyncHandler& onConnect) {
            KARABO_LOG_FRAMEWORK_ERROR << "Declare anonymous queue failed : " << message;
            onConnect(KARABO_ERROR_CODE_IO_ERROR);
        }


        void AmqpClient::onDeclareQueueSuccess(const std::string& name, uint32_t messageCount, uint32_t consumerCount,
                                               const AsyncHandler& onConnect) {
            m_queue = name;
            std::lock_guard<std::mutex> lock(amqpMutex);
            m_channel->consume(name)
                  .onReceived(bind_weak(&AmqpClient::onMessageReceived, this, _1, _2, _3))
                  .onSuccess([this, wptr{weak_from_this()}, onConnect](const std::string& tag) {
                      auto guard = wptr.lock();
                      if (!guard) return;
                      m_consumerTag = tag;
                      onConnect(KARABO_ERROR_CODE_SUCCESS);
                  })
                  .onError(bind_weak(&AmqpClient::onConsumerConnectError, this, _1, onConnect));
        }


        void AmqpClient::onConsumerConnectError(const char* message, const AsyncHandler& onConnect) {
            KARABO_LOG_FRAMEWORK_ERROR << "Error in call to 'consume()': " << message;
            onConnect(KARABO_ERROR_CODE_CONNECT_REFUSED);
        }


        void AmqpClient::onMessageReceived(const AMQP::Message& m, uint64_t deliveryTag, bool /*redelivered*/) {
            {
                std::lock_guard<std::mutex> lock(amqpMutex);
                m_channel->ack(deliveryTag);
            }
            if (!m_onRead) return;
            karabo::util::Hash::Pointer msg = boost::make_shared<Hash>();
            m_binarySerializer->load(*msg, m.body(), m.bodySize());
            m_onRead(KARABO_ERROR_CODE_SUCCESS, m.exchange(), m.routingkey(), msg);
        }


        void AmqpClient::connectAsync(const AsyncHandler& onConnect) {
            // If the client is already connected, calls the m_onConnect handler
            if (this->isConnected()) {
                if (onConnect) {
                    post(boost::bind(onConnect, KARABO_ERROR_CODE_SUCCESS));
                }
                return;
            }

            // start connection attempts using m_brokerUrls
            std::shared_ptr<AMQP::TcpConnection> connection = createClientForUrls(m_brokerUrls);
            if (!connection) {
                post(boost::bind(onConnect, KARABO_ERROR_CODE_CONNECT_REFUSED));
                return;
            }
            createChannel(connection, onConnect);
        }


        bool AmqpClient::isConnected() const {
            std::lock_guard<std::mutex> lock(amqpMutex);
            return (m_channel && m_channel->connected());
        }


        boost::system::error_code AmqpClient::disconnect() {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;

            // Uses a pair of promise and future for synchronization
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();

            // Calls disconnectAsycn passing as argument a lambda that sets the promise value
            disconnectAsync([prom](const boost::system::error_code& ec) { prom->set_value(ec); });

            // Wait on the future for the operation completion or a specified timeout
            auto status = fut.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void AmqpClient::disconnectAsync(const AsyncHandler& onComplete) {
            closeAsync(onComplete);
        }


        boost::system::error_code AmqpClient::close() {
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto future = prom->get_future();
            closeAsync([prom](const boost::system::error_code& ec) { prom->set_value(ec); });
            auto status = future.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return future.get();
        }


        void AmqpClient::closeAsync(const AsyncHandler& onComplete) {
            std::lock_guard<std::mutex> lock(amqpMutex);
            m_channel->cancel(m_consumerTag);
            m_channel->close()
                  .onError(bind_weak(&AmqpClient::onCloseError, this, _1, onComplete))
                  .onSuccess(bind_weak(&AmqpClient::onCloseSuccess, this, onComplete));
        }


        void AmqpClient::onCloseError(const char* message, const AsyncHandler& onComplete) {
            KARABO_LOG_FRAMEWORK_ERROR << "Channel close() failed : " << message;
            if (onComplete) onComplete(KARABO_ERROR_CODE_IO_ERROR);
        }


        void AmqpClient::onCloseSuccess(const AsyncHandler& onComplete) {
            if (onComplete) onComplete(KARABO_ERROR_CODE_SUCCESS);
        }


        void AmqpClient::disconnectForced() {
            close();
        }


        boost::system::error_code AmqpClient::subscribe(const std::string& exchange, const std::string& bindingKey) {
            if (isSubscribed(exchange, bindingKey)) return KARABO_ERROR_CODE_SUCCESS;
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto future = prom->get_future();

            subscribeAsync(exchange, bindingKey, [prom](const boost::system::error_code& ec) { prom->set_value(ec); });

            auto status = future.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return future.get();
        }


        void AmqpClient::subscribeAsync(const std::string& exchange, const std::string& bindingKey,
                                        const AsyncHandler& onComplete) {
            using namespace boost::algorithm;
            if (isSubscribed(exchange, bindingKey)) {
                if (onComplete) post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                return;
            }

            std::lock_guard<std::mutex> lock(amqpMutex);
            m_channel->declareExchange(exchange, AMQP::topic, 0)
                  .onError(bind_weak(&AmqpClient::onDeclareExchangeError, this, _1, exchange, onComplete))
                  .onSuccess(bind_weak(&AmqpClient::onDeclareExchangeSuccess, this, exchange, bindingKey, onComplete));
        }


        void AmqpClient::onDeclareExchangeError(const char* message, const std::string& exchange,
                                                const AsyncHandler& onComplete) {
            KARABO_LOG_FRAMEWORK_ERROR << "Declare exchange \"" << exchange << "\" failed : " << message;
            onComplete(KARABO_ERROR_CODE_IO_ERROR);
        }


        void AmqpClient::onDeclareExchangeSuccess(const std::string& exchange, const std::string& bindingKey,
                                                  const AsyncHandler& onComplete) {
            std::lock_guard<std::mutex> lock(amqpMutex);
            m_registeredExchanges.insert(exchange);
            m_channel->bindQueue(exchange, m_queue, bindingKey)
                  .onError(bind_weak(&AmqpClient::onBindQueueError, this, _1, exchange, bindingKey, onComplete))
                  .onSuccess(bind_weak(&AmqpClient::onBindQueueSuccess, this, exchange, bindingKey, onComplete));
        }


        void AmqpClient::onBindQueueError(const char* message, const std::string& exchange,
                                          const std::string& bindingKey, const AsyncHandler& onComplete) {
            KARABO_LOG_FRAMEWORK_ERROR << "Bind exchange \"" << exchange << "\" to internal queue \"" << m_queue
                                       << "\" with binding key->\"" << bindingKey << "\" failed : " << message;
            onComplete(KARABO_ERROR_CODE_IO_ERROR);
        }


        void AmqpClient::onBindQueueSuccess(const std::string& exchange, const std::string& bindingKey,
                                            const AsyncHandler& onComplete) {
            {
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                m_subscriptions.insert(exchange + bindingKey);
            }
            onComplete(KARABO_ERROR_CODE_SUCCESS);
        }


        boost::system::error_code AmqpClient::unsubscribe(const std::string& exchange, const std::string& routingKey) {
            if (!isSubscribed(exchange, routingKey)) return KARABO_ERROR_CODE_SUCCESS;

            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto future = prom->get_future();

            unsubscribeAsync(exchange, routingKey,
                             [prom](const boost::system::error_code& ec) { prom->set_value(ec); });

            auto status = future.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return future.get();
        }


        void AmqpClient::unsubscribeAsync(const std::string& exchange, const std::string& routingKey,
                                          const AsyncHandler& onComplete) {
            if (!isConnected()) {
                if (onComplete) post(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                // Check if there is a subscription to the topic
                auto it = m_subscriptions.find(exchange + routingKey);
                // return immediately if not found
                if (it == m_subscriptions.end()) {
                    if (onComplete) {
                        post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                    }
                    return;
                }
                m_subscriptions.erase(it);
            }

            std::lock_guard<std::mutex> lk(amqpMutex);
            m_channel->unbindQueue(exchange, m_queue, routingKey)
                  .onSuccess(bind_weak(&AmqpClient::onUnbindQueueSuccess, this, onComplete))
                  .onError(bind_weak(&AmqpClient::onUnbindQueueError, this, _1, exchange, routingKey, onComplete));
        }


        void AmqpClient::onUnbindQueueError(const char* message, const std::string& exchange,
                                            const std::string& routingKey, const AsyncHandler& onComplete) {
            KARABO_LOG_FRAMEWORK_ERROR << "Unbind exchange->\"" << exchange << "\", internal queue->\"" << m_queue
                                       << "\", routing key->\"" << routingKey << "\" failed: " << message;
            if (onComplete) onComplete(KARABO_ERROR_CODE_IO_ERROR);
        }


        void AmqpClient::onUnbindQueueSuccess(const AsyncHandler& onComplete) {
            if (!onComplete) return;
            onComplete(KARABO_ERROR_CODE_SUCCESS);
        }


        bool AmqpClient::isSubscribed(const std::string& exchange, const std::string& routingKey) {
            std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
            if (m_subscriptions.find(exchange + routingKey) != m_subscriptions.end()) return true;
            return false;
        }


        boost::system::error_code AmqpClient::publish(const std::string& exchange, const std::string& routingKey,
                                                      const karabo::util::Hash::Pointer& msg) {
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto future = prom->get_future();

            publishAsync(exchange, routingKey, msg,
                         [prom](const boost::system::error_code& ec) { prom->set_value(ec); });

            auto status = future.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return future.get();
        }


        void AmqpClient::publishAsync(const std::string& exchange, const std::string& routingKey,
                                      const karabo::util::Hash::Pointer& msg, const AsyncHandler& onComplete) {
            std::shared_ptr<std::vector<char> > payload(new std::vector<char>());
            if (msg) m_binarySerializer->save(*msg, *payload); // msg -> payload
            std::lock_guard<std::mutex> lock(amqpMutex);
            if (m_registeredExchanges.find(exchange) == m_registeredExchanges.end()) {
                m_channel->declareExchange(exchange, AMQP::topic);
                m_registeredExchanges.insert(exchange);
            }
            // AMQP::Reliable<> reliable(*m_channel);
            m_reliable->publish(exchange, routingKey, payload->data(), payload->size())
                  .onAck([this, payload, onComplete]() { onComplete(KARABO_ERROR_CODE_SUCCESS); })
                  .onError([this, exchange, routingKey, onComplete](const char* message) {
                      KARABO_LOG_FRAMEWORK_ERROR << "Publish error: exchange->\"" << exchange << "\", routing key->\""
                                                 << routingKey << "\" failed: " << message;
                      if (onComplete) onComplete(KARABO_ERROR_CODE_IO_ERROR);
                  });
        }


        const std::string& AmqpClient::getBrokerUrl() const {
            std::lock_guard<std::mutex> lock(amqpMutex);
            return m_connector->url();
        }
    } // namespace net
} // namespace karabo
