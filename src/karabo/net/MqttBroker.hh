/*
 * File:   MqttBroker.hh
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * Created on June 27, 2020, 9:23 PM
 */

#ifndef KARABO_NET_MQTTBROKER_HH
#define KARABO_NET_MQTTBROKER_HH


#include "karabo/net/Broker.hh"
#include "karabo/net/MqttClient.hh"
#include "karabo/net/Strand.hh"


namespace karabo {
    namespace net {


        class MqttBroker : public Broker {
            /**
             * MqttBroker operates currently with the following set of topics..
             *
             * <domain>/signals/<<signalInstanceId>/<signalName>  <-- the signals are emitted to this topic
             *     The slotInstanceIds should subscribe to the topic and register itself and its slot on
             *     signalinstanceId for message ordering to work.
             *
             * <domain>/slots/<slotInstanceId>    <--  all requests/calls/replies to the device send to this topic
             *     The further message dispatching to slots is provided by using info in message header.
             *
             * <domain>/global_slots              <-- there is a way of implementing "broadcast" messages like in
             *     JmsBroker.  In JMS it was enough to use "|*|" in header's slotInstanceIds.  In MQTT we have to
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
            KARABO_CLASSINFO(MqttBroker, "mqtt", "1.0")

            static void expectedParameters(karabo::util::Schema& s);

            explicit MqttBroker(const karabo::util::Hash& configuration = karabo::util::Hash());

            virtual ~MqttBroker();

            Broker::Pointer clone(const std::string& instanceId) override;

            void connect() override;

            void disconnect() override;

            bool isConnected() const override;

            std::string getBrokerUrl() const override;

            std::string getBrokerType() const override {
                return getClassInfo().getClassId();
            }

            std::string getClientId() const;

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
             * MQTT subscription:
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
             * MQTT subscription
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
            virtual void publish(const std::string& topic, const karabo::util::Hash::Pointer& msg, PubOpts options);

           private:
            MqttBroker(const MqttBroker& o) = delete;
            MqttBroker(const MqttBroker& o, const std::string& newInstanceId);

            void mqttReadHashHandler(const boost::system::error_code& ec, const std::string& topic,
                                     const karabo::util::Hash::Pointer& msg, const consumer::MessageHandler& handler,
                                     const consumer::ErrorNotifier& errorNotifier);

            void registerMqttTopic(const std::string& topic, const karabo::net::SubOpts& subopts,
                                   const consumer::MessageHandler& handler,
                                   const consumer::ErrorNotifier& errorNotifier);

            void unregisterMqttTopic(const std::string& topic);

            void registerMqttTopics(const std::vector<std::string>& topics,
                                    const std::vector<karabo::net::SubOpts>& options,
                                    const consumer::MessageHandler& handler,
                                    const consumer::ErrorNotifier& errorNotifier);

            void unregisterMqttTopics(const std::vector<std::string>& topics);

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
            karabo::net::MqttClient::Pointer m_client;

           private:
            karabo::net::Strand::Pointer m_handlerStrand;
            consumer::MessageHandler m_messageHandler;
            consumer::ErrorNotifier m_errorNotifier;
            // Message ordering.
            // ----------------
            // Some messages are required to be received in the same order in which they were sent.  This makes
            // sense when considering a communication between specific producer and specific consumer (one-to-one).
            // Because there is a broker in between the message may come not in order.  More precisely, the broker
            // guarantees the ordering of messages sending via the same topic with QoS > 0, but not for messages send
            // via different topics.  In practice, the broker shows the ordering even for messages sent via different
            // topics.  But it may be specific for particular broker and the protocol itself gives no guarantees.
            // To discover a disorder the producer has to account messages sent to every consumer by using
            //                     producerMap[consumerId] = serial number.
            // The consumers, in turn, have to account message numbers for all producers they are interested in by
            // using consumerMap[producerId]. Important!  The producer has to know to whom the message should be sent.
            // Fortunately, in karabo messaging the producer knows this because consumers (slotInstanceIds) register
            // themselves on producer (signalInstanceId) side. The message contains list of consumer IDs and,
            // in parallel, the list of "order" (serial) numbers (vector of long long).
            // The counting starts from 1 and incremented by 1 in every following message.  The message number 1 forces
            // the consumer counter to reset.  After receiving a message the consumer compares the number in message
            // with number in consumerMap[producerId] and can judge if disorder happens: the difference should be 1.
            //
            // Caveat:
            //   1.  Message accounting works only if messages are not dropped by broker.   So QoS = 0 are not
            // accounted (can be dropped) and "broadcasts" are not because the destinations are unknown
            //   2.  After restarting the counters are initialized to 0, and, in case that some devices can be restarted
            // and others are not the consumers should always be synchronized with producers

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

            typedef boost::shared_ptr<boost::asio::deadline_timer> DeadlinePointer;
            // Deadline is established on producer: any disorder should be resolved before deadline.
            std::unordered_map<std::string, DeadlinePointer> m_deadlines;
            // Deadline timer setup: timeout
            unsigned int m_deadlineTimeout;
            // producer timestamp is a "marker" of MqttBroker instance incarnation for m_instanceId in time
            const double m_timestamp; // timestamp used by this instance when in producer role
        };

    } // namespace net
} // namespace karabo

#endif /* KARABO_NET_MQTTBROKER_HH */
