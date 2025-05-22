/*
 * File:   Broker.hh
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
 * Created on May 4, 2020, 8:27 AM
 */

#ifndef KARABO_NET_BROKER_HH
#define KARABO_NET_BROKER_HH

#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/ClassInfo.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/net/utils.hh"


namespace karabo {
    namespace net {


        namespace consumer {

            enum class Error {

                drop = 0, /// messages have been dropped
                type,     /// message of wrong type (i.e. non binary format, serialisation failure, ...) received and
                          /// dropped
                unknown   /// status reported is not specially treated or unknown
            };
            // TODO: Why not 'const karabo::data::Hash::Pointer&'?
            using MessageHandler = // slot, isBroadcast, header, body
                  std::function<void(const std::string&, bool, karabo::data::Hash::Pointer,
                                     karabo::data::Hash::Pointer)>;

            using ErrorNotifier = std::function<void(Error, const std::string& description)>;
        } // namespace consumer


        class Broker : public std::enable_shared_from_this<Broker> {
           public:
            KARABO_CLASSINFO(Broker, "Broker", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::data::Schema& s);

            Broker(const karabo::data::Hash& configuration);

            virtual ~Broker();

            /**
             * The function creates broker communication object with the new identity
             * by cloning the parent object.  Concrete meaning of cloning  strategy
             * is an implementation detail.
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
             * Example: "AmqpBroker"
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
             * It influences on subscription to such messages, i.e. has to be called before startReading(..)
             * @param consumeBroadcasts true means subscription
             */
            void setConsumeBroadcasts(bool consumeBroadcasts) {
                m_consumeBroadcasts = consumeBroadcasts;
            }

            /**
             * Establish logical signal-slot connection between 2 devices that
             * is required by used protocol for registration
             * @param slot that should be called for messages from the given signal
             * @param signalInstanceId device instance ID of a signal
             * @param signalFunction   signal name
             */
            virtual boost::system::error_code subscribeToRemoteSignal(const std::string& slot,
                                                                      const std::string& signalInstanceId,
                                                                      const std::string& signalFunction) = 0;

            /**
             * Close logical signal-slot connection.  De-registration in broker
             * specific API.
             * @param slot that should have been called for messages from the given signal
             * @param signalInstanceId
             * @param signalFunction
             */
            virtual boost::system::error_code unsubscribeFromRemoteSignal(const std::string& slot,
                                                                          const std::string& signalInstanceId,
                                                                          const std::string& signalFunction) = 0;

            /**
             * Establish signal-slot connection asynchronously
             * @param slot that should be called for messages from the given signal
             * @param signalInstanceId
             * @param signalFunction
             * @param completionHandler this callback is called when complete
             */
            virtual void subscribeToRemoteSignalAsync(const std::string& slot, const std::string& signalInstanceId,
                                                      const std::string& signalFunction,
                                                      const AsyncHandler& completionHandler) = 0;

            /**
             * Unsubscribe from (remote) signal asynchronously
             * @param slot that should have been called for messages fromn the given signal
             * @param signalInstanceId
             * @param signalFunction
             * @param completionHandler
             */
            virtual void unsubscribeFromRemoteSignalAsync(const std::string& slot, const std::string& signalInstanceId,
                                                          const std::string& signalFunction,
                                                          const AsyncHandler& completionHandler) = 0;

            /**
             * Set up handlers for processing messages arriving via main communication path
             * @param handler       - read handler
             * @param errorNotifier - error handler
             */
            virtual void startReading(const consumer::MessageHandler& handler,
                                      const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) = 0;

            /**
             * Stop processing messages coming via main path
             */
            virtual void stopReading() = 0;

            /**
             * Heartbeat is used for tracking instances (tracking all instances or no tracking at all)
             *
             * Must be called after startReading, heartbeats are fed to the same handler
             */
            virtual void startReadingHeartbeats() = 0;

            /**
             * Send a signal message
             *
             * @param signal the signal
             * @param header message header
             * @param body message body
             */
            virtual void sendSignal(const std::string& signal, const karabo::data::Hash::Pointer& header,
                                    const karabo::data::Hash::Pointer& body) = 0;

            /**
             * Send a broadcast message
             *
             * @param slot the slot to call
             * @param header message header
             * @param body message body
             */
            virtual void sendBroadcast(const std::string& slot, const karabo::data::Hash::Pointer& header,
                                       const karabo::data::Hash::Pointer& body) = 0;

            /**
             * Send a 1-to-1 message
             *
             * @param receiverId the instance id of the addressee
             * @param slot the addressee's slot to call
             * @param header message header
             * @param body message body
             */
            virtual void sendOneToOne(const std::string& receiverId, const std::string& slot,
                                      const karabo::data::Hash::Pointer& header,
                                      const karabo::data::Hash::Pointer& body) = 0;

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
             *  Specifies the broker type as the protocol of the given broker URLs.
             *  Throws LogicException if broker addresses specified with different types or without protocol.
             */
            static std::string brokerTypeFrom(const std::vector<std::string>& urls);


            /**
             * Specify broker domain (topic) from environment variables.
             *
             * First source is KARABO_BROKER_TOPIC, as a fall back the environment variables
             * LOGNAME, USER, LNAME and USERNAME are checked in that order.
             */
            static std::string brokerDomainFromEnv();

           protected:
            Broker(const Broker& o) = delete;
            Broker(const Broker& o, const std::string& newInstanceId);

            std::vector<std::string> m_availableBrokerUrls;
            const std::string m_topic;
            std::string m_instanceId;
            bool m_consumeBroadcasts;
            static const std::vector<std::string>
                  m_broadcastSlots; // Accepted broadcast slots (besides "slotHeartbeat" )
            consumer::MessageHandler m_messageHandler;
            consumer::ErrorNotifier m_errorNotifier;
        };
    } // namespace net
} // namespace karabo

#endif /* KARABO_NET_BROKER_HH */
