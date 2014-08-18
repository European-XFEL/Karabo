/*
 * $Id: JmsService.cc 3392 2011-04-28 12:49:18Z heisenb@DESY.DE $
 *
 * File:   JmsService.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "JmsBrokerConnection.hh"
#include "JmsBrokerChannel.hh"
#include "JmsBrokerIOService.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::io;
using namespace boost::signals2;

namespace karabo {
    namespace net {


        JmsBrokerChannel::JmsBrokerChannel(JmsBrokerConnection& connection) :
        BrokerChannel(connection), m_jmsConnection(connection), m_serializationType(connection.m_serializationType), m_filterCondition(""),
        m_isStopped(false), m_hasAsyncHandler(false), m_syncReadTimeout(100000) {

            // Get the type specific IO service
            m_ioService = m_jmsConnection.getIOService()->castTo<JmsBrokerIOService > ();

            // Transaction mode
            m_isTransacted = MQ_FALSE;
            if (m_jmsConnection.m_acknowledgeMode == MQ_SESSION_TRANSACTED) {
                m_isTransacted = MQ_TRUE;
            }

            MQ_SAFE_CALL(MQCreateSession(m_jmsConnection.m_connectionHandle, m_isTransacted, m_jmsConnection.m_acknowledgeMode, MQ_SESSION_SYNC_RECEIVE, &m_sessionHandle))
            MQ_SAFE_CALL(MQCreateDestination(m_sessionHandle, m_jmsConnection.m_destinationName.c_str(), m_jmsConnection.m_destinationType, &m_destinationHandle))
            
            // Create the serializers
            m_textSerializer = TextSerializer<Hash>::create("Xml", Hash("indentation", -1));
            m_binarySerializer = BinarySerializer<Hash>::create("Bin");
            
            m_producerHandle.handle =(MQInt32)0xFEEEFEEE;
        }


        JmsBrokerChannel::~JmsBrokerChannel() {
            // Close everything
            if (m_producerHandle.handle != (MQInt32)0xFEEEFEEE) {
                MQCloseMessageProducer(m_producerHandle);
            }
            MQFreeDestination(m_destinationHandle);
            MQCloseSession(m_sessionHandle);
        }


        void JmsBrokerChannel::setFilter(const std::string& filterCondition) {
            m_filterCondition = filterCondition;
        }


        const string& JmsBrokerChannel::getFilter() const {
            return m_filterCondition;
        }


        void JmsBrokerChannel::setTimeoutSyncRead(int milliseconds) {
            m_syncReadTimeout = milliseconds;
        }


        void JmsBrokerChannel::preRegisterSynchronousRead() {
            
            boost::mutex::scoped_lock lock(m_synchReadWriteMutex);
            
            MQ_SAFE_CALL(MQCreateMessageConsumer(m_sessionHandle, m_destinationHandle, m_filterCondition.c_str(), m_jmsConnection.m_deliveryInhibition, &m_syncConsumerHandle));
            m_hasSyncConsumer = true;
        }


        void JmsBrokerChannel::read(std::vector<char>& body, karabo::util::Hash& header) {
            string data;
            this->read(data, header);
            body.reserve(body.size() + data.size());
            std::copy(data.begin(), data.end(), back_inserter(body));
        }


        void JmsBrokerChannel::read(std::string& body, karabo::util::Hash& header) {
            
            boost::mutex::scoped_lock lock(m_synchReadWriteMutex);
            
            try {
                            
                MQConsumerHandle consumerHandle = MQ_INVALID_HANDLE;
                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQMessageType messageType;

                if (m_hasSyncConsumer) {
                    consumerHandle = m_syncConsumerHandle;
                } else {
                    MQ_SAFE_CALL(MQCreateMessageConsumer(m_sessionHandle, m_destinationHandle, m_filterCondition.c_str(), m_jmsConnection.m_deliveryInhibition, &consumerHandle));
                }
                MQStatus status = MQReceiveMessageWithTimeout(consumerHandle, m_syncReadTimeout, &messageHandle);
                if (MQStatusIsError(status)) throw KARABO_TIMEOUT_EXCEPTION("Synchronous read timed out");
                MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType))
                if (messageType == MQ_BYTES_MESSAGE) {
                    // Body
                    int nBytes;
                    const MQInt8* bytes;
                    MQGetBytesMessageBytes(messageHandle, &bytes, &nBytes);
                    body = string(reinterpret_cast<const char*> (bytes), nBytes);
                    MQPropertiesHandle propertiesHandle, headerHandle;
                    MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
                    MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                    getProperties(header, propertiesHandle);
                    getProperties(header, headerHandle);
                    MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                    MQ_SAFE_CALL(MQFreeProperties(headerHandle))
                } else if (messageType == MQ_TEXT_MESSAGE) {
                    ConstMQString msgBody;
                    MQ_SAFE_CALL(MQGetTextMessageText(messageHandle, &msgBody))
                    body = msgBody;
                    MQPropertiesHandle propertiesHandle, headerHandle;
                    MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
                    MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                    getProperties(header, propertiesHandle);
                    getProperties(header, headerHandle);
                    MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                    MQ_SAFE_CALL(MQFreeProperties(headerHandle))
                } else {
                    throw KARABO_MESSAGE_EXCEPTION("Received message which is neither text nor binary");
                }
                MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle))
                // Clean up
                MQ_SAFE_CALL(MQFreeMessage(messageHandle));
                MQ_SAFE_CALL(MQCloseMessageConsumer(consumerHandle))
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::read(karabo::util::Hash& body, karabo::util::Hash& header) {
            
            boost::mutex::scoped_lock lock(m_synchReadWriteMutex);
            
            try {

                MQConsumerHandle consumerHandle = MQ_INVALID_HANDLE;
                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQMessageType messageType;

                if (m_hasSyncConsumer) {
                    consumerHandle = m_syncConsumerHandle;
                } else {
                    MQ_SAFE_CALL(MQCreateMessageConsumer(m_sessionHandle, m_destinationHandle, m_filterCondition.c_str(), m_jmsConnection.m_deliveryInhibition, &consumerHandle));
                }
                MQStatus status = MQReceiveMessageWithTimeout(consumerHandle, m_syncReadTimeout, &messageHandle);
                if (MQStatusIsError(status)) throw KARABO_TIMEOUT_EXCEPTION("Synchronous read timed out");
                MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType))
                if (messageType == MQ_BYTES_MESSAGE) {
                    int nBytes;
                    const MQInt8* bytes;
                    MQGetBytesMessageBytes(messageHandle, &bytes, &nBytes);
                    m_binarySerializer->load(body, reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes));
                    MQPropertiesHandle propertiesHandle, headerHandle;
                    MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
                    MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                    getProperties(header, propertiesHandle);
                    getProperties(header, headerHandle);
                    MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                    MQ_SAFE_CALL(MQFreeProperties(headerHandle))
                } else if (messageType == MQ_TEXT_MESSAGE) {
                    ConstMQString msgBody;
                    MQ_SAFE_CALL(MQGetTextMessageText(messageHandle, &msgBody))
                    m_textSerializer->load(body, msgBody);
                    MQPropertiesHandle propertiesHandle, headerHandle;
                    MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
                    MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                    getProperties(header, propertiesHandle);
                    getProperties(header, headerHandle);
                    MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                    MQ_SAFE_CALL(MQFreeProperties(headerHandle))
                } else {
                    throw KARABO_MESSAGE_EXCEPTION("Received non-text message, but tried to read as text");
                }
                MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle))
                // Clean up
                MQ_SAFE_CALL(MQFreeMessage(messageHandle));
                MQ_SAFE_CALL(MQCloseMessageConsumer(consumerHandle))
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::getProperties(Hash& properties, const MQPropertiesHandle& propertiesHandle) const {
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
                            cout << "### WARNING: Ignoring header value \"" << key << "\" of unknown type" << endl;
                            break;
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::readAsyncStringHash(const ReadStringHashHandler& readHandler) {

            if (m_hasAsyncHandler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("You may only register exactly one handler per channel, "
                                                     "if you need more handlers create a new channel on the connection and register there");
            }
            m_hasAsyncHandler = true;

            // Save the callback function
            m_readStringHashHandler = readHandler;

            // Set up a consumer
            MQ_SAFE_CALL(MQCreateMessageConsumer(m_sessionHandle, m_destinationHandle, m_filterCondition.c_str(), m_jmsConnection.m_deliveryInhibition, &m_asyncConsumerHandle));

            // Start listening for messages by starting an individual thread
            m_ioService->registerTextMessageChannel(this);
        }


        void JmsBrokerChannel::listenForTextMessages() {

            try {

                bool messageReceived = false;
                do {
                    messageReceived = signalIncomingTextMessage();
                } while (!m_isStopped && ((!messageReceived && m_ioService->isRunning()) || m_ioService->isWorking()));

            } catch (const Exception& e) {
                cout << e;
                Exception::memorize();
            } catch (...) {
                Exception::memorize();
            }
        }


        bool JmsBrokerChannel::signalIncomingTextMessage() {
            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQMessageType messageType;

                MQStatus status = MQReceiveMessageWithTimeout(m_asyncConsumerHandle, 2000, &messageHandle);
                if (!MQStatusIsError(status) && !m_isStopped) {
                    //MQ_SAFE_CALL(MQReceiveMessageWait(consumerHandle, &messageHandle))
                    MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType))
                    if (messageType == MQ_TEXT_MESSAGE) {
                        ConstMQString msgBody;
                        MQ_SAFE_CALL(MQGetTextMessageText(messageHandle, &msgBody))
                        Hash header;
                        MQPropertiesHandle propertiesHandle, headerHandle;
                        MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
                        MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                        getProperties(header, propertiesHandle);
                        getProperties(header, headerHandle);
                        MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                        MQ_SAFE_CALL(MQFreeProperties(headerHandle))
                        MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle))
                        if (!m_ioService->isWorking()) {
                            MQ_SAFE_CALL(MQCloseMessageConsumer(m_asyncConsumerHandle));
                            m_hasAsyncHandler = false;
                        }
                        m_readStringHashHandler(shared_from_this(), string(msgBody), header);
                        MQ_SAFE_CALL(MQFreeMessage(messageHandle));
                    } else if (messageType == MQ_BYTES_MESSAGE) {
                        // Just ignore binary messages
                    } else {
                        // Give an error if unexpected message types are going round the broker
                        throw KARABO_MESSAGE_EXCEPTION("Received message of unsupported type (i.e. neither <text> nor <bytes>)");
                    }
                    return true;
                }
            } catch (const Exception& e) {
                m_signalError(shared_from_this(), e.userFriendlyMsg());
            } catch (...) {
                m_signalError(shared_from_this(), "Unknown exception was raised whilst reading asynchronously");
            }
            return false;
        }


        void JmsBrokerChannel::readAsyncRawHash(const ReadRawHashHandler& readHandler) {
            if (m_hasAsyncHandler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("You may only register exactly one handler per channel, "
                                                     "if you need more handlers create a new channel on the connection and register there");
            }
            m_hasAsyncHandler = true;

            // Save the callback function
            m_readRawHashHandler = readHandler;

            // Set up a consumer
            MQ_SAFE_CALL(MQCreateMessageConsumer(m_sessionHandle, m_destinationHandle, m_filterCondition.c_str(), m_jmsConnection.m_deliveryInhibition, &m_asyncConsumerHandle));

            // Start listening for messages by starting an individual thread
            m_ioService->registerBinaryMessageChannel(this);
        }


        void JmsBrokerChannel::readAsyncHashHash(const ReadHashHashHandler& handler) {
            m_readHashHashHandler = handler;
            readAsyncRawHash(boost::bind(&karabo::net::JmsBrokerChannel::rawHash2HashHash, this, _1, _2, _3, _4));
        }


        void JmsBrokerChannel::listenForBinaryMessages() {

            try {

                bool messageReceived = false;
                do {
                    messageReceived = signalIncomingBinaryMessage();
                } while (!m_isStopped && ((!messageReceived && m_ioService->isRunning()) || m_ioService->isWorking()));

            } catch (const Exception& e) {
                cout << e;
                Exception::memorize();
            } catch (...) {
                Exception::memorize();
            }
        }


        bool JmsBrokerChannel::signalIncomingBinaryMessage() {
            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQMessageType messageType;

                MQStatus status = MQReceiveMessageWithTimeout(m_asyncConsumerHandle, 2000, &messageHandle);
                if (!MQStatusIsError(status) && !m_isStopped) {
                    MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType))
                    if (messageType == MQ_BYTES_MESSAGE) {
                        // Body
                        int nBytes;
                        const MQInt8* bytes;
                        MQGetBytesMessageBytes(messageHandle, &bytes, &nBytes);
                        // Header
                        Hash header;
                        MQPropertiesHandle propertiesHandle, headerHandle;
                        MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
                        MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                        getProperties(header, propertiesHandle);
                        getProperties(header, headerHandle);
                        MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                        MQ_SAFE_CALL(MQFreeProperties(headerHandle))
                        // Thank you, broker!
                        MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle));
                        // Free consumer if not needed anymore
                        if (!m_ioService->isWorking()) {
                            MQ_SAFE_CALL(MQCloseMessageConsumer(m_asyncConsumerHandle));
                            m_hasAsyncHandler = false;
                        }
                        // TODO Check, whether to free the propertiesHandle
                        m_readRawHashHandler(shared_from_this(), reinterpret_cast<const char*> (bytes), (size_t) nBytes, header);
                        MQ_SAFE_CALL(MQFreeMessage(messageHandle));
                    } else if (messageType == MQ_TEXT_MESSAGE) {
                        ConstMQString msgBody;
                        MQ_SAFE_CALL(MQGetTextMessageText(messageHandle, &msgBody))
                        Hash header;
                        MQPropertiesHandle propertiesHandle, headerHandle;
                        MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
                        MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                        getProperties(header, propertiesHandle);
                        getProperties(header, headerHandle);
                        MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                        MQ_SAFE_CALL(MQFreeProperties(headerHandle))
                        MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle));
                        if (!m_ioService->isWorking()) {
                            MQ_SAFE_CALL(MQCloseMessageConsumer(m_asyncConsumerHandle));
                            m_hasAsyncHandler = false;
                        }
                        // TODO Check, whether to free the propertiesHandle
                        m_readRawHashHandler(shared_from_this(), msgBody, strlen(msgBody), header);
                        MQ_SAFE_CALL(MQFreeMessage(messageHandle));
                    } else {
                        // Give an error if unexpected message types are going round the broker
                        throw KARABO_MESSAGE_EXCEPTION("Received message of unsupported type (i.e. neither <text> nor <bytes>)");
                    }
                    return true;
                }
            } catch (const Exception& e) {
                m_signalError(shared_from_this(), e.userFriendlyMsg());
            } catch (...) {
                m_signalError(shared_from_this(), "Unknown exception was raised whilst reading asynchronously");
            }
            return false;
        }


        void JmsBrokerChannel::write(const std::string& messageBody, const Hash& header) {
            
            try {
                
                boost::mutex::scoped_lock lock(m_synchReadWriteMutex);

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;

                if (m_producerHandle.handle == (MQInt32)0xFEEEFEEE) {
                    MQ_SAFE_CALL(MQCreateMessageProducerForDestination(m_sessionHandle, m_destinationHandle, &m_producerHandle));
                }
                            
                MQ_SAFE_CALL(MQCreateTextMessage(&messageHandle))
                MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle))

                // Add some default properties
                Hash properties(header);
                //properties.set<long long>("__timestamp", karabo::util::Time::getMsSinceEpoch());
                setProperties(properties, propertiesHandle);

                MQ_SAFE_CALL(MQSetMessageProperties(messageHandle, propertiesHandle))

                // TODO Care about the proper freeing of propertiesHandle

                MQ_SAFE_CALL(MQSetTextMessageText(messageHandle, messageBody.c_str()))

                //cout << " Sending message: " << endl << messageBody << endl;

                MQ_SAFE_CALL(MQSendMessageExt(m_producerHandle, messageHandle, MQ_NON_PERSISTENT_DELIVERY, 4, m_jmsConnection.m_messageTimeToLive))

                // Clean up
                //MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                MQ_SAFE_CALL(MQFreeMessage(messageHandle))
                

            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::write(const char* messageBody, const size_t& size, const Hash& header) {

            try {
                
                boost::mutex::scoped_lock lock(m_synchReadWriteMutex);        

                //MQProducerHandle producerHandle = MQ_INVALID_HANDLE;
                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;

                if (m_producerHandle.handle == (MQInt32)0xFEEEFEEE) {
                    MQ_SAFE_CALL(MQCreateMessageProducerForDestination(m_sessionHandle, m_destinationHandle, &m_producerHandle));
                }                
                
                MQ_SAFE_CALL(MQCreateBytesMessage(&messageHandle))
                MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle))

                // Add some default properties
                Hash properties(header);
                //properties.set<long long>("__timestamp", karabo::util::Time::getMsSinceEpoch());
                setProperties(properties, propertiesHandle);

                MQ_SAFE_CALL(MQSetMessageProperties(messageHandle, propertiesHandle))
                        // TODO Care about the proper freeing of propertiesHandle

                if (size > 0) {
                    MQ_SAFE_CALL(MQSetBytesMessageBytes(messageHandle, reinterpret_cast<MQInt8*> (const_cast<char*> (messageBody)), size));
                }

                MQ_SAFE_CALL(MQSendMessageExt(m_producerHandle, messageHandle, MQ_NON_PERSISTENT_DELIVERY, 4, m_jmsConnection.m_messageTimeToLive))

                // Clean up
                //MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                MQ_SAFE_CALL(MQFreeMessage(messageHandle))

            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::write(const karabo::util::Hash& data, const karabo::util::Hash& header) {

            Hash modifiedHeader(header);
            if (m_serializationType == "text") {
                modifiedHeader.set("__format", "Xml");
                string buffer;
                m_textSerializer->save(data, buffer);
                this->write(buffer, modifiedHeader);
            } else if (m_serializationType == "binary") {
                modifiedHeader.set("__format", "Bin");
                std::vector<char> buffer;
                m_binarySerializer->save(data, buffer);
                this->write(&buffer[0], buffer.size(), modifiedHeader);
            }
        }


        void JmsBrokerChannel::setProperties(const Hash& properties, const MQPropertiesHandle& propertiesHandle) {
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


        void JmsBrokerChannel::setErrorHandler(const BrokerErrorHandler& handler) {
            m_signalError.connect(handler);
        }


        void JmsBrokerChannel::waitAsync(int milliseconds, const WaitHandler& handler) {
            m_ioService->registerWaitChannel(this, handler, milliseconds);
        }


        void JmsBrokerChannel::deadlineTimer(const WaitHandler& handler, int milliseconds) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(milliseconds));
            handler(shared_from_this());
        }


        void JmsBrokerChannel::stop() {
            m_isStopped = true;
        }


        void JmsBrokerChannel::close() {
            stop();
            unregisterChannel(shared_from_this());
        }


        void JmsBrokerChannel::rawHash2HashHash(BrokerChannel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash& header) {
            Hash h;
            if (header.has("__format")) {
                std::string format = header.get<string>("__format");
                if (format == "Xml") {
                    try {
                        m_textSerializer->load(h, data);
                    } catch (const Exception& e) {
                        throw KARABO_MESSAGE_EXCEPTION("Could not de-serialize text message into Hash");
                    }
                } else if (format == "Bin") {
                    try {
                        m_binarySerializer->load(h, data, size);
                    } catch (const Exception& e) {
                        throw KARABO_MESSAGE_EXCEPTION("Could not de-serialize binary message into Hash");
                    }
                } else {
                    throw KARABO_MESSAGE_EXCEPTION("Encountered message with unknown format: \"" + format + "\"");
                }
            } else {
                throw KARABO_MESSAGE_EXCEPTION("De-serialization of message without __format tag is not possible");
            }
            m_readHashHashHandler(channel, h, header);
        }
    }
}
