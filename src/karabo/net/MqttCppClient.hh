/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   MqttCppClient.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on August 28, 2018, 1:39 PM
 */

#ifndef KARABO_NET_MQTTCPPCLIENT_HH
#define KARABO_NET_MQTTCPPCLIENT_HH

#include <mqtt_client_cpp.hpp>
#include <unordered_map>
#include <vector>

#include "MqttClient.hh"
#include "karabo/io/BinarySerializer.hh"
#include "karabo/util/Hash.hh"


namespace karabo {
    namespace util {
        class Schema;
    }
    namespace net {

        using TcpEndPoint = MQTT_NS::tcp_endpoint<boost::asio::ip::tcp::socket, boost::asio::io_context::strand>;
        // using TcpEndPoint = MQTT_NS::tcp_endpoint<boost::asio::ip::tcp::socket, boost::asio::io_service::strand>;
        using MqttNsAsyncClient = MQTT_NS::callable_overlay<MQTT_NS::async_client<TcpEndPoint>>;
        using MqttNsClient = MQTT_NS::callable_overlay<MQTT_NS::client<TcpEndPoint>>;
        using MqttNsSyncClient = MQTT_NS::callable_overlay<MQTT_NS::sync_client<TcpEndPoint>>;
        using MqttNsClientPacketId = typename std::remove_reference_t<MqttNsClient>::packet_id_t; // since C++14

        class MqttCppClient : public MqttClient {
            MqttCppClient(const MqttCppClient&) = delete;
            MqttCppClient& operator=(const MqttCppClient&) = delete;

           public:
            KARABO_CLASSINFO(MqttCppClient, "MqttCppClient", "2.0")

            static void expectedParameters(karabo::util::Schema& expected);

            MqttCppClient(const karabo::util::Hash& input);

            virtual ~MqttCppClient();

            boost::system::error_code connect() override;

            void connectAsync(const AsyncHandler& onConnect) override;

            bool isConnected() const override;
            ;

            boost::system::error_code disconnect() override;

            void disconnectAsync(const AsyncHandler& onComplete) override;

            void disconnectForced() override;

            boost::system::error_code subscribe(const TopicSubOptions& params) override;

            void subscribeAsync(const TopicSubOptions& params, const AsyncHandler& onComplete) override;

            boost::system::error_code unsubscribe(const std::string& topic) override;

            void unsubscribeAsync(const std::string& topic, const AsyncHandler& onComplete) override;

            boost::system::error_code unsubscribe(const std::vector<std::string>& tuples) override;

            void unsubscribeAsync(const std::vector<std::string>& topics, const AsyncHandler& onComplete) override;

            boost::system::error_code unsubscribeAll() override;

            void unsubscribeAllAsync(const AsyncHandler& onComplete) override;

            bool isSubscribed(const std::string& topic) const override;

            bool isMatched(const std::string& topic);

            std::string getClientId() const override;

            const std::string& getBrokerUrl() const override;

            std::vector<std::string> getSubscriptions() const override;

            std::vector<ReadHashHandler> getSubscribeHandler(const std::string& topic) const override;

            ReadHashHandler getReadHashHandler(const std::string& topic) const;

           protected:
            boost::system::error_code subscribe(const std::string& topic, std::uint8_t subopts,
                                                const ReadHashHandler& onRead) override;

            void subscribeAsync(const std::string& topic, std::uint8_t subopts, const ReadHashHandler& onRead,
                                const AsyncHandler& onComplete) override;

            boost::system::error_code publish(const std::string& topic, const karabo::util::Hash::Pointer& msg,
                                              std::uint8_t pubopts) override;

            void publishAsync(const std::string& topic, const karabo::util::Hash::Pointer& msg, std::uint8_t pubopts,
                              const AsyncHandler& onComplete) override;


           private:
            void createClientForUrl(const std::string& url, const AsyncHandler& onConnect);

            void setup(const karabo::util::Hash& input);

            bool handleConnect(const bool sp, const MQTT_NS::connect_return_code& rc, const AsyncHandler& onConnect);

            void registerSubscription(const std::string& topic, const ReadHashHandler& onRead);

            void unregisterSubscription(const std::string& topic);

            void handleSubscription(const boost::system::error_code& ec, const ReadHashHandler& onRead,
                                    const AsyncHandler& handler, const std::string& topic);

            void handleUnsubscription(const boost::system::error_code& ec, const AsyncHandler& handler,
                                      const std::string& topic);

            void registerManySubscriptions(const TopicSubOptions& params);

            void handleManySubscriptions(const boost::system::error_code& ec, const AsyncHandler& handler,
                                         const TopicSubOptions& params);

            void unregisterManySubscriptions(const std::vector<std::string>& topics);

            void handleManyUnsubscriptions(const boost::system::error_code& ec, const AsyncHandler& handler,
                                           const std::vector<std::string>& topics);

            // Template function used to call an asynchronous MQTT operation
            // When the operation completes, call the handler

            template <typename Operation, typename Handler>
            void performAsyncOperation(const Operation& op, const Handler& handler) {
                if (isConnected()) {
                    // Call the MQTT operation
                    uint16_t packetId = op();

                    // Adds the request to the global (static) pending requests map
                    // When the request finishes, posts the handler function call on the main event loop

                    std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
                    m_pendingRequestsMap.emplace(
                          packetId, std::make_pair(karabo::util::Epochstamp(),
                                                   [this, h{std::move(handler)}](const boost::system::error_code ec) {
                                                       // Direct call looking more reasonable fails to work so ...
                                                       // ... call via event loop ...
                                                       post(boost::bind(h, ec));
                                                   }));
                } else {
                    post(boost::bind(handler, KARABO_ERROR_CODE_NOT_CONNECTED));
                }
            }

            // This function is called by the MQTT library if the socket is closed
            // without client's disconnect call
            void handleError(const boost::system::error_code& ec);

            // Callback called by the MQTT library every time a new message arrives

            bool handleMessage(MQTT_NS::optional<MqttNsClientPacketId> packetId, MQTT_NS::publish_options pubopts,
                               MQTT_NS::buffer topic_name, MQTT_NS::buffer contents);

            // Callback called by the MQTT library every time a request is finished (subscribe, post, etc.)
            bool handleRequestResponse(const uint16_t packetId);

            // Callback function used to check for expired requests
            void handleRequestTimeout(const boost::system::error_code& ec);

            void resetRequestTimeoutTimer();

            karabo::util::Hash::Pointer deserializeFrom(const MQTT_NS::buffer& archive);

            boost::optional<std::string> checkForMatchingWildcardTopic(const std::string& topic);

            boost::optional<std::string> checkForMatchingOtherParams(const std::string& topic,
                                                                     const TopicSubOptions& params);

           private:
            std::shared_ptr<MqttNsAsyncClient> m_client;
            std::size_t m_brokerIndex;
            bool m_cleanSession;
            std::string m_clientId;
            std::string m_username;
            std::string m_password;
            std::uint16_t m_keepAlive;
            std::uint32_t m_pingInterval;
            std::uint32_t m_mqttRequestTimeout;
            std::shared_ptr<std::string> m_willTopic;
            std::shared_ptr<MQTT_NS::will> m_will;
            std::mutex m_gMutex;

            // Mutex used to avoid concurrent calls of connectAsync
            std::mutex m_connectionMutex;
            // Mutex used to avoid concurrent calls of disconnectAsync
            std::mutex m_disconnectionMutex;
            // Mutex used to avoid concurrent calls of subscribeAsync
            std::mutex m_subscribeMutex;
            // Pending requests map:
            // - Key: packet id (MQTT request unique identifier)
            // - Value: pair<request_initial_time, handler to be called on request completion>
            std::unordered_map<uint16_t, std::pair<karabo::util::Epochstamp, AsyncHandler>> m_pendingRequestsMap;
            std::mutex m_pendingRequestsMutex;
            // Timer used to check for expired pending requests
            boost::asio::deadline_timer m_pendingRequestsTimer;
            // Subscriptions map:
            // - Key: topic
            // - Value: tuple of ...
            //                - Flag indicating if the Topic has Wildcards,
            //                - ReadHashHandler that is called when topic related message arrived
            std::unordered_map<std::string, std::tuple<bool, ReadHashHandler>> m_subscriptionsMap;
            std::mutex m_subscriptionsMutex;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;
        };
    } // namespace net
} // namespace karabo

// KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::net::MqttClient)

#endif /* KARABO_NET_MQTTCPPCLIENT_HH */
