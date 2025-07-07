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
 * File:   AmqpClient.hh
 * Author: Gero Flucke, based on prior work of Sergey Esenov
 *
 * Created on April 3, 2024
 */

#ifndef KARABO_NET_AMQPCLIENT_HH
#define KARABO_NET_AMQPCLIENT_HH

#include <amqpcpp.h>

#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility> // for std::pair
#include <vector>

#include "AmqpConnection.hh"
#include "karabo/data/types/Hash.hh"
#include "utils.hh" // for AsyncHandler

namespace AMQP {
    class Channel;
}

namespace karabo::net {

    /**
     * @brief Class that exposes an AMQP client
     *
     * It will create a unique queue and consume from it exclusively and with automatic acknowledgment. Its queue name
     * will start with the given instanceId and will potentially be suffixed by some characters to ensure uniqueness.
     *
     * To actually receive messages via the handlers specified in the constructor, the client has to subscribe to
     * exchanges, potentially with routing keys to select messages on the broker side.
     *
     * @note:
     * This client does not know about Karabo "domains" (a.k.a. "topics"), i.e. exchanges and queues created are
     * "shared" among all clients connected to the same broker.
     *
     */
    class AmqpClient : public std::enable_shared_from_this<AmqpClient> {
       public:
        KARABO_CLASSINFO(AmqpClient, "AmqpClient", "3.0")

        /** Channel status tells what should be the next step to do in channel preparation */
        enum class ChannelStatus { REQUEST, CREATE, CREATE_QUEUE, CREATE_CONSUMER, READY };
        // Since exchanges are created with autodelete flag, we must ensure they are bound to a queue
        // as long as we want to publish to them. If the exchange is declared, we can subscribe to it,
        // if we want to publish, we need to add some dummy binding to be sure, only then it is "publishable".
        /** Exchange status tells about the status of a known exchange */
        enum class ExchangeStatus { DECLARING, DECLARED, BINDING, PUBLISHABLE };
        /** Subscription status tells in which status a registered subscription currently is */
        enum class SubscriptionStatus { PENDING, CHECK_EXCHANGE, DECLARE_EXCHANGE, BIND_QUEUE, READY, UNBIND_QUEUE };

        // Handler to receive raw data
        using ReadHandler = std::function<void(const std::shared_ptr<std::vector<char>>& data,
                                               const std::string& exchange, const std::string& routingKey)>;

        static size_t m_maxMessageSize;

        /**
         * Create client with raw data interface from connection
         *
         * @param connection the connection, all internal data access will run in its io context
         * @param instanceId the client id - will usually be the name of the queue that will be subscribed
         * @param queueArgs the arguments passed to queue creation
         * @param readHandler a read handler for all received messages
         *                    (if an invalid function, must call setReadHandler before the first subscription)
         */
        AmqpClient(AmqpConnection::Pointer connection, std::string instanceId, AMQP::Table queueArgs,
                   ReadHandler readHandler);

        virtual ~AmqpClient();

        /**
         * (Re-)set the read handler that will be called for all received messages
         *
         * @param readHandler A valid read function (karabo::data::ParameterException if not valid)
         */
        void setReadHandler(ReadHandler readHandler);

        /**
         * Asynchronously subscribes client
         *
         * If subscription is reported to have failed, it will be tried again
         *   - at next subscription or
         *   - if reviveIfReconnected() is called.
         *
         * @param exchange name of AMQP exchange that will be created if not yet existing
         * @param routingKey the AMQP routing key
         * @param onSubscriptionDone a valid handler called in AMQP io context (so please no mutex inside, please)
         *                           when subscription established or failed
         */
        void asyncSubscribe(const std::string& exchange, const std::string& routingKey,
                            AsyncHandler onSubscriptionDone);

        /**
         * Asynchronously unsubscribes client

         * Note:
         * Success will be reported for an unsubscription from exchange/routing key that it was not subscribed before
         *
         * @param exchange name of AMQP exchange that will be unsubscribed from
         * @param routingKey the AMQP routing key to unsubscribe from
         * @param onUnsubscriptionDone a valid handler called in AMQP io context (so please no mutex inside, please)
         *                             when unsubscription succeeded or failed
         */
        void asyncUnsubscribe(const std::string& exchange, const std::string& routingKey,
                              AsyncHandler onUnsubscriptionDone);

        /**
         * Asynchronously unsubscribes client from all subscriptions
         *
         * @param onUnsubscriptionDone a valid handler called in AMQP io context (so please no mutex inside, please)
         *                             when all unsubscription requests are done. If any of them failed, the error
         *                             code passed is the one of the last failure
         */
        void asyncUnsubscribeAll(AsyncHandler onUnsubscriptionsDone);

        /**
         * Asynchronously publish data
         *
         * @param exchange the exchange...
         * @param routingKey ...and the routingkey for the data
         * @param data a raw data container fo the message to be published (must be non-zero pointer)
         * @param onPublishDone handler called in AMQP io context (so please no mutex inside, please)
         *                      when data published
         */
        void asyncPublish(const std::string& exchange, const std::string& routingKey,
                          const std::shared_ptr<std::vector<char>>& data, AsyncHandler onPublishDone);

        /**
         * Revice after connection was lost and re-established
         *
         * Means to recreate channel, redo all subscriptions and publish postponed messages
         *
         * To be called if AmqpConnection is connected again after connection loss
         * Must be called within io context of AmqpConnection
         */
        void reviveIfReconnected();

       private:
        struct PostponedMessage {
            PostponedMessage(std::string exchange_, std::string routingKey_, std::shared_ptr<std::vector<char>> data_,
                             AsyncHandler handler)
                : exchange(std::move(exchange_)),
                  routingKey(std::move(routingKey_)),
                  data(std::move(data_)),
                  onPublishDone(std::move(handler)) {}
            std::string exchange;
            std::string routingKey;
            std::shared_ptr<std::vector<char>> data;
            AsyncHandler onPublishDone;
        };

        /**
         * Prepare m_channel until it is ChannelStatus::READY
         *
         * @param onChannelPrepared handler called when m_channel READY or if failure on the way
         *
         * Must be called in the io context of the AmqpConnection
         */
        void asyncPrepareChannel(AsyncHandler onChannelPrepared);

        /**
         * Helper to move the created channel through its states, asynchronously calling itself.
         * If READY (or failure), call and erase the m_channelPreparationCallback
         */
        void moveChannelState();

        void channelErrorHandler(const char* errMsg);

        void doSubscribePending(const boost::system::error_code& ec);

        void moveSubscriptionState(const std::string& exchange, const std::string& routingKey);

        void asyncPrepareExchangeThenPublish(const std::string& exchange);

        /**
         * Helper to publish, must run in io context and only when channel is READY and exchange declared
         */
        void doPublish(const std::string& exchange, const std::string& routingKey,
                       const std::shared_ptr<std::vector<char>>& data, const AsyncHandler& onPublishDone);

        /**
         * Queue message (or drop if queueu too long), must run in io context
         */
        void queueMessage(PostponedMessage&& message);

        /**
         * Helper to publish postponed messages until first found with an exchange that is not yet declared
         */
        void publishPostponed();

        AmqpConnection::Pointer m_connection;
        const std::string m_instanceId;
        std::string m_queue; // may differ from id since queue needs to be unique
        const AMQP::Table m_queueArgs;
        ReadHandler m_readHandler;

        // No need to take care of mutex protections if we ensure that all data member access is happening within
        // single threaded io context of m_connection
        std::shared_ptr<AMQP::Channel> m_channel;
        ChannelStatus m_channelStatus;

        AsyncHandler m_channelPreparationCallback;
        struct SubscriptionStatusHandler {
            SubscriptionStatusHandler(SubscriptionStatus status_, AsyncHandler onSubscription_)
                : status(status_), onSubscription(std::move(onSubscription_)) {}
            SubscriptionStatus status;
            AsyncHandler onSubscription;
        };
        // maybe can use unordered_map?
        std::map<std::pair<std::string, std::string>, SubscriptionStatusHandler> m_subscriptions;

        /// Messages postponed since channel not yet ready or exchange not yet declared
        std::queue<PostponedMessage> m_postponedPubMessages;

        std::unordered_map<std::string, ExchangeStatus> m_exchanges; // known exchanges and their status

    }; // AmqpClient

} // namespace karabo::net

#endif /* KARABO_NET_AMQPCLIENT_HH */
