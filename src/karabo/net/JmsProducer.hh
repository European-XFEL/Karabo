/*
 * $Id$
 *
 * File:   JmsProducer.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 18, 2016, 9:47 AM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef KARABO_NET_JMSPRODUCER_HH
#define KARABO_NET_JMSPRODUCER_HH

#include <openmqc/mqtypes.h>

#include <boost/enable_shared_from_this.hpp>

#include "JmsConnection.hh"
#include "karabo/io/BinarySerializer.hh"
#include "karabo/util/MetaTools.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        /**
         * @class JmsProducer
         * @brief A class producing messages to send to a JMS broker
         */
        class JmsProducer : public boost::enable_shared_from_this<JmsProducer> {
            friend class JmsConnection;

           public:
            KARABO_CLASSINFO(JmsProducer, "JmsProducer", "0.1")

            /**
             * Writes a message containing header and body (expressed as Hashes) to the broker.
             * This function runs asynchronously, it only blocks in case the connection to the broker is not available.
             * @param topic The topic to which this message should be sent
             * @param header The message header, all keys in here qualify for selector statements on the consumer side
             * @param body The message body
             * @param priority The message priority from 0 (lowest) - 9 (highest), default: 4
             * @param timeToLive The life time of the message in ms, default: 0 (lives forever)
             */
            void write(const std::string& topic, const karabo::util::Hash::Pointer& header,
                       const karabo::util::Hash::Pointer& body, const int priority = 4, const int timeToLive = 0);

            virtual ~JmsProducer();

           private:
            JmsProducer(const JmsConnection::Pointer& connection);

            MQProducerHandle getProducer(const std::string& topic);

            MQSessionHandle ensureProducerSessionAvailable();

            std::pair<MQSessionHandle, MQDestinationHandle> ensureProducerDestinationAvailable(
                  const std::string& topic);

            void clearProducerHandles();

            void setProperties(const karabo::util::Hash& properties, const MQPropertiesHandle& propertiesHandle) const;

            void asyncWrite(const std::string& topic, const karabo::util::Hash::Pointer& header,
                            const karabo::util::Hash::Pointer& body, const int priority = 4, const int timeToLive = 0);

           private:
            // OpenMQ failed to provide an publicly available constant to check handle validity
            // This constant is copied from the openMQ source in which
            // it is used for exactly the aforementioned purpose
            static const int HANDLED_OBJECT_INVALID_HANDLE = 0xFEEEFEEE;

            JmsConnection::Pointer m_connection;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;

            MQSessionHandle m_producerSessionHandle;

            typedef std::map<std::string, std::pair<MQSessionHandle, MQDestinationHandle> > ProducerDestinations;
            ProducerDestinations m_producerDestinations;

            typedef std::map<std::string, MQProducerHandle> Producers;
            Producers m_producers;

            boost::asio::io_service::strand m_mqStrand;
        };
    } // namespace net
} // namespace karabo

#endif
