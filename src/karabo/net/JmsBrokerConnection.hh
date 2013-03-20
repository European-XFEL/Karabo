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
    public:

      KARABO_CLASSINFO(JmsBrokerConnection, "Jms", "1.0")
      
      friend class JmsBrokerChannel;

      virtual ~JmsBrokerConnection();

      static void expectedParameters(karabo::util::Schema& expected);

      JmsBrokerConnection(const karabo::util::Hash& input);

      BrokerChannel::Pointer start();

      void stop();

      void close();

      BrokerChannel::Pointer createChannel();
      
      bool getDeliveryInhibition() const { return m_deliveryInhibition; }

    private:

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

      MQConnectionHandle m_connectionHandle;

      void setConnectionProperties(const MQPropertiesHandle& propertiesHandle);

      static void onException(const MQConnectionHandle connectionHandle, MQStatus status, void* callbackData);

    };




  } // namespace net
} // namespace karabo

#endif	/* KARABO_NET_AJMSCONNECTION_HH */
