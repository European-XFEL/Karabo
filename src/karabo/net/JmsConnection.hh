/* 
 * File:   JmsConnection.hh
 * Author: heisenb
 *
 * Created on July 15, 2016, 4:08 PM
 */

#ifndef KARABO_NET_JMSCONNECTION_HH
#define	KARABO_NET_JMSCONNECTION_HH

#include <string>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/asio.hpp>
#include <openmqc/mqcrt.h>

#include <karabo/util/ClassInfo.hh>

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

        class JmsConnection : public boost::enable_shared_from_this<JmsConnection> {

            friend class JmsConsumer;
            friend class JmsProducer;
            
        public:

            KARABO_CLASSINFO(JmsConnection, "JmsConnection", "1.0")

            /**
             * This class allows to create a single TCP connection to a JMS (openMQ) broker.
             * One or more broker URLs can be provided. If several URLs are provided they will be tried
             * in order once a previous connection failed.
             * NOTE: Automatic reconnection needs a running event-loop
             * @param brokerUrl A single or a comma separated list of broker URLs (tcp://<host>:<port>)
             */
            JmsConnection(const std::string& brokerUrl = std::string("tcp://exfl-broker.desy.de:7777"));

            ~JmsConnection();

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
             * In order to consume from different topics (selectors) in parallel several instances of consumers
             * must be created
             * NOTE: Each call to this function, will open new thread in the central event-loop
             * @return JmsConsumer
             */
            boost::shared_ptr<JmsConsumer> createConsumer();

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


        private:

            // OpenMQ failed to provide an publicly available constant to check handle validity
            // This constant is copied from the openMQ source in which
            // it is used for exactly the aforementioned purpose
            static const int HANDLED_OBJECT_INVALID_HANDLE = 0xFEEEFEEE;

            std::string m_availableBrokerUrls;

            std::string m_connectedBrokerUrl;

            MQConnectionHandle m_connectionHandle;

            boost::asio::io_service::strand m_reconnectStrand;

            bool m_isConnected;
            boost::condition_variable m_isConnectedCond;
            mutable boost::mutex m_isConnectedMutex;

            // Represents the scheme, host and port part of a standard URL
            typedef boost::tuple<std::string, std::string, std::string> BrokerAddress;
            std::vector<BrokerAddress > m_brokerAddresses;

            static const int ping = 20;

            static const bool trustBroker = true;

            static const bool blockUntilAcknowledge = false;

            static const unsigned int acknowledgeTimeout = 0;
        };

    }
}

#endif

