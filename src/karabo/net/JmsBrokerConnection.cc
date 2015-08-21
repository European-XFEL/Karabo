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
#include "karabo/log/Logger.hh"
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace net {


        KARABO_REGISTER_FOR_CONFIGURATION(BrokerConnection, JmsBrokerConnection)

        static const MQConnectionHandle invalidConnection = MQ_INVALID_HANDLE;

        JmsBrokerConnection::~JmsBrokerConnection() {
            //cout << "****** " << BOOST_CURRENT_FUNCTION << " ENTRY ******" << endl;
            close();
            //cout << "****** " << BOOST_CURRENT_FUNCTION << " EXIT  ******" << endl;
        }


        void JmsBrokerConnection::close() {
            //cout << "****** " << BOOST_CURRENT_FUNCTION << " ENTRY ******" << endl;
            m_channels.clear();
            MQCloseConnection(m_connectionHandle);
            MQFreeConnection(m_connectionHandle);
            m_connectionHandle.handle = invalidConnection.handle;
            //cout << "****** " << BOOST_CURRENT_FUNCTION << " EXIT  ******" << endl;
        }


        void JmsBrokerConnection::expectedParameters(Schema& expected) {

            // Some tricks with environment here
            char* env = 0;
            string defaultBrokerHosts = ""; // no defaults; it serves as a flag for clustering setup
            string defaultHostname = "exfl-broker.desy.de:7777";
            unsigned int defaultPort = 7777;
            string defaultTopic = string(getenv("USER"));
            
            env = getenv("KARABO_BROKER_PORT");
            if (env != 0) defaultPort = fromString<unsigned int>(string(env));
            env = getenv("KARABO_BROKER_HOST");
            if (env != 0) defaultHostname = string(env);
            env = getenv("KARABO_BROKER_HOSTS");
            if (env != 0) {
                defaultBrokerHosts = string(env); // List of host1:port1, host2:port2,  ...
                vector<string> hostports = fromString<string, vector>(defaultBrokerHosts);
                vector<string> hostport = fromString<string, vector>(hostports[0], ":");
                if (hostport[0] != "") defaultHostname = hostport[0];
                if (hostport.size() >= 2 && hostport[1] != "") defaultPort = fromString<unsigned int>(hostport[1]);
            }
            {
                vector<string> hostport = fromString<string, vector>(defaultHostname, ":");
                if (hostport.size() < 2)
                    defaultHostname += ":" + toString(defaultPort);
                else if (hostport[1] == "")
                    defaultHostname += toString(defaultPort);
            }
            env = getenv("KARABO_BROKER_TOPIC");
            if (env != 0) defaultTopic = string(env);

            VECTOR_STRING_ELEMENT(expected)
                    .key("brokerHosts")
                    .displayedName("Broker hosts")
                    .description("List of brokers participating in cluster setup.")
                    .assignmentOptional().defaultValueFromString(defaultBrokerHosts)
                    .commit();

            STRING_ELEMENT(expected)
                    .key("hostname")
                    .displayedName("Broker Hostname")
                    .description("Broker Hostname:Port")
                    .assignmentOptional().defaultValue(defaultHostname)
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("port")
                    .displayedName("Broker Hostport")
                    .description("Broker Hostport")
                    .assignmentOptional().defaultValue(defaultPort)
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
                    .expertAccess()
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("ping")
                    .displayedName("Ping")
                    .description("Client ping interval to test whether the connection to the broker is still alive [seconds]")
                    .assignmentOptional().defaultValue(20)
                    .minInc(1)
                    .expertAccess()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("trustBroker")
                    .displayedName("Trust Broker")
                    .description("Should the broker certificate be trusted?")
                    .assignmentOptional().defaultValue(true)
                    .expertAccess()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("acknowledgeSent")
                    .displayedName("Acknowledge Message Sent")
                    .description("Should senders be blocked until the broker acknowledges message receipt?")
                    .assignmentOptional().defaultValue(false)
                    .expertAccess()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("deliveryInhibition")
                    .displayedName("Message Self Delivery Inhibition")
                    .description("If true, messages delivered to the broker on the same topic and connection will not be consumed.")
                    .assignmentOptional().defaultValue(false)
                    .expertAccess()
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("acknowledgeTimeout")
                    .displayedName("Acknowledge Timeout")
                    .description("Maximum waiting time for any broker acknowledge")
                    .assignmentOptional().defaultValue(0)
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("acknowledgeMode")
                    .displayedName("Acknowledge Mode")
                    .description("General Acknowledge Mode")
                    .assignmentOptional().defaultValue("explicit")
                    .options("dupsOk,auto,explicit,transacted")
                    .expertAccess()
                    .commit();

            INT32_ELEMENT(expected)
                    .key("messageTimeToLive")
                    .displayedName("Message Time to Live")
                    .description("Time to live for an individual message send by a producer (0 = unlimited) [ms]")
                    .assignmentOptional().defaultValue(30000)
                    .minInc(0)
                    .expertAccess()
                    .commit();

            INT32_ELEMENT(expected)
                    .key("compressionUsageThreshold")
                    .displayedName("Compression Usage Threshold")
                    .description("The limit size to decide about applying a compression to the message. '-1' means 'compression is off'.")
                    .reconfigurable()
                    .unit(Unit::BYTE)
                    .assignmentOptional().defaultValue(-1)
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("compression")
                    .displayedName("Compression")
                    .description("Compression library used")
                    .init()
                    .assignmentOptional().defaultValue(string("snappy"))
                    .options("snappy")
                    .expertAccess()
                    .commit();

            // TODO See whether to include also flow control parameters

        }


        JmsBrokerConnection::JmsBrokerConnection(const karabo::util::Hash& input)
        : BrokerConnection(input)
        , m_hasConnection(false)
        , m_closeOldConnection(false)
        {
            m_connectionHandle.handle = invalidConnection.handle;
            this->setIOServiceType("Jms"); // Defines the type of the composed IO service within abstract IO service

            input.get("brokerHosts", m_brokerHosts);
            if (m_brokerHosts.empty()) {
                // Standalone broker setup
                m_clusterMode = false;
                m_brokerHosts.push_back(input.get<string>("hostname"));
            } else {
                // Cluster broker setup.
                m_clusterMode = true;
            }

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
            input.get("compressionUsageThreshold", m_compressionUsageThreshold);
            input.get("compression", m_compression);

            string acknowledgeMode;
            input.get("acknowledgeMode", acknowledgeMode);
            if (acknowledgeMode == "auto") m_acknowledgeMode = MQ_AUTO_ACKNOWLEDGE;
            else if (acknowledgeMode == "explicit") m_acknowledgeMode = MQ_CLIENT_ACKNOWLEDGE;
            else if (acknowledgeMode == "transacted") m_acknowledgeMode = MQ_SESSION_TRANSACTED;
            else if (acknowledgeMode == "dupsOk") m_acknowledgeMode = MQ_DUPS_OK_ACKNOWLEDGE;

            try {
                connectToBrokers();
            } catch (const SystemException& e) {
                KARABO_RETHROW
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_OPENMQ_EXCEPTION("Problems whilst connecting to broker"));
            }
        }


        void JmsBrokerConnection::connectToBrokers() {
            // The lock below is vital, because this code is executed on many threads
            boost::mutex::scoped_lock lock(m_connectionHandleMutex);
            if (m_closeOldConnection) {
                MQCloseConnection(m_connectionHandle);
                MQFreeConnection(m_connectionHandle);
                m_closeOldConnection = false;
                m_hasConnection = false;
            }
            if (!m_hasConnection) {
                if (m_clusterMode)
                    connectCluster();
                else {
                    connectStandalone();
                }

                MQ_SAFE_CALL(MQStartConnection(m_connectionHandle));
                m_hasConnection = true;
                for (set<BrokerChannel::Pointer>::iterator it = m_channels.begin(); it != m_channels.end(); it++) {
                    boost::shared_ptr<JmsBrokerChannel> p = boost::dynamic_pointer_cast<JmsBrokerChannel>(*it);
                    p->setSessionFalse();
                }
            }
        }


        void JmsBrokerConnection::connectStandalone() {
            // Establish the Jms connection (in stopped mode)
            MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;

            try {
                vector<string> hostport = fromString<string, vector>(m_brokerHosts[0], ":");
                if (hostport.size() == 2) {
                    m_hostname = hostport[0] == "" ? "exfl-broker.desy.de" : hostport[0];
                    m_port = hostport[1] == "" ? 7777 : fromString<unsigned int>(hostport[1]);
                } else {
                    m_hostname = hostport[0] == "" ? "exfl-broker.desy.de" : hostport[0];
                    m_port = 7777;
                }
                // Retrieve property handle
                MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));
                // Set all properties
                setConnectionProperties(propertiesHandle);
                // Initiate connection
                // Check if onException is a good idea
                m_connectionHandle.handle = invalidConnection.handle;
                MQ_SAFE_CALL(MQCreateConnection(propertiesHandle, m_username.c_str(), m_password.c_str(), NULL, &onException, this, &m_connectionHandle));
            } catch (...) {
                throw KARABO_SYSTEM_EXCEPTION("Cannot connect to the broker with given parameters. Exit...");
            }
        }


        void JmsBrokerConnection::connectCluster() {
            // Establish the Jms connection (in stopped mode)
            MQPropertiesHandle propertiesHandle = MQ_INVALID_HANDLE;
            while (true) {
                for (size_t i = 0; i < m_brokerHosts.size(); ++i) {

                    vector<string> hostport = fromString<string, vector>(m_brokerHosts[i], ":");
                    if (hostport.size() == 2) {
                        m_hostname = hostport[0] == "" ? "exfl-broker.desy.de" : hostport[0];
                        m_port = hostport[1] == "" ? 7777 : fromString<unsigned int>(hostport[1]);
                    } else {
                        m_hostname = hostport[0] == "" ? "exfl-broker.desy.de" : hostport[0];
                        m_port = 7777;
                    }

                    // Retrieve property handle
                    MQ_SAFE_CALL(MQCreateProperties(&propertiesHandle));
                    // Set all properties
                    setConnectionProperties(propertiesHandle);
                    // Initiate connection
                    {
                        MQStatus status;
                        boost::mutex::scoped_lock lock(m_openMQMutex);
                        m_connectionHandle.handle = invalidConnection.handle;
                        status = MQCreateConnection(propertiesHandle, m_username.c_str(), m_password.c_str(), NULL, &onException, this, &m_connectionHandle);
                        if (MQStatusIsError(status) == MQ_TRUE) {
                            MQString tmp = MQGetStatusString(status);
                            std::string errorString(tmp);
                            MQFreeString(tmp);
                            KARABO_LOG_FRAMEWORK_WARN << errorString;
                        } else {
                            MQFreeProperties(propertiesHandle);
                            return;
                        }
                    }
                }
                boost::this_thread::sleep(boost::posix_time::seconds(10));
            }
        }


        void JmsBrokerConnection::onException(const MQConnectionHandle connectionHandle, MQStatus status, void* callbackData) {
            JmsBrokerConnection* that = reinterpret_cast<JmsBrokerConnection*> (callbackData);
            string host = that->getBrokerHostname() + ":" + toString(that->getBrokerPort());
            MQError code = MQGetStatusCode(status);
            {
                MQString tmp = MQGetStatusString(status);
                std::string errorString(tmp);
                MQFreeString(tmp);
                KARABO_LOG_FRAMEWORK_ERROR << "Current broker \"" << host << "\" is in trouble: " << errorString;
            }

            switch (code) {
                case MQ_BROKER_CONNECTION_CLOSED:
                case MQ_TCP_CONNECTION_CLOSED:
                    // Broker closed a connection:
                    //     If it stops gracefully one get MQ_BROKER_CONNECTION_CLOSED
                    //     if it is killed -9 one get MQ_TCP_CONNECTION_CLOSED
                    // Close the connection locally and free the resources. It should be done
                    // outside this handler, so activate "close connection flag".
                {
                    boost::mutex::scoped_lock lock(that->m_connectionHandleMutex);
                    that->m_closeOldConnection = true;
                    break;
                }
                default:
                    break;
            }
        }


        void JmsBrokerConnection::setConnectionProperties(const MQPropertiesHandle& propertiesHandle) {
            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle, MQ_BROKER_HOST_PROPERTY, m_hostname.c_str()));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_BROKER_PORT_PROPERTY, m_port));
            MQ_SAFE_CALL(MQSetStringProperty(propertiesHandle, MQ_CONNECTION_TYPE_PROPERTY, m_protocol.c_str()));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_PING_INTERVAL_PROPERTY, m_ping));
            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle, MQ_SSL_BROKER_IS_TRUSTED, m_trustBroker));
            MQ_SAFE_CALL(MQSetBoolProperty(propertiesHandle, MQ_ACK_ON_PRODUCE_PROPERTY, m_acknowledgeSent));
            MQ_SAFE_CALL(MQSetInt32Property(propertiesHandle, MQ_ACK_TIMEOUT_PROPERTY, m_acknowledgeTimeout));
        }


        void JmsBrokerConnection::start() {
        }


        void JmsBrokerConnection::stop() {
            try {
                MQ_SAFE_CALL(MQStopConnection(m_connectionHandle));
            } catch (...) {
                // Cleanup C garbage
                //MQFreeConnection(m_connectionHandle);
                KARABO_RETHROW
            }
        }


        const std::string& JmsBrokerConnection::getBrokerHostname() const {
            return m_hostname;
        }


        unsigned int JmsBrokerConnection::getBrokerPort() const {
            return m_port;
        }


        const std::string& JmsBrokerConnection::getBrokerTopic() const {
            return m_destinationName;
        }


        const std::vector<std::string>& JmsBrokerConnection::getBrokerHosts() const {
            return m_brokerHosts;
        }


        BrokerChannel::Pointer JmsBrokerConnection::createChannel(const std::string& subDestination) {
            BrokerChannel::Pointer channel(new JmsBrokerChannel(shared_from_this(), subDestination));
            m_channels.insert(channel); // add this additional bookkeeping needed for live reconnection on the fly
            return channel;
        }

    } // namespace net
} // namespace karabo
