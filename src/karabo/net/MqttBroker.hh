/* 
 * File:   MqttBroker.hh
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 * Created on June 27, 2020, 9:23 PM
 */

#ifndef KARABO_NET_MQTTBROKER_HH
#define	KARABO_NET_MQTTBROKER_HH


#include "karabo/net/Broker.hh"
#include "karabo/net/Strand.hh"
#include "karabo/net/MqttClient.hh"


namespace karabo {
    namespace net {
        

        class MqttBroker : public Broker  {
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
             * <domain>/global_slots/<slotFunction>   <-- there is a way of implementing "broadcast" messages like in
             *     JmsBroker.  In JMS it was enough to use "|*|" in header's slotInstanceIds.  In MQTT we have to
             *     be subscribe to such topic (to receive broadcast messages).  Known global slots:
             *     slotInstanceNew      -- to announce the new device in Karabo network
             *     slotInstanceUpdated  -- to announce the device info to be updated
             *     slotInstanceGone     -- to announce device death,
             *     slotPing             -- to trigger sending their status by all devices received such message
             *
             * <domain/log        <--  this is a place where all printing directed
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

            std::string getBrokerType() const override { return getClassInfo().getClassId(); }

            std::string getClientId() const;

            boost::system::error_code subscribeToRemoteSignal(
                    const std::string& signalInstanceId,
                    const std::string& signalFunction) override;

            boost::system::error_code unsubscribeFromRemoteSignal(
                    const std::string& signalInstanceId,
                    const std::string& signalFunction) override;

            void subscribeToRemoteSignalAsync(const std::string& signalInstanceId,
                                        const std::string& signalFunction,
                                        const AsyncHandler& completionHandler) override;

            void unsubscribeFromRemoteSignalAsync(const std::string& signalInstanceId,
                                           const std::string& signalFunction,
                                           const AsyncHandler& completionHandler) override;

            /**
             * MQTT subscription:
             * subscribe to group of topics...
             *   "m_domain/slots/m_instanceId"
             *   "m_domain/global_slots/+"      Example: "SPB/global_slots/slotInstanceNew"
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
            void startReadingHeartbeats(const consumer::MessageHandler& handler,
                                const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override;

            /**
             * MQTT subscription.
             * Subscribe to topic:
             *   "m_domain/log"
             *
             * @param handler       - success handler
             * @param errorNotifier - error handler
             */
            void startReadingLogs(const consumer::MessageHandler& handler,
                          const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override;

            void write(const std::string& topic,
                       const karabo::util::Hash::Pointer& header,
                       const karabo::util::Hash::Pointer& body,
                       const int priority = 4,
                       const int timeToLive = 0) override;

            void writeLocal(const consumer::MessageHandler& handler,
                            const karabo::util::Hash::Pointer& header,
                            const karabo::util::Hash::Pointer& body) override;

            bool checkForGlobalCalls(const std::string& id, const karabo::util::Hash::Pointer& header) override {
                if (id == "*") return true;
                if (header->has("slotInstanceIds")) {
                    const std::string& slotInstanceIds = header->get<std::string>("slotInstanceIds");
                    // special case (broadcast): send via broker's "global slots"
                    // id != "*" and slotInstanceIds == "|*|" comes from DeviceServer::onBroadcastMessage
                    if (slotInstanceIds == "|*|") return true;
                }
                return false;
            }

        private:

            MqttBroker(const MqttBroker& o) = delete;
            MqttBroker(const MqttBroker& o, const std::string& newInstanceId);

            virtual void publish(const std::string& topic,
                                 const karabo::util::Hash::Pointer& msg,
                                 PubOpts options);

            void mqttReadHashHandler(const boost::system::error_code& ec,
                                     const std::string& topic,
                                     const karabo::util::Hash::Pointer & msg,
                                     const consumer::MessageHandler& handler,
                                     const consumer::ErrorNotifier& errorNotifier);

            void registerMqttTopic(const std::string& topic,
                                   const karabo::net::SubOpts& subopts,
                                   const consumer::MessageHandler& handler,
                                   const consumer::ErrorNotifier& errorNotifier);

            void unregisterMqttTopic(const std::string& topic);

            void registerMqttTopics(const std::vector<std::string>& topics,
                                    const std::vector<karabo::net::SubOpts>& options,
                                    const consumer::MessageHandler& handler,
                                    const consumer::ErrorNotifier& errorNotifier);

            void unregisterMqttTopics(const std::vector<std::string>& topics);

            void checkOrder(const consumer::MessageHandler& handler,
                            const karabo::util::Hash::Pointer& header,
                            const karabo::util::Hash::Pointer& body, bool local=false);

        protected:

            karabo::net::MqttClient::Pointer  m_client;

        private:

            karabo::net::Strand::Pointer m_handlerStrand;
            consumer::MessageHandler m_messageHandler;
            consumer::ErrorNotifier m_errorNotifier;
        };

    }
}

#endif	/* KARABO_NET_MQTTBROKER_HH */

