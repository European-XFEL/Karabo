/*
 * $Id$
 *
 * File:   JmsChannel.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 18, 2016, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_JMSCHANNEL_HH
#define	KARABO_NET_JMSCHANNEL_HH

#include <openmqc/mqcrt.h>
#include <karabo/io/BinarySerializer.hh>
#include "JmsConnection.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        class JmsChannel {


            friend class JmsConnection;

            // OpenMQ failed to provide an publicly available constant to check handle validity
            // This constant is copied from the openMQ source in which
            // it is used for exactly the aforementioned purpose
            static const int HANDLED_OBJECT_INVALID_HANDLE = 0xFEEEFEEE;

            JmsConnection::Pointer m_connection;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;

            mutable boost::mutex m_consumerHandlesMutex;

            MQSessionHandle m_producerSessionHandle;

            typedef std::map<std::string, std::pair<MQSessionHandle, MQDestinationHandle > > ProducerDestinations;
            ProducerDestinations m_producerDestinations;

            typedef std::map<std::string, MQProducerHandle > Producers;
            Producers m_producers;

            typedef std::map<std::string, MQSessionHandle > ConsumerSessions;
            ConsumerSessions m_consumerSessions;

            typedef std::map<std::string, std::pair<MQSessionHandle, MQDestinationHandle > > ConsumerDestinations;
            ConsumerDestinations m_consumerDestinations;

            typedef std::map<std::string, MQConsumerHandle > Consumers;
            std::map<std::string, MQConsumerHandle > m_consumers;

            typedef boost::shared_ptr<boost::asio::io_service> IOServicePointer;
            IOServicePointer m_ioService;

            boost::asio::io_service::strand m_writeStrand;

        public:

            KARABO_CLASSINFO(JmsChannel, "JmsChannel", "someVersion")

            typedef boost::function< void (karabo::util::Hash::Pointer, karabo::util::Hash::Pointer) > MessageHandler;

            void readAsync(const MessageHandler handler, const std::string& topic, const std::string& selector = "");

            void write(const std::string& topic,
                       const karabo::util::Hash::Pointer& header,
                       const karabo::util::Hash::Pointer& body,
                       const int priority = 4,
                       const int timeToLive = 0);

            ~JmsChannel();

        private:

            JmsChannel(const JmsConnection::Pointer& connection);

            MQProducerHandle getProducer(const std::string& topic);

            MQSessionHandle ensureProducerSessionAvailable();

            std::pair<MQSessionHandle, MQDestinationHandle> ensureProducerDestinationAvailable(const std::string& topic);

            void clearProducerHandles();

            void setProperties(const karabo::util::Hash& properties, const MQPropertiesHandle& propertiesHandle);

            void asyncConsumeMessage(const MessageHandler handler, const std::string& topic, const std::string& selector);

            bool hasConsumer(const std::string& topic, const std::string& selector) const;

            MQConsumerHandle getConsumer(const std::string& topic, const std::string& selector);

            std::pair<MQSessionHandle, MQDestinationHandle> ensureConsumerDestinationAvailable(const std::string& topic);

            MQSessionHandle ensureConsumerSessionAvailable(const std::string& topic);

            void parseHeader(const MQMessageHandle& messageHandle, karabo::util::Hash& header);

            void getProperties(karabo::util::Hash& properties, const MQPropertiesHandle& propertiesHandle) const;

            /**
             * Clears the all consumer related cached handles
             * NOTE: This function is thread-safe, locks m_consumerHandlesMutex
             */
            void clearConsumerHandles();


            void asyncWrite(const std::string& topic,
                            const karabo::util::Hash::Pointer& header,
                            const karabo::util::Hash::Pointer& body,
                            const int priority = 4,
                            const int timeToLive = 0);
        };
    }
}

#endif	
