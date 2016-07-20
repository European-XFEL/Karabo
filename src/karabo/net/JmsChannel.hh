/*
 * $Id: JmsChannel.hh 3322 2011-04-15 15:32:14Z heisenb@DESY.DE $
 *
 * File:   JmsChannel.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_JMSCHANNEL_HH
#define	KARABO_NET_JMSCHANNEL_HH

#include <boost/signals2.hpp>

#include <openmqc/mqcrt.h>
#include "Channel.hh"


// JMS-Error handling convenience
#define MQ_SAFE_CALL(mqCall)\
{\
  MQStatus status;\
  if (MQStatusIsError(status = (mqCall)) == MQ_TRUE) {\
    MQString tmp = MQGetStatusString(status);\
    std::string errorString(tmp);\
    MQFreeString(tmp);\
    throw OPENMQ_EXCEPTION(errorString);\
  }\
}


/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package msg
     */
    namespace net {

        class AJmsConnection;

        class JmsIOService;

/**
         * Implementation of the Channel class specifially for Oracle's implementation of JMS within the OpenMQ
         * broker.
         */
        class JmsChannel : public Channel, public boost::enable_shared_from_this<JmsChannel> {


            typedef boost::shared_ptr<JmsIOService> JmsIOServicePointer;

            typedef karabo::io::Format<karabo::util::Hash> HashFormat;

            typedef boost::signals2::signal<void (Channel::Pointer, const std::string&) > SignalError;

        public:

            KARABO_CLASSINFO(JmsChannel, "JmsChannel", "1.0")

            JmsChannel(AJmsConnection& connection);

            virtual ~JmsChannel();

            /**
             * This function allows to specify arbitrary JMS conform selections
             * Example:
             * @code
             * servicePointer->setWhatToRead("target = '2D-Devices'");
             * @endcode
             * @param selection selection string
             */
            void setFilter(const std::string& filterCondition);

            /**
             * This function returns the currently set JMS selection
             * @return The currently set JMS selector
             */
            std::string getFilter() const;

            void setTimeoutSyncRead(int milliseconds);

            /**
             * Synchronous (blocking) low-level reading within the JMS framework
             * This method will read a JMS text or binary message
             * @param data text content of the message
             * @param header properties of the read message
             */
            void read(std::string& data, karabo::util::Hash& header);

            void read(karabo::util::Hash& body, karabo::util::Hash& header);

            //void read(const char* data, const size_t& dataSize, const karabo::util::Hash& header);


            void readAsyncStringHash(const ReadStringHashHandler& readHandler);

            void readAsyncRawHash(const ReadRawHashHandler& readHandler);

            void readAsyncHashHash(const ReadHashHashHandler& handler);

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

            void waitAsync(int milliseconds, const WaitHandler& handler);

            void setErrorHandler(const ErrorHandler& handler);

            /**
             * This stops the asynchronous processing.
             */
            void stop();

            void close();

            // This function listens for text messages and may block depending on the IOSerice mode (e.g. work())
            void listenForTextMessages();

            // This function listens for text messages and may block depending on the IOSerice mode (e.g. work())
            void listenForBinaryMessages();

            void deadlineTimer(const WaitHandler& handler, int milliseconds);

        private: // members

            // Provides access to the JmsConnection object
            const AJmsConnection& m_jmsConnection;

            // Provides access to the JmsIOService object
            JmsIOServicePointer m_ioService;

            // Signals occurence of errors
            SignalError m_signalError;

            // Types needed by the OpenMQ API
            MQSessionHandle m_sessionHandle;
            MQDestinationHandle m_destinationHandle;
            MQBool m_isTransacted;

            // Save for asynchronous communication
            MQConsumerHandle m_consumerHandle;
            ReadStringHashHandler m_readStringHashHandler;
            ReadRawHashHandler m_readRawHashHandler;
            ReadHashHashHandler m_readHashHashHandler;

            // Private hash formating for dynamic de-serialization on consumer side
            typedef std::map<std::string, HashFormat::Pointer > HashFormats;
            typedef HashFormats::const_iterator HashFormatsConstIt;
            HashFormats m_hashFormats;

            // User supplied filtering on broker
            std::string m_filterCondition;

            bool m_isStopped;

            // Protects agains registering multiple async handlers
            bool m_hasAsyncHandler;

            // Time out for synchronous reads (milliseconds)
            int m_syncReadTimeout;

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

            void rawHash2HashHash(Channel::Pointer channel, const char* data, const size_t& size, const karabo::util::Hash& header);

        };

    } // namespace packageName
} // namespace karabo

#endif	
