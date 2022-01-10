#include "karabo/util/SimpleElement.hh"
#include "karabo/util/VectorElement.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/RedisClient.hh"

using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::RedisClient);


namespace karabo {
    namespace net {


        void RedisClient::expectedParameters(Schema& expected) {

            VECTOR_STRING_ELEMENT(expected).key("brokers")
                    .displayedName("Broker URLs")
                    .description("Vector of URLs {\"redis://hostname:port\",...}")
                    .assignmentMandatory()
                    .minSize(1)
                    .commit();

            STRING_ELEMENT(expected).key("instanceId")
                    .displayedName("Instance ID")
                    .description("Instance ID")
                    .assignmentOptional().defaultValue("none")
                    .commit();

            STRING_ELEMENT(expected).key("domain")
                    .displayedName("Domain")
                    .description("Domain is root topic (former JMS topic)")
                    .assignmentMandatory()
                    .commit();

            unsigned int defTimeout = 10;
            const char* env = getenv("KARABO_REDIS_TIMEOUT");
            if (env) {
                const unsigned int envInt = util::fromString<unsigned int>(env);
                defTimeout = (envInt > 0 ? envInt : defTimeout);
                KARABO_LOG_FRAMEWORK_INFO << "REDIS timeout from environment: " << defTimeout;
            }

            UINT32_ELEMENT(expected).key("requestTimeout")
                .displayedName("REDIS request timeout")
                .description("REDIS request timeout in seconds")
                .assignmentOptional().defaultValue(defTimeout)
                .unit(Unit::SECOND)
                .commit();
        }


        RedisClient::RedisClient(const karabo::util::Hash& input)
                : m_ios(boost::make_shared<boost::asio::io_context>())
                , m_thread()
                , m_producer(boost::make_shared<redisclient::RedisAsyncClient>(*m_ios))
                , m_consumer(boost::make_shared<redisclient::RedisAsyncClient>(*m_ios))
                , m_resolver(*m_ios)
                , m_brokerIndex(0)
                , m_brokerUrls(input.get<std::vector<std::string> >("brokers"))
                , m_binarySerializer(karabo::io::BinarySerializer<karabo::util::Hash>::create("Bin"))
                , m_requestTimeout(input.get<std::uint32_t>("requestTimeout")) {
            run();
        }


        RedisClient::~RedisClient() {
            disconnect();
            m_ios->stop();
            if (m_thread->get_id() != boost::this_thread::get_id()) {
                m_thread->join();
            }
        }


        boost::system::error_code RedisClient::connect() {
            boost::system::error_code ec = KARABO_ERROR_CODE_CONN_REFUSED;
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            // Calls connectAsycn passing as argument a lambda that sets the promise value
            connectAsync(
                [prom]
                (const boost::system::error_code& ec) {
                    prom->set_value(ec);
                }
            );
            auto status = fut.wait_for(std::chrono::seconds(m_requestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            ec = fut.get();
            return ec;
        }


        void RedisClient::createClientForUrl(const std::string& url, const AsyncHandler& onConnect) {
            using std::string;

            KARABO_LOG_FRAMEWORK_INFO << "Attempt to connect to REDIS broker : \"" << url << "\"";

            // Parse input url into parts...
            const boost::tuple<string, string, string, string, string> urlParts = karabo::net::parseUrl(url);
            const string& host = urlParts.get<1>();
            string sport = urlParts.get<2>();
            // Set default port if needed ...
            if (sport.empty()) sport = "6379";
            // Resolve hostname in DNS ...
            try {
                boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), host, sport);
                m_resolver.async_resolve(query, bind_weak(&RedisClient::resolveHandler, this,
                                                          boost::asio::placeholders::error,
                                                          boost::asio::placeholders::iterator,
                                                          onConnect));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void RedisClient::resolveHandler(const boost::system::error_code& e,
                                         boost::asio::ip::tcp::resolver::iterator it,
                                         const AsyncHandler& onConnect) {
            try {
                auto conHandler =
                    [this, wptr{weak_from_this()}, onConnect{std::move(onConnect)}]
                    (const boost::system::error_code& ec) {
                        auto guard = wptr.lock();
                        if (!guard) return;
                        // 'this' is valid till end of this lambda
                        if (ec) {
                            m_producer->disconnect();
                            // Failed to connect ... try next url
                            if (m_brokerIndex < m_brokerUrls.size() - 1) {
                                createClientForUrl(m_brokerUrls[++m_brokerIndex], onConnect);
                                return;
                            }
                            // The whole list failed ... inform user via callback
                        }
                        onConnect(ec);
                    };
                
                auto proHandler =
                    [this, wptr{weak_from_this()}, it, conHandler{std::move(conHandler)}, onConnect{std::move(onConnect)}]
                    (const boost::system::error_code& ec) {
                        // attempt to convert to shared pointer
                        auto guard = wptr.lock();
                        if (!guard) return;
                        // 'guard' prevents the destructor to be called, so ...
                        // 'this' is valid till end of lambda and allows to refer
                        // to class members directly
                        if (ec) {
                            // Failed to connect ... try next url
                            if (m_brokerIndex < m_brokerUrls.size() - 1) {
                                createClientForUrl(m_brokerUrls[++m_brokerIndex], onConnect);
                                return;
                            }
                            // The whole list failed ... inform user via callback
                            onConnect(ec);
                            return;
                        }
                        // Producer is connected successfully. Connect consumer...
                        m_consumer->connect(*it, conHandler);
                    };
                if (!e) {
                    // Call Redis connect... asynchronously
                    m_producer->connect(*it, proHandler);
                } else {
                    // DNS 'resolve' failed ...
                    proHandler(e);
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void RedisClient::connectAsync(const AsyncHandler& onConnect) {
            // Use double-checked locking pattern here to allow re-entering
            if (this->isConnected()) {
                if (onConnect) {
                    post(boost::bind(onConnect, KARABO_ERROR_CODE_ALREADY_CONNECTED));
                }
                return;
            }
            //Concurrent calls to this function should be serialized
            std::lock_guard<std::mutex> lock(m_connectionMutex);

            // If the client is already connected, calls the m_onConnect handler
            if (this->isConnected()) {
                if (onConnect) {
                    post(boost::bind(onConnect, KARABO_ERROR_CODE_SUCCESS));
                }
                return;
            }

            m_brokerIndex = 0;  // start connection attempts using m_brokerUrls
            createClientForUrl(m_brokerUrls[m_brokerIndex], onConnect);
        }


        bool RedisClient::isConnected() const {
            if (m_producer && m_consumer && m_producer->isConnected() && m_consumer->isConnected()) return true;
            return false;
        }


        boost::system::error_code RedisClient::disconnect() {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            m_producer->disconnect();
            m_consumer->disconnect();
            return KARABO_ERROR_CODE_SUCCESS;
        }


        void RedisClient::disconnectAsync(const AsyncHandler& onComplete) {
            // Run on event loop to make this function non-blocking ...
            post
            (
                [this, wptr{weak_from_this()}, onComplete]
                () {
                    auto guard = wptr.lock();
                    if (!guard) return;
                    m_producer->disconnect();
                    m_consumer->disconnect();
                    onComplete(KARABO_ERROR_CODE_SUCCESS);
                }
            );
        }


        void RedisClient::disconnectForced() {
            m_producer->disconnect();
            m_consumer->disconnect();
        }


        /**
         * Subscribe to 'topic' with QoS 'subopts' and call 'onRead' callback if
         * the broker sends the message associated to 'topic'.  Attempt to subscribe
         * to the same 'topic' again but with another 'onRead' will replace callback.
         * @param topic
         * @param subopts
         * @param onRead
         * @return
         */
        boost::system::error_code RedisClient::subscribe(const std::string& topic, const ReadHashHandler& onRead) {
            if (!m_consumer->isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto future = prom->get_future();
            subscribeAsync(topic, onRead, [prom](const boost::system::error_code& ec) {
                prom->set_value(ec);
            });
            auto status = future.wait_for(std::chrono::seconds(m_requestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return future.get();
        }


        void RedisClient::subscribeAsync(const std::string& topic,
                                         const ReadHashHandler& onRead,
                                         const AsyncHandler& onComplete) {
            if (!m_consumer->isConnected()) {
                if (onComplete) post(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }
            // Check if the client is already subscribed to the topic
            if (isSubscribed(topic)) {
                if (onComplete) post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                return;
            }
            // Subscribe to the requested topic
            auto completionHandler = [this, onComplete{std::move(onComplete)}] (const redisclient::RedisValue& value) {
                if (value.isOk()) {
                    if (onComplete)   post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                } else {
                    KARABO_LOG_FRAMEWORK_ERROR << "subscribe error : \"" << value.toString() << "\"";
                    if (onComplete)   post(boost::bind(onComplete, KARABO_ERROR_CODE_IO_ERROR));
                }
            };
            auto msgHandler = [this, wptr{weak_from_this()}, topic, onRead{std::move(onRead)}]
                              (const std::vector<char>& payload) {
                auto guard = wptr.lock();
                if (!guard) return;
                // 'guard' prevents object destruction, so 'this' is valid
                karabo::util::Hash::Pointer result = boost::make_shared<Hash>();
                m_binarySerializer->load(*result, payload);
                // onRead(KARABO_ERROR_CODE_SUCCESS, topic, result);
                post(boost::bind(onRead, KARABO_ERROR_CODE_SUCCESS, topic, result));
            };
            // search the wildcard character in topic
            std::size_t found = topic.find_first_of("*");
            if (found == std::string::npos) {
                // topic -> normal channelName
                redisclient::RedisAsyncClient::Handle handle =
                        m_consumer->subscribe(topic, msgHandler, completionHandler);
                m_subscriptionsMap.emplace(topic, std::make_tuple(false, handle));
            } else {
                // topic -> pattern with wildcard character
                redisclient::RedisAsyncClient::Handle handle =
                        m_consumer->psubscribe(topic, msgHandler, completionHandler);
                m_subscriptionsMap.emplace(topic, std::make_tuple(true, handle));
            }
        }


        boost::system::error_code RedisClient::subscribe(const RedisTopicSubOptions& params) {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            subscribeAsync(params, [prom] (boost::system::error_code ec) {
                prom->set_value(ec);
            });
            auto status = fut.wait_for(std::chrono::seconds(m_requestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void RedisClient::subscribeAsync(const RedisTopicSubOptions& params, const AsyncHandler& onComplete) {
            // Let's restrict artificially the possible number of params.... 
            if (params.size() >= 64) throw KARABO_PARAMETER_EXCEPTION("subscribeAsync: cannot handle too many subscriptions");
            if (!isConnected()) {
                if (onComplete) post(boost::bind(onComplete,KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }

            std::shared_ptr<long long> bits(new long long);
            *bits = -1;
            unsigned int n = 0;
            for (auto param : params) {
                std::lock_guard<std::mutex> lock(m_subscribeMutex);
                *bits &= ~(1ULL << n);
                std::string topic;
                ReadHashHandler onRead;
                std::tie(topic, onRead) = param;
                subscribeAsync(topic, onRead, [this, wptr{weak_from_this()}, n, bits, onComplete{std::move(onComplete)}]
                                              (const boost::system::error_code& e) {
                    auto guard = wptr.lock();
                    if (!guard) return;
                    std::lock_guard<std::mutex> lock(m_subscribeMutex);
                    *bits |= (1ULL << n);
                    if (*bits == -1) {
                        post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                    }
                });
                n++;
            }
        }


        boost::system::error_code RedisClient::unsubscribe(const std::string& topic) {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto future = prom->get_future();
            unsubscribeAsync(topic, [prom] (const boost::system::error_code& ec) {
                prom->set_value(ec);
            });
            auto status = future.wait_for(std::chrono::seconds(m_requestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return future.get();
        }
        

        void RedisClient::unsubscribeAsync(const std::string& topic, const AsyncHandler& onComplete) {
            if (!isConnected()) {
                if (onComplete) post(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }
            auto completionHandler =
            [this, wptr{std::move(weak_from_this())}, onComplete{std::move(onComplete)}]
            (const redisclient::RedisValue& value) {
                auto guard = wptr.lock();
                if (!guard) return;
                if (value.isOk()) {
                    if (onComplete)   post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                } else {
                    KARABO_LOG_FRAMEWORK_ERROR << "unsubscribe error : \"" << value.toString() << "\"";
                    if (onComplete)   post(boost::bind(onComplete, KARABO_ERROR_CODE_IO_ERROR));
                }
            };
            std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
            // Check if there is a subscription to the topic 
            auto it = m_subscriptionsMap.find(topic);
            if (it != m_subscriptionsMap.end()) {
                bool pattern;
                redisclient::RedisAsyncClient::Handle handle;
                std::tie(pattern, handle) = it->second;
                m_subscriptionsMap.erase(it);
                if (pattern) {
                    m_consumer->punsubscribe(handle, completionHandler);
                } else {
                    m_consumer->unsubscribe(handle, completionHandler);
                }
            }
        }


        boost::system::error_code RedisClient::unsubscribe(const std::vector<std::string>& topics) {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            unsubscribeAsync(topics, [prom] (const boost::system::error_code& ec) {
                prom->set_value(ec);
            });
            auto status = fut.wait_for(std::chrono::seconds(m_requestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void RedisClient::unsubscribeAsync(const std::vector<std::string>& topics, const AsyncHandler& onComplete) {
            if (!isConnected()) {
                if (onComplete) post(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }

            std::shared_ptr<long long> bits(new long long);
            *bits = -1;
            unsigned int n = 0;
            for (auto& topic : topics) {
                std::lock_guard<std::mutex> lock(m_subscribeMutex);
                *bits &= ~(1ULL << n);
                auto it = m_subscriptionsMap.find(topic);
                if (it == m_subscriptionsMap.end()) continue;
                bool pattern;
                redisclient::RedisAsyncClient::Handle handle;
                std::tie(pattern, handle) = it->second;
                m_subscriptionsMap.erase(it);
                unsubscribeAsync(topic, [this, wptr{weak_from_this()}, n, bits, onComplete{std::move(onComplete)}]
                                        (const boost::system::error_code& ec) {
                    auto guard = wptr.lock();
                    if (!guard) return;
                    std::lock_guard<std::mutex> lock(m_subscribeMutex);
                    *bits |= (1ULL << n);
                    // report error immediately or success for all
                    if (ec || *bits == -1) {
                        post(boost::bind(onComplete, ec));
                    }
                });
                n++;
            }
        }


        boost::system::error_code RedisClient::unsubscribeAll() {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            unsubscribeAllAsync([prom] (const boost::system::error_code& ec) {
                prom->set_value(ec);
            });
            auto status = fut.wait_for(std::chrono::seconds(m_requestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void RedisClient::unsubscribeAllAsync(const AsyncHandler& onComplete) {
            if (!isConnected()) {
                if (onComplete) post(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }
            std::vector<std::string> allsubscriptions;
            {
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                for (auto& kv : m_subscriptionsMap) allsubscriptions.push_back(kv.first);
            }
            unsubscribeAsync(allsubscriptions, onComplete);
        }


        bool RedisClient::isSubscribed(const std::string& topic) {
            if (isConnected()) {
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                if (m_subscriptionsMap.find(topic) != m_subscriptionsMap.end()) return true;
            }
            return false;
        }


        boost::system::error_code RedisClient::publish(
                const std::string& topic, const karabo::util::Hash::Pointer& msg) {
            if (!m_producer->isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            publishAsync(topic, msg, [prom](const boost::system::error_code& ec) {
                prom->set_value(ec);
            });
            auto status = fut.wait_for(std::chrono::seconds(m_requestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void RedisClient::publishAsync(const std::string& topic,
                                       const karabo::util::Hash::Pointer& msg,
                                       const AsyncHandler& onComplete) {
            auto payload = std::vector<char>();
            if (msg) m_binarySerializer->save(*msg, payload); // msg -> payload
            auto callback = [this, onComplete{std::move(onComplete)}](const redisclient::RedisValue& value) {
                if (value.isOk()) {
                    if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                } else {
                    KARABO_LOG_FRAMEWORK_ERROR << "publish error : \"" << value.toString() << "\"";
                    if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_IO_ERROR));
                }
            };
            if (!m_producer->isConnected()) {
                if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
            } else {
                m_producer->publish(topic, payload, callback);
            }
        }


        const std::string& RedisClient::getBrokerUrl() const {
            return m_brokerUrls[m_brokerIndex];
        }


        std::vector<std::string> RedisClient::getSubscriptions() {
            std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
            std::vector<std::string> result;
            for (const auto& item : m_subscriptionsMap) {
                result.push_back(item.first);
            }
            return result;
        }


        void RedisClient::run() {

            m_thread = boost::make_shared<boost::thread>(
                    [this]
                    () {
                        boost::asio::io_context::work work(*m_ios);
                        m_ios->run();
                    });
        }

    }
}

