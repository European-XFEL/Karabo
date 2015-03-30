/*
 * $Id: JmsBrokerChannel.hh 3322 2011-04-15 15:32:14Z heisenb@DESY.DE $
 *
 * File:   JmsBrokerChannel.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_BROKERJMSCHANNEL_HH
#define	KARABO_NET_BROKERJMSCHANNEL_HH

#include <boost/signals2.hpp>
#include <openmqc/mqcrt.h>

#include "BrokerConnection.hh"
#include "BrokerChannel.hh"
#include "JmsBrokerIOService.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        class JmsBrokerConnection;

        class JmsBrokerIOService;

        /**
         * Implementation of the BrokerChannel class specifically for Oracle's implementation of JMS within the OpenMQ
         * broker.
         */
        class JmsBrokerChannel : public BrokerChannel, public boost::enable_shared_from_this<JmsBrokerChannel> {
            // JMS-Error handling convenience
#define MQ_SAFE_CALL(mqCall) \
            { \
              MQStatus status; \
              boost::mutex::scoped_lock lock(m_openMQMutex); \
              if (MQStatusIsError(status = (mqCall)) == MQ_TRUE) { \
                MQString tmp = MQGetStatusString(status); \
                std::string errorString(tmp); \
                MQFreeString(tmp); \
                throw KARABO_OPENMQ_EXCEPTION(errorString); \
              } \
            }

#define MQ_SAFE_CALL_STATUS(mqCall, status) \
            { \
              boost::mutex::scoped_lock lock(m_openMQMutex); \
              if (MQStatusIsError(status = (mqCall)) == MQ_TRUE) { \
                MQString tmp = MQGetStatusString(status); \
                std::string errorString(tmp); \
                MQFreeString(tmp); \
                throw KARABO_OPENMQ_EXCEPTION(errorString); \
              } \
            }

            typedef boost::signals2::signal<void (BrokerChannel::Pointer, const std::string&) > SignalError;

            // Provides access to the JmsConnection object
            boost::weak_ptr<JmsBrokerConnection> m_jmsConnection;

            const std::string& m_serializationType;

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_textSerializer;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;

            // Provides access to the JmsIOService object
            JmsBrokerIOService::Pointer m_ioService;

            // Signals occurrence of errors
            SignalError m_signalError;

            // Types needed by the OpenMQ API
            boost::mutex m_sessionHandleMutex;
            MQSessionHandle m_sessionHandle;
            MQDestinationHandle m_destinationHandle;
            MQBool m_isTransacted;

            // Save for synchronous communication
            MQConsumerHandle m_consumerHandle;

            MQProducerHandle m_producerHandle;

            // User supplied filtering on broker
            std::string m_filterCondition;

            bool m_isStopped;

            // Protects against registering multiple async handlers
            bool m_hasAsyncHandler;

            // Time out for synchronous reads (milliseconds)
            int m_syncReadTimeout;

            // Flags whether a consumer exists
            bool m_hasConsumer;

            // Flags whether a producer exists
            bool m_hasProducer;

            // Consumer mutex
            mutable boost::mutex m_openMQMutex;

            // Sub-destination
            std::string m_subDestination;

            // Flag: initialization required
            bool m_hasSession;

        public:

            KARABO_CLASSINFO(JmsBrokerChannel, "JmsBrokerChannel", "1.0")

            JmsBrokerChannel(BrokerConnection::Pointer connection, const std::string& subDestination);

            virtual ~JmsBrokerChannel();
            
            BrokerConnection::Pointer getConnection() {
                boost::shared_ptr<JmsBrokerConnection> jbc(m_jmsConnection.lock());
                return boost::reinterpret_pointer_cast<BrokerConnection, JmsBrokerConnection>(jbc);
            }

            /**************************************************************/
            /*              Synchronous Read - No Header                  */
            /**************************************************************/
            
            /**
             * This function reads binary messages into vector of chars. 
             * The reading will block until the data record is read.
             * The vector will be updated accordingly.
             * @param data The received binary data             
             */
            virtual void read(std::vector<char>& data);

            /**
             * This function reads text messages into a string.
             * The reading will block until the data record is read.
             * The string will be updated accordingly.
             * @param data The received textual data
             */
            virtual void read(std::string& data);

            /**
             * This function reads messages into a Hash object.
             * The reading will block until the data record is read.
             * The hash will be updated accordingly.
             * @param data The received data serialized into Hash
             */
            virtual void read(karabo::util::Hash& data);


            /**************************************************************/
            /*              Synchronous Read - With Header                */
            /**************************************************************/

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the header and data records are read.
             * @return void 
             */
            virtual void read(karabo::util::Hash& header, std::vector<char>& body);

            /**
             * This function reads from a channel into std::string 
             * The reading will block until the header and data records are read.
             * @return void 
             */
            virtual void read(karabo::util::Hash& header, std::string& body);

            virtual void read(karabo::util::Hash& header, karabo::util::Hash& body);


            //**************************************************************/
            //*           Asynchronous Read - Without Header               */
            //**************************************************************/

            void readAsyncRaw(const ReadRawHandler& readHandler);

            void readAsyncString(const ReadStringHandler& readHandler);

            void readAsyncHash(const ReadHashHandler& handler);


            //**************************************************************/
            //*              Asynchronous Read - With Header               */
            //**************************************************************/

            void readAsyncHashRaw(const ReadHashRawHandler& readHandler);

            void readAsyncHashString(const ReadHashStringHandler& readHandler);

            void readAsyncHashHash(const ReadHashHashHandler& handler);

            //**************************************************************/
            //*              Synchronous Write - With Header               */
            //**************************************************************/

            /**
             * Low level writing within the JMS framework
             * This method will actually compose a JMS text message and set JMS message properties
             * @param data The textual data (body) of the message
             * @param header The list of properties to be send as JMS properties
             */
            void write(const karabo::util::Hash& header, const std::string& data, const int priority = 4);

            /**
             * Low level writing within the JMS framework
             * This method will actually compose a JMS binary message and set JMS message properties
             * @param data The binary data (body) of the message
             * @param header The list of properties to be send as JMS properties
             */
            void write(const karabo::util::Hash& header, const char* data, const size_t& size, const int priority = 4);

            void write(const karabo::util::Hash& header, const karabo::util::Hash& data, const int priority = 4);

            //**************************************************************/
            //*                Errors, Timing, Selections                  */
            //**************************************************************/

            /**
             * This function allows to specify arbitrary JMS conform selections
             * Example:
             * @code
             * channel->setFilter("target='myTag'");
             * @endcode
             * @param filterCondition A SQL compliant (WHERE clause) condition
             */
            void setFilter(const std::string& filterCondition);

            /**
             * This function returns the currently set JMS selection
             * @return The currently set JMS selector
             */
            const std::string& getFilter() const;

            void setTimeoutSyncRead(int milliseconds);

            //void waitAsync(int milliseconds, const WaitHandler& handler, const std::string& id);

            void setErrorHandler(const BrokerErrorHandler& handler);

            void listenForRawMessages();

            void listenForStringMessages();

            void listenForHashMessages();

            void listenForHashRawMessages();

            void listenForHashStringMessages();

            void listenForHashHashMessages();

            void deadlineTimer(const WaitHandler& handler, int milliseconds, const std::string& id);

            void setSessionFalse();

        private: //functions

            void init();

            /**
             * This stops the asynchronous processing.
             */
            void close();

            void ensureExistanceOfConsumer();

            void readBinaryMessage(karabo::util::Hash& header, std::vector<char>& body, bool withHeader);

            void readTextMessage(karabo::util::Hash& header, std::string& body, bool withHeader);

            void readHashMessage(karabo::util::Hash& header, karabo::util::Hash& body, bool withHeader);

            MQStatus consumeMessage(MQMessageHandle& messageHandle, const int timeout);

            void parseHeader(const MQMessageHandle& messageHandle, karabo::util::Hash& header);

            void ensureSingleAsyncHandler();

            void ensureProducerAvailable();

            void ensureSessionAvailable();

            void ensureConnectionAvailable();

            /**
             * Signals arrival of a message to the private m_readStringHashHandler
             * @return true if a message was received, false otherwise (timed out)
             */
            bool signalIncomingTextMessage(const bool withHeader);

            /**
             * Signals arrival of a message to the private m_readVectorHashHandler
             * @return true if a message was received, false otherwise (timed out)
             */
            bool signalIncomingBinaryMessage(const bool withHeader);

            bool signalIncomingHashMessage(const bool withHeader);

            void setProperties(const karabo::util::Hash& properties, const MQPropertiesHandle& propertiesHandle);

            void getProperties(karabo::util::Hash& properties, const MQPropertiesHandle& propertiesHandle) const;

            std::string prepareSelector() const;

            void rawHash2HashHash(BrokerChannel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash::Pointer& header);

            void sendTextMessage(const karabo::util::Hash& header, const std::string& messageBody, const int priority);

            void sendBinaryMessage(const karabo::util::Hash& header, const char* messageBody, const size_t& size, const int priority);

            void sendBinaryMessageCompressed(const karabo::util::Hash& header, const char* messageBody, const size_t& size, const int priority);

            void compressSnappy(const char* source, const size_t& source_length, std::vector<char>& target);

            void compress(karabo::util::Hash& header, const std::string& cmprs, const char* source, const size_t& source_length, std::vector<char>& target);

            void decompressSnappy(const char* compressed, size_t compressed_length, std::vector<char>& data);

            void decompressSnappy(const char* compressed, size_t compressed_length, std::string& data);

            void decompress(karabo::util::Hash& header, const char* compressed, size_t compressed_length, std::vector<char>& data);

            void decompress(karabo::util::Hash& header, const char* compressed, size_t compressed_length, std::string& data);
        };
    }
}

#endif	
