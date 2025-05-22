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

#include <map>
#include <set>

#include "AmqpHashClient.hh"
#include "Broker.hh"
#include "Strand.hh"
#include "karabo/data/types/Hash.hh"

namespace karabo {
    namespace data {
        class Schema;
    }
    namespace net {


        class AmqpBroker : public Broker {
            /**
             *
             * AmqpBroker works as follows on top of AmqpHashClient
             *
             *
             *  Signals are sent with
             *  -------
             *  exchange    = <domain>.signals
             *  routing_key = <senderInstanceId>.<signalName>   <-- selector
             *
             *
             *  One-to-one slot calls (including replies) are sent with
             *  ----------------------------------
             *  exchange    = <domain>.slots
             *  routing_key = <receiverInstanceId>.<slotToCall>
             *
             *
             *  Broadcast messages are sent with
             *  ------------------
             *  exchange    = <domain>.global_slots
             *  routing_key = <senderInstanceId>.<slotToCall>
             *
             * Note that only a limited list of broadcast slot are supported.
             *
             *
             * All messages are received on the read handler passed to the constructor.
             * The first argument of the read handler is the slot to call.
             * For 1-to-1 and broadcast messages that is the "slotToCall" of the routing key,
             * i.e. the "slot" argument of the respective 'send*' method.
             * For signals, the slot is defined when subscribing to the signal.
             *
             * Broadcast messages are only received if they are not deselected before starting to read
             * via setConsumeBroadcasts(false).
             * The slotHeartbeat is even more special, it is only received if startReadingHeartbeats()
             * is called.
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

            boost::system::error_code subscribeToRemoteSignal(const std::string& slot,
                                                              const std::string& signalInstanceId,
                                                              const std::string& signalFunction) override;

            boost::system::error_code unsubscribeFromRemoteSignal(const std::string& slot,
                                                                  const std::string& signalInstanceId,
                                                                  const std::string& signalFunction) override;

            void subscribeToRemoteSignalAsync(const std::string& slot, const std::string& signalInstanceId,
                                              const std::string& signalFunction,
                                              const AsyncHandler& completionHandler) override;

            void unsubscribeFromRemoteSignalAsync(const std::string& slot, const std::string& signalInstanceId,
                                                  const std::string& signalFunction,
                                                  const AsyncHandler& completionHandler) override;

            /**
             * Start reading messages from the broker
             *
             * By default, AmqpBroker subscribes to 1-to-1 slots for this instance
             * (exchange "<domain>.slots", routing key "<instanceId>.*") and the normal broadcast slots
             * (exchange "<domain>.global_slots", routing key "<instanceId>.<global_slot>"), i.e.
             * not for slotHeartbeat.
             * If setConsumeBroadcasts(false) was called before, the subscription to the broadcast slots is skipped.
             *
             * @param handler       - success handler
             * @param errorNotifier - error handler
             */
            void startReading(const consumer::MessageHandler& handler,
                              const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override;

            void stopReading() override;

            /**
             * Subscribe to heart beat broadcast messages
             *
             * Broadcast messages to "slotHeartbeat" will now be read
             * (exchange "<domain>.global_slots", routing key "*.slotHeartbeat").
             *
             * Must be called after startReading, heartbeats are fed to the same handler.
             */
            void startReadingHeartbeats() override;

            /**
             * Send a signal message
             *
             * @param signal the signal
             * @param header message header
             * @param body message body
             */
            void sendSignal(const std::string& signal, const karabo::data::Hash::Pointer& header,
                            const karabo::data::Hash::Pointer& body) override;

            /**
             * Send a broadcast message
             *
             * @param slot the broadcast slot
             * @param header message header
             * @param body message body
             */
            void sendBroadcast(const std::string& slot, const karabo::data::Hash::Pointer& header,
                               const karabo::data::Hash::Pointer& body) override;

            /**
             * Send a 1-to-1 message
             *
             * @param receiverId the instance id of the addressee
             * @param slot the addressee's slot to call
             * @param header message header
             * @param body message body
             */
            void sendOneToOne(const std::string& receiverId, const std::string& slot,
                              const karabo::data::Hash::Pointer& header,
                              const karabo::data::Hash::Pointer& body) override;

           private:
            AmqpBroker(const AmqpBroker& o) = delete;
            AmqpBroker(const AmqpBroker& o, const std::string& newInstanceId);

            void amqpReadHandler(const data::Hash::Pointer& header, const data::Hash::Pointer& body,
                                 const std::string& exchange, const std::string& key);
            void amqpErrorNotifier(const std::string& msg);

            void publish(const std::string& exchange, const std::string& routingKey,
                         const karabo::data::Hash::Pointer& header, const karabo::data::Hash::Pointer& body);

            karabo::net::AmqpConnection::Pointer m_connection;

            karabo::net::AmqpHashClient::Pointer m_client;
            karabo::net::Strand::Pointer m_handlerStrand;
            karabo::net::consumer::MessageHandler m_readHandler;
            karabo::net::consumer::ErrorNotifier m_errorNotifier;


            /// Key is routing key of a signal (<instanceId>.<signalName>),
            /// value is set of slot names subscribed
            /// Concurrency protection by ensuring that touched only within m_handlerStrand
            std::map<std::string, std::set<std::string>> m_slotsForSignals;

            const std::string m_slotExchange;
            const std::string m_globalSlotExchange;
        };

    } // namespace net
} // namespace karabo


#endif /* KARABO_NET_AMQPBROKER_HH */
