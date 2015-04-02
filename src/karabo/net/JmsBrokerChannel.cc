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

#include <snappy.h>
#include <karabo/log.hpp>
//#include <karabo/util/SignalHandler.hh>
#include "JmsBrokerConnection.hh"
#include "JmsBrokerChannel.hh"
#include "JmsBrokerIOService.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::io;
using namespace boost::signals2;

namespace karabo {
    namespace net {


        JmsBrokerChannel::JmsBrokerChannel(BrokerConnection::Pointer connection, const std::string& subDestination)
        : BrokerChannel()
        , m_jmsConnection(boost::dynamic_pointer_cast<JmsBrokerConnection>(connection))
        , m_serializationType(boost::dynamic_pointer_cast<JmsBrokerConnection>(connection)->m_serializationType)
        , m_sessionHandle(MQ_INVALID_HANDLE)
        , m_destinationHandle(MQ_INVALID_HANDLE)
        , m_consumerHandle(MQ_INVALID_HANDLE)
        , m_producerHandle(MQ_INVALID_HANDLE)
        , m_filterCondition("")
        , m_isStopped(false)
        , m_hasAsyncHandler(false)
        , m_syncReadTimeout(100000)
        , m_hasConsumer(false)
        , m_hasProducer(false)
        , m_subDestination(subDestination)
        , m_hasSession(false) {

            //cout << "JmsBrokerChannel::JmsBrokerChannel: connection.use_count()=" << connection.use_count() << endl << StackTrace() << endl;
            boost::shared_ptr<JmsBrokerConnection> jbc = m_jmsConnection.lock();
            if (!jbc)
                throw KARABO_IO_EXCEPTION("BrokerChannel found parent connection destroyed");
            // Get the type specific IO service
            m_ioService = jbc->getIOService()->castTo<JmsBrokerIOService > ();

            // Transaction mode
            m_isTransacted = MQ_FALSE;
            if (jbc->m_acknowledgeMode == MQ_SESSION_TRANSACTED) {
                m_isTransacted = MQ_TRUE;
            }

            // Create the serializers
            m_textSerializer = TextSerializer<Hash>::create("Xml", Hash("indentation", -1));
            m_binarySerializer = BinarySerializer<Hash>::create("Bin");
            
            ensureSessionAvailable();
        }


        JmsBrokerChannel::~JmsBrokerChannel() {
            //cout << "****** " << BOOST_CURRENT_FUNCTION << " ENTRY ******" << endl;
            close();

            boost::shared_ptr<JmsBrokerConnection> jbc = m_jmsConnection.lock();
            if (jbc) {
                set<BrokerChannel::Pointer>& channels = jbc->m_channels;
                for (set<BrokerChannel::Pointer>::iterator it = channels.begin(); it != channels.end(); it++) {
                    if ((*it).get() == this) {
                        channels.erase(it);
                        break;
                    }
                }
            }
            //cout << "****** " << BOOST_CURRENT_FUNCTION << " EXIT  ******" << endl;
        }


        void JmsBrokerChannel::ensureConnectionAvailable() {
            boost::shared_ptr<JmsBrokerConnection> jbc = m_jmsConnection.lock();
            if (!jbc)
                throw KARABO_IO_EXCEPTION("BrokerChannel found parent connection destroyed");

            try {
                jbc->connectToBrokers();
            } catch (const SystemException& e) {
                m_isStopped = true;
                KARABO_RETHROW
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_OPENMQ_EXCEPTION("Problems whilst connecting to broker"));
            }
            assert(jbc->m_hasConnection == true);
        }


        void JmsBrokerChannel::ensureSessionAvailable() {

            ensureConnectionAvailable();

            boost::shared_ptr<JmsBrokerConnection> jbc = m_jmsConnection.lock();
            if (!jbc)
                throw KARABO_IO_EXCEPTION("BrokerChannel found parent connection destroyed");

            // this code runs "once per channel"
            boost::mutex::scoped_lock lock(m_sessionHandleMutex);
            if (!m_hasSession) {
                MQ_SAFE_CALL(MQCreateSession(jbc->m_connectionHandle,
                                             m_isTransacted,
                                             jbc->m_acknowledgeMode,
                                             MQ_SESSION_SYNC_RECEIVE,
                                             &m_sessionHandle));

                string destination = jbc->m_destinationName;
                if (!m_subDestination.empty()) destination += "_" + m_subDestination;
                MQ_SAFE_CALL(MQCreateDestination(m_sessionHandle, destination.c_str(),
                                                 jbc->m_destinationType,
                                                 &m_destinationHandle));
                m_hasSession = true;
                boost::mutex::scoped_lock lock(m_openMQMutex);
                m_hasConsumer = false;
                m_hasProducer = false;
            }
        }


        void JmsBrokerChannel::read(std::vector<char>& data) {

            Hash headerDummy;
            readBinaryMessage(headerDummy, data, false);
        }


        void JmsBrokerChannel::decompressSnappy(const char* compressed, size_t compressed_length, std::vector<char>& target) {
            // Get uncompressed length
            size_t uncompressedLength;
            bool res = snappy::GetUncompressedLength(compressed, compressed_length, &uncompressedLength);
            if (!res) {
                throw KARABO_MESSAGE_EXCEPTION("Failed to call to GetUncompressedLength() for \"snappy\" compressed data.");
            }
            // Decompress to the vector
            target.resize(uncompressedLength);
            if (!snappy::RawUncompress(compressed, compressed_length, &target[0])) {
                throw KARABO_NETWORK_EXCEPTION("Failed to uncompress \"snappy\" compressed data.");
            }
        }


        void JmsBrokerChannel::decompress(karabo::util::Hash& header, const char* compressed, size_t compressed_length, std::string& target) {
            std::vector<char> uncompressed;
            decompress(header, compressed, compressed_length, uncompressed);
            target.assign(uncompressed.begin(), uncompressed.end());
        }


        void JmsBrokerChannel::decompress(karabo::util::Hash& header, const char* compressed, size_t compressed_length, std::vector<char>& target) {
            if (header.get<string>("__compression__") == "snappy")
                decompressSnappy(compressed, compressed_length, target);
            else
                throw KARABO_MESSAGE_EXCEPTION("Unsupported compression algorithm: \"" + header.get<string>("__compression__") + "\".");
            header.erase("__compression__");
        }


        void JmsBrokerChannel::readBinaryMessage(karabo::util::Hash& header, std::vector<char>& body, bool withHeader) {

            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQStatus status = consumeMessage(messageHandle, m_syncReadTimeout);
                if (MQStatusIsError(status)) throw KARABO_TIMEOUT_EXCEPTION("Synchronous read timed out");

                MQMessageType messageType;
                MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType));

                if (messageType == MQ_BYTES_MESSAGE) {

                    int nBytes;
                    const MQInt8* bytes;
                    MQGetBytesMessageBytes(messageHandle, &bytes, &nBytes);
                    if (withHeader) parseHeader(messageHandle, header);
                    if (header.has("__compression__")) {
                        decompress(header, reinterpret_cast<const char*> (bytes), nBytes, body);
                    } else {
                        body.resize(nBytes);
                        // Copy once here
                        memcpy(&body[0], bytes, nBytes);
                    }
                } else {
                    throw KARABO_MESSAGE_EXCEPTION("Received message in wrong format (expecting binary)");
                }
                MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle));
                MQ_SAFE_CALL(MQFreeMessage(messageHandle));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        MQStatus JmsBrokerChannel::consumeMessage(MQMessageHandle& messageHandle, const int timeout) {
            MQStatus status;

            while (!m_isStopped) {

                ensureExistenceOfConsumer();

                boost::mutex::scoped_lock lock(m_openMQMutex);
                status = MQReceiveMessageWithTimeout(m_consumerHandle, timeout, &messageHandle);
                if (MQStatusIsError(status) == MQ_FALSE)
                    break; // Success
                if (MQGetStatusCode(status) == MQ_TIMEOUT_EXPIRED)
                    break; // in this particular case the timeout is not the error
                switch (MQGetStatusCode(status)) {
                    case MQ_STATUS_INVALID_HANDLE:
                    case MQ_BROKER_CONNECTION_CLOSED:
                    case MQ_SESSION_CLOSED:
                    case MQ_CONSUMER_CLOSED:
                        //ensureExistenceOfConsumer();
                        continue; // repeat operation
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
            return status;
        }


        void JmsBrokerChannel::ensureExistenceOfConsumer() {

            ensureSessionAvailable();

            bool deliveryInhibition;
            {
                boost::shared_ptr<JmsBrokerConnection> jbc = m_jmsConnection.lock();
                if (!jbc)
                    throw KARABO_IO_EXCEPTION("BrokerChannel found parent connection destroyed");
                else
                    deliveryInhibition = jbc->m_deliveryInhibition;
            }

            if (!m_hasConsumer) {
                //KARABO_LOG_FRAMEWORK_DEBUG << "ensureExistenceOfConsumer";
                MQ_SAFE_CALL(MQCreateMessageConsumer(m_sessionHandle, m_destinationHandle,
                                                     m_filterCondition.c_str(), deliveryInhibition,
                                                     &m_consumerHandle));
                boost::mutex::scoped_lock lock(m_openMQMutex);
                m_hasConsumer = true;
            }
        }


        void JmsBrokerChannel::parseHeader(const MQMessageHandle& messageHandle, karabo::util::Hash& header) {
            MQPropertiesHandle propertiesHandle, headerHandle;
            MQ_SAFE_CALL(MQGetMessageProperties(messageHandle, &propertiesHandle))
            MQ_SAFE_CALL(MQGetMessageHeaders(messageHandle, &headerHandle))
            getProperties(header, propertiesHandle);
            getProperties(header, headerHandle);
            MQ_SAFE_CALL(MQFreeProperties(propertiesHandle))
            MQ_SAFE_CALL(MQFreeProperties(headerHandle))
        }


        void JmsBrokerChannel::read(std::string& data) {
            Hash headerDummy;
            readTextMessage(headerDummy, data, false);
        }


        void JmsBrokerChannel::readTextMessage(karabo::util::Hash& header, std::string& body, bool withHeader) {

            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQStatus status = consumeMessage(messageHandle, m_syncReadTimeout);
                if (MQStatusIsError(status)) throw KARABO_TIMEOUT_EXCEPTION("Synchronous read timed out");

                MQMessageType messageType;
                MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType));

                if (messageType == MQ_TEXT_MESSAGE) {
                    ConstMQString msgBody;
                    MQ_SAFE_CALL(MQGetTextMessageText(messageHandle, &msgBody));
                    if (withHeader) parseHeader(messageHandle, header);
                    if (header.has("__compression__")) {
                        decompress(header, reinterpret_cast<const char*> (msgBody), strlen(msgBody), body);
                    } else {
                        // Copy once here
                        body = msgBody;
                    }
                } else {
                    throw KARABO_MESSAGE_EXCEPTION("Received message in wrong format (expecting text)");
                }
                MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle));
                // Clean up
                MQ_SAFE_CALL(MQFreeMessage(messageHandle));

            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::read(karabo::util::Hash& data) {

            Hash dummyHeader;
            readHashMessage(dummyHeader, data, false);

        }


        void JmsBrokerChannel::readHashMessage(karabo::util::Hash& header, karabo::util::Hash& body, bool withHeader) {

            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQStatus status = consumeMessage(messageHandle, m_syncReadTimeout);
                if (MQStatusIsError(status)) throw KARABO_TIMEOUT_EXCEPTION("Synchronous read timed out");

                MQMessageType messageType;
                MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType))

                if (messageType == MQ_BYTES_MESSAGE) {
                    int nBytes;
                    const MQInt8* bytes;
                    MQ_SAFE_CALL(MQGetBytesMessageBytes(messageHandle, &bytes, &nBytes));
                    if (withHeader) parseHeader(messageHandle, header);
                    if (header.has("__compression__")) {
                        std::vector<char> tmp;
                        decompress(header, reinterpret_cast<const char*> (bytes), nBytes, tmp);
                        m_binarySerializer->load(body, &tmp[0], tmp.size());
                    } else {
                        m_binarySerializer->load(body, reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes));
                    }
                } else if (messageType == MQ_TEXT_MESSAGE) {
                    ConstMQString msgBody;
                    MQ_SAFE_CALL(MQGetTextMessageText(messageHandle, &msgBody))
                    if (withHeader) parseHeader(messageHandle, header);
                    if (header.has("__compression__")) {
                        std::string tmp;
                        decompress(header, reinterpret_cast<const char*> (msgBody), strlen(msgBody), tmp);
                        m_textSerializer->load(body, tmp);
                    } else {
                        m_textSerializer->load(body, msgBody);
                    }
                } else {
                    throw KARABO_MESSAGE_EXCEPTION("Received invalid message type (neither text nor binary)");
                }
                MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle))
                // Clean up
                MQ_SAFE_CALL(MQFreeMessage(messageHandle));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::read(karabo::util::Hash& header, std::vector<char>& body) {

            readBinaryMessage(header, body, true);
        }


        void JmsBrokerChannel::read(karabo::util::Hash& header, std::string& body) {

            readTextMessage(header, body, true);
        }


        void JmsBrokerChannel::read(karabo::util::Hash& header, karabo::util::Hash& body) {

            readHashMessage(header, body, true);

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


        void JmsBrokerChannel::readAsyncRaw(const ReadRawHandler& readHandler) {

            ensureSingleAsyncHandler();

            ensureExistenceOfConsumer();

            // Save the callback function
            m_readRawHandler = readHandler;

            // Start listening for messages by starting an individual thread
            m_ioService->registerMessageReceiver(boost::bind(&karabo::net::JmsBrokerChannel::listenForRawMessages, this));
        }


        void JmsBrokerChannel::ensureSingleAsyncHandler() {
            if (m_hasAsyncHandler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("You may only register exactly one handler per channel, "
                                                     "if you need more handlers create a new channel on the connection and register there");
            }
            m_hasAsyncHandler = true;
        }


        void JmsBrokerChannel::listenForRawMessages() {

            try {

                bool messageReceived = false;
                do {
                    messageReceived = signalIncomingBinaryMessage(false);
                } while (!m_isStopped && ((!messageReceived && m_ioService->isRunning()) || m_ioService->isWorking()));

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_DEBUG << "An exception during JMS broker message reception occurred: \n" << e;
            } catch (...) {
                KARABO_LOG_FRAMEWORK_DEBUG << "An unknown exception during JMS broker message reception occurred";
            }
        }


        bool JmsBrokerChannel::signalIncomingBinaryMessage(const bool withHeader) {
            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQStatus status = consumeMessage(messageHandle, 2000);

                if (!MQStatusIsError(status) && !m_isStopped) {

                    MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle));

                    // Allow registration of another handler
                    if (!m_ioService->isWorking()) m_hasAsyncHandler = false;

                    MQMessageType messageType;
                    MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType));
                    if (messageType == MQ_BYTES_MESSAGE) {
                        // Body
                        int nBytes;
                        const MQInt8* bytes;
                        MQ_SAFE_CALL(MQGetBytesMessageBytes(messageHandle, &bytes, &nBytes));


                        if (withHeader) {
                            Hash::Pointer header(new Hash());
                            parseHeader(messageHandle, *header);
                            if (header->has("__compression__")) {
                                std::vector<char> tmp;
                                decompress(*header, reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes), tmp);
                                m_readHashRawHandler(shared_from_this(), header, &tmp[0], tmp.size());
                            } else {
                                m_readHashRawHandler(shared_from_this(), header, reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes));
                            }
                        } else {
                            m_readRawHandler(shared_from_this(), reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes));
                        }

                        MQ_SAFE_CALL(MQFreeMessage(messageHandle));

                    } else {
                        // Give an error if unexpected message types are going round the broker
                        throw KARABO_MESSAGE_EXCEPTION("Received message of unsupported type (expecting bytes)");
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


        void JmsBrokerChannel::readAsyncString(const ReadStringHandler& readHandler) {

            ensureSingleAsyncHandler();

            ensureExistenceOfConsumer();

            // Save the callback function
            m_readStringHandler = readHandler;

            // Start listening for messages by starting an individual thread
            m_ioService->registerMessageReceiver(boost::bind(&karabo::net::JmsBrokerChannel::listenForStringMessages, this));

        }


        void JmsBrokerChannel::listenForStringMessages() {

            try {
                bool messageReceived = false;
                do {
                    messageReceived = signalIncomingTextMessage(false);
                } while (!m_isStopped && ((!messageReceived && m_ioService->isRunning()) || m_ioService->isWorking()));

            } catch (const Exception& e) {
                cout << e;
                Exception::memorize();
            } catch (...) {
                Exception::memorize();
            }
        }


        bool JmsBrokerChannel::signalIncomingTextMessage(const bool withHeader) {
            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;

                MQStatus status = consumeMessage(messageHandle, 2000);

                if (!MQStatusIsError(status) && !m_isStopped) {
                    MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle));

                    // Allow registration of another handler
                    if (!m_ioService->isWorking()) m_hasAsyncHandler = false;

                    MQMessageType messageType;
                    MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType));

                    if (messageType == MQ_TEXT_MESSAGE) {
                        ConstMQString msgBody;
                        MQ_SAFE_CALL(MQGetTextMessageText(messageHandle, &msgBody));
                        if (withHeader) {
                            Hash::Pointer header(new Hash());
                            parseHeader(messageHandle, *header);
                            if (header->has("__compression__")) {
                                std::string tmp;
                                decompress(*header, reinterpret_cast<const char*> (msgBody), strlen(msgBody), tmp);
                                m_readHashStringHandler(shared_from_this(), header, tmp);
                            } else {
                                m_readHashStringHandler(shared_from_this(), header, string(msgBody));
                            }
                        } else {
                            m_readStringHandler(shared_from_this(), string(msgBody));
                        }

                        MQ_SAFE_CALL(MQFreeMessage(messageHandle));
                    } else {
                        // Give an error if unexpected message types are going round the broker
                        // TODO call back error function
                        throw KARABO_MESSAGE_EXCEPTION("Received message of unsupported type (expecting text)");
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


        void JmsBrokerChannel::readAsyncHash(const ReadHashHandler& readHandler) {

            ensureSingleAsyncHandler();

            ensureExistenceOfConsumer();

            // Save the callback function
            m_readHashHandler = readHandler;

            // Start listening for messages by starting an individual thread
            m_ioService->registerMessageReceiver(boost::bind(&karabo::net::JmsBrokerChannel::listenForHashMessages, this));
        }


        void JmsBrokerChannel::listenForHashMessages() {
            try {
                bool messageReceived = false;
                do {
                    messageReceived = signalIncomingHashMessage(false);
                } while (!m_isStopped && ((!messageReceived && m_ioService->isRunning()) || m_ioService->isWorking()));

            } catch (const Exception& e) {
                cout << e;
                Exception::memorize();
            } catch (...) {
                Exception::memorize();
            }
        }


        bool JmsBrokerChannel::signalIncomingHashMessage(const bool withHeader) {
            try {

                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQStatus status = consumeMessage(messageHandle, 2000);

                if (!MQStatusIsError(status) && !m_isStopped) {

                    MQ_SAFE_CALL(MQAcknowledgeMessages(m_sessionHandle, messageHandle));

                    // Allow registration of another handler
                    if (!m_ioService->isWorking()) m_hasAsyncHandler = false;

                    MQMessageType messageType;
                    MQ_SAFE_CALL(MQGetMessageType(messageHandle, &messageType));

                    Hash::Pointer body(new Hash());

                    if (messageType == MQ_BYTES_MESSAGE) {
                        int nBytes;
                        const MQInt8* bytes;
                        MQ_SAFE_CALL(MQGetBytesMessageBytes(messageHandle, &bytes, &nBytes));
                        if (withHeader) {
                            Hash::Pointer header(new Hash());
                            parseHeader(messageHandle, *header);
                            if (header->has("__compression__")) {
                                std::vector<char> tmp;
                                decompress(*header, reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes), tmp);
                                m_binarySerializer->load(*body, tmp);
                            } else {
                                m_binarySerializer->load(*body, reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes));
                            }
                            m_readHashHashHandler(shared_from_this(), header, body);
                        } else {
                            m_binarySerializer->load(*body, reinterpret_cast<const char*> (bytes), static_cast<size_t> (nBytes));
                            m_readHashHandler(shared_from_this(), body);
                        }
                    } else if (messageType == MQ_TEXT_MESSAGE) {
                        ConstMQString msgBody;
                        MQ_SAFE_CALL(MQGetTextMessageText(messageHandle, &msgBody));
                        if (withHeader) {
                            Hash::Pointer header(new Hash());
                            parseHeader(messageHandle, *header);
                            if (header->has("__compression__")) {
                                std::string tmp;
                                decompress(*header, reinterpret_cast<const char*> (msgBody), strlen(msgBody), tmp);
                                m_textSerializer->load(*body, tmp);
                            } else {
                                m_textSerializer->load(*body, msgBody);
                            }
                            m_readHashHashHandler(shared_from_this(), header, body);
                        } else {
                            m_textSerializer->load(*body, msgBody);
                            m_readHashHandler(shared_from_this(), body);
                        }
                    } else {
                        // Give an error if unexpected message types are going round the broker
                        throw KARABO_MESSAGE_EXCEPTION("Received message of unsupported type");
                    }

                    MQ_SAFE_CALL(MQFreeMessage(messageHandle));

                    return true;
                }
            } catch (const Exception& e) {
                m_signalError(shared_from_this(), e.userFriendlyMsg());
            } catch (...) {
                m_signalError(shared_from_this(), "Unknown exception was raised whilst reading asynchronously");
            }
            return false;
        }


        void JmsBrokerChannel::readAsyncHashRaw(const ReadHashRawHandler& readHandler) {
            ensureSingleAsyncHandler();

            ensureExistenceOfConsumer();

            // Save the callback function
            m_readHashRawHandler = readHandler;

            // Start listening for messages by starting an individual thread          
            m_ioService->registerMessageReceiver(boost::bind(&karabo::net::JmsBrokerChannel::listenForHashRawMessages, this));

        }


        void JmsBrokerChannel::listenForHashRawMessages() {

            try {

                bool messageReceived = false;
                do {
                    messageReceived = signalIncomingBinaryMessage(true);
                } while (!m_isStopped && ((!messageReceived && m_ioService->isRunning()) || m_ioService->isWorking()));

            } catch (const Exception& e) {
                cout << e;
                Exception::memorize();
            } catch (...) {
                Exception::memorize();
            }
        }


        void JmsBrokerChannel::readAsyncHashString(const ReadHashStringHandler& readHandler) {

            ensureSingleAsyncHandler();

            ensureExistenceOfConsumer();

            // Save the callback function
            m_readHashStringHandler = readHandler;

            // Start listening for messages by starting an individual thread                        
            m_ioService->registerMessageReceiver(boost::bind(&karabo::net::JmsBrokerChannel::listenForHashStringMessages, this));
        }


        void JmsBrokerChannel::listenForHashStringMessages() {

            try {

                bool messageReceived = false;
                do {
                    messageReceived = signalIncomingTextMessage(true);
                } while (!m_isStopped && ((!messageReceived && m_ioService->isRunning()) || m_ioService->isWorking()));

            } catch (const Exception& e) {
                cout << e;
                Exception::memorize();
            } catch (...) {
                Exception::memorize();
            }
        }


        void JmsBrokerChannel::readAsyncHashHash(const ReadHashHashHandler& readHandler) {
            ensureSingleAsyncHandler();

            ensureExistenceOfConsumer();

            // Save the callback function
            m_readHashHashHandler = readHandler;

            // Start listening for messages by starting an individual thread                        
            m_ioService->registerMessageReceiver(boost::bind(&karabo::net::JmsBrokerChannel::listenForHashHashMessages, this));
        }


        void JmsBrokerChannel::listenForHashHashMessages() {

            try {

                bool messageReceived = false;
                do {
                    messageReceived = signalIncomingHashMessage(true);
                } while (!m_isStopped && ((!messageReceived && m_ioService->isRunning()) || m_ioService->isWorking()));

            } catch (const Exception& e) {
                cout << e;
                Exception::memorize();
            } catch (...) {
                Exception::memorize();
            }

        }


        void JmsBrokerChannel::sendTextMessage(const karabo::util::Hash& header, const std::string& messageBody, const int priority) {
            int messageTimeToLive;
            {
                boost::shared_ptr<JmsBrokerConnection> jbc = m_jmsConnection.lock();
                if (!jbc)
                    throw KARABO_IO_EXCEPTION("BrokerChannel found parent connection destroyed");
                else
                    messageTimeToLive = jbc->m_messageTimeToLive;
            }

            try {
                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;

                MQ_SAFE_CALL(MQCreateTextMessage(&messageHandle));
                MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));

                // Add some default properties
                Hash properties(header);
                //properties.set<long long>("__timestamp", karabo::util::Time::getMsSinceEpoch());
                setProperties(properties, propertiesHandle);

                MQ_SAFE_CALL(MQSetMessageProperties(messageHandle, propertiesHandle));

                // TODO Care about the proper freeing of propertiesHandle

                MQ_SAFE_CALL(MQSetTextMessageText(messageHandle, messageBody.c_str()));

                do {
                    MQStatus status;
                    {
                        //cout << "*** Sending Text message: " << endl << messageBody << endl;

                        boost::mutex::scoped_lock lock(m_openMQMutex);
                        status = MQSendMessageExt(m_producerHandle, messageHandle, MQ_NON_PERSISTENT_DELIVERY,
                                                  priority, messageTimeToLive);
                    }
                    if (MQStatusIsError(status) == MQ_FALSE) break;
                    switch (MQGetStatusCode(status)) {
                        case MQ_BROKER_CONNECTION_CLOSED:
                        case MQ_SESSION_CLOSED:
                        case MQ_PRODUCER_NO_DESTINATION:
                        case MQ_PRODUCER_CLOSED:
                        case MQ_STATUS_INVALID_HANDLE:
                        {
                            ensureProducerAvailable();
                            continue;
                        }
                        default:
                        {
                            MQFreeMessage(messageHandle); // free resource because we are going to throw exception
                            MQString tmp = MQGetStatusString(status);
                            std::string errorString(tmp);
                            MQFreeString(tmp);
                            throw KARABO_OPENMQ_EXCEPTION(errorString);
                        }
                    }
                } while (!m_isStopped);

                // Clean up
                MQ_SAFE_CALL(MQFreeMessage(messageHandle));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::sendBinaryMessage(const Hash& header, const char* messageBody, const size_t& size, const int priority) {
            int messageTimeToLive;
            {
                boost::shared_ptr<JmsBrokerConnection> jbc = m_jmsConnection.lock();
                if (!jbc)
                    throw KARABO_IO_EXCEPTION("BrokerChannel found parent connection destroyed");
                else
                    messageTimeToLive = jbc->m_messageTimeToLive;
            }

            try {
                //MQProducerHandle producerHandle = MQ_INVALID_HANDLE;
                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;

                MQ_SAFE_CALL(MQCreateBytesMessage(&messageHandle));
                MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));

                // Add some default properties
                Hash properties(header);
                //properties.set<long long>("__timestamp", karabo::util::Time::getMsSinceEpoch());
                setProperties(properties, propertiesHandle);

                MQ_SAFE_CALL(MQSetMessageProperties(messageHandle, propertiesHandle));
                // TODO Care about the proper freeing of propertiesHandle

                if (size > 0) {
                    MQ_SAFE_CALL(MQSetBytesMessageBytes(messageHandle, reinterpret_cast<MQInt8*> (const_cast<char*> (messageBody)), size));
                }

                do {
                    MQStatus status;
                    {
                        //cout << "*** Sending Binary message: size=" << size << endl;

                        boost::mutex::scoped_lock lock(m_openMQMutex);
                        status = MQSendMessageExt(m_producerHandle, messageHandle,
                                                  MQ_NON_PERSISTENT_DELIVERY,
                                                  priority, messageTimeToLive);
                    }
                    if (MQStatusIsError(status) == MQ_FALSE) break;
                    switch (MQGetStatusCode(status)) {
                        case MQ_BROKER_CONNECTION_CLOSED:
                        case MQ_SESSION_CLOSED:
                        case MQ_PRODUCER_NO_DESTINATION:
                        case MQ_PRODUCER_CLOSED:
                        case MQ_STATUS_INVALID_HANDLE:
                        {
                            ensureProducerAvailable();
                            continue;
                        }
                        default:
                        {
                            MQFreeMessage(messageHandle);
                            MQString tmp = MQGetStatusString(status);
                            std::string errorString(tmp);
                            MQFreeString(tmp);
                            throw KARABO_OPENMQ_EXCEPTION(errorString);
                        }
                    }
                } while (!m_isStopped);
                // Clean up
                MQ_SAFE_CALL(MQFreeMessage(messageHandle));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::sendBinaryMessageCompressed(const Hash& header, const char* messageBody, const size_t& size, const int priority) {
            int messageTimeToLive;
            string compression;

            {
                boost::shared_ptr<JmsBrokerConnection> jbc = m_jmsConnection.lock();
                if (!jbc)
                    throw KARABO_IO_EXCEPTION("BrokerChannel found parent connection destroyed");
                else {
                    compression = jbc->m_compression;
                    messageTimeToLive = jbc->m_messageTimeToLive;
                }
            }

            try {
                //MQProducerHandle producerHandle = MQ_INVALID_HANDLE;
                MQMessageHandle messageHandle = MQ_INVALID_HANDLE;
                MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;

                MQ_SAFE_CALL(MQCreateBytesMessage(&messageHandle));
                MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));

                // Add some default properties
                Hash properties(header);
                std::vector<char> tmp;
                compress(properties, compression, messageBody, size, tmp);

                //properties.set<long long>("__timestamp", karabo::util::Time::getMsSinceEpoch());
                setProperties(properties, propertiesHandle);

                MQ_SAFE_CALL(MQSetMessageProperties(messageHandle, propertiesHandle));
                // TODO Care about the proper freeing of propertiesHandle

                if (tmp.size() > 0) {
                    MQ_SAFE_CALL(MQSetBytesMessageBytes(messageHandle, reinterpret_cast<MQInt8*> (const_cast<char*> (&tmp[0])), tmp.size()));
                }

                do {
                    MQStatus status;
                    {
                        boost::mutex::scoped_lock lock(m_openMQMutex);
                        MQStatus status = MQSendMessageExt(m_producerHandle, messageHandle,
                                                           MQ_NON_PERSISTENT_DELIVERY,
                                                           priority, messageTimeToLive);
                    }
                    if (MQStatusIsError(status) == MQ_FALSE) break;
                    switch (MQGetStatusCode(status)) {
                        case MQ_BROKER_CONNECTION_CLOSED:
                        case MQ_SESSION_CLOSED:
                        case MQ_PRODUCER_NO_DESTINATION:
                        case MQ_PRODUCER_CLOSED:
                        case MQ_STATUS_INVALID_HANDLE:
                        {
                            ensureProducerAvailable();
                            continue;
                        }
                        default:
                        {
                            MQFreeMessage(messageHandle);
                            MQString tmp = MQGetStatusString(status);
                            std::string errorString(tmp);
                            MQFreeString(tmp);
                            throw KARABO_OPENMQ_EXCEPTION(errorString);
                        }
                    }
                } while (!m_isStopped);
                // Clean up
                MQ_SAFE_CALL(MQFreeMessage(messageHandle));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::compressSnappy(const char* source, const size_t& source_length, std::vector<char>& target) {
            try {
                size_t maxlen = snappy::MaxCompressedLength(source_length);
                target.resize(maxlen);
                size_t compressed_length;
                snappy::RawCompress(source, source_length, &target[0], &compressed_length);
                target.resize(compressed_length);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::compress(karabo::util::Hash& header, const std::string& cmprs, const char* source, const size_t& source_length, std::vector<char>& target) {
            try {
                if (cmprs == "snappy")
                    compressSnappy(source, source_length, target);
                else
                    throw KARABO_PARAMETER_EXCEPTION("Unsupported compression algorithm: \"" + cmprs + "\".");
                header.set("__compression__", cmprs);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::write(const karabo::util::Hash& header, const std::string& messageBody, const int priority) {

            try {

                //ensureProducerAvailable();

                sendTextMessage(header, messageBody, priority);

            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::ensureProducerAvailable() {

            ensureSessionAvailable();

            if (!m_hasProducer) {
                MQ_SAFE_CALL(MQCreateMessageProducerForDestination(m_sessionHandle, m_destinationHandle, &m_producerHandle));
                boost::mutex::scoped_lock lock(m_openMQMutex);
                m_hasProducer = true;
            }
        }


        void JmsBrokerChannel::write(const Hash& header, const char* messageBody, const size_t& size, const int priority) {
            int compressionUsageThreshold;
            //string compression;

            {
                boost::shared_ptr<JmsBrokerConnection> jbc = m_jmsConnection.lock();
                if (!jbc)
                    throw KARABO_IO_EXCEPTION("BrokerChannel found parent connection destroyed");
                else {
                    compressionUsageThreshold = jbc->m_compressionUsageThreshold;
                    //compression = jbc->m_compression;
                }
            }

            try {
                //ensureProducerAvailable();
                //                std::cout << "JmsBrokerChannel::write: threshold=" << compressionUsageThreshold
                //                        << ", compression = \"" <<  jbc->m_compression << "\"." << std::endl;
                if (compressionUsageThreshold >= 0 && compressionUsageThreshold < int(size)) {
                    sendBinaryMessageCompressed(header, messageBody, size, priority);
                } else {
                    sendBinaryMessage(header, messageBody, size, priority);
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void JmsBrokerChannel::write(const karabo::util::Hash& header, const karabo::util::Hash& data, const int priority) {
            //cout << "JmsBrokerChannel::write ....\n\theader\n" << header << "\n\tdata\n" << data << "------------------\n" << endl;
            Hash modifiedHeader(header);
            if (m_serializationType == "text") {
                modifiedHeader.set("__format", "Xml");
                string buffer;
                m_textSerializer->save(data, buffer);
                this->write(modifiedHeader, buffer, priority);
            } else if (m_serializationType == "binary") {
                modifiedHeader.set("__format", "Bin");
                std::vector<char> buffer;
                m_binarySerializer->save(data, buffer);
                this->write(modifiedHeader, &buffer[0], buffer.size(), priority);
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


        //        void JmsBrokerChannel::waitAsync(int milliseconds, const WaitHandler& handler, const std::string& id) {
        //            m_ioService->registerWaitChannel(this, handler, milliseconds, id);
        //        }


        void JmsBrokerChannel::deadlineTimer(const WaitHandler& handler, int milliseconds, const std::string& id) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(milliseconds));
            handler(shared_from_this(), id);
        }


        void JmsBrokerChannel::close() {
            m_isStopped = true;

            MQCloseMessageProducer(m_producerHandle);
            m_producerHandle = MQ_INVALID_HANDLE;
            m_hasProducer = false;

            MQCloseMessageConsumer(m_consumerHandle);
            m_consumerHandle = MQ_INVALID_HANDLE;
            m_hasConsumer = false;

            MQFreeDestination(m_destinationHandle);
            m_destinationHandle = MQ_INVALID_HANDLE;

            MQCloseSession(m_sessionHandle);
            m_sessionHandle = MQ_INVALID_HANDLE;

            m_hasAsyncHandler = false;
        }


        void JmsBrokerChannel::setSessionFalse() {
            m_hasSession = false;
        }


        void JmsBrokerChannel::rawHash2HashHash(BrokerChannel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash::Pointer& header) {
            Hash::Pointer body(new Hash());
            if (header->has("__format")) {
                std::string format = header->get<string>("__format");
                if (format == "Xml") {
                    try {
                        m_textSerializer->load(*body, data);
                    } catch (const Exception& e) {
                        throw KARABO_MESSAGE_EXCEPTION("Could not de-serialize text message into Hash");
                    }
                } else if (format == "Bin") {
                    try {
                        m_binarySerializer->load(*body, data, size);
                    } catch (const Exception& e) {
                        throw KARABO_MESSAGE_EXCEPTION("Could not de-serialize binary message into Hash");
                    }
                } else {
                    throw KARABO_MESSAGE_EXCEPTION("Encountered message with unknown format: \"" + format + "\"");
                }
            } else {
                throw KARABO_MESSAGE_EXCEPTION("De-serialization of message without __format tag is not possible");
            }
            m_readHashHashHandler(channel, body, header);
        }
    }
}
