/*
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
/*
 * File:   JmsConnection.hh
 * Author: heisenb
 *
 * Created on July 15, 2016, 4:08 PM
 */

#ifndef KARABO_NET_JMSCONNECTION_HH
#define KARABO_NET_JMSCONNECTION_HH

#include <openmqc/mqlogutil-priv.h>
#include <openmqc/mqtypes.h>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <string>

#include "karabo/util/ClassInfo.hh"
#include "karabo/util/Schema.hh"

#define MQ_SAFE_CALL(mqCall)                                 \
    {                                                        \
        MQStatus status;                                     \
        if (MQStatusIsError(status = (mqCall)) == MQ_TRUE) { \
            MQString tmp = MQGetStatusString(status);        \
            std::string errorString(tmp);                    \
            MQFreeString(tmp);                               \
            throw KARABO_OPENMQ_EXCEPTION(errorString);      \
        }                                                    \
    }

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        // Forward declarations
        class JmsConsumer;
        class JmsProducer;

        /**
         * @class JmsConnection
         * @brief A class handling connections to a JMS message broker
         *
         * This class allows to create a single TCP connection to a JMS (openMQ) broker.
         * One or more broker URLs can be provided. If several URLs are provided they will be tried
         * in order once a previous connection failed.
         * NOTE: Automatic reconnection needs a running event-loop
         */
        class JmsConnection : public boost::enable_shared_from_this<JmsConnection> {
            friend class JmsConsumer;
            friend class JmsProducer;

           public:
            KARABO_CLASSINFO(JmsConnection, "JmsConnection", "1.0")


            static void expectedParameters(karabo::util::Schema& s);


            JmsConnection(const karabo::util::Hash& config);

            /**
             * Create a tcp connection to a JMS broker
             * @param brokerUrls A vector of broker URLs (tcp://<host>:<port>)
             */
            JmsConnection(const std::vector<std::string>& brokerUrls);

            /**
             * Create a tcp connection to a JMS broker
             * @param brokerUrls A single or a comma separated list of broker URLs (tcp://<host>:<port>)
             */
            JmsConnection(const std::string& brokerUrls);

            virtual ~JmsConnection();

            /**
             * Tries to establish a connection to the broker as provided in the constructor
             * If a connection can not be established, the next address (if available) is tried.
             * This function will try to connect (cycling the provided URLs) forever, until a connection is established.
             */
            void connect();

            /**
             * Disconnects from the broker
             */
            void disconnect();

            /**
             * Indicates whether a connection is available.
             * @return true if connected, false otherwise
             */
            bool isConnected() const;

            /**
             * Reports the url of the currently connected-to broker
             * In case no connection is established returns and empty string
             * @return broker url
             */
            std::string getBrokerUrl() const;


            /**
             * Creates a new consumer channel
             * In order to consume from different topics or selectors in parallel, several instances of consumers
             * must be created.
             * The 'skipSerialisation' flag is for expert use: Instead of deserialising the message body, the body
             * provided to the consumer::MessageHandler will be a Hash with a single key "raw" containing an
             * std::vector<char> of the serialised message.
             *
             * NOTE: Each call to this function will open a new thread in the central event-loop
             * @return JmsConsumer
             */
            boost::shared_ptr<JmsConsumer> createConsumer(const std::string& topic, const std::string& selector = "",
                                                          bool skipSerialisation = false);

            /**
             * Creates a new producer channel.
             * Messages can be send to different topics via a single instance of this object
             * @return JmsProducer
             */
            boost::shared_ptr<JmsProducer> createProducer();


           private:
            static void onException(const MQConnectionHandle connectionHandle, MQStatus status, void* callbackData);

            void setFlagConnected();

            void setFlagDisconnected();

            /**
             * This functions blocks the current thread in case no connection is available.
             * It will return in case a connection gets or is available.
             */
            void waitForConnectionAvailable();

            MQConnectionHandle getConnection() const;

            void parseBrokerUrl();

            void setConnectionProperties(const std::string& scheme, const std::string& host, const int port,
                                         const MQPropertiesHandle& propertiesHandle) const;

            static void onOpenMqLog(const MQLoggingLevel severity, const MQInt32 logCode, ConstMQString logMessage,
                                    const MQInt64 timeOfMessage, const MQInt64 connectionID, ConstMQString filename,
                                    const MQInt32 fileLineNumber, void* callbackData);


           private:
            // OpenMQ failed to provide an publicly available constant to check handle validity
            // This constant is copied from the openMQ source in which
            // it is used for exactly the aforementioned purpose
            static const int HANDLED_OBJECT_INVALID_HANDLE = 0xFEEEFEEE;

            std::vector<std::string> m_availableBrokerUrls;

            std::string m_connectedBrokerUrl;

            MQConnectionHandle m_connectionHandle;

            boost::asio::io_service::strand m_reconnectStrand;

            bool m_isConnected;
            boost::condition_variable m_isConnectedCond;
            mutable boost::mutex m_isConnectedMutex;

            // Represents the scheme, host and port part of a standard URL
            typedef boost::tuple<std::string, std::string, std::string> BrokerAddress;
            std::vector<BrokerAddress> m_brokerAddresses;

            static const int ping = 20;

            static const bool trustBroker = true;

            static const bool blockUntilAcknowledge = false;

            static const unsigned int acknowledgeTimeout = 0;
        };

    } // namespace net
} // namespace karabo

#endif
