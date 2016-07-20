/*
 * $Id$
 *
 * File:   JmsChannel.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 18, 2016, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/io/HashBinarySerializer.hh>

#include "JmsChannel.hh"
#include "JmsConnection.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace net {


        JmsChannel::JmsChannel(const JmsConnection::Pointer& connection) : m_connection(connection) {
            m_binarySerializer = BinarySerializer<Hash>::create("Bin");
            m_producerSessionHandle.handle = HANDLED_OBJECT_INVALID_HANDLE;
        }


        void JmsChannel::write(const std::string& topic,
                               const Hash::Pointer& header,
                               const Hash::Pointer& body,
                               const int priority,
                               const int timeToLive) {

            std::vector<char> buffer;
            m_binarySerializer->save(body, buffer);

            MQMessageHandle messageHandle;
            MQ_SAFE_CALL(MQCreateBytesMessage(&messageHandle));

            MQPropertiesHandle propertiesHandle;
            MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));

            this->setProperties(*header, propertiesHandle);

            MQ_SAFE_CALL(MQSetMessageProperties(messageHandle, propertiesHandle));

            if (buffer.size() > 0) {
                MQ_SAFE_CALL(MQSetBytesMessageBytes(messageHandle,
                                                    reinterpret_cast<MQInt8*> (const_cast<char*> (&buffer[0])),
                                                    buffer.size()));
            }

            MQProducerHandle producerHandle = this->getProducer(topic);
            MQStatus status = MQSendMessageExt(producerHandle, messageHandle,
                                               MQ_NON_PERSISTENT_DELIVERY,
                                               priority, timeToLive);

            if (MQStatusIsError(status) != MQ_FALSE) {
                MQString tmp = MQGetStatusString(status);
                std::string errorString(tmp);
                MQFreeString(tmp);
                cout << errorString << endl;
            }

        }


        MQProducerHandle JmsChannel::getProducer(const std::string& topic) {

            Producers::const_iterator it = m_producers.find(topic);
            if (it != m_producers.end()) return it->second;

            std::pair<MQSessionHandle, MQDestinationHandle> handles = ensureProducerDestinationAvailable(topic);
            MQProducerHandle producerHandle;
            MQ_SAFE_CALL(MQCreateMessageProducerForDestination(handles.first, handles.second, &producerHandle));
            m_producers[topic] = producerHandle;
            return producerHandle;
        }


        MQSessionHandle JmsChannel::ensureProducerSessionAvailable() {

            if (m_producerSessionHandle.handle == HANDLED_OBJECT_INVALID_HANDLE) {
                MQ_SAFE_CALL(MQCreateSession(m_connection->m_connectionHandle,
                                             MQ_FALSE, /* isTransacted */
                                             MQ_CLIENT_ACKNOWLEDGE,
                                             MQ_SESSION_SYNC_RECEIVE,
                                             &m_producerSessionHandle));
            }
            return m_producerSessionHandle;
        }


        std::pair<MQSessionHandle, MQDestinationHandle>
        JmsChannel::ensureProducerDestinationAvailable(const std::string& topic) {

            ProducerDestinations::const_iterator it = m_producerDestinations.find(topic);
            if (it != m_producerDestinations.end()) return it->second;

            MQSessionHandle sessionHandle = ensureProducerSessionAvailable();
            MQDestinationHandle destinationHandle;
            MQ_SAFE_CALL(MQCreateDestination(sessionHandle, topic.c_str(), MQ_TOPIC_DESTINATION, &destinationHandle))
            m_producerDestinations[topic] = make_pair(sessionHandle, destinationHandle);
            return make_pair(sessionHandle, destinationHandle);
        }


        void JmsChannel::setProperties(const karabo::util::Hash& properties, const MQPropertiesHandle & propertiesHandle) {
            try {
                for (Hash::const_iterator it = properties.begin(); it != properties.end(); it++) {
                    Types::ReferenceType type = it->getType();
                    switch (type) {
                        case Types::STRING:
                            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle, it->getKey().c_str(), it->getValue<string>().c_str()))
                            break;
                        case Types::INT8:
                            MQ_SAFE_CALL(MQSetInt8Property(propertiesHandle, it->getKey().c_str(), it->getValue<signed char>()))
                            break;
                        case Types::UINT16:
                        case Types::INT16:
                            MQ_SAFE_CALL(MQSetInt16Property(propertiesHandle, it->getKey().c_str(), it->getValue<short>()))
                            break;
                        case Types::UINT32:
                            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, it->getKey().c_str(), it->getValue<unsigned int>()))
                            break;
                        case Types::INT32:
                            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, it->getKey().c_str(), it->getValue<int>()))
                            break;
                        case Types::UINT64:
                            MQ_SAFE_CALL(MQSetInt64Property(propertiesHandle, it->getKey().c_str(), it->getValue<unsigned long long>()))
                            break;
                        case Types::INT64:
                            MQ_SAFE_CALL(MQSetInt64Property(propertiesHandle, it->getKey().c_str(), it->getValue<long long>()))
                            break;
                        case Types::FLOAT:
                            MQ_SAFE_CALL(MQSetFloat32Property(propertiesHandle, it->getKey().c_str(), it->getValue<float>()))
                            break;
                        case Types::DOUBLE:
                            MQ_SAFE_CALL(MQSetFloat64Property(propertiesHandle, it->getKey().c_str(), it->getValue<double>()))
                            break;
                        case Types::BOOL:
                            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle, it->getKey().c_str(), it->getValue<bool>()))
                            break;
                        default:
                            throw KARABO_NOT_SUPPORTED_EXCEPTION("Given property value type (" + Types::to<ToLiteral>(type) + ") is not supported by the OpenMQ");
                            break;
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }

    }
}

