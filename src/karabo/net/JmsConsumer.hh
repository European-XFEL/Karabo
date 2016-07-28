/*
 * $Id$
 *
 * File:   JmsConsumer.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 18, 2016, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_JMSCONSUMER_HH
#define	KARABO_NET_JMSCONSUMER_HH

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

        class JmsConsumer {

            friend class JmsConnection;            

        public:

            KARABO_CLASSINFO(JmsConsumer, "JmsChannel", "0.1")

            typedef boost::function< void (karabo::util::Hash::Pointer, karabo::util::Hash::Pointer) > MessageHandler;

            /**
             * This function registers a message handler, which will be called exactly once in case a message
             * on the given topic and obeying the provided selector will be available.
             * @param handler Message handler of signature <void (Hash::Pointer header, Hash::Pointer body)>
             * @param topic The topic to consume on
             * @param selector The selector expression (works on header keys only!)
             */
            void readAsync(const MessageHandler handler, const std::string& topic, const std::string& selector = "");

            ~JmsConsumer();

        private:

            JmsConsumer(const JmsConnection::Pointer& connection);           

            void asyncConsumeMessage(const MessageHandler handler, const std::string& topic, const std::string& selector);
          
            MQConsumerHandle getConsumer(const std::string& topic, const std::string& selector);

            std::pair<MQSessionHandle, MQDestinationHandle> ensureConsumerDestinationAvailable(const std::string& topic,
                                                                                               const std::string& selector);

            MQSessionHandle ensureConsumerSessionAvailable(const std::string& topic, const std::string& selector);

            void parseHeader(const MQMessageHandle& messageHandle, karabo::util::Hash& header);

            void getProperties(karabo::util::Hash& properties, const MQPropertiesHandle& propertiesHandle) const;

            /**
             * Clears the all consumer related cached handles
             * NOTE: This function is thread-safe, locks m_consumerHandlesMutex
             */
            void clearConsumerHandles();

        private:

            // OpenMQ failed to provide an publicly available constant to check handle validity
            // This constant is copied from the openMQ source in which
            // it is used for exactly the aforementioned purpose
            static const int HANDLED_OBJECT_INVALID_HANDLE = 0xFEEEFEEE;

            JmsConnection::Pointer m_connection;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;           

            typedef std::map<std::string, MQSessionHandle > ConsumerSessions;
            ConsumerSessions m_consumerSessions;

            typedef std::map<std::string, std::pair<MQSessionHandle, MQDestinationHandle > > ConsumerDestinations;
            ConsumerDestinations m_consumerDestinations;

            typedef std::map<std::string, MQConsumerHandle > Consumers;
            std::map<std::string, MQConsumerHandle > m_consumers;

            boost::asio::io_service::strand m_strand;
        };
    }
}

#endif	
