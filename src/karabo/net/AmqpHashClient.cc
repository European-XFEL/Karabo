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
 * File:   AmqpHashClient.cc
 * Author: Gero Flucke
 *
 * Created on April 30, 2024
 */


#include "AmqpHashClient.hh"

#include "EventLoop.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/log/Logger.hh"
#include "karabo/util/MetaTools.hh" // for bind_weak


using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace karabo::net {

    AmqpHashClient::Pointer AmqpHashClient::create(AmqpConnection::Pointer connection, std::string instanceId,
                                                   AMQP::Table queueArgs, AmqpHashClient::HashReadHandler readHandler,
                                                   AmqpHashClient::ErrorReadHandler errorReadHandler) {
        Pointer result(new AmqpHashClient(std::move(connection), std::move(instanceId), std::move(queueArgs),
                                          std::move(readHandler), std::move(errorReadHandler)));
        // Cannot use yet use bind_weak in constructor, so do here
        result->m_rawClient->setReadHandler(util::bind_weak(&AmqpHashClient::onRead, result.get(), _1, _2, _3));
        return result;
    }

    AmqpHashClient::AmqpHashClient(AmqpConnection::Pointer connection, std::string instanceId, AMQP::Table queueArgs,
                                   AmqpHashClient::HashReadHandler readHandler,
                                   AmqpHashClient::ErrorReadHandler errorReadHandler)
        : m_rawClient(std::make_shared<AmqpClient>(std::move(connection), std::move(instanceId), std::move(queueArgs),
                                                   AmqpClient::ReadHandler())), // Cannot use bind_weak in constructor,
                                                                                // so handler setting must be postponed
          m_serializer(io::BinarySerializer<data::Hash>::create("Bin")),
          m_deserializeStrand(std::make_shared<Strand>(EventLoop::getIOService())),
          m_readHandler(std::move(readHandler)),
          m_errorReadHandler(std::move(errorReadHandler)) {}

    AmqpHashClient::~AmqpHashClient() {}

    void AmqpHashClient::asyncPublish(const std::string& exchange, const std::string& routingKey,
                                      const data::Hash::Pointer& header, const data::Hash::Pointer& body,
                                      AsyncHandler onPublishDone) {
        // Instead of permanent re-allocation we could have a re-used (and growing) cache.
        // But then we have to make sure (or require?) that this function is not called again until onPublishDone
        // is called (or so)
        auto data = std::make_shared<std::vector<char>>();
        data->reserve(1024); // Avoid too many re-allocations: Most messages are several hundred bytes

        // Serialize header and body into the raw data container, just one after another
        m_serializer->save2(*header, *data); // header -> data
        m_serializer->save2(*body, *data);   // body   -> data

        m_rawClient->asyncPublish(exchange, routingKey, data, std::move(onPublishDone));
    }

    void AmqpHashClient::onRead(const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                                const std::string& routingKey) {
        // Leave single thread of io context of our connection, but keep order, so post on Strand
        m_deserializeStrand->post(util::bind_weak(&AmqpHashClient::deserialize, this, data, exchange, routingKey));
    }

    void AmqpHashClient::deserialize(const std::shared_ptr<std::vector<char>>& data, const std::string& exchange,
                                     const std::string& routingKey) {
        data::Hash::Pointer header(std::make_shared<data::Hash>());
        data::Hash::Pointer body(std::make_shared<data::Hash>());
        try {
            const size_t bytes = m_serializer->load(*header, data->data(), data->size());
            header->set("exchange", exchange);
            header->set("routingkey", routingKey);

            // TODO:
            // The old client had a m_skipFlag here to potentially avoid deserialisation of 'body'.
            // This is used in the broker rates tool.
            m_serializer->load(*body, data->data() + bytes, data->size() - bytes);
        } catch (const data::Exception& e) {
            const std::string userMsg(e.userFriendlyMsg(false));                       // Do not clear trace yet
            KARABO_LOG_FRAMEWORK_WARN << "Failed to deserialize: " << e.detailedMsg(); // Clears exception trace
            m_errorReadHandler(userMsg);
            return;
        } catch (const std::exception& e) {
            KARABO_LOG_FRAMEWORK_WARN << "Failed to deserialize: " << e.what();
            m_errorReadHandler(e.what());
            return;
        }
        // Deserialization succeeded, so call handler
        m_readHandler(header, body);
    }
} // namespace karabo::net
