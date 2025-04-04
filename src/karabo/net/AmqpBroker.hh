/*
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
 */
/*
 * File:   AmqpBroker.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on May 18, 2021, 1:20 PM

 * Refactored by gero.flucke@xfel.eu in May 2024
 */

#ifndef KARABO_NET_AMQPBROKER_HH
#define KARABO_NET_AMQPBROKER_HH

#include <amqpcpp/table.h>

#include "AmqpHashClient.hh"
#include "Broker.hh"
#include "Strand.hh"


namespace karabo {
    namespace util {
        class Hash;
        class Schema;
    } // namespace util
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
             *  exchange    = <domain>.slots
             *  routing_key = <slotInstanceId>
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
            KARABO_CLASSINFO(AmqpBroker, "amqp", "2.0")

            static void expectedParameters(karabo::data::Schema& s);

            /**
             * Fill argument with default AMQP message queue creation arguments
             */
            static void defaultQueueArgs(AMQP::Table& args);

            explicit AmqpBroker(const karabo::data::Hash& configuration = karabo::data::Hash());

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
             *   "m_domain.slots" with routingKey m_instanceId
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

            /**
             * Write message to broker, blocks until written
             *
             * @param topic Either the "domain" as passed to the Broker base class,
             *              the "domain" with the suffix "_beats", or "karaboGuiDebug"
             * @param header of the message - must contain
             * @param body of the message
             * @param priority unused (needed by JmsBroker)
             * @param timeToLive unused (needed by JmsBroker)
             */
            void write(const std::string& topic, const karabo::data::Hash::Pointer& header,
                       const karabo::data::Hash::Pointer& body, const int /*priority*/ = 4,
                       const int /*timeToLive*/ = 0) override;

           private:
            AmqpBroker(const AmqpBroker& o) = delete;
            AmqpBroker(const AmqpBroker& o, const std::string& newInstanceId);

            void amqpReadHandler(const data::Hash::Pointer& header, const data::Hash::Pointer& body);
            void amqpReadHandlerBeats(const data::Hash::Pointer& header, const data::Hash::Pointer& body);
            void amqpErrorNotifier(const std::string& msg);
            void amqpErrorNotifierBeats(const std::string& msg);

            karabo::net::AmqpConnection::Pointer m_connection;

            karabo::net::AmqpHashClient::Pointer m_client;
            karabo::net::Strand::Pointer m_handlerStrand;
            karabo::net::consumer::MessageHandler m_readHandler;
            karabo::net::consumer::ErrorNotifier m_errorNotifier;

            karabo::net::AmqpHashClient::Pointer m_heartbeatClient;
            karabo::net::Strand::Pointer m_handlerStrandBeats;
            karabo::net::consumer::MessageHandler m_readHandlerBeats;
            karabo::net::consumer::ErrorNotifier m_errorNotifierBeats;
        };

    } // namespace net
} // namespace karabo


#endif /* KARABO_NET_AMQPBROKER_HH */
