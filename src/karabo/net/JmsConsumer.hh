/*
 * $Id$
 *
 * File:   JmsConsumer.hh
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


#ifndef KARABO_NET_JMSCONSUMER_HH
#define KARABO_NET_JMSCONSUMER_HH

#include <openmqc/mqtypes.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "Broker.hh"
#include "JmsConnection.hh"
#include "Strand.hh"
#include "karabo/io/BinarySerializer.hh"
#include "utils.hh"


/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        /**
         * @class JmsConsumer
         * @brief A class consuming messages from a JMS broker
         */
        class JmsConsumer : public boost::enable_shared_from_this<JmsConsumer> {
            friend class JmsConnection;

           public:
            KARABO_CLASSINFO(JmsConsumer, "JmsChannel", "0.1")

            /**
             * This triggers permanent reading of messages on the current topic that obey the provided selector.
             * The given MessageHandler will be called sequentially for each message. An error notifier
             * can be specified as well and will, in case of an error, be called (and have finished) before
             * the message handler is called.
             * Permanent reading can be interrupted by stopReading.
             *
             * @param handler Message handler of signature <void (Hash::Pointer header, Hash::Pointer body)>
             * @param errorNotifier Error notifier of signature <void (JmsConsumer::Error, string)> with an Error and a
             *                      string indicating the problem
             */
            void startReading(const consumer::MessageHandler& handler,
                              const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier());

            /**
             * Stop permanent reading after having started it with startReading(..).
             *
             * Note: Use with care!
             * 1) This does not stop receiving messages from the broker. On the contrary, messages are received and pile
             *    up both locally and on the broker since they are not acknowledged!
             *    To stop receiving, destruct this JmsConsumer.
             * 2) Usually blocks until reading thread is joined - up to 100 ms.
             */
            void stopReading();

            /**
             * Set the broker topic to consume messages from
             *
             * Use only when not reading, otherwise potential race condition.
             * @param topic
             */
            void setTopic(const std::string& topic);

            /**
             * Set a selector by which to select messages to consume. See
             * https://docs.wso2.com/display/MB300/Using+Message+Selectors on how
             * selectors should be specified.
             * @param selector
             */
            void setSelector(const std::string& selector);

            virtual ~JmsConsumer();

           private:
            // The 'skipSerialisation' flag is for expert use: Instead of deserialising the message body, the body
            // provided to the consumer::MessageHandler will be a Hash with a single key "raw" containing an
            // std::vector<char> of the serialised message.
            // This is e.g. used in karabo-brokerrates to speed up (it digests all messages of a topic!).
            JmsConsumer(const JmsConnection::Pointer& connection, const std::string& topic, const std::string& selector,
                        bool skipSerialisation = false);

            void setHandlers(const consumer::MessageHandler& handler, const consumer::ErrorNotifier& errorNotifier);

            void postSetHandlers(const Strand::Pointer& strand, const consumer::MessageHandler& handler,
                                 const consumer::ErrorNotifier& errorNotifier);

            void consumeMessages(JmsConsumer::Pointer& selfGuard);

            /// A shared pointer to an MQMessageHandle that takes care to correctly free its memory
            typedef std::shared_ptr<MQMessageHandle> MQMessageHandlePointer;

            void deserialize(const MQMessageHandlePointer& messageHandlerPtr);

            void postErrorOnHandlerStrand(consumer::Error error, const std::string& msg);

            void notifyError(consumer::Error error, const std::string& msg);

            MQConsumerHandle getConsumer(const std::string& topic, const std::string& selector);

            std::pair<MQSessionHandle, MQDestinationHandle> ensureConsumerDestinationAvailable(
                  const std::string& topic, const std::string& selector);

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

            typedef std::map<std::string, MQSessionHandle> ConsumerSessions;
            ConsumerSessions m_consumerSessions;

            typedef std::map<std::string, std::pair<MQSessionHandle, MQDestinationHandle> > ConsumerDestinations;
            ConsumerDestinations m_consumerDestinations;

            typedef std::map<std::string, MQConsumerHandle> Consumers;
            std::map<std::string, MQConsumerHandle> m_consumers;

            bool m_reading;
            consumer::MessageHandler m_messageHandler;
            consumer::ErrorNotifier m_errorNotifier;

            karabo::net::Strand::Pointer m_serializerStrand;
            karabo::net::Strand::Pointer m_handlerStrand;

            boost::thread m_readThread;

            std::string m_topic;

            std::string m_selector;
        };
    } // namespace net
} // namespace karabo

#endif
