/*
 * $Id: JmsBrokerConnection.cc 6930 2012-08-03 10:45:21Z heisenb $
 *
 * File:   Connection.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 2, 2010, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "JmsBrokerConnection.hh"

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace net {

        KARABO_REGISTER_FACTORY_CC(BrokerConnection, JmsBrokerConnection)

        JmsBrokerConnection::JmsBrokerConnection() {
            // Invalidate connectionHandle initially for defensive programming reasons
            // m_connectionHandle TODO Investigate how to!
        }

        JmsBrokerConnection::~JmsBrokerConnection() {
            MQFreeConnection(m_connectionHandle);
        }

        void JmsBrokerConnection::expectedParameters(Schema& expected) {
            
            // Some tricks with environment here
            string defaultHostname = "exflserv01.desy.de";
            char* env = getenv("XFEL_BROKER_HOST");
            if (env != 0) defaultHostname = string(env);
            
            // TODO Implement, once fromString is there
//            unsigned int defaultPort = 7676;
//            char* env = getenv("XFEL_BROKER_PORT");
//            if (env != 0) defaultHostname = string(env);
            
            string defaultTopic = string(getenv("USER"));
            env = getenv("XFEL_BROKER_TOPIC");
            if (env != 0) defaultTopic = string(env);

            STRING_ELEMENT(expected)
                    .key("hostname")
                    .displayedName("Broker Hostname")
                    .description("Broker Hostname")
                    .assignmentOptional().defaultValue(defaultHostname)
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("port")
                    .displayedName("Broker Hostport")
                    .description("Broker Hostport")
                    .assignmentOptional().defaultValue(7676)
                    .commit();

            // TODO Check for WIN32 compatibility
            STRING_ELEMENT(expected)
                    .key("destinationName")
                    .displayedName("Destination Name")
                    .description("The name of the physical destination (topic.queue) on the broker")
                    .assignmentOptional().defaultValue(defaultTopic)
                    .commit();

            STRING_ELEMENT(expected)
                    .key("messagingDomain")
                    .displayedName("Messaging Domain")
                    .description("Messaging domain, i.e. point-to-point (JMS-Queue) or publish-subscribe (JMS-Topic)")
                    .assignmentOptional().defaultValue("publish-subscribe")
                    .options("publish-subscribe,point-to-point")
                    .commit();

            STRING_ELEMENT(expected)
                    .key("username")
                    .displayedName("Username")
                    .description("Username")
                    .assignmentOptional().defaultValue("guest")
                    .commit();

            STRING_ELEMENT(expected)
                    .key("password")
                    .displayedName("Password")
                    .description("Password")
                    .assignmentOptional().defaultValue("guest")
                    .commit();

            STRING_ELEMENT(expected)
                    .key("protocol")
                    .displayedName("Protocol")
                    .description("Underlying transport protocol for jms based messages")
                    .assignmentOptional().defaultValue("TCP")
                    .options("TCP,SSL")
                    .advanced()
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("ping")
                    .displayedName("Ping")
                    .description("Client's ping interval to test whether the connection to the broker is still alive [seconds]")
                    .assignmentOptional().defaultValue(10)
                    .minInc(1)
                    .advanced()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("trustBroker")
                    .displayedName("Trust Broker")
                    .description("Should the broker's certificate be trusted?")
                    .assignmentOptional().defaultValue(true)
                    .advanced()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("acknowledgeSent")
                    .displayedName("Acknowledge Message Sent")
                    .description("Should senders be blocked until the broker acknowledges message receipt?")
                    .assignmentOptional().defaultValue(true)
                    .advanced()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("deliveryInhibition")
                    .displayedName("Message Self Delivery Inhibition")
                    .description("Should be inhibited delivery of messages published on the topic by this consumer's own connection?")
                    .assignmentOptional().defaultValue(false)
                    .advanced()
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("acknowledgeTimeout")
                    .displayedName("Acknowledge Timeout")
                    .description("Maximum waiting time for any broker acknowledgement")
                    .assignmentOptional().defaultValue(0)
                    .advanced()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("acknowledgeMode")
                    .displayedName("Acknowledge Mode")
                    .description("General Acknowledgement Mode")
                    .assignmentOptional().defaultValue("explicit")
                    .options("auto,explicit,transacted")
                    .advanced()
                    .commit();

            INT32_ELEMENT(expected)
                    .key("messageTimeToLive")
                    .displayedName("Message's Time to Live")
                    .description("Time to live for an individual message send by a producer (0 = unlimited) [ms]")
                    .assignmentOptional().defaultValue(10000)
                    .minInc(0)
                    .advanced()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("autoDetectMessageFormat")
                    .displayedName("Auto-detect message format")
                    .description("If true the hashSerialization parameter will be ignored and messages will be decoded according to the formatTag they carry.")
                    .assignmentOptional().defaultValue(true)
                    .advanced()
                    .commit();
            
            OVERWRITE_ELEMENT(expected)
                    .key("hashSerialization")
                    .setNewDefault("Xml")
                    .commit();
            
            OVERWRITE_ELEMENT(expected)
                    .key("hashSerialization.Xml.indentation")
                    .setNewDefault(-1)
                    .commit();

            OVERWRITE_ELEMENT(expected)
                    .key("hashSerialization.Xml.printDataType")
                    .setNewDefault(true)
                    .commit();

            // TODO See whether to include also flow control parameters

        }

        void JmsBrokerConnection::configure(const karabo::util::Hash& input) {

            setIOServiceType("Jms");

            input.get("hostname", m_hostname);
            //if (m_hostname.empty()) 
                
            input.get("port", m_port);
            input.get("destinationName", m_destinationName);

            string destinationType;
            input.get("messagingDomain", destinationType);
            if (destinationType == "publish-subscribe") m_destinationType = MQ_TOPIC_DESTINATION;
            else if (destinationType == "point-to-point") m_destinationType = MQ_QUEUE_DESTINATION;

            input.get("username", m_username);
            input.get("password", m_password);
            input.get("protocol", m_protocol);
            input.get("ping", m_ping);
            input.get("trustBroker", m_trustBroker);
            input.get("acknowledgeSent", m_acknowledgeSent);
            input.get("deliveryInhibition", m_deliveryInhibition);
            input.get("acknowledgeTimeout", m_acknowledgeTimeout);
            input.get("messageTimeToLive", m_messageTimeToLive);

            string acknowledgeMode;
            input.get("acknowledgeMode", acknowledgeMode);
            if (acknowledgeMode == "auto") m_acknowledgeMode = MQ_AUTO_ACKNOWLEDGE;
            else if (acknowledgeMode == "explicit") m_acknowledgeMode = MQ_CLIENT_ACKNOWLEDGE;
            else if (acknowledgeMode == "transacted") m_acknowledgeMode = MQ_SESSION_TRANSACTED;
            
            input.get("autoDetectMessageFormat", m_autoDetectMessageFormat);

            // Establish the Jms connection (in stopped mode)
            MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;

            try {
                // Retrieve property handle
                MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));
                // Set all properties
                setConnectionProperties(propertiesHandle);
                // Initiate connection
                // Check if onException is a good idea
                MQ_SAFE_CALL(MQCreateConnection(propertiesHandle, m_username.c_str(), m_password.c_str(), NULL, &onException, NULL, &m_connectionHandle))
                        
            } catch (...) {
	      RETHROW_AS(OPENMQ_EXCEPTION("Propagating OpenMQ exception"));
            }
        }

        void JmsBrokerConnection::onException(const MQConnectionHandle connectionHandle, MQStatus status, void* callbackData) {
            MQString tmp = MQGetStatusString(status);
            std::string errorString(tmp);
            MQFreeString(tmp);
            throw MESSAGE_EXCEPTION(errorString);
        }

        BrokerChannel::Pointer JmsBrokerConnection::start() {
            MQ_SAFE_CALL(MQStartConnection(m_connectionHandle))
            return createChannel();
        }

        void JmsBrokerConnection::setConnectionProperties(const MQPropertiesHandle& propertiesHandle) {
            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle,
                    MQ_BROKER_HOST_PROPERTY, m_hostname.c_str()));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_BROKER_PORT_PROPERTY, m_port));
            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle,
                    MQ_CONNECTION_TYPE_PROPERTY, m_protocol.c_str()));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle,
                    MQ_PING_INTERVAL_PROPERTY, m_ping));
            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle,
                    MQ_SSL_BROKER_IS_TRUSTED, m_trustBroker));
            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle,
                    MQ_ACK_ON_PRODUCE_PROPERTY, m_acknowledgeSent));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle,
                    MQ_ACK_TIMEOUT_PROPERTY, m_acknowledgeTimeout));
        }

        void JmsBrokerConnection::stop() {
            try {
                MQ_SAFE_CALL(MQStopConnection(m_connectionHandle));
            } catch (...) {
                // Cleanup C garbage
                //MQFreeConnection(m_connectionHandle);
                RETHROW
            }
        }

        void JmsBrokerConnection::close() {
            try {
                MQ_SAFE_CALL(MQCloseConnection(m_connectionHandle));
                //MQFreeConnection(m_connectionHandle);
            } catch (...) {
                //MQFreeConnection(m_connectionHandle);
                RETHROW
            }
        }

        BrokerChannel::Pointer JmsBrokerConnection::createChannel() {
            BrokerChannel::Pointer channel(new JmsBrokerChannel(*this));
            registerChannel(channel);
            return channel;
        }


    } // namespace net
} // namespace karabo
