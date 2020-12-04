/* 
 * File:   Broker.hh
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 * Created on May 4, 2020, 8:27 AM
 */

#ifndef KARABO_NET_BROKER_HH
#define	KARABO_NET_BROKER_HH

#include <list>
#include <vector>
#include "karabo/util/ClassInfo.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/Schema.hh"
#include "karabo/net/utils.hh"

#include <boost/enable_shared_from_this.hpp>


namespace karabo {
    namespace net {


        namespace consumer {

            enum class Error {

                drop = 0, /// messages have been dropped
                type, /// message of wrong type (i.e. non binary format) received and dropped
                unknown /// status reported by openmqc that is not specially treated
            };

            typedef boost::function< void (karabo::util::Hash::Pointer, karabo::util::Hash::Pointer) > MessageHandler;

            typedef boost::function< void (Error, const std::string& description) > ErrorNotifier;
        }


        class Broker : public boost::enable_shared_from_this<Broker>  {
        public:

            KARABO_CLASSINFO(Broker, "Broker", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS
            
            static void expectedParameters(karabo::util::Schema& s);      

            Broker(const karabo::util::Hash& configuration);

            Broker(const Broker& o);

            virtual ~Broker();

            /**
             * The function creates broker communication object with the new identity
             * by cloning the parent object.  Concrete meaning of cloning  strategy
             * is an implementation detail. Possible strategies might:
             *  - use the same physical connection but new session (identity) in case of JMS
             *  - build the new physical connection (and new identity) but using connection
             *    parameters of parent object in case of MQTT
             * @param instanceId - unique ID
             * @return new broker communication object
             */
            virtual Broker::Pointer clone(const std::string& instanceId) = 0;

            /**
             * This function establishes  connection to the broker
             * otherwise throws exception
             */
            virtual void connect() = 0;

            /**
             * Close broker connection
             */
            virtual void disconnect() = 0;

            /**
             * Predicate to check broker connection status
             * @return true if connection is open
             */
            virtual bool isConnected() const = 0;

            /**
             * Get active URI used for establishing connection to broker
             * @return uri like "mqtt://localhost:1883"
             */
            virtual std::string getBrokerUrl() const = 0;

            /**
             * Get type string identifying broker.
             * Example: "OpenMQBroker"
             * @return the type defined by active uri
             */
            virtual std::string getBrokerType() const = 0;

            /**
             * Get current instance ID associated with this broker object
             * @return instanceId
             */
            const std::string& getInstanceId() const {
                return m_instanceId;
            }

            /**
             * Get the domain this broker is communicating to
             * @return domain
             */
            const std::string& getDomain() const {
                return m_topic;
            }

            /**
             * Set flag defining the way how to handle broadcast messages.
             * It influences on subscription to such messages
             * @param consumeBroadcasts true means subscription
             */
            void setConsumeBroadcasts(bool consumeBroadcasts) {
                m_consumeBroadcasts = consumeBroadcasts;
            }

            /**
             * Establish logical signal-slot connection between 2 devices that
             * is required by used protocol for registration
             * @param signalInstanceId device instance ID of a signal
             * @param signalFunction   signal name
             */
            virtual boost::system::error_code subscribeToRemoteSignal(
                    const std::string& signalInstanceId,
                    const std::string& signalFunction) = 0;

            /**
             * Close logical signal-slot connection.  De-registration in broker
             * specific API.
             * @param signalInstanceId
             * @param signalFunction
             */
            virtual boost::system::error_code unsubscribeFromRemoteSignal(
                    const std::string& signalInstanceId,
                    const std::string& signalFunction) = 0;

            /**
             * Establish signal-slot connection asynchronously 
             * @param signalInstanceId
             * @param signalFunction
             * @param completionHandler this callback is called when complete
             */
            virtual void subscribeToRemoteSignalAsync(
                    const std::string& signalInstanceId,
                    const std::string& signalFunction,
                    const AsyncHandler& completionHandler) = 0;

            /**
             * Unsubscribe from (remote) signal asynchronously
             * @param signalInstanceId
             * @param signalFunction
             * @param completionHandler
             */
            virtual void unsubscribeFromRemoteSignalAsync(
                    const std::string& signalInstanceId,
                    const std::string& signalFunction,
                    const AsyncHandler& completionHandler) = 0;

            /**
             * Set up handlers for processing messages arriving via main communication path
             * @param handler       - read handler
             * @param errorNotifier - error handler
             */
            virtual void startReading(const consumer::MessageHandler& handler,
                                      const consumer::ErrorNotifier& errorNotifier
                                          = consumer::ErrorNotifier()) = 0;

            /**
             * Stop processing messages coming via main path
             */
            virtual void stopReading() = 0;

            /**
             * Set up handlers for processing heartbeat messages arriving via special path.
             *
             * Heartbeat is used for tracking instances (tracking all instances or no tracking at all)
             *
             * @param handler       - read message handler
             * @param errorNotifier - error handler
             */
            virtual void startReadingHeartbeats(const consumer::MessageHandler& handler,
                                        const consumer::ErrorNotifier& errorNotifier) = 0;

            /**
             * Stop processing heartbeat messages
             */
            virtual void stopReadingHeartbeats() = 0;

            /**
             * Set up handlers for processing log messages using special path.
             *
             * @param handler       - read message handler
             * @param errorNotifier - error handler
             */
            virtual void startReadingLogs(const consumer::MessageHandler& handler,
                                  const consumer::ErrorNotifier& errorNotifier) = 0;

            /**
             * Stop processing of log messages
             */
            virtual void stopReadingLogs() = 0;

            /**
             * Send message to broker
             *
             * @param topic
             * @param header
             * @param body
             * @param priority
             * @param timeToLive
             */
            virtual void write(const std::string& topic,
                               const karabo::util::Hash::Pointer& header,
                               const karabo::util::Hash::Pointer& body,
                               const int priority,
                               const int timeToLive) = 0;

            /**
             * Send message locally (same process) to another device
             * @param handler  consumer handler of "foreign" device
             * @param header   message header (routing information)
             * @param body     message body
             */
            virtual void writeLocal(const consumer::MessageHandler& handler,
                                    const karabo::util::Hash::Pointer& header,
                                    const karabo::util::Hash::Pointer& body) = 0;

            /**
             *  Specifies the string of broker URLs from the environment variable KARABO_BROKER.
             *  If KARABO_BROKER is not defined, uses a hard coded fallback.
             */
            static std::vector<std::string> brokersFromEnv();

            /**
             *  Specifies the broker type as the protocol of the broker URLs defined by brokersFromEnv().
             *  Throws LogicException if broker addresses specified with different types or without protocol.
             */
            static std::string brokerTypeFromEnv();

            /**
             * Specify broker domain (i.e. topic for OpenMQBroker) from environment variables.
             *
             * First source is KARABO_BROKER_TOPIC, as a fall back the environment variables
             * LOGNAME, USER, LNAME and USERNAME are checked in that order.
             */
            static std::string brokerDomainFromEnv();

        protected:

            std::vector<std::string> m_availableBrokerUrls;
            const std::string m_topic;
            std::string m_instanceId;
            bool m_consumeBroadcasts;
            consumer::MessageHandler m_messageHandler;
            consumer::ErrorNotifier m_errorNotifier;
        };
    }
}

#endif	/* KARABO_NET_BROKER_HH */

