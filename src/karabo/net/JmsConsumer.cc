/*
 * $Id$
 *
 * File:   JmsConsumer.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 18, 2016, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/io/HashBinarySerializer.hh>
#include <karabo/log.hpp>
#include "JmsConsumer.hh"
#include "JmsConnection.hh"
#include "EventLoop.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace net {


        JmsConsumer::JmsConsumer(const JmsConnection::Pointer& connection) :
            m_connection(connection),
            m_strand(*EventLoop::getIOService()) {
            m_binarySerializer = BinarySerializer<Hash>::create("Bin");
            EventLoop::addThread();
        }


        JmsConsumer::~JmsConsumer() {
            this->clearConsumerHandles();
            EventLoop::removeThread();
        }


        void JmsConsumer::readAsync(const MessageHandler handler, const std::string& topic, const std::string& selector) {

            m_connection->waitForConnectionAvailable();
            // This calls may happen concurrently, hence both functions are thread-safe
            this->ensureConsumerSessionAvailable(topic, selector);
            this->getConsumer(topic, selector);

            // Posting through strand guarantees thread-safety, never will the posted message run concurrently
            m_strand.post(boost::bind(&karabo::net::JmsConsumer::asyncConsumeMessage, this, handler, topic, selector));
        }


        void JmsConsumer::asyncConsumeMessage(const MessageHandler handler, const std::string& topic, const std::string& selector) {

            m_connection->waitForConnectionAvailable();

            // This calls may happen concurrently, hence both functions are thread-safe
            MQSessionHandle sessionHandle = this->ensureConsumerSessionAvailable(topic, selector);
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
                        m_strand.post(boost::bind(&karabo::net::JmsConsumer::asyncConsumeMessage, this,
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
                    EventLoop::getIOService()->post(boost::bind(handler, header, body));
                    break;
                }

                case MQ_TIMEOUT_EXPIRED:
                { // No message received, post again
                    m_strand.post(boost::bind(&karabo::net::JmsConsumer::asyncConsumeMessage, this,
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
                    m_strand.post(boost::bind(&karabo::net::JmsConsumer::asyncConsumeMessage, this,
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


        MQConsumerHandle JmsConsumer::getConsumer(const std::string& topic, const std::string& selector) {

            Consumers::const_iterator it = m_consumers.find(topic + selector);
            if (it != m_consumers.end()) return it->second;

            std::pair<MQSessionHandle, MQDestinationHandle> handles = this->ensureConsumerDestinationAvailable(topic, selector);
            MQConsumerHandle consumerHandle;

            MQ_SAFE_CALL(MQCreateMessageConsumer(handles.first, handles.second, selector.c_str(), MQ_FALSE /*noLocal*/, &consumerHandle));
            m_consumers[topic + selector] = consumerHandle;

            return consumerHandle;
        }


        std::pair<MQSessionHandle, MQDestinationHandle> JmsConsumer::ensureConsumerDestinationAvailable(const std::string& topic, const std::string& selector) {

            ConsumerDestinations::const_iterator it = m_consumerDestinations.find(topic);
            if (it != m_consumerDestinations.end()) return it->second;

            MQSessionHandle sessionHandle = this->ensureConsumerSessionAvailable(topic, selector);
            MQDestinationHandle destinationHandle;

            MQ_SAFE_CALL(MQCreateDestination(sessionHandle, topic.c_str(), MQ_TOPIC_DESTINATION, &destinationHandle))
            m_consumerDestinations[topic] = make_pair(sessionHandle, destinationHandle);

            return make_pair(sessionHandle, destinationHandle);
        }


        MQSessionHandle JmsConsumer::ensureConsumerSessionAvailable(const std::string& topic, const std::string& selector) {

            ConsumerSessions::const_iterator it = m_consumerSessions.find(topic + selector);
            if (it != m_consumerSessions.end()) return it->second;

            MQSessionHandle consumerSessionHandle;
            MQ_SAFE_CALL(MQCreateSession(m_connection->m_connectionHandle,
                                         MQ_FALSE, /* isTransacted */
                                         MQ_CLIENT_ACKNOWLEDGE,
                                         MQ_SESSION_SYNC_RECEIVE,
                                         &consumerSessionHandle));
            m_consumerSessions[topic + selector] = consumerSessionHandle;
            return consumerSessionHandle;
        }


        void JmsConsumer::clearConsumerHandles() {


            BOOST_FOREACH(const Consumers::value_type& i, m_consumers) {
                MQCloseMessageConsumer(i.second);
            }
            m_consumers.clear();


            BOOST_FOREACH(const ConsumerDestinations::value_type& i, m_consumerDestinations) {
                MQFreeDestination(i.second.second);
            }
            m_consumerDestinations.clear();


            BOOST_FOREACH(const ConsumerSessions::value_type& i, m_consumerSessions) {
                MQCloseSession(i.second);
            }
            m_consumerSessions.clear();
        }


        void JmsConsumer::parseHeader(const MQMessageHandle& messageHandle, karabo::util::Hash& header) {
            MQPropertiesHandle propertiesHandle, headerHandle;
            MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
            MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                    this->getProperties(header, propertiesHandle);
            this->getProperties(header, headerHandle);
            MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
            MQ_SAFE_CALL(MQFreeProperties(headerHandle))
        }


        void JmsConsumer::getProperties(Hash& properties, const MQPropertiesHandle& propertiesHandle) const {
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