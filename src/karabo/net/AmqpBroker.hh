/*
 * File:   AmqpBroker.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on May 18, 2021, 1:20 PM
 */

#ifndef KARABO_NET_AMQPBROKER_HH
#define KARABO_NET_AMQPBROKER_HH

#include <unordered_set>

#include "karabo/net/AmqpClient.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/Strand.hh"


namespace karabo {
    namespace net {


        class AmqpBroker : public Broker {
            /**
             * AmqpBroker operates currently with the following set of ...
             *
             *  Signals are sent to the exchange ...
             *  -------
             *  exchange    = <domain>.signals
             *  routing_key = <signalInstanceId>.<signalName>   <-- selector
             *  queue       = <m_instanceId>              <-- common queue
             *
             *  the signals are emitted to the exchange bound via routing_key to the queue.
             *  The slotInstanceIds should subscribe to the AMQP::topic type exchange
             *  with the 'routing_key' and queue = <slotInstanceId>
             *
             *
             * Special case of above signals... signalHeartbeat ...
             * ------------                     ----------------
             *  exchange    = <domain>.signals
             *  routing_key = <signalInstanceId>.signalHeartbeat
             *  queue       = <m_instanceId>
             *
             *  Calls, commands, requests, replies are sent to
             *  ----------------------------------
             *  exchange    = <domain>.slots.<slotInstanceId>
             *  routing_key = ""
             *  queue       = <m_instanceId>              <-- common queue
             *
             *  all requests/calls/replies to the device send to this exchange
             *  The further message dispatching to slots is provided by using info in message header.
             *
             *
             *  Broadcast messages should be sent to ...
             *  ------------------
             *  exchange    = <domain>.global_slots
             *  routing_key = ""
             *  queue       = <m_instanceId>
             *
             *  there is a way of implementing "broadcast" messages like in
             *  JmsBroker.  In JMS it was enough to use "|*|" in header's slotInstanceIds.  In AMQP we have to
             *  be subscribed to such exchange (to receive broadcast messages).  Known global slots:
             *     slotInstanceNew      -- to announce the new device in Karabo network
             *     slotInstanceUpdated  -- to announce the device info to be updated
             *     slotInstanceGone     -- to announce device death,
             *     slotPing             -- to trigger sending their status by all devices received such message
             *
             *  GUI debug
             *  ---------
             *  exchange    = <domain>.karaboGuiDebug
             *  routing_key = ""
             *  queue       = <as gui debug listener defines>
             *
             *   GUI debugging channel
             *
             */

           public:
            KARABO_CLASSINFO(AmqpBroker, "amqp", "1.0")

            static void expectedParameters(karabo::util::Schema& s);

            explicit AmqpBroker(const karabo::util::Hash& configuration = karabo::util::Hash());

            virtual ~AmqpBroker();

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
             * AMQP subscription:
             * subscribe to the following exchanges...
             *   "m_domain.slots.m_instanceId"
             *   "m_domain.global_slots"
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
             * AMQP subscription
             * Subscribe to the exchange "m_domain.signals" with the
             * routing key: "*.signalHeartbeat"  heartbeats of all known connections
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
            virtual void publish(const std::string& exchange, const std::string& routingkey,
                                 const karabo::util::Hash::Pointer& msg);

           private:
            AmqpBroker(const AmqpBroker& o) = delete;
            AmqpBroker(const AmqpBroker& o, const std::string& newInstanceId);

            void amqpReadHashHandler(const boost::system::error_code& ec, const karabo::util::Hash::Pointer& msg,
                                     const consumer::MessageHandler& handler,
                                     const consumer::ErrorNotifier& errorNotifier);

           protected:
            // "main" producer/consumer client
            karabo::net::AmqpClient::Pointer m_client;
            karabo::net::consumer::MessageHandler m_clientConsumerHandler;
            karabo::net::consumer::ErrorNotifier m_clientErrorNotifier;
            // optional consumers ...
            karabo::net::AmqpClient::Pointer m_heartbeatClient;
            karabo::net::consumer::MessageHandler m_heartbeatConsumerHandler;
            karabo::net::consumer::ErrorNotifier m_heartbeatErrorNotifier;

           private:
            karabo::net::Strand::Pointer m_handlerStrand;
            // producer timestamp is a "marker" of AmqpBroker instance incarnation for m_instanceId in time
            const double m_timestamp; // timestamp used by this instance when in producer role
        };

    } // namespace net
} // namespace karabo


#endif /* KARABO_NET_AMQPBROKER_HH */
