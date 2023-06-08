/*
 * $Id$
 *
 * File:   JmsChannel.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 18, 2016, 9:47 AM
 *
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

#include "JmsProducer.hh"

#include <openmqc/mqcrt.h>

#include <karabo/io/HashBinarySerializer.hh>
#include <karabo/log.hpp>

#include "EventLoop.hh"
#include "JmsConnection.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace net {


        JmsProducer::JmsProducer(const JmsConnection::Pointer& connection)
            : m_connection(connection), m_mqStrand(EventLoop::getIOService()) {
            m_binarySerializer = BinarySerializer<Hash>::create("Bin");
            m_producerSessionHandle.handle = HANDLED_OBJECT_INVALID_HANDLE;
        }


        JmsProducer::~JmsProducer() {
            this->clearProducerHandles();
        }


        void JmsProducer::write(const std::string& topic, const karabo::util::Hash::Pointer& header,
                                const karabo::util::Hash::Pointer& body, const int priority, const int timeToLive) {
            this->asyncWrite(topic, header, body, priority, timeToLive);
        }


        void JmsProducer::asyncWrite(const std::string& topic, const Hash::Pointer& header, const Hash::Pointer& body,
                                     const int priority, const int timeToLive) {
            std::vector<char> buffer;
            m_binarySerializer->save(body, buffer);

            MQMessageHandle messageHandle;
            MQ_SAFE_CALL(MQCreateBytesMessage(&messageHandle));

            MQPropertiesHandle propertiesHandle;
            MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));

            this->setProperties(*header, propertiesHandle);

            MQ_SAFE_CALL(MQSetMessageProperties(messageHandle, propertiesHandle));

            if (buffer.size() > 0) {
                MQ_SAFE_CALL(MQSetBytesMessageBytes(
                      messageHandle, reinterpret_cast<MQInt8*>(const_cast<char*>(&buffer[0])), buffer.size()));
            }

            MQProducerHandle producerHandle = this->getProducer(topic);
            MQStatus status =
                  MQSendMessageExt(producerHandle, messageHandle, MQ_NON_PERSISTENT_DELIVERY, priority, timeToLive);
            MQFreeMessage(messageHandle);
            MQError statusCode = MQGetStatusCode(status);
            switch (statusCode) {
                case MQ_SUCCESS:
                    // Do nothing
                    break;
                case MQ_STATUS_INVALID_HANDLE:
                case MQ_BROKER_CONNECTION_CLOSED: {
                    // Need to clear old handles
                    this->clearProducerHandles();
                    m_connection->waitForConnectionAvailable();
                    // Next trial will re-cache all handles
                    m_mqStrand.post(bind_weak(&karabo::net::JmsProducer::asyncWrite, this, topic, header, body,
                                              priority, timeToLive));
                    break;
                }
                default: {
                    MQString tmp = MQGetStatusString(status);
                    std::string errorString(tmp);
                    MQFreeString(tmp);
                    throw KARABO_OPENMQ_EXCEPTION("Problem during message sending: " + errorString);
                }
            }
        }


        MQProducerHandle JmsProducer::getProducer(const std::string& topic) {
            Producers::const_iterator it = m_producers.find(topic);
            if (it != m_producers.end()) return it->second;


            std::pair<MQSessionHandle, MQDestinationHandle> handles = ensureProducerDestinationAvailable(topic);
            MQProducerHandle producerHandle;

            m_connection->waitForConnectionAvailable();

            MQ_SAFE_CALL(MQCreateMessageProducerForDestination(handles.first, handles.second, &producerHandle));
            m_producers[topic] = producerHandle;

            return producerHandle;
        }


        std::pair<MQSessionHandle, MQDestinationHandle> JmsProducer::ensureProducerDestinationAvailable(
              const std::string& topic) {
            ProducerDestinations::const_iterator it = m_producerDestinations.find(topic);
            if (it != m_producerDestinations.end()) return it->second;

            m_connection->waitForConnectionAvailable();

            MQSessionHandle sessionHandle = ensureProducerSessionAvailable();
            MQDestinationHandle destinationHandle;


            MQ_SAFE_CALL(MQCreateDestination(sessionHandle, topic.c_str(), MQ_TOPIC_DESTINATION, &destinationHandle))
            m_producerDestinations[topic] = std::make_pair(sessionHandle, destinationHandle);

            return std::make_pair(sessionHandle, destinationHandle);
        }


        MQSessionHandle JmsProducer::ensureProducerSessionAvailable() {
            if (m_producerSessionHandle.handle == HANDLED_OBJECT_INVALID_HANDLE) {
                MQ_SAFE_CALL(MQCreateSession(m_connection->m_connectionHandle, MQ_FALSE, /* isTransacted */
                                             MQ_CLIENT_ACKNOWLEDGE, MQ_SESSION_SYNC_RECEIVE, &m_producerSessionHandle));
            }
            return m_producerSessionHandle;
        }


        void JmsProducer::clearProducerHandles() {
            // Clear producers


            for (const Producers::value_type& i : m_producers) {
                MQCloseMessageProducer(i.second);
            }
            m_producers.clear();

            // Clear producer destinations


            for (const ProducerDestinations::value_type& i : m_producerDestinations) {
                MQFreeDestination(i.second.second);
            }
            m_producerDestinations.clear();

            // Flag session invalid
            MQCloseSession(m_producerSessionHandle);
            m_producerSessionHandle.handle = HANDLED_OBJECT_INVALID_HANDLE;
        }


        void JmsProducer::setProperties(const karabo::util::Hash& properties,
                                        const MQPropertiesHandle& propertiesHandle) const {
            try {
                for (Hash::const_iterator it = properties.begin(); it != properties.end(); ++it) {
                    Types::ReferenceType type = it->getType();
                    switch (type) {
                        case Types::STRING:
                            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle, it->getKey().c_str(),
                                                             it->getValue<std::string>().c_str()))
                            break;
                        case Types::UINT8:
                            MQ_SAFE_CALL(MQSetInt8Property(propertiesHandle, it->getKey().c_str(),
                                                           it->getValueAs<signed char>()))
                            break;
                        case Types::INT8:
                            MQ_SAFE_CALL(MQSetInt8Property(propertiesHandle, it->getKey().c_str(),
                                                           it->getValue<signed char>()))
                            break;
                        case Types::UINT16:
                            MQ_SAFE_CALL(
                                  MQSetInt16Property(propertiesHandle, it->getKey().c_str(), it->getValueAs<short>()))
                            break;
                        case Types::INT16:
                            MQ_SAFE_CALL(
                                  MQSetInt16Property(propertiesHandle, it->getKey().c_str(), it->getValue<short>()))
                            break;
                        case Types::UINT32:
                            MQ_SAFE_CALL(
                                  MQSetInt32Property(propertiesHandle, it->getKey().c_str(), it->getValueAs<int>()))
                            break;
                        case Types::INT32:
                            MQ_SAFE_CALL(
                                  MQSetInt32Property(propertiesHandle, it->getKey().c_str(), it->getValue<int>()))
                            break;
                        case Types::UINT64:
                            MQ_SAFE_CALL(MQSetInt64Property(propertiesHandle, it->getKey().c_str(),
                                                            it->getValueAs<long long>()))
                            break;
                        case Types::INT64:
                            MQ_SAFE_CALL(
                                  MQSetInt64Property(propertiesHandle, it->getKey().c_str(), it->getValue<long long>()))
                            break;
                        case Types::FLOAT:
                            MQ_SAFE_CALL(
                                  MQSetFloat32Property(propertiesHandle, it->getKey().c_str(), it->getValue<float>()))
                            break;
                        case Types::DOUBLE:
                            MQ_SAFE_CALL(
                                  MQSetFloat64Property(propertiesHandle, it->getKey().c_str(), it->getValue<double>()))
                            break;
                        case Types::BOOL:
                            MQ_SAFE_CALL(
                                  MQSetBoolProperty(propertiesHandle, it->getKey().c_str(), it->getValue<bool>()))
                            break;
                        default:
                            throw KARABO_NOT_SUPPORTED_EXCEPTION("Given property value type (" +
                                                                 Types::to<ToLiteral>(type) +
                                                                 ") is not supported by the OpenMQ");
                            break;
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }
    } // namespace net
} // namespace karabo
