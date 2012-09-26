/*
 * $Id: JmsConnection.hh 3392 2011-04-28 12:49:18Z heisenb@DESY.DE $
 *
 * File:   Connection.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_AJMSCONNECTION_HH
#define	KARABO_NET_AJMSCONNECTION_HH

#include <openmqc/mqcrt.h>
#include <karabo/util/Factory.hh>

#include "Connection.hh"
#include "JmsChannel.hh"

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
    class AJmsConnection : public Connection {
    public:

      KARABO_CLASSINFO(AJmsConnection, "Jms", "1.0")
      
      friend class JmsChannel;

      AJmsConnection();
      
      ~AJmsConnection();

      static void expectedParameters(karabo::util::Schema& expected);

      void configure(const karabo::util::Hash& input);

      Channel::Pointer start();

      void stop();

      void close();

      Channel::Pointer createChannel();
      
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
      bool m_autoDetectMessageFormat;

      MQConnectionHandle m_connectionHandle;

      void setConnectionProperties(const MQPropertiesHandle& propertiesHandle);

      static void onException(const MQConnectionHandle connectionHandle, MQStatus status, void* callbackData);

    };




  } // namespace net
} // namespace karabo

#endif	/* KARABO_NET_AJMSCONNECTION_HH */
