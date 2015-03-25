/*
 * $Id: JmsBrokerConnection.hh 6930 2012-08-03 10:45:21Z heisenb $
 *
 * File:   Connection.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_JMSCONNECTION_HH
#define	KARABO_NET_JMSCONNECTION_HH

#include <openmqc/mqcrt.h>
#include <karabo/util/Factory.hh>

#include "BrokerConnection.hh"
#include "JmsBrokerChannel.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package msg
     */
    namespace net {

        /**
         * The Connection class.
         */
        class JmsBrokerConnection : public BrokerConnection {
            bool m_clusterMode;
            std::vector<std::string> m_brokerHosts;
            std::string m_hostname;
            unsigned int m_port;
            std::string m_destinationName;
            MQDestinationType m_destinationType;
            std::string m_username;
            std::string m_password;
            std::string m_protocol;
            unsigned int m_ping;
            bool m_trustBroker;
            bool m_acknowledgeSent;
            bool m_deliveryInhibition;
            unsigned int m_acknowledgeTimeout;
            MQAckMode m_acknowledgeMode;
            int m_messageTimeToLive;
            int m_compressionUsageThreshold;
            std::string m_compression;

            mutable boost::mutex m_openMQMutex;

            boost::mutex m_connectionHandleMutex;
            MQConnectionHandle m_connectionHandle;


        public:

            KARABO_CLASSINFO(JmsBrokerConnection, "Jms", "1.0")

            friend class JmsBrokerChannel;

            virtual ~JmsBrokerConnection();

            static void expectedParameters(karabo::util::Schema& expected);

            JmsBrokerConnection(const karabo::util::Hash& input);

            void start();

            void stop();

            const std::string& getBrokerHostname() const;

            unsigned int getBrokerPort() const;

            const std::string& getBrokerTopic() const;

            const std::vector<std::string>& getBrokerHosts() const;

            BrokerChannel::Pointer createChannel(const std::string& subDestionation = "");

            bool getDeliveryInhibition() const {
                return m_deliveryInhibition;
            }

        private:

            void close();

            void setConnectionProperties(const MQPropertiesHandle& propertiesHandle);

            static void onException(const MQConnectionHandle connectionHandle, MQStatus status, void* callbackData);
            
            void connectToBrokers();
            
            void connectStandalone();
            
            void connectCluster();

        };




    } // namespace net
} // namespace karabo

#endif	/* KARABO_NET_AJMSCONNECTION_HH */
