/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include "MqttCppClient.hh"

#include "karabo/log/Logger.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/SimpleElement.hh"
#include "utils.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::log;
using namespace boost::system;
using namespace boost::placeholders;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::MqttClient, karabo::net::MqttCppClient)

namespace karabo {
    namespace net {


        void MqttCppClient::expectedParameters(Schema& expected) {
            UINT32_ELEMENT(expected)
                  .key("pingMs")
                  .displayedName("Ping interval")
                  .description("Ping interval in milliseconds")
                  .assignmentOptional()
                  .defaultValue(10000)
                  .unit(Unit::SECOND)
                  .metricPrefix(MetricPrefix::MILLI)
                  .commit();

            unsigned int defTimeout = 100;
            const char* env = getenv("KARABO_MQTT_TIMEOUT");
            if (env) {
                const unsigned int envInt = util::fromString<unsigned int>(env);
                defTimeout = (envInt > 0 ? envInt : defTimeout);
                KARABO_LOG_FRAMEWORK_INFO << "MQTT timeout from environment: " << defTimeout;
            }

            UINT32_ELEMENT(expected)
                  .key("mqttRequestTimeout")
                  .displayedName("MQTT request timeout")
                  .description("MQTT request timeout in seconds")
                  .assignmentOptional()
                  .defaultValue(defTimeout)
                  .unit(Unit::SECOND)
                  .commit();
        }


        MqttCppClient::MqttCppClient(const karabo::util::Hash& input)
            : MqttClient(input),
              m_cleanSession(true),
              m_clientId(MqttClient::getUuidAsString()) // boost::uuids::to_string(m_uuidGenerator())
              ,
              m_pendingRequestsTimer(*m_ios),
              m_binarySerializer(karabo::io::BinarySerializer<karabo::util::Hash>::create("Bin")) {
            setup(input);
        }


        MqttCppClient::~MqttCppClient() {
            disconnect();
        }


        boost::system::error_code MqttCppClient::connect() {
            boost::system::error_code ec = errc::make_error_code(errc::connection_refused);
            auto prom = std::make_shared<std::promise<boost::system::error_code>>();
            auto fut = prom->get_future();
            // Calls connectAsycn passing as argument a lambda that sets the promise value
            connectAsync([prom](const boost::system::error_code& ec) { prom->set_value(ec); });
            ec = fut.get();
            return ec;
        }


        void MqttCppClient::createClientForUrl(const std::string& url, const AsyncHandler& onConnect) {
            using std::string;

            KARABO_LOG_FRAMEWORK_INFO << "Attempt to connect to MQTT broker : \"" << url << "\"  clientId=\""
                                      << m_clientId << "\"";

            const boost::tuple<string, string, string, string, string> urlParts = karabo::net::parseUrl(url);
            const string& host = urlParts.get<1>();
            string sport = urlParts.get<2>();
            if (sport.empty()) sport = "1883";

            m_client = MQTT_NS::make_async_client(*m_ios, host, sport);

            if (m_clientId.empty()) m_clientId = MqttClient::getUuidAsString();

            // Setup client
            m_client->set_client_id(m_clientId);
            m_client->set_clean_session(m_cleanSession);
            if (!m_username.empty() && !m_password.empty()) {
                m_client->set_user_name(m_username);
                m_client->set_password(m_password);
            }
            m_client->set_keep_alive_sec_ping_ms(m_keepAlive, size_t(m_pingInterval));
            m_client->set_will(*m_will);

            m_client->set_auto_pub_response(true);
            // Register broker "handshake" handler 'handleConnect'
            m_client->set_connack_handler(bind_weak(&MqttCppClient::handleConnect, this, _1, _2, onConnect));
            m_client->set_error_handler(bind_weak(&MqttCppClient::handleError, this, _1));
            m_client->set_publish_handler(bind_weak(&MqttCppClient::handleMessage, this, _1, _2, _3, _4));
            m_client->set_puback_handler(bind_weak(&MqttCppClient::handleRequestResponse, this, _1));
            m_client->set_pubcomp_handler(bind_weak(&MqttCppClient::handleRequestResponse, this, _1));
            m_client->set_unsuback_handler(bind_weak(&MqttCppClient::handleRequestResponse, this, _1));

            auto weakPtr = this->weak_from_this();
            auto subAckHandler = [weakPtr](std::uint16_t packetId,
                                           std::vector<MQTT_NS::suback_return_code> returnCodes) {
                if (auto ptr = weakPtr.lock()) {
                    boost::shared_ptr<MqttCppClient> that = boost::static_pointer_cast<MqttCppClient>(ptr);
                    return that->handleRequestResponse(packetId);
                } else {
                    return false;
                }
            };

            m_client->set_suback_handler(subAckHandler);

            //"async_connect" requires that "set" functions are called above
            // Establish physical TCP connection to the broker...
            m_client->async_connect(
                  [this, wptr{weak_from_this()}, onConnect{std::move(onConnect)}](const MQTT_NS::error_code& ec) {
                      auto ptr = wptr.lock();
                      if (!ptr) return;
                      // Check if physical TCP connection is established
                      if (ec) {
                          if (m_brokerIndex < m_brokerUrls.size() - 1) {
                              // Fail to connect ... try the next broker in the list
                              createClientForUrl(m_brokerUrls[++m_brokerIndex], onConnect);
                          } else {
                              // Fail to connect after checking all brokers
                              dispatch(boost::bind(onConnect, ec));
                          }
                      }
                      // Success! Nothing to do here ... the "handshake" handler
                      // above is already registered via set_connack_handler!
                  });
        }


        void MqttCppClient::connectAsync(const AsyncHandler& onConnect) {
            // Concurrent calls to this function should be serialized
            std::lock_guard<std::mutex> lock(m_connectionMutex);

            // If the client is already connected, calls the m_onConnect handler
            if (this->isConnected()) {
                if (onConnect) {
                    dispatch(boost::bind(onConnect, KARABO_ERROR_CODE_ALREADY_CONNECTED));
                }
                return;
            }

            m_brokerIndex = 0; // start connection attempts using m_brokerUrls
            createClientForUrl(m_brokerUrls[m_brokerIndex], onConnect);
        }


        bool MqttCppClient::isConnected() const {
            return m_client ? m_client->connected() : false;
        }


        error_code MqttCppClient::disconnect() {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;

            // Uses a pair of promise and future for synchronization
            auto prom = std::make_shared<std::promise<error_code>>();
            auto fut = prom->get_future();

            // Calls disconnectAsycn passing as argument a lambda that sets the promise value
            disconnectAsync([prom](const error_code ec) { prom->set_value(ec); });

            // Wait on the future for the operation completion or a specified timeout
            auto status = fut.wait_for(std::chrono::seconds(m_mqttRequestTimeout));
            if (status == std::future_status::timeout) {
                return errc::make_error_code(errc::timed_out);
            }
            return fut.get();
        }


        void MqttCppClient::disconnectAsync(const AsyncHandler& onComplete) {
            // Concurrent calls to this function should be serialized
            std::lock_guard<std::mutex> lock(m_disconnectionMutex);
            m_client->set_close_handler(boost::bind(onComplete, errc::make_error_code(errc::success)));
            m_client->async_disconnect();
        }


        void MqttCppClient::disconnectForced() {
            m_client->force_disconnect();
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
        error_code MqttCppClient::subscribe(const std::string& topic, std::uint8_t subopts,
                                            const ReadHashHandler& onRead) {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<error_code>>();
            auto future = prom->get_future();
            subscribeAsync(topic, subopts, onRead, [prom](error_code ec) { prom->set_value(ec); });
            auto status = future.wait_for(std::chrono::seconds(m_mqttRequestTimeout));
            if (status == std::future_status::timeout) {
                return errc::make_error_code(errc::timed_out);
            }
            return future.get();
        }


        void MqttCppClient::subscribeAsync(const std::string& topic, std::uint8_t subopts,
                                           const ReadHashHandler& onRead, const AsyncHandler& onComplete) {
            if (isConnected()) {
                // Check if the client is already subscribed to the topic
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                auto it = m_subscriptionsMap.find(topic);
                // If it is not subscribed, call the  MQTT library async_subscribe method
                if (it == m_subscriptionsMap.end()) {
                    // Subscribe to the requested topic
                    auto op = [this, &topic, subopts] {
                        // Concurrent calls to this function should be serialized
                        std::lock_guard<std::mutex> lk(m_subscribeMutex);
                        auto packetId = m_client->acquire_unique_packet_id();
                        m_client->async_subscribe(packetId, topic, MQTT_NS::subscribe_options(subopts));
                        return packetId;
                    };
                    auto handler = bind_weak(&MqttCppClient::handleSubscription, this, _1, onRead, onComplete, topic);
                    performAsyncOperation(op, handler);
                } else {
                    // Replace ReadHashHandler only in the tuple ...
                    // The tuple (it->second) is 0 - wildcard flag, 1 - subscription flag, 2 - ReadHashHandler
                    std::get<1>(it->second) = std::move(onRead);
                    if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                }
            } else {
                if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
            }
        }


        ReadHashHandler MqttCppClient::getReadHashHandler(const std::string& topic) const {
            auto it = m_subscriptionsMap.find(topic);
            if (it == m_subscriptionsMap.end()) return ReadHashHandler();
            return std::get<1>(it->second);
        }


        boost::system::error_code MqttCppClient::subscribe(const TopicSubOptions& params) {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<error_code>>();
            auto fut = prom->get_future();
            subscribeAsync(params, [prom](error_code ec) { prom->set_value(ec); });
            auto status = fut.wait_for(std::chrono::seconds(m_mqttRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void MqttCppClient::subscribeAsync(const TopicSubOptions& params, const AsyncHandler& onComplete) {
            if (!isConnected()) {
                if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }

            std::vector<std::tuple<std::string, MQTT_NS::subscribe_options>> topics;

            {
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                for (const auto& t : params) {
                    const auto& topic = std::get<0>(t);
                    MQTT_NS::subscribe_options opts(std::uint8_t(std::get<1>(t)));
                    if (m_subscriptionsMap.find(topic) == m_subscriptionsMap.end()) {
                        // This topic is not subscribed to ...
                        topics.push_back(std::make_tuple(topic, opts));
                    }
                }
            }

            if (!topics.empty()) {
                auto op = [this, topics{std::move(topics)}] {
                    // Concurrent calls to this function should be serialized
                    std::lock_guard<std::mutex> lk(m_subscribeMutex);
                    auto packetId = m_client->acquire_unique_packet_id();
                    m_client->async_subscribe(packetId, topics);
                    return packetId;
                };
                auto handler = bind_weak(&MqttCppClient::handleManySubscriptions, this, _1, onComplete, params);
                performAsyncOperation(op, handler);
            } else {
                // Subscription
                auto handler = bind_weak(&MqttCppClient::handleManySubscriptions, this, KARABO_ERROR_CODE_SUCCESS,
                                         onComplete, params);
                dispatch(handler);
            }
        }


        error_code MqttCppClient::unsubscribe(const std::string& topic) {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<error_code>>();
            auto future = prom->get_future();
            unsubscribeAsync(topic, [prom](error_code ec) { prom->set_value(ec); });
            auto status = future.wait_for(std::chrono::seconds(m_mqttRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return future.get();
        }


        void MqttCppClient::unsubscribeAsync(const std::string& topic, const AsyncHandler& onComplete) {
            if (!isConnected()) {
                if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }
            std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
            // Check if there is a subscription to the topic
            auto it = m_subscriptionsMap.find(topic);
            if (it != m_subscriptionsMap.end()) {
                auto op = [this, &topic] {
                    // Concurrent calls to this function should be serialized
                    std::lock_guard<std::mutex> lk(m_subscribeMutex);
                    auto packetId = m_client->acquire_unique_packet_id();
                    m_client->async_unsubscribe(packetId, topic);
                    return packetId;
                };
                auto handler = bind_weak(&MqttCppClient::handleUnsubscription, this, _1, onComplete, topic);
                performAsyncOperation(op, handler);
            } else {
                if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
            }
        }


        error_code MqttCppClient::unsubscribe(const std::vector<std::string>& topics) {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<error_code>>();
            auto fut = prom->get_future();
            unsubscribeAsync(topics, [prom](error_code ec) { prom->set_value(ec); });
            auto status = fut.wait_for(std::chrono::seconds(m_mqttRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void MqttCppClient::unsubscribeAsync(const std::vector<std::string>& topics, const AsyncHandler& onComplete) {
            if (!isConnected()) {
                if (onComplete) {
                    dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                }
                return;
            }

            // collect only real subscriptions on the broker
            std::vector<std::string> selected;
            {
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                for (auto& topic : topics) {
                    auto it = m_subscriptionsMap.find(topic);
                    if (it != m_subscriptionsMap.end()) {
                        // check that "real" subscribe was done for this topic before
                        // The tuple (it->second) is ...
                        // 0 - wildcard flag,
                        // 1 - ReadHashHandler
                        selected.push_back(topic);
                    }
                }
            }
            if (!selected.empty()) {
                auto op = [this, &selected] {
                    // Concurrent calls to this function should be serialized
                    std::lock_guard<std::mutex> lk(m_subscribeMutex);
                    auto packetId = m_client->acquire_unique_packet_id();
                    m_client->async_unsubscribe(packetId, selected);
                    return packetId;
                };
                auto handler = bind_weak(&MqttCppClient::handleManyUnsubscriptions, this, _1, onComplete, topics);
                performAsyncOperation(op, handler);
            } else {
                // Nothing to un-subscribe
                if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
            }
        }


        error_code MqttCppClient::unsubscribeAll() {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<error_code>>();
            auto fut = prom->get_future();
            unsubscribeAllAsync([prom](error_code ec) { prom->set_value(ec); });
            auto status = fut.wait_for(std::chrono::seconds(m_mqttRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void MqttCppClient::unsubscribeAllAsync(const AsyncHandler& onComplete) {
            if (!isConnected()) {
                if (onComplete) dispatch(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }
            std::vector<std::string> allsubscriptions;
            {
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                for (auto& kv : m_subscriptionsMap) allsubscriptions.push_back(kv.first);
            }
            unsubscribeAsync(allsubscriptions, onComplete);
        }


        bool MqttCppClient::isMatched(const std::string& topic) {
            if (isConnected()) {
                if (m_subscriptionsMap.find(topic) != m_subscriptionsMap.end()) return true;
                if (checkForMatchingWildcardTopic(topic)) return true;
            }
            return false;
        }


        bool MqttCppClient::isSubscribed(const std::string& topic) const {
            if (isConnected()) {
                if (m_subscriptionsMap.find(topic) != m_subscriptionsMap.end()) return true;
            }
            return false;
        }


        boost::system::error_code MqttCppClient::publish(const std::string& topic,
                                                         const karabo::util::Hash::Pointer& msg, std::uint8_t options) {
            if (!isConnected()) return KARABO_ERROR_CODE_NOT_CONNECTED;
            auto prom = std::make_shared<std::promise<error_code>>();
            auto fut = prom->get_future();
            publishAsync(topic, msg, options, [prom](const error_code& ec) { prom->set_value(ec); });
            auto status = fut.wait_for(std::chrono::seconds(m_mqttRequestTimeout));
            if (status == std::future_status::timeout) {
                KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": Timeout of publishing " << *msg;
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return fut.get();
        }


        void MqttCppClient::publishAsync(const std::string& topic, const karabo::util::Hash::Pointer& msg,
                                         std::uint8_t options, const AsyncHandler& onComplete) {
            using namespace boost::asio;

            MQTT_NS::publish_options pubopts(options);

            MqttNsAsyncClient::packet_id_t packetId = 0;
            if (pubopts.get_qos() != MQTT_NS::qos::at_most_once) packetId = m_client->acquire_unique_packet_id();
            auto spPayload = std::make_shared<std::vector<char>>();
            if (msg) {
                m_binarySerializer->save2(msg->get<Hash>("header"), *spPayload); // header -> payload
                m_binarySerializer->save2(msg->get<Hash>("body"), *spPayload);   // body   -> payload
            }
            auto spTopic = std::make_shared<std::string>(std::move(topic));

            if (packetId) {
                auto op = [this, spTopic, spPayload, pubopts{std::move(pubopts)}, packetId] {
                    m_client->async_publish(packetId, buffer(*spTopic), buffer(*spPayload), pubopts,
                                            std::make_pair(spTopic, spPayload));
                    return packetId;
                };
                performAsyncOperation(op, onComplete);
            } else {
                dispatch([this, spTopic, spPayload, pubopts, onComplete]() {
                    m_client->async_publish(0, buffer(*spTopic), buffer(*spPayload), pubopts,
                                            std::make_pair(spTopic, spPayload), onComplete);
                });
            }
        }


        std::string MqttCppClient::getClientId() const {
            return m_clientId;
        }


        const std::string& MqttCppClient::getBrokerUrl() const {
            return m_brokerUrls[m_brokerIndex];
        }


        bool MqttCppClient::handleConnect(const bool sp, const MQTT_NS::connect_return_code& rc,
                                          const AsyncHandler& onConnect) {
            KARABO_LOG_FRAMEWORK_INFO << "MQTT  :  Connection to the broker "
                                      << MQTT_NS::connect_return_code_to_str(rc);
            KARABO_LOG_FRAMEWORK_INFO << "MQTT  :  Clean session flag : " << std::boolalpha << sp;
            errc::errc_t ecode;
            switch (rc) {
                case MQTT_NS::connect_return_code::accepted:
                    ecode = errc::success;
                    break;
                case MQTT_NS::connect_return_code::unacceptable_protocol_version:
                    ecode = errc::protocol_not_supported;
                    break;
                case MQTT_NS::connect_return_code::identifier_rejected:
                    ecode = errc::invalid_argument;
                    break;
                case MQTT_NS::connect_return_code::server_unavailable:
                    ecode = errc::resource_unavailable_try_again;
                    break;
                case MQTT_NS::connect_return_code::bad_user_name_or_password:
                case MQTT_NS::connect_return_code::not_authorized:
                    ecode = errc::permission_denied;
                    break;
                default:
                    ecode = errc::bad_message;
                    break;
            };
            auto ec = errc::make_error_code(ecode);
            if (onConnect) {
                dispatch(boost::bind(onConnect, ec));
            }
            if (!ec) {
                resetRequestTimeoutTimer();
            }
            return true;
        }


        void MqttCppClient::registerSubscription(const std::string& topic, const ReadHashHandler& onRead) {
            std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
            // Check if the client is already subscribed to the topic
            auto it = m_subscriptionsMap.find(topic);
            if (it == m_subscriptionsMap.end()) {
                m_subscriptionsMap.emplace(topic,
                                           std::make_tuple(mqtttools::topicHasWildcard(topic), std::move(onRead)));
            } else {
                std::get<1>(it->second) = std::move(onRead);
            }
        }


        void MqttCppClient::handleSubscription(const error_code& ec, const ReadHashHandler& onRead,
                                               const AsyncHandler& handler, const std::string& topic) {
            if (!ec) {
                registerSubscription(topic, onRead);
            }
            // Call the user callback
            handler(ec);
        }


        void MqttCppClient::unregisterSubscription(const std::string& topic) {
            std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
            // Check if the client is subscribed to the topic
            auto it = m_subscriptionsMap.find(topic);
            if (it != m_subscriptionsMap.end()) {
                // Remove the current client entry
                m_subscriptionsMap.erase(it);
            }
        }


        void MqttCppClient::handleUnsubscription(const error_code& ec, const AsyncHandler& handler,
                                                 const std::string& topic) {
            if (!ec) unregisterSubscription(topic);
            // Call the user callback
            handler(ec);
        }


        void MqttCppClient::registerManySubscriptions(const TopicSubOptions& params) {
            // Register read callbacks: one callback per topic
            std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
            for (auto t : params) {
                const std::string& topic = std::get<0>(t);
                ReadHashHandler onRead = std::move(std::get<2>(t));
                auto it = m_subscriptionsMap.find(topic);
                if (it == m_subscriptionsMap.end()) {
                    m_subscriptionsMap.emplace(topic,
                                               std::make_tuple(mqtttools::topicHasWildcard(topic), std::move(onRead)));
                } else {
                    std::get<1>(it->second) = std::move(onRead);
                }
            }
        }


        void MqttCppClient::handleManySubscriptions(const boost::system::error_code& ec, const AsyncHandler& onComplete,
                                                    const TopicSubOptions& params) {
            if (!ec) registerManySubscriptions(params);
            // Call the user callback
            if (onComplete) onComplete(ec);
        }


        void MqttCppClient::unregisterManySubscriptions(const std::vector<std::string>& topics) {
            // Unregister topics ... and callbacks
            std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
            for (const auto& topic : topics) m_subscriptionsMap.erase(topic);
        }


        void MqttCppClient::handleManyUnsubscriptions(const boost::system::error_code& ec,
                                                      const AsyncHandler& onComplete,
                                                      const std::vector<std::string>& topics) {
            if (!ec) unregisterManySubscriptions(topics);
            // Call the user callback
            onComplete(ec);
        }


        // This function is called by the MQTT library if the socket is closed
        // without client's disconnect call


        void MqttCppClient::handleError(const error_code& ec) {
            {
                // Notify all subscribers
                karabo::util::Hash::Pointer hash;
                std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
                for (auto& kv : m_subscriptionsMap) {
                    if (std::get<1>(kv.second)) std::get<1>(kv.second)(ec, kv.first, hash);
                }
                // Remove all subscriptions????
                // m_subscriptionsMap.clear();)
            }
            {
                // Call all pending requests handlers with the error messsage
                std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
                for (auto& kv : m_pendingRequestsMap) {
                    if (kv.second.second) kv.second.second(ec);
                }
                m_pendingRequestsMap.clear();
            }
        }

        // Callback called by the MQTT library every time a new message arrives


        bool MqttCppClient::handleMessage(MQTT_NS::optional<MqttNsClientPacketId> packetId,
                                          MQTT_NS::publish_options pubopts, MQTT_NS::buffer topicName,
                                          MQTT_NS::buffer contents) {
            std::string topic(topicName.cbegin(), topicName.cend());
            // The rule:  one callback per subscription
            // Try to call "exact" subscription having priority over wildcard ...
            std::lock_guard<std::mutex> lock(m_subscriptionsMutex);
            auto it = m_subscriptionsMap.find(topic);
            if (it != m_subscriptionsMap.end()) {
                if (std::get<1>(it->second)) {
                    auto hash = deserializeFrom(contents);
                    post([handler{std::get<1>(it->second)}, topic{std::move(topic)}, hash{std::move(hash)}]() {
                        if (handler) handler(errc::make_error_code(errc::success), topic, hash);
                    });
                }
                return true;
            }
            // Check "wildcard" subscription .... use the first that fits ...
            for (auto& kv : m_subscriptionsMap) {
                if (mqtttools::topicMatches(kv.first, topic)) {
                    if (std::get<1>(kv.second)) {
                        auto hash = deserializeFrom(contents);
                        post([handler{std::get<1>(kv.second)}, topic{std::move(topic)}, hash{std::move(hash)}]() {
                            if (handler) handler(errc::make_error_code(errc::success), topic, hash);
                        });
                    }
                    return true;
                }
            }
            return true;
        }

        // Callback called by the MQTT library every time a request is finished (subscribe, post, etc.)


        bool MqttCppClient::handleRequestResponse(const uint16_t packetId) {
            std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
            // Searches for the request on the global pending requests map
            auto it = m_pendingRequestsMap.find(packetId);
            if (it != m_pendingRequestsMap.end()) {
                // Call the corresponding callback function
                if (it->second.second) {
                    it->second.second(errc::make_error_code(errc::success));
                }
                // Remove the request from the map
                m_pendingRequestsMap.erase(it);
            }
            return true;
        }

        // Callback function used to check for expired requests


        void MqttCppClient::handleRequestTimeout(const error_code& ec) {
            if (!ec) {
                auto now = karabo::util::Epochstamp();
                std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
                for (auto it = m_pendingRequestsMap.begin(); it != m_pendingRequestsMap.end();) {
                    const karabo::util::TimeDuration duration(now - it->second.first);
                    // If the request has expired
                    if (duration.getTotalSeconds() >= m_mqttRequestTimeout) {
                        // Call the callback with an timed_out error code and remove the corresponding entry
                        // from the pending requests map
                        KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ": MQTT request " << it->first << "from "
                                                  << it->second.first << " timed out at " << now;
                        if (it->second.second) {
                            it->second.second(errc::make_error_code(errc::timed_out));
                        }
                        it = m_pendingRequestsMap.erase(it);
                    } else {
                        ++it;
                    }
                }
                resetRequestTimeoutTimer();
            }
        }


        void MqttCppClient::resetRequestTimeoutTimer() {
            m_pendingRequestsTimer.expires_from_now(boost::posix_time::seconds(1));
            m_pendingRequestsTimer.async_wait(boost::bind(&MqttCppClient::handleRequestTimeout, this, _1));
        }


        karabo::util::Hash::Pointer MqttCppClient::deserializeFrom(const MQTT_NS::buffer& archive) {
            karabo::util::Hash::Pointer result = boost::make_shared<Hash>();
            Hash& header = result->bindReference<Hash>("header");
            size_t bytes = m_binarySerializer->load(header, archive.data(), archive.length());
            if (m_skipFlag) {
                std::vector<char>& raw = result->bindReference<std::vector<char>>("raw");
                std::copy(archive.data() + bytes, archive.data() + archive.length(), std::back_inserter(raw));
            } else {
                Hash& body = result->bindReference<Hash>("body");
                m_binarySerializer->load(body, archive.data() + bytes, archive.length() - bytes);
            }
            return result;
        }


        void MqttCppClient::setup(const karabo::util::Hash& input) {
            using namespace boost::asio;
            if (input.has("brokers")) {
                if (m_client && this->isConnected()) {
                    disconnectForced();
                    m_client.reset();
                }
                m_brokerUrls = input.get<std::vector<std::string>>("brokers");
            } else {
                throw KARABO_PARAMETER_EXCEPTION("No \"brokers\" parameter was defined for MqttCppClient");
            }

            m_brokerIndex = 0;

            if (input.has("domain")) input.get("domain", m_domain);
            if (input.has("instanceId")) input.get("instanceId", m_instanceId);
            if (input.has("cleanSession")) input.get("cleanSession", m_cleanSession);
            if (input.has("username")) input.get("username", m_username);
            if (input.has("password")) input.get("password", m_password);
            if (input.has("keepAliveSec")) input.get("keepAliveSec", m_keepAlive);
            if (input.has("pingMs")) input.get("pingMs", m_pingInterval);
            if (input.has("mqttRequestTimeout")) input.get("mqttRequestTimeout", m_mqttRequestTimeout);
            m_willTopic = std::make_shared<std::string>(m_domain + "/topology/" +
                                                        boost::replace_all_copy(m_instanceId, "/", "|"));
            m_will = std::make_shared<MQTT_NS::will>(
                  MQTT_NS::literals::operator""_mb(m_willTopic->c_str(), m_willTopic->size()),
                  MQTT_NS::literals::operator""_mb(0, 0), MQTT_NS::retain::no | MQTT_NS::qos::at_most_once);
        }


        boost::optional<std::string> MqttCppClient::checkForMatchingWildcardTopic(const std::string& topic) {
            for (auto& kv : m_subscriptionsMap) {
                // If it points to a topic that contains wildcards and matches the topic passed as argument
                if (std::get<0>(kv.second) && mqtttools::topicMatches(kv.first, topic)) {
                    return kv.first;
                }
            }
            return boost::none;
        }


        boost::optional<std::string> MqttCppClient::checkForMatchingOtherParams(const std::string& topic,
                                                                                const TopicSubOptions& params) {
            for (const auto& t : params) {
                const std::string& other = std::get<0>(t); // other topic
                if (other != topic && mqtttools::topicHasWildcard(other) && mqtttools::topicMatches(other, topic)) {
                    return other;
                }
            }
            return boost::none;
        }


        std::vector<std::string> MqttCppClient::getSubscriptions() const {
            std::vector<std::string> result;
            for (const auto& item : m_subscriptionsMap) {
                result.push_back(item.first);
            }
            return result;
        }


        std::vector<ReadHashHandler> MqttCppClient::getSubscribeHandler(const std::string& topic) const {
            std::vector<ReadHashHandler> handlers;
            auto it = m_subscriptionsMap.find(topic);
            if (it != m_subscriptionsMap.end()) {
                bool wildcardFlag;
                ReadHashHandler handler;
                std::tie(wildcardFlag, handler) = it->second;
                handlers.push_back(handler);
            }
            return handlers;
        }
    } // namespace net
} // namespace karabo
