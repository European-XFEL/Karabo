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
#include <karabo/log.hpp>
#include "JmsChannel.hh"
#include "JmsConnection.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace net {


        JmsChannel::JmsChannel(const JmsConnection::Pointer& connection, const IOServicePointer& ioService) :
            m_connection(connection),
            m_ioService(ioService) {
            m_binarySerializer = BinarySerializer<Hash>::create("Bin");
            m_producerSessionHandle.handle = HANDLED_OBJECT_INVALID_HANDLE;
        }


        JmsChannel::~JmsChannel() {
        }


        void JmsChannel::write(const std::string& topic,
                               const Hash::Pointer& header,
                               const Hash::Pointer& body,
                               const int priority,
                               const int timeToLive) {

            // This function will block in case no connection is available and return immediately otherwise
            m_connection->waitForConnectionAvailable();

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

            if (MQStatusIsError(status) == MQ_TRUE) {
                if (MQGetStatusCode(status) == MQ_STATUS_INVALID_HANDLE) {
                    // Most probably connection was dropped in between
                    // Need to clear old handles
                    this->clearProducerHandles();
                    // Next trial will re-cache all handles
                    this->write(topic, header, body, priority, timeToLive);
                } else {
                    MQString tmp = MQGetStatusString(status);
                    std::string errorString(tmp);
                    MQFreeString(tmp);
                    throw KARABO_OPENMQ_EXCEPTION("Problem during message sending: " + errorString);
                }
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


        void JmsChannel::clearProducerHandles() {

            // Flag session invalid
            m_producerSessionHandle.handle = HANDLED_OBJECT_INVALID_HANDLE;

            // Clear producer destinations
            m_producerDestinations.clear();

            // Clear producers
            m_producers.clear();
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


        void JmsChannel::readAsync(const MessageHandler handler, const std::string& topic, const std::string& selector) {

            m_connection->m_openMQService->post(boost::bind(&karabo::net::JmsChannel::asyncConsumeMessage, this, handler, topic, selector));
        }


        void JmsChannel::asyncConsumeMessage(const MessageHandler handler, const std::string& topic, const std::string& selector) {

            m_connection->waitForConnectionAvailable();

            // For any new consumer, create a new thread in the openMQ thread pool
            if (!this->hasConsumer(topic, selector)) m_connection->addOpenMQServiceThread();

            // This calls may happen concurrently, hence both functions are thread-safe
            MQSessionHandle sessionHandle = this->ensureConsumerSessionAvailable(topic);
            MQConsumerHandle consumerHandle = this->getConsumer(topic, selector);

            MQMessageHandle messageHandle;
            MQStatus status = MQReceiveMessageWithTimeout(consumerHandle, 100, &messageHandle);

            MQError statusCode = MQGetStatusCode(status);
            switch (statusCode) {

                case MQ_CONSUMER_DROPPED_MESSAGES:
                { // Deal with hand-crafted error code
                    MQString statusString = MQGetStatusString(status);
                    // post error handler, or so
                    KARABO_LOG_FRAMEWORK_ERROR << "Problem during message consumption: " << statusString;
                    MQFreeString(statusString);
                }
                case MQ_SUCCESS:
                { // Message received
                    MQ_SAFE_CALL(MQAcknowledgeMessages(sessionHandle, messageHandle));

                    MQMessageType messageType;
                    MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType));

                    // Wrong message type -> notify error, ignore this message and re-post
                    if (messageType != MQ_BYTES_MESSAGE) {
                        m_connection->m_openMQService->post(boost::bind(&karabo::net::JmsChannel::asyncConsumeMessage, this,
                                                                        handler, topic, selector));
                        return;
                    }
                    Hash::Pointer header(new Hash());
                    Hash::Pointer body(new Hash());
                    int nBytes;
                    const MQInt8* bytes;

                    MQ_SAFE_CALL(MQGetBytesMessageBytes(messageHandle, &bytes, &nBytes));
                    this->parseHeader(messageHandle, *header);
                    //                    if (header->has("__compression__")) {
                    //                        std::vector<char> tmp;
                    //                        //decompress(*header, reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes), tmp);
                    //                        m_binarySerializer->load(*body, tmp);
                    //                    } else {
                    m_binarySerializer->load(*body, reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes));
                    //}
                    // Cross post into channel's io_service
                    m_ioService->post(boost::bind(handler, header, body));
                    break;
                }

                case MQ_TIMEOUT_EXPIRED:
                { // No message received, post again
                    m_connection->m_openMQService->post(boost::bind(&karabo::net::JmsChannel::asyncConsumeMessage, this,
                                                                    handler, topic, selector));
                    break;
                }
                case MQ_STATUS_INVALID_HANDLE:
                case MQ_BROKER_CONNECTION_CLOSED:
                case MQ_SESSION_CLOSED:
                case MQ_CONSUMER_CLOSED:
                { // Invalidate handles and re-post
                    // This function may be called concurrently, hence its thread-safe
                    this->clearConsumerHandles();
                    m_connection->m_openMQService->post(boost::bind(&karabo::net::JmsChannel::asyncConsumeMessage, this,
                                                                    handler, topic, selector));
                    break;
                }
                default:
                {
                    // Report error via exception existence
                    MQString tmp = MQGetStatusString(status);
                    std::string errorString(tmp);
                    MQFreeString(tmp);
                    throw KARABO_OPENMQ_EXCEPTION(errorString);
                }
            }
        }


        bool JmsChannel::hasConsumer(const std::string& topic, const std::string& selector) const {
            boost::mutex::scoped_lock lock(m_consumerHandlesMutex);
            return (m_consumers.find(topic + selector) != m_consumers.end());
        }


        MQConsumerHandle JmsChannel::getConsumer(const std::string& topic, const std::string& selector) {
            {
                boost::mutex::scoped_lock lock(m_consumerHandlesMutex);
                Consumers::const_iterator it = m_consumers.find(topic + selector);
                if (it != m_consumers.end()) return it->second;
            }
            std::pair<MQSessionHandle, MQDestinationHandle> handles = this->ensureConsumerDestinationAvailable(topic);
            MQConsumerHandle consumerHandle;
            {
                boost::mutex::scoped_lock lock(m_consumerHandlesMutex);
                MQ_SAFE_CALL(MQCreateMessageConsumer(handles.first, handles.second, selector.c_str(), MQ_FALSE /*noLocal*/, &consumerHandle));
                m_consumers[topic + selector] = consumerHandle;
            }
            return consumerHandle;
        }


        std::pair<MQSessionHandle, MQDestinationHandle> JmsChannel::ensureConsumerDestinationAvailable(const std::string& topic) {
            {
                boost::mutex::scoped_lock lock(m_consumerHandlesMutex);
                ConsumerDestinations::const_iterator it = m_consumerDestinations.find(topic);
                if (it != m_consumerDestinations.end()) return it->second;
            }
            MQSessionHandle sessionHandle = ensureConsumerSessionAvailable(topic);
            MQDestinationHandle destinationHandle;
            {
                boost::mutex::scoped_lock lock(m_consumerHandlesMutex);
                MQ_SAFE_CALL(MQCreateDestination(sessionHandle, topic.c_str(), MQ_TOPIC_DESTINATION, &destinationHandle))
                m_consumerDestinations[topic] = make_pair(sessionHandle, destinationHandle);
            }
            return make_pair(sessionHandle, destinationHandle);
        }


        MQSessionHandle JmsChannel::ensureConsumerSessionAvailable(const std::string& topic) {
            boost::mutex::scoped_lock lock(m_consumerHandlesMutex);
            ConsumerSessions::const_iterator it = m_consumerSessions.find(topic);
            if (it != m_consumerSessions.end()) return it->second;

            MQSessionHandle consumerSessionHandle;
            MQ_SAFE_CALL(MQCreateSession(m_connection->m_connectionHandle,
                                         MQ_FALSE, /* isTransacted */
                                         MQ_CLIENT_ACKNOWLEDGE,
                                         MQ_SESSION_SYNC_RECEIVE,
                                         &consumerSessionHandle));
            m_consumerSessions[topic] = consumerSessionHandle;
            return consumerSessionHandle;
        }


        void JmsChannel::clearConsumerHandles() {
            boost::mutex::scoped_lock lock(m_consumerHandlesMutex);
            m_consumerSessions.clear();
            m_consumerDestinations.clear();
            m_consumers.clear();
        }


        void JmsChannel::parseHeader(const MQMessageHandle& messageHandle, karabo::util::Hash& header) {
            MQPropertiesHandle propertiesHandle, headerHandle;
            MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
            MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                    this->getProperties(header, propertiesHandle);
            this->getProperties(header, headerHandle);
            MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
            MQ_SAFE_CALL(MQFreeProperties(headerHandle))
        }


        void JmsChannel::getProperties(Hash& properties, const MQPropertiesHandle& propertiesHandle) const {
            try {
                MQ_SAFE_CALL(MQPropertiesKeyIterationStart(propertiesHandle))
                while (MQPropertiesKeyIterationHasNext(propertiesHandle)) {
                    ConstMQString mqKey;
                    MQ_SAFE_CALL(MQPropertiesKeyIterationGetNext(propertiesHandle, &mqKey));
                    string key(mqKey);
                    MQType type;
                    MQ_SAFE_CALL(MQGetPropertyType(propertiesHandle, mqKey, &type))
                    switch (type) {
                        case MQ_STRING_TYPE:
                        {
                            ConstMQString mqValue;
                            MQ_SAFE_CALL(MQGetStringProperty(propertiesHandle, mqKey, &mqValue))
                            properties.set<string > (key, string(mqValue));
                            break;
                        }
                        case MQ_INT8_TYPE:
                        {
                            MQInt8 mqValue;
                            MQ_SAFE_CALL(MQGetInt8Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<signed char>(key, mqValue);
                            break;
                        }
                        case MQ_INT16_TYPE:
                        {
                            MQInt16 mqValue;
                            MQ_SAFE_CALL(MQGetInt16Property(propertiesHandle, mqKey, &mqValue))
                            // TODO Check the char issues, KW mentioned !!
                            properties.set<short>(key, mqValue);
                            break;
                        }
                        case MQ_INT32_TYPE:
                        {
                            MQInt32 mqValue;
                            MQ_SAFE_CALL(MQGetInt32Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<int>(key, mqValue);
                            break;
                        }
                        case MQ_INT64_TYPE:
                        {
                            MQInt64 mqValue;
                            MQ_SAFE_CALL(MQGetInt64Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<long long>(key, mqValue);
                            break;
                        }
                        case MQ_FLOAT32_TYPE:
                        {
                            MQFloat32 mqValue;
                            MQ_SAFE_CALL(MQGetFloat32Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<float>(key, mqValue);
                            break;
                        }
                        case MQ_FLOAT64_TYPE:
                        {
                            MQFloat64 mqValue;
                            MQ_SAFE_CALL(MQGetFloat64Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<double>(key, mqValue);
                            break;
                        }
                        case MQ_BOOL_TYPE:
                        {
                            MQBool mqValue;
                            MQ_SAFE_CALL(MQGetBoolProperty(propertiesHandle, mqKey, &mqValue))
                            properties.set<bool>(key, mqValue);
                            break;
                        }
                        default:
                            KARABO_LOG_FRAMEWORK_WARN << "Ignoring header value '" << key << "' of unknown type '" << type << "'";
                            break;
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }
    }
}

