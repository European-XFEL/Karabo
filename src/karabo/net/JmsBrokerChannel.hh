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
              if (MQStatusIsError(status = (mqCall)) == MQ_TRUE) { \
                MQString tmp = MQGetStatusString(status); \
                std::string errorString(tmp); \
                MQFreeString(tmp); \
                throw KARABO_OPENMQ_EXCEPTION(errorString); \
              } \
            }         

            typedef boost::signals2::signal<void (BrokerChannel::Pointer, const std::string&) > SignalError;

            // Provides access to the JmsConnection object
            const JmsBrokerConnection& m_jmsConnection;

            const std::string& m_serializationType;

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_textSerializer;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;

            // Provides access to the JmsIOService object
            JmsBrokerIOService::Pointer m_ioService;

            // Signals occurrence of errors
            SignalError m_signalError;

            // Types needed by the OpenMQ API
            MQSessionHandle m_sessionHandle;
            MQDestinationHandle m_destinationHandle;
            MQBool m_isTransacted;

            // Save for asynchronous communication
            MQConsumerHandle m_asyncConsumerHandle;
            ReadStringHashHandler m_readStringHashHandler;
            ReadRawHashHandler m_readRawHashHandler;
            ReadHashHashHandler m_readHashHashHandler;

            // Save for synchronous communication
            MQConsumerHandle m_syncConsumerHandle;
            
            MQProducerHandle m_producerHandle;
            
            // User supplied filtering on broker
            std::string m_filterCondition;

            bool m_isStopped;

            // Protects against registering multiple async handlers
            bool m_hasAsyncHandler;

            // Flags whether a synchronous consumer was already pre-registered
            bool m_hasSyncConsumer;

            // Time out for synchronous reads (milliseconds)
            int m_syncReadTimeout;
            
            // Consumer mutex
            boost::mutex m_synchReadWriteMutex;
            

        public:

            KARABO_CLASSINFO(JmsBrokerChannel, "JmsBrokerChannel", "1.0")

            JmsBrokerChannel(JmsBrokerConnection& connection);

            virtual ~JmsBrokerChannel();

            /**************************************************************/
            /*              Synchronous Read - With Header                */
            /**************************************************************/

            /**
             * This function reads from a channel into vector of chars 
             * The reading will block until the header and data records are read.
             * @return void 
             */
            virtual void read(std::vector<char>& body, karabo::util::Hash& header);
            /**
             * This function reads from a channel into std::string 
             * The reading will block until the header and data records are read.
             * @return void 
             */
            virtual void read(std::string& body, karabo::util::Hash& header);

            virtual void read(karabo::util::Hash& body, karabo::util::Hash& header);


            //**************************************************************/
            //*              Asynchronous Read - With Header               */
            //**************************************************************/

            void readAsyncRawHash(const ReadRawHashHandler& readHandler);

            void readAsyncHashHash(const ReadHashHashHandler& handler);

            void readAsyncStringHash(const ReadStringHashHandler& readHandler);

            //**************************************************************/
            //*              Synchronous Write - With Header               */
            //**************************************************************/

            /**
             * Low level writing within the JMS framework
             * This method will actually compose a JMS text message and set JMS message properties
             * @param data The textual data (body) of the message
             * @param header The list of properties to be send as JMS properties
             */
            void write(const std::string& data, const karabo::util::Hash& header);

            /**
             * Low level writing within the JMS framework
             * This method will actually compose a JMS binary message and set JMS message properties
             * @param data The binary data (body) of the message
             * @param header The list of properties to be send as JMS properties
             */
            void write(const char* data, const size_t& size, const karabo::util::Hash& header);

            void write(const karabo::util::Hash& data, const karabo::util::Hash& header);

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

            void preRegisterSynchronousRead();

            /**
             * This function returns the currently set JMS selection
             * @return The currently set JMS selector
             */
            const std::string& getFilter() const;

            void setTimeoutSyncRead(int milliseconds);

            void waitAsync(int milliseconds, const WaitHandler& handler);

            void setErrorHandler(const BrokerErrorHandler& handler);

            /**
             * This stops the asynchronous processing.
             */
            void stop();

            void close();

            // This function listens for text messages and may block depending on the IOSerice mode (e.g. work())
            void listenForTextMessages();

            // This function listens for binary messages and may block depending on the IOSerice mode (e.g. work())
            void listenForBinaryMessages();

            void deadlineTimer(const WaitHandler& handler, int milliseconds);

        private: //functions

            /**
             * Signals arrival of a message to the private m_readStringHashHandler
             * @return true if a message was received, false otherwise (timed out)
             */
            bool signalIncomingTextMessage();

            /**
             * Signals arrival of a message to the private m_readVectorHashHandler
             * @return true if a message was received, false otherwise (timed out)
             */
            bool signalIncomingBinaryMessage();

            void setProperties(const karabo::util::Hash& properties, const MQPropertiesHandle& propertiesHandle);

            void getProperties(karabo::util::Hash& properties, const MQPropertiesHandle& propertiesHandle) const;

            std::string prepareSelector() const;

            void rawHash2HashHash(BrokerChannel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash& header);

        };
    }
}

#endif	
