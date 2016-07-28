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


            // OpenMQ failed to provide an publicly available constant to check handle validity
            // This constant is copied from the openMQ source in which
            // it is used for exactly the aforementioned purpose
            static const int HANDLED_OBJECT_INVALID_HANDLE = 0xFEEEFEEE;

            friend class JmsConsumer;
            friend class JmsProducer;

            std::string m_availableBrokerUrls;

            std::string m_connectedBrokerUrl;

            boost::condition_variable m_cond;

            mutable boost::mutex m_mut;

            bool m_isConnected;

            typedef boost::tuple<std::string, std::string, std::string> BrokerAddress;
            std::vector<BrokerAddress > m_brokerAddresses;

            MQConnectionHandle m_connectionHandle;

            static const int ping = 20;

            //static const std::string username = "guest";

            //static const std::string password = "guest";

            static const bool trustBroker = true;

            static const bool blockUntilAcknowledge = false;

            static const unsigned int acknowledgeTimeout = 0;

            typedef std::set<boost::weak_ptr<JmsConsumer> > Consumers;
            Consumers m_consumers;

            typedef std::set<boost::weak_ptr<JmsProducer> > Producers;
            Producers m_producers;

            boost::asio::io_service::strand m_reconnectStrand;



        public:

            KARABO_CLASSINFO(JmsConnection, "JmsConnection", "1.0")

            JmsConnection(const std::string& brokerUrl = std::string("tcp://exfl-broker.desy.de:7777"),
                          const int nThreads = 10);

            ~JmsConnection();

            void connect();

            void disconnect();

            bool isConnected() const;

            /**
             * Reports the url of the currently connected-to broker
             * In case no connection is established returns and empty string
             * @return broker url
             */
            std::string getBrokerUrl() const;

            boost::shared_ptr<JmsConsumer> createConsumer();

            boost::shared_ptr<JmsProducer> createProducer();


        private:

            static void onException(const MQConnectionHandle connectionHandle, MQStatus status, void* callbackData);          

            void setFlagConnected();

            void setFlagDisconnected();

            void waitForConnectionAvailable();

            MQConnectionHandle getConnection() const;

            void parseBrokerUrl();

            void setConnectionProperties(const std::string& scheme, const std::string& host, const int port,
                                         const MQPropertiesHandle& propertiesHandle) const;

        };

    }
}

#endif

