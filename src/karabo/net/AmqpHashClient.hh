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
 * File:   AmqpHashClient.hh
 * Author: Gero Flucke
 *
 * Created on April 30, 2024
 */

#ifndef KARABO_NET_AMQPHASHCLIENT_HH
#define KARABO_NET_AMQPHASHCLIENT_HH

#include <string>

#include "AmqpClient.hh"
#include "AmqpConnection.hh"
#include "Strand.hh"
#include "karabo/io/BinarySerializer.hh"
#include "karabo/util/Hash.hh"
#include "utils.hh" // for AsyncHandler

namespace karabo::net {

    /**
     * @brief Class that wraps around AmqpClient to provide a message interface with Hash header and body
     *
     * Deserialisation of incoming messages is done via a karabo::net::Strand,
     * i.e. a running karabo::net::EventLoop is needed.
     *
     */
    class AmqpHashClient : public std::enable_shared_from_this<AmqpHashClient> {
       public:
        KARABO_CLASSINFO(AmqpHashClient, "AmqpHashClient", "2.0")

        // std::function fits better than std::function to assigning bind_weak results to these handlers in AmqpBroker
        using HashReadHandler = std::function<void(const util::Hash::Pointer&, const util::Hash::Pointer&)>;
        using ErrorReadHandler = std::function<void(const std::string&)>;

        /**
         * Create client with message interface based on two Hashes (header and body).
         *
         * @param connection the connection, all internal data access will run in its io context
         * @param instanceId the client id - will usually be the name of the queue that will be subscribed
         * @param queueArgs the arguments passed to queue creation
         * @param readHandler a valid read handler for all received messages
         * @param errorReadHandler a valid handler called when a received message could not be processed, e.g. due to
         *                         serialisation problems
         */
        static Pointer create(AmqpConnection::Pointer connection, std::string instanceId, AMQP::Table queueArgs,
                              HashReadHandler readHandler, ErrorReadHandler errorReadHandler);

        virtual ~AmqpHashClient();

        /**
         * Asynchronously subscribes client by just forwarding to AmqpClient::asyncSubscribe
         *
         *  ==> See docs of that.
         */
        inline void asyncSubscribe(const std::string& exchange, const std::string& routingKey,
                                   AsyncHandler onSubscriptionDone) {
            m_rawClient->asyncSubscribe(exchange, routingKey, std::move(onSubscriptionDone));
        }

        /**
         * Asynchronously unsubscribes client by just forwarding to AmqpClient::asyncUnsubscribe
         *
         *  ==> See docs of that.
         */
        inline void asyncUnsubscribe(const std::string& exchange, const std::string& routingKey,
                                     AsyncHandler onUnsubscriptionDone) {
            m_rawClient->asyncUnsubscribe(exchange, routingKey, std::move(onUnsubscriptionDone));
        }

        /**
         * Asynchronously unsubscribes client from all subscriptions by just forwarding to
         * AmqpClient::asyncUnsubscribeAll
         *
         *  ==> See docs of that.
         */
        inline void asyncUnsubscribeAll(AsyncHandler onUnsubscriptionDone) {
            m_rawClient->asyncUnsubscribeAll(std::move(onUnsubscriptionDone));
        }

        /**
         * Asynchronously publish data from header and body
         *
         * Hashes are serialised such that AmqpClient::asyncPublish can be use internally.
         *  ==> See docs of that.
         */
        void asyncPublish(const std::string& exchange, const std::string& routingKey, const util::Hash::Pointer& header,
                          const util::Hash::Pointer& body, AsyncHandler onPublishDone);

       private:
        /**
         * Internal constructor, use static create instead: raw clients read handler has to be set after construction
         */
        AmqpHashClient(AmqpConnection::Pointer connection, std::string instanceId, AMQP::Table queueArgs,
                       HashReadHandler readHandler, ErrorReadHandler errorReadHandler);

        /**
         * Handler passed to raw client (i.e. runs in io context of connection).
         *
         * Post arguments for deserialzation on the respective strand that runs in Karabo event loop
         */
        void onRead(const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                    const std::string& routingKey);

        /**
         * Deserializes 'data' input into Hash for header and body, adds exchange and key to the header and calls
         * handler passed to constructor
         */
        void deserialize(const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                         const std::string& routingKey);

        AmqpClient::Pointer m_rawClient;

        karabo::io::BinarySerializer<util::Hash>::Pointer m_serializer;
        karabo::net::Strand::Pointer m_deserializeStrand;
        const HashReadHandler m_readHandler;
        const ErrorReadHandler m_errorReadHandler;

    }; // AmqpHashClient

} // namespace karabo::net

#endif /* KARABO_NET_AMQPHASHCLIENT_HH */
