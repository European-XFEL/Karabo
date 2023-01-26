/*
 * File:   RedisBroker.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on May 10, 2021, 2:32 PM
 */

#ifndef KARABO_NET_REDISBROKER_HH
#define KARABO_NET_REDISBROKER_HH


#include "karabo/net/Broker.hh"
#include "karabo/net/RedisClient.hh"
#include "karabo/net/Strand.hh"


namespace karabo {
    namespace net {


        class RedisBroker : public Broker {
            /**
             * RedisBroker operates currently with the following set of topics..
             *
             * <domain>/signals/<signalInstanceId>/<signalName>  <-- the signals are emitted to this topic
             *     The slotInstanceIds should subscribe to the topic and register itself and its slot on
             *     signalinstanceId for message ordering to work.
             *
             * <domain>/slots/<slotInstanceId>    <--  all requests/calls/replies to the device send to this topic
             *     The further message dispatching to slots is provided by using info in message header.
             *
             * <domain>/global_slots              <-- there is a way of implementing "broadcast" messages like in
             *     JmsBroker.  In JMS it was enough to use "|*|" in header's slotInstanceIds.  In REDIS we have to
             *     be subscribe to such topic (to receive broadcast messages).  Known global slots:
             *     slotInstanceNew      -- to announce the new device in Karabo network
             *     slotInstanceUpdated  -- to announce the device info to be updated
             *     slotInstanceGone     -- to announce device death,
             *     slotPing             -- to trigger sending their status by all devices received such message
             *
             * karaboGuiDebug     <--   GUI debugging channel
             *
             * Later the following topics will be used ...
             *
             * <domain>/topology/<instanceId>  [retain]   <--  topology registration or name service as a replacement
             *     for slotPing service. The value of topology entry is 'instanceInfo'.
             *
             * <domain>/props/<instanceId>/<property>  [retain]  <-- this topic keeps "last" value of a property.
             *     Consider to use "retain" message from time to time to update "retained" property value on the broker.
             *     DataLogger, InfluxDB and similar monitoring services can use such topics.
             *     Only device can publish on "props" topic.
             *
             * Probably it can be more ideas to store on the broker some information to be shared.  Possible examples
             * are signals, slots, signalslot connections and etc.
             *
             */

           public:
            KARABO_CLASSINFO(RedisBroker, "redis", "1.0")

            static void expectedParameters(karabo::util::Schema& s);

            explicit RedisBroker(const karabo::util::Hash& configuration = karabo::util::Hash());

            virtual ~RedisBroker();

            Broker::Pointer clone(const std::string& instanceId) override;

            void connect() override;

            void disconnect() override;

            bool isConnected() const override;

            std::string getBrokerUrl() const override;

            std::string getBrokerType() const override {
                return getClassInfo().getClassId();
            }

            boost::system::error_code subscribeToRemoteSignal(const std::string& signalInstanceId,
                                                              const std::string& signalFunction) override;

            boost::system::error_code unsubscribeFromRemoteSignal(const std::string& signalInstanceId,
                                                                  const std::string& signalFunction) override;

            void subscribeToRemoteSignalAsync(const std::string& signalInstanceId, const std::string& signalFunction,
                                              const AsyncHandler& completionHandler) override;

            void unsubscribeFromRemoteSignalAsync(const std::string& signalInstanceId,
                                                  const std::string& signalFunction,
                                                  const AsyncHandler& completionHandler) override;

            /**
             * REDIS subscription:
             * subscribe to group of topics...
             *   "m_domain/slots/m_instanceId"
             *   "m_domain/global_slots"
             *
             * @param handler       - success handler
             * @param errorNotifier - error handler
             */
            void startReading(const consumer::MessageHandler& handler,
                              const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override;

            void stopReading() override;

            /**
             * Heartbeat is used for tracking instances (tracking all instances or no tracking at all)
             *
             * REDIS subscription
             * Subscribe to all topics satisfied the following pattern ...
             *   "m_domain/signals/+/signalHeartbeat"
             *
             * The topic for specific (instance, signal) is ...
             *   "m_domain/signals/signalInstanceId/signalFunction"
             *
             * @param handler       - success handler
             * @param errorNotifier - error handler
             */
            void startReadingHeartbeats(
                  const consumer::MessageHandler& handler,
                  const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override;

            void write(const std::string& topic, const karabo::util::Hash::Pointer& header,
                       const karabo::util::Hash::Pointer& body, const int priority = 4,
                       const int timeToLive = 0) override;

           protected:
            virtual void publish(const std::string& topic, const karabo::util::Hash::Pointer& msg);

           private:
            RedisBroker(const RedisBroker& o) = delete;
            RedisBroker(const RedisBroker& o, const std::string& newInstanceId);

            void redisReadHashHandler(const boost::system::error_code& ec, const std::string& topic,
                                      const karabo::util::Hash::Pointer& msg, const consumer::MessageHandler& handler,
                                      const consumer::ErrorNotifier& errorNotifier);

            void registerRedisTopic(const std::string& topic, const consumer::MessageHandler& handler,
                                    const consumer::ErrorNotifier& errorNotifier);

            void unregisterRedisTopic(const std::string& topic);

            void registerRedisTopics(const std::vector<std::string>& topics, const consumer::MessageHandler& handler,
                                     const consumer::ErrorNotifier& errorNotifier);

            void unregisterRedisTopics(const std::vector<std::string>& topics);

            void checkOrder(const std::string& topic, const karabo::util::Hash::Pointer& msg,
                            const consumer::MessageHandler& handler);

            void setOrderNumbers(const std::string& consumers, const karabo::util::Hash::Pointer& header);

            void handleStore(const std::string& producerId, long long lastReceivedNumber);

            /**
             * Remove all entries in m_store[producerId] that have timestamp field
             * other than validTimestamp.
             * @param producerId
             * @param validTimestamp
             */
            void cleanObsolete(const std::string& producerId, const double validTimestamp);

           protected:
            karabo::net::RedisClient::Pointer m_client;

           private:
            karabo::net::Strand::Pointer m_handlerStrand;
            // Store handlers set during 'startReading' for use in signal subscriptions
            consumer::MessageHandler m_messageHandler;
            consumer::ErrorNotifier m_errorNotifier;
            boost::mutex m_handlerMutex; // to protect m_messageHandler & m_errorNotifier settings
            // Message ordering.
            // ----------------
            //                 consumerId,  last serial number sent
            std::unordered_map<std::string, long long> m_producerMap;
            boost::mutex m_producerMapMutex;

            //                 producerId,  last serial number received   timestamp
            std::unordered_map<std::string, long long> m_consumerMap;
            std::unordered_map<std::string, double> m_consumerTimestamp;
            boost::mutex m_consumerMapMutex;
            // storage for temporarily keeping "pending" messages in hope the message with number that restores the
            // order will come soon ...
            //                 producerId -> map orderNumber -> (producer timestamp, callback)
            std::unordered_map<std::string, std::map<long long, std::pair<double, boost::function<void()> > > > m_store;

            unsigned int m_subscribeTimeout;
            // producer timestamp is a "marker" of RedisBroker instance incarnation for m_instanceId in time
            const double m_timestamp; // timestamp used by this instance when in producer role
        };

    } // namespace net
} // namespace karabo


#endif /* KARABO_NET_REDISBROKER_HH */
