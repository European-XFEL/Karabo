/*
 * $Id$
 *
 * File:   JmsConsumer.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 18, 2016, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */
#include "JmsConsumer.hh"

#include <openmqc/mqcrt.h>

#include <karabo/io/HashBinarySerializer.hh>
#include <karabo/log.hpp>
#include <memory>

#include "EventLoop.hh"
#include "JmsConnection.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace net {


        JmsConsumer::JmsConsumer(const JmsConnection::Pointer& connection, const std::string& topic,
                                 const std::string& selector, bool skipSerialisation)
            : m_connection(connection),
              m_binarySerializer(skipSerialisation ? nullptr : BinarySerializer<Hash>::create("Bin")),
              m_reading(false),
              m_serializerStrand(boost::make_shared<Strand>(karabo::net::EventLoop::getIOService())),
              m_handlerStrand(boost::make_shared<Strand>(karabo::net::EventLoop::getIOService())),
              m_topic(topic),
              m_selector(selector) {}


        JmsConsumer::~JmsConsumer() {
            this->stopReading();
            this->clearConsumerHandles();
        }


        void JmsConsumer::startReading(const consumer::MessageHandler& handler,
                                       const consumer::ErrorNotifier& errorNotifier) {
            if (m_reading) {
                KARABO_LOG_FRAMEWORK_ERROR << "Refuse 'startReading' since already reading!";
                return;
            }
            m_reading = true;
            // Set handlers on strand that uses them to circumvent mutex locks.
            // Do same "strand hops" that messages (and therefore handler resetting) do to guarantee that this handler
            // setting here cannot overtake a resetting triggered by a previously called stopReading:
            m_serializerStrand->post(
                  bind_weak(&JmsConsumer::postSetHandlers, this, m_handlerStrand, handler, errorNotifier));

            // If startReading is scheduled before the event-loop is started, corresponding writes
            // (that are also only scheduled) may be executed first once the event-loop is started.
            // Registering the consumers to the broker BEFORE the event-loop runs protects from message loss.
            this->ensureConsumerSessionAvailable(m_topic, m_selector);
            this->getConsumer(m_topic, m_selector);

            // Cannot just post with bind_weak to the event loop:
            // Then the destructor could never be called since bind_weak holds a shared_ptr when executing the function.
            // That's why we end up with an ugly individual thread... :-(.
            // For the same reason we have to boost::bind to a bare this. To ensure a proper lifetime of 'this':
            // - we hand over a shared_ptr to boost::bind as argument for consumeMessages,
            // - the returned function object carries a copy (!) of it,
            // - the copy is then forwarded to consumeMessages by reference,
            // - so consumeMessages has control to check whether there is any external shared_ptr alive.
            m_readThread = boost::thread(boost::bind(&JmsConsumer::consumeMessages, this, shared_from_this()));
        }


        void JmsConsumer::stopReading() {
            if (m_reading) {
                m_reading = false;
                if (m_readThread.get_id() != boost::this_thread::get_id()) {
                    // Hangs if stuck in ensureConsumerSessionAvailable(..) or getConsumer(..)
                    // via m_connection->waitForConnectionAvailable().
                    // But very unlikely... Or better play with thread interruptions as in DataLogger in version 1.4.7?
                    m_readThread.join();
                }
                // Else prevent exception about "boost thread: trying joining itself: Resource deadlock avoided"
                // that can happen when stopReading is called in destructor. Since consumeMessages holds a
                // 'self'-guard shared_ptr, this is even the normal scenario.
                // But do not use Karabo logging in this else case:
                // The extra thread might call this when the logger singleton is already being cleaned-up.
            }
        }


        void JmsConsumer::postSetHandlers(const Strand::Pointer& strand, const consumer::MessageHandler& handler,
                                          const consumer::ErrorNotifier& errorNotifier) {
            strand->post(bind_weak(&JmsConsumer::setHandlers, this, handler, errorNotifier));
        }


        void JmsConsumer::setHandlers(const consumer::MessageHandler& handler,
                                      const consumer::ErrorNotifier& errorNotifier) {
            m_messageHandler = handler;
            m_errorNotifier = errorNotifier;
        }

        void JmsConsumer::consumeMessages(JmsConsumer::Pointer& selfGuard) { // Sic! Non-const reference!
            // We go for an endless loop instead of reposting to the event loop after a single message has been read.
            // In this way we are safe against deadlocks blocking all threads in the event loop. Note that not being
            // able to read (and thus acknowledge!) would create a "black hole" that compromises the whole system!
            while (true) {
                MQSessionHandle sessionHandle = this->ensureConsumerSessionAvailable(m_topic, m_selector);
                MQConsumerHandle consumerHandle = this->getConsumer(m_topic, m_selector);

                JmsConsumer::MQMessageHandlePointer messageHandlePtr(new MQMessageHandle, [](MQMessageHandle* r) {
                    MQFreeMessage(*r);
                    delete r;
                });
                MQStatus status = MQReceiveMessageWithTimeout(consumerHandle, 100, messageHandlePtr.get());
                MQError statusCode = MQGetStatusCode(status);
                switch (statusCode) {
                    case MQ_CONSUMER_DROPPED_MESSAGES: { // Deal with hand-crafted error code
                        MQString statusString = MQGetStatusString(status);
                        const std::string stdStatusString(statusString);
                        MQFreeString(statusString);
                        m_serializerStrand->post(bind_weak(&JmsConsumer::postErrorOnHandlerStrand, this,
                                                           consumer::Error::drop, stdStatusString));
                        // MQ_CONSUMER_DROPPED_MESSAGES is an error code introduced by an XFEL modification
                        // to OpenMQ. It means: Here is a new message, but be aware that other messages
                        // received before have been dropped. These dropped messages have already been acknowledged,
                        // but the current one not, so after informing about the error we should proceed with
                        // the normal message processing logic. That's the reason for the intentional 'no break'
                        // behavior of this case.
                    }
                    case MQ_SUCCESS: { // Message received
                        MQ_SAFE_CALL(MQAcknowledgeMessages(sessionHandle, *messageHandlePtr));

                        MQMessageType messageType;
                        MQ_SAFE_CALL(MQGetMessageType(*messageHandlePtr, &messageType));

                        // Wrong message type -> notify error, but ignore this message
                        if (messageType != MQ_BYTES_MESSAGE) {
                            const std::string msg("Received a message of wrong type");
                            KARABO_LOG_FRAMEWORK_WARN << msg;
                            m_serializerStrand->post(
                                  bind_weak(&JmsConsumer::postErrorOnHandlerStrand, this, consumer::Error::type, msg));
                            break;
                        }
                        m_serializerStrand->post(bind_weak(&JmsConsumer::deserialize, this, messageHandlePtr));
                        break;
                    }

                    case MQ_TIMEOUT_EXPIRED:
                        // No message received, just try again
                        break;
                    case MQ_STATUS_INVALID_HANDLE:
                    case MQ_BROKER_CONNECTION_CLOSED:
                    case MQ_SESSION_CLOSED:
                    case MQ_CONSUMER_CLOSED:
                        // Invalidate handles and re-post, i.e. go on once broker etc. are back.
                        // This function may be called concurrently, hence its thread-safe
                        this->clearConsumerHandles();
                        break;
                    default: {
                        MQString tmp = MQGetStatusString(status);
                        const std::string errorString(tmp);
                        MQFreeString(tmp);
                        const std::string msg("Untreated message consumption error '" + errorString + "', try again.");
                        KARABO_LOG_FRAMEWORK_WARN << msg;
                        m_serializerStrand->post(
                              bind_weak(&JmsConsumer::postErrorOnHandlerStrand, this, consumer::Error::unknown, msg));
                    }
                }
                if (selfGuard.unique()) {
                    // The copy with that the boost::bind-ed 'functor' called consumeMessages is the only one left,
                    // so let's trigger the destruction of ourselves and leave the while loop.
                    selfGuard.reset();
                    return;
                }
                if (!m_reading) {
                    // We are done and reset handlers to avoid them dangling.
                    // Reseting takes the same "route" as messages, i.e. via serialiser strand to the handler strand
                    // where the latter is the only one allowed to touch the cached handlers. This duplicated hop
                    // guarantees that no message can get lost because it is processed in the handler strand when the
                    // handler is already an empty function pointer.
                    m_serializerStrand->post(bind_weak(&JmsConsumer::postSetHandlers, this, m_handlerStrand,
                                                       consumer::MessageHandler(), consumer::ErrorNotifier()));
                    break;
                }
            }
        }


        void JmsConsumer::deserialize(const JmsConsumer::MQMessageHandlePointer& messageHandlePtr) {
            Hash::Pointer header(boost::make_shared<Hash>());
            this->parseHeader(*messageHandlePtr, *header);

            int nBytes;
            const MQInt8* bytes;
            MQ_SAFE_CALL(MQGetBytesMessageBytes(*messageHandlePtr, &bytes, &nBytes));

            Hash::Pointer body(boost::make_shared<Hash>());
            const char* constChars = reinterpret_cast<const char*>(bytes);
            if (m_binarySerializer) {
                m_binarySerializer->load(*body, constChars, static_cast<size_t>(nBytes));
            } else {
                // Just copy raw bytes as vector<char> under key "raw":
                std::vector<char>& raw = body->bindReference<std::vector<char> >("raw");
                raw.assign(constChars, constChars + nBytes);
            }
            m_handlerStrand->post(boost::bind(m_messageHandler, header, body));
        }


        void JmsConsumer::postErrorOnHandlerStrand(consumer::Error error, const std::string& msg) {
            m_handlerStrand->post(bind_weak(&JmsConsumer::notifyError, this, error, msg));
        }


        void JmsConsumer::notifyError(consumer::Error error, const std::string& msg) {
            if (m_errorNotifier) {
                m_errorNotifier(error, msg);
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Error " << static_cast<int>(error) << ": " << msg;
            }
        }

        MQConsumerHandle JmsConsumer::getConsumer(const std::string& topic, const std::string& selector) {
            Consumers::const_iterator it = m_consumers.find(topic + selector);
            if (it != m_consumers.end()) return it->second;

            m_connection->waitForConnectionAvailable();

            std::pair<MQSessionHandle, MQDestinationHandle> handles =
                  this->ensureConsumerDestinationAvailable(topic, selector);
            MQConsumerHandle consumerHandle;

            MQ_SAFE_CALL(MQCreateMessageConsumer(handles.first, handles.second, selector.c_str(), MQ_FALSE /*noLocal*/,
                                                 &consumerHandle));
            m_consumers[topic + selector] = consumerHandle;

            return consumerHandle;
        }


        std::pair<MQSessionHandle, MQDestinationHandle> JmsConsumer::ensureConsumerDestinationAvailable(
              const std::string& topic, const std::string& selector) {
            ConsumerDestinations::const_iterator it = m_consumerDestinations.find(topic);
            if (it != m_consumerDestinations.end()) return it->second;

            m_connection->waitForConnectionAvailable();

            MQSessionHandle sessionHandle = this->ensureConsumerSessionAvailable(topic, selector);
            MQDestinationHandle destinationHandle;

            MQ_SAFE_CALL(MQCreateDestination(sessionHandle, topic.c_str(), MQ_TOPIC_DESTINATION, &destinationHandle))
            m_consumerDestinations[topic] = std::make_pair(sessionHandle, destinationHandle);

            return std::make_pair(sessionHandle, destinationHandle);
        }


        MQSessionHandle JmsConsumer::ensureConsumerSessionAvailable(const std::string& topic,
                                                                    const std::string& selector) {
            ConsumerSessions::const_iterator it = m_consumerSessions.find(topic + selector);
            if (it != m_consumerSessions.end()) return it->second;

            m_connection->waitForConnectionAvailable();

            MQSessionHandle consumerSessionHandle;
            MQ_SAFE_CALL(MQCreateSession(m_connection->m_connectionHandle, MQ_FALSE, /* isTransacted */
                                         MQ_CLIENT_ACKNOWLEDGE, MQ_SESSION_SYNC_RECEIVE, &consumerSessionHandle));
            m_consumerSessions[topic + selector] = consumerSessionHandle;
            return consumerSessionHandle;
        }


        void JmsConsumer::clearConsumerHandles() {
            for (const Consumers::value_type& i : m_consumers) {
                MQCloseMessageConsumer(i.second);
            }
            m_consumers.clear();


            for (const ConsumerDestinations::value_type& i : m_consumerDestinations) {
                MQFreeDestination(i.second.second);
            }
            m_consumerDestinations.clear();


            for (const ConsumerSessions::value_type& i : m_consumerSessions) {
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
                    std::string key(mqKey);
                    MQType type;
                    MQ_SAFE_CALL(MQGetPropertyType(propertiesHandle, mqKey, &type))
                    switch (type) {
                        case MQ_STRING_TYPE: {
                            ConstMQString mqValue;
                            MQ_SAFE_CALL(MQGetStringProperty(propertiesHandle, mqKey, &mqValue))
                            properties.set<std::string>(key, std::string(mqValue));
                            break;
                        }
                        case MQ_INT8_TYPE: {
                            MQInt8 mqValue;
                            MQ_SAFE_CALL(MQGetInt8Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<signed char>(key, mqValue);
                            break;
                        }
                        case MQ_INT16_TYPE: {
                            MQInt16 mqValue;
                            MQ_SAFE_CALL(MQGetInt16Property(propertiesHandle, mqKey, &mqValue))
                            // TODO Check the char issues, KW mentioned !!
                            properties.set<short>(key, mqValue);
                            break;
                        }
                        case MQ_INT32_TYPE: {
                            MQInt32 mqValue;
                            MQ_SAFE_CALL(MQGetInt32Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<int>(key, mqValue);
                            break;
                        }
                        case MQ_INT64_TYPE: {
                            MQInt64 mqValue;
                            MQ_SAFE_CALL(MQGetInt64Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<long long>(key, mqValue);
                            break;
                        }
                        case MQ_FLOAT32_TYPE: {
                            MQFloat32 mqValue;
                            MQ_SAFE_CALL(MQGetFloat32Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<float>(key, mqValue);
                            break;
                        }
                        case MQ_FLOAT64_TYPE: {
                            MQFloat64 mqValue;
                            MQ_SAFE_CALL(MQGetFloat64Property(propertiesHandle, mqKey, &mqValue))
                            properties.set<double>(key, mqValue);
                            break;
                        }
                        case MQ_BOOL_TYPE: {
                            MQBool mqValue;
                            MQ_SAFE_CALL(MQGetBoolProperty(propertiesHandle, mqKey, &mqValue))
                            properties.set<bool>(key, mqValue);
                            break;
                        }
                        default:
                            KARABO_LOG_FRAMEWORK_WARN << "Ignoring header value '" << key << "' of unknown type '"
                                                      << type << "'";
                            break;
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsConsumer::setTopic(const std::string& topic) {
            m_topic = topic;
        }


        void JmsConsumer::setSelector(const std::string& selector) {
            m_selector = selector;
        }

    } // namespace net
} // namespace karabo
