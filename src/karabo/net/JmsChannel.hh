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

            boost::mutex::scoped_lock m_writeMutex;

            MQSessionHandle m_producerSessionHandle;

            typedef std::map<std::string, std::pair<MQSessionHandle, MQDestinationHandle > > ProducerDestinations;
            ProducerDestinations m_producerDestinations;

            typedef std::map<std::string, MQProducerHandle > Producers;
            Producers m_producers;


            std::map<std::string, MQSessionHandle > m_consumerSessions;          

            std::map<std::string, MQConsumerHandle > m_consumers;

        public:

            KARABO_CLASSINFO(JmsChannel, "JmsChannel", "someVersion")

            typedef boost::function< void (karabo::util::Hash::Pointer, karabo::util::Hash::Pointer) > MessageHandler;

            void readAsync(const std::string& topic, const MessageHandler handler, const std::string& selector = "");

            void write(const std::string& topic,
                       const karabo::util::Hash::Pointer& header,
                       const karabo::util::Hash::Pointer& body,
                       const int priority = 4,
                       const int timeToLive = 0);

        private:

            JmsChannel(const JmsConnection::Pointer&);

            MQProducerHandle getProducer(const std::string& topic);

            MQSessionHandle ensureProducerSessionAvailable();

            std::pair<MQSessionHandle, MQDestinationHandle> ensureProducerDestinationAvailable(const std::string& topic);                       

            void setProperties(const karabo::util::Hash& properties, const MQPropertiesHandle& propertiesHandle);           
            
        };
    }
}

#endif	
