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

#include <karabo/util/Time.hh>

#include "AJmsConnection.hh"
#include "JmsChannel.hh"
#include "JmsIOService.hh"

using namespace std;
using namespace karabo::util;
using namespace boost::signals2;

namespace karabo {
    namespace net {


        JmsChannel::JmsChannel(AJmsConnection& connection) : Channel(connection), m_jmsConnection(connection), m_filterCondition(""),
            m_isStopped(false), m_hasAsyncHandler(false), m_syncReadTimeout(100000) {
            m_ioService = m_jmsConnection.getIOService()->castTo<JmsIOService > ();
            m_isTransacted = MQ_FALSE;
            if (m_jmsConnection.m_acknowledgeMode == MQ_SESSION_TRANSACTED) {
                m_isTransacted = MQ_TRUE;
            }

            MQ_SAFE_CALL(MQCreateSession(m_jmsConnection.m_connectionHandle, m_isTransacted, m_jmsConnection.m_acknowledgeMode, MQ_SESSION_SYNC_RECEIVE, &m_sessionHandle))
            MQ_SAFE_CALL(MQCreateDestination(m_sessionHandle, m_jmsConnection.m_destinationName.c_str(), m_jmsConnection.m_destinationType, &m_destinationHandle))
        }


        JmsChannel::~JmsChannel() {
            // Close everything
            MQFreeDestination(m_destinationHandle);
            MQCloseSession(m_sessionHandle);
        }


        void JmsChannel::setFilter(const std::string& filterCondition) {
            m_filterCondition = filterCondition;
        }


        string JmsChannel::getFilter() const {
            return m_filterCondition;
        }


        void JmsChannel::setTimeoutSyncRead(int milliseconds) {
            m_syncReadTimeout = milliseconds;
        }


        void JmsChannel::read(string& data, Hash& header) {
            try {

                MQConsumerHandle consumerHandle = MQ_INVALID_HANDLE;
                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQMessageType messageType;

                MQ_SAFE_CALL(MQCreateMessageConsumer(m_sessionHandle, m_destinationHandle, m_filterCondition.c_str(), m_jmsConnection.m_deliveryInhibition, &consumerHandle));
                MQStatus status = MQReceiveMessageWithTimeout(consumerHandle, m_syncReadTimeout, &messageHandle);
                if (MQStatusIsError(status)) throw TIMEOUT_EXCEPTION("Synchronous read timed out");
                MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType))
                if (messageType == MQ_TEXT_MESSAGE) {
                    ConstMQString msgBody;
                    MQ_SAFE_CALL(MQGetTextMessageText(messageHandle, &msgBody))
                    data = msgBody;
                    MQPropertiesHandle propertiesHandle, headerHandle;
                    MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
                    MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                    getProperties(header, propertiesHandle);
                    getProperties(header, headerHandle);
                    MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                    MQ_SAFE_CALL(MQFreeProperties(headerHandle))
                            // TODO Check, wheter to free the propertiesHandle
                } else if (messageType == MQ_BYTES_MESSAGE) {
                    // Body
                    int nBytes;
                    const MQInt8* bytes;
                    MQGetBytesMessageBytes(messageHandle, &bytes, &nBytes);
                    data = string(reinterpret_cast<const char*> (bytes), nBytes);
                    MQPropertiesHandle propertiesHandle, headerHandle;
                    MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
                    MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
                    getProperties(header, propertiesHandle);
                    getProperties(header, headerHandle);
                    MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                    MQ_SAFE_CALL(MQFreeProperties(headerHandle))
                } else {
                    throw MESSAGE_EXCEPTION("Recieved non-text message, but tried to read as text");
                }
                MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle))
                // Clean up
                MQ_SAFE_CALL(MQFreeMessage(messageHandle));
                MQ_SAFE_CALL(MQCloseMessageConsumer(consumerHandle))
            } catch (...) {
                RETHROW
            }
        }


        void JmsChannel::read(karabo::util::Hash& body, karabo::util::Hash& header) {
            std::string s;
            try {
                this->read(s, header);
            } catch (...) {
                RETHROW
            }
            stringstream ss(s);
            if (m_jmsConnection.m_autoDetectMessageFormat) {
                string format("Xml"); // Default if no format is given
                header.tryToGet("__format", format);
                HashFormatsConstIt it = m_hashFormats.find(format);
                if (it == m_hashFormats.end()) {
                    try {
                        m_hashFormats[format] = HashFormat::create(Hash(format));
                    } catch (const Exception& e) {
                        throw MESSAGE_EXCEPTION("Could not understand/automatically create a Hash from the given \"" + format + "\" format");
                    }
                }
                m_hashFormats[format]->convert(ss, body);
            } else {
                stringToHash(s, body);
            }
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
                            cout << "### WARNING: Ignoring header value \"" << key << "\" of unknown type" << endl;
                            break;
                    }
                }
            } catch (...) {
                RETHROW
            }
        }


        void JmsChannel::readAsyncStringHash(const ReadStringHashHandler& readHandler) {

            if (m_hasAsyncHandler) {
                throw NOT_SUPPORTED_EXCEPTION("You may only register exactly one handler per channel, "
                                              "if you need more handlers create a new channel on the connection and register there");
            }
            m_hasAsyncHandler = true;

            // Save the callback function
            m_readStringHashHandler = readHandler;

            // Set up a consumer
            MQ_SAFE_CALL(MQCreateMessageConsumer(m_sessionHandle, m_destinationHandle, m_filterCondition.c_str(), m_jmsConnection.m_deliveryInhibition, &m_consumerHandle));

            // Start listening for messages by starting an individual thread
            m_ioService->registerTextMessageChannel(this);
        }


        void JmsChannel::listenForTextMessages() {

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


        bool JmsChannel::signalIncomingTextMessage() {

            cout << "GOT MESSAGE" << endl;
            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQMessageType messageType;

                MQStatus status = MQReceiveMessageWithTimeout(m_consumerHandle, 2000, &messageHandle);
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
                            MQ_SAFE_CALL(MQCloseMessageConsumer(m_consumerHandle));
                            m_hasAsyncHandler = false;
                        }
                        m_readStringHashHandler(shared_from_this(), string(msgBody), header);
                        MQ_SAFE_CALL(MQFreeMessage(messageHandle));
                    } else if (messageType == MQ_BYTES_MESSAGE) {
                        // Just ignore binary messages
                    } else {
                        // Give an error if unexpected message types are going round the broker
                        throw MESSAGE_EXCEPTION("Received message of unsupported type (i.e. neither <text> nor <bytes>)");
                    }
                    return true;
                }
            } catch (const Exception& e) {
                m_signalError(shared_from_this(), e.userFriendlyMsg());
            } catch (...) {
                m_signalError(shared_from_this(), "Unknown expection was raised whilst reading asynchronously");
            }
            return false;
        }


        void JmsChannel::readAsyncRawHash(const ReadRawHashHandler& readHandler) {
            if (m_hasAsyncHandler) {
                throw NOT_SUPPORTED_EXCEPTION("You may only register exactly one handler per channel, "
                                              "if you need more handlers create a new channel on the connection and register there");
            }
            m_hasAsyncHandler = true;

            // Save the callback function
            m_readRawHashHandler = readHandler;

            // Set up a consumer
            MQ_SAFE_CALL(MQCreateMessageConsumer(m_sessionHandle, m_destinationHandle, m_filterCondition.c_str(), m_jmsConnection.m_deliveryInhibition, &m_consumerHandle));

            // Start listening for messages by starting an individual thread
            m_ioService->registerBinaryMessageChannel(this);
        }


        void JmsChannel::readAsyncHashHash(const ReadHashHashHandler& handler) {
            m_readHashHashHandler = handler;
            readAsyncRawHash(boost::bind(&karabo::net::JmsChannel::rawHash2HashHash, this, _1, _2, _3, _4));
        }


        void JmsChannel::listenForBinaryMessages() {

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


        bool JmsChannel::signalIncomingBinaryMessage() {
            cout << "GOT MESSAGE" << endl;

            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQMessageType messageType;

                MQStatus status = MQReceiveMessageWithTimeout(m_consumerHandle, 2000, &messageHandle);
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
                            MQ_SAFE_CALL(MQCloseMessageConsumer(m_consumerHandle));
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
                            MQ_SAFE_CALL(MQCloseMessageConsumer(m_consumerHandle));
                            m_hasAsyncHandler = false;
                        }
                        // TODO Check, wheter to free the propertiesHandle
                        m_readRawHashHandler(shared_from_this(), reinterpret_cast<const char*> (msgBody), string(msgBody).size(), header);
                        MQ_SAFE_CALL(MQFreeMessage(messageHandle));
                    } else {
                        // Give an error if unexpected message types are going round the broker
                        throw MESSAGE_EXCEPTION("Received message of unsupported type (i.e. neither <text> nor <bytes>)");
                    }
                    return true;
                }
            } catch (const Exception& e) {
                m_signalError(shared_from_this(), e.userFriendlyMsg());
            } catch (...) {
                m_signalError(shared_from_this(), "Unknown expection was raised whilst reading asynchronously");
            }
            return false;
        }


        void JmsChannel::write(const std::string& messageBody, const Hash& header) {

            try {

                MQProducerHandle producerHandle = MQ_INVALID_HANDLE;
                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;

                MQ_SAFE_CALL(MQCreateMessageProducerForDestination(m_sessionHandle, m_destinationHandle, &producerHandle))
                MQ_SAFE_CALL(MQCreateTextMessage(&messageHandle))
                MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle))

                // Add some default properties
                Hash properties(header);
                properties.set<long long>("__timestamp", karabo::util::Time::getMsSinceEpoch());
                setProperties(properties, propertiesHandle);

                MQ_SAFE_CALL(MQSetMessageProperties(messageHandle, propertiesHandle))

                // TODO Care about the proper freeing of propertiesHandle

                MQ_SAFE_CALL(MQSetTextMessageText(messageHandle, messageBody.c_str()))

                //cout << " Sending message: " << endl << messageBody << endl;

                MQ_SAFE_CALL(MQSendMessageExt(producerHandle, messageHandle, MQ_PERSISTENT_DELIVERY, 4, m_jmsConnection.m_messageTimeToLive))

                // Clean up
                //MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                MQ_SAFE_CALL(MQFreeMessage(messageHandle))
                MQ_SAFE_CALL(MQCloseMessageProducer(producerHandle))

            } catch (...) {
                RETHROW
            }
        }


        void JmsChannel::write(const char* messageBody, const size_t& size, const Hash& header) {

            try {

                MQProducerHandle producerHandle = MQ_INVALID_HANDLE;
                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;

                MQ_SAFE_CALL(MQCreateMessageProducerForDestination(m_sessionHandle, m_destinationHandle, &producerHandle))
                MQ_SAFE_CALL(MQCreateBytesMessage(&messageHandle))
                MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle))

                // Add some default properties
                Hash properties(header);
                properties.set<long long>("__timestamp", karabo::util::Time::getMsSinceEpoch());
                setProperties(properties, propertiesHandle);

                MQ_SAFE_CALL(MQSetMessageProperties(messageHandle, propertiesHandle))
                        // TODO Care about the proper freeing of propertiesHandle

                if (size > 0) {
                    MQ_SAFE_CALL(MQSetBytesMessageBytes(messageHandle, reinterpret_cast<MQInt8*> (const_cast<char*> (messageBody)), size));
                }

                MQ_SAFE_CALL(MQSendMessageExt(producerHandle, messageHandle, MQ_PERSISTENT_DELIVERY, 4, m_jmsConnection.m_messageTimeToLive))

                // Clean up
                //MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
                MQ_SAFE_CALL(MQFreeMessage(messageHandle))

                MQ_SAFE_CALL(MQCloseMessageProducer(producerHandle))
            } catch (...) {
                RETHROW
            }
        }


        void JmsChannel::write(const karabo::util::Hash& data, const karabo::util::Hash& header) {
            string s;
            string format = getHashFormat();
            Hash modifiedHeader(header);
            modifiedHeader.set("__format", format);
            hashToString(data, s);
            if (format == "Xml" || format == "LibConfig") {
                write(s, modifiedHeader);
            } else if (format == "Bin") {
                std::vector<char> v(s.begin(), s.end());
                write(&v[0], v.size(), modifiedHeader);
            }
        }


        void JmsChannel::setProperties(const Hash& properties, const MQPropertiesHandle& propertiesHandle) {
            try {
                for (Hash::const_iterator it = properties.begin(); it != properties.end(); it++) {
                    Types::Type type = properties.getTypeAsId(it);
                    switch (type) {
                        case Types::STRING:
                            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle, it->first.c_str(), properties.get<string > (it).c_str()))
                            break;
                        case Types::INT8:
                            MQ_SAFE_CALL(MQSetInt8Property(propertiesHandle, it->first.c_str(), properties.get<signed char>(it)))
                            break;
                        case Types::UINT16:
                        case Types::INT16:
                            MQ_SAFE_CALL(MQSetInt16Property(propertiesHandle, it->first.c_str(), properties.getNumeric<short>(it)))
                            break;
                        case Types::UINT32:
                        case Types::INT32:
                            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, it->first.c_str(), properties.getNumeric<int>(it)))
                            break;
                        case Types::UINT64:
                        case Types::INT64:
                            MQ_SAFE_CALL(MQSetInt64Property(propertiesHandle, it->first.c_str(), properties.getNumeric<long long>(it)))
                            break;
                        case Types::FLOAT:
                            MQ_SAFE_CALL(MQSetFloat32Property(propertiesHandle, it->first.c_str(), properties.get<float>(it)))
                            break;
                        case Types::DOUBLE:
                            MQ_SAFE_CALL(MQSetFloat64Property(propertiesHandle, it->first.c_str(), properties.get<double>(it)))
                            break;
                        case Types::BOOL:
                            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle, it->first.c_str(), properties.get<bool>(it)))
                            break;
                        default:
                            throw NOT_SUPPORTED_EXCEPTION("Given property value type (" + Types::convert(type) + ") is not supported by the OpenMQ");
                            break;
                    }
                }
            } catch (...) {
                RETHROW
            }
        }


        void JmsChannel::setErrorHandler(const ErrorHandler& handler) {
            m_signalError.connect(handler);
        }


        void JmsChannel::waitAsync(int milliseconds, const WaitHandler& handler) {
            m_ioService->registerWaitChannel(this, handler, milliseconds);
        }


        void JmsChannel::deadlineTimer(const WaitHandler& handler, int milliseconds) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(milliseconds));
            handler(shared_from_this());
        }


        void JmsChannel::stop() {
            m_isStopped = true;
        }


        void JmsChannel::close() {
            stop();
            unregisterChannel(shared_from_this());
        }


        void JmsChannel::rawHash2HashHash(Channel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash& header) {
            Hash h;
            if (m_jmsConnection.m_autoDetectMessageFormat) {
                stringstream ss(string(data, size));
                string format("Xml"); // Default if no format is given
                header.tryToGet("__format", format);
                HashFormatsConstIt it = m_hashFormats.find(format);
                if (it == m_hashFormats.end()) {
                    try {
                        m_hashFormats[format] = HashFormat::create(Hash(format));
                    } catch (const Exception& e) {
                        throw MESSAGE_EXCEPTION("Could not understand/automatically create a Hash from the given \"" + format + "\" format");
                    }
                }
                m_hashFormats[format]->convert(ss, h);
            } else {
                string s(data, size);
                stringToHash(s, h);
            }
            m_readHashHashHandler(channel, h, header);
        }
    } // namespace net
} // namespace karabo
