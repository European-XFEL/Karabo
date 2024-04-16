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

#ifndef KARABO_NET_AMQPCLIENT2_HH
#define KARABO_NET_AMQPCLIENT2_HH

#include <amqpcpp.h>

#include <map>
#include <mutex>
#include <string>
#include <utility> // for std::pair
#include <vector>

#include "AmqpConnection.hh"
#include "utils.hh" // for AsyncHandler

namespace AMQP {
    class Channel;
}

namespace karabo::net {

    /**
     * @brief Class that exposes an AMQP client
     *
     * It receives messages directed to the... queue name is instaceId
     *
     */
    class AmqpClient2 : public boost::enable_shared_from_this<AmqpClient2> {
       public:
        KARABO_CLASSINFO(AmqpClient2, "AmqpClient2", "2.0")
        // KARABO_CONFIGURATION_BASE_CLASS

        /** Channel status tells what should be the next step to do in channel preparation */
        // not clear - all steps without subscriptions?
        enum class ChannelStatus {
            REQUEST,
            CREATE,
            // CHECK_QUEUE,
            // RECREATE,
            CREATE_QUEUE,
            // CREATE_EXCHANGE,
            // BIND_QUEUE,
            CREATE_CONSUMER,
            READY
        };
        enum class SubscriptionStatus { PENDING, CREATE_EXCHANGE, BIND_QUEUE, READY };

        // Let's see whether that is a good signature here - maybe better directly provide the deserialised header/body
        // (i.e. run deserialisation of a Strand running in Karabo's normal eventloop)
        using ReadHandler = std::function<void(const std::shared_ptr<std::vector<char>>& data,
                                               const std::string& exchange, const std::string& routingKey)>;
        // static void expectedParameters(karabo::util::Schema& expected);
        /**
         * Create client from connection
         *
         * @param connection must already be connected
         * @param instanceId the client id - will usually be the name of the queue that will be subscribed
         * @param queueArgs the arguments passed to channel creation
         * @param readHandler a read handler for all received messages - must be a valid, callable function
         */
        AmqpClient2(AmqpConnection::Pointer connection, std::string instanceId, AMQP::Table queueArgs,
                    ReadHandler readHandler);

        virtual ~AmqpClient2();

        /**
         * Asynchronously subscribes client
         *
         * @param exchange name of AMQP exchange that will be created if not yet existing
         * @param routingKey the AMQP routing key
         * @param callback handler called in AMQP io context (so please no mutex inside, please)
         *                 when subscription established or failed
         */
        void asyncSubscribe(const std::string& exchange, const std::string& routingKey,
                            AsyncHandler onSubscriptionDone);

        void asyncUnsubscribe(const std::string& exchange, const std::string& routingKey,
                              AsyncHandler onUnsubscriptionDone);

        void asyncPublish(const std::string& exchange, const std::string& routingKey,
                          const std::shared_ptr<std::vector<char>>& data, AsyncHandler onPublishDone);

       private:
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

        void doSubscribePending(const boost::system::error_code& ec);

        void moveSubscriptionState(const std::string& exchange, const std::string& routingKey);

        AmqpConnection::Pointer m_connection;
        const std::string m_instanceId;
        const AMQP::Table m_queueArgs;
        const ReadHandler m_readHandler;

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
        std::vector<PostponedMessage> m_postponedPubMessages;

    }; // AmqpClient2

} // namespace karabo::net

#endif /* KARABO_NET_AMQPCLIENT2_HH */
