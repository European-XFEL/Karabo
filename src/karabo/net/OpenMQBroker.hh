/* 
 * File:   OpenMQBroker.hh
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 * Created on June 27, 2020, 9:10 PM
 */

#ifndef KARABO_NET_OPENMQBROKER_HH
#define	KARABO_NET_OPENMQBROKER_HH

#include <list>
#include <vector>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "karabo/util/ClassInfo.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/Schema.hh"
#include "utils.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/JmsConnection.hh"
#include "karabo/net/JmsConsumer.hh"
#include "karabo/net/JmsProducer.hh"
#include "EventLoop.hh"



namespace karabo {
    namespace net {

        class OpenMQBroker : public Broker {

        public:

            // Register under protocol alias  (tcp) instead of class name (OpenMQBroker) to generically choose
            // the broker implementation from the connection string and still stay backward compatible.
            KARABO_CLASSINFO(OpenMQBroker, "tcp", "1.0")
            
            static void expectedParameters(karabo::util::Schema& s);      

            OpenMQBroker(const karabo::util::Hash& configuration);

            OpenMQBroker(const OpenMQBroker& o);

            virtual ~OpenMQBroker();

            Broker::Pointer clone(const std::string& instanceId) override;

            void connect() override;

            void disconnect() override;

            bool isConnected() const override;

            std::string getBrokerUrl() const override;

            std::string getBrokerType() const override { return getClassInfo().getClassId(); }


            /**
             * There is no need to subscribe in OpenMQBroker case.  "Subscription"
             * (message filtering on the broker) happens via "properties" settings
             * in message header.
             *
             * @param signalInstanceId
             * @param signalFunction
             * @return boost error code
             */
            boost::system::error_code subscribeToRemoteSignal(
                    const std::string& signalInstanceId,
                    const std::string& signalFunction,
                    const consumer::MessageHandler& handler,
                    const consumer::ErrorNotifier& errorNotifier) override;

            /**
             * There is no need to un-subscribe in OpenMQBroker case.
             *
             * @param signalInstanceId
             * @param signalFunction
             * @return boost error code
             */
            boost::system::error_code unsubscribeFromRemoteSignal(
                    const std::string& signalInstanceId,
                    const std::string& signalFunction) override;

            /**
             * There is no need to subscribe in OpenMQBroker case.  "Subscription"
             * (message filtering on the broker) happens via "properties" settings
             * in message header.
             *
             * @param signalInstanceId
             * @param signalFunction
             * @param completionHandler  called when subscribing is done
             * @param handler            read message handler
             * @param errorNotifier      read error handler
             */
            void subscribeToRemoteSignalAsync(
                    const std::string& signalInstanceId,
                    const std::string& signalFunction,
                    const AsyncECHandler& completionHandler,
                    const consumer::MessageHandler& handler = consumer::MessageHandler(),
                    const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override {
                EventLoop::getIOService().post(boost::bind(
                        completionHandler,
                        boost::system::errc::make_error_code(boost::system::errc::success)));
            }

            /**
             * There is no need to un-subscribe in OpenMQBroker case.
             *
             * @param signalInstanceId
             * @param signalFunction
             * @param completionHandler
             */
            void unsubscribeFromRemoteSignalAsync(const std::string& signalInstanceId,
                                    const std::string& signalFunction,
                                    const AsyncECHandler& completionHandler) override {
                EventLoop::getIOService().post(boost::bind(
                        completionHandler,
                        boost::system::errc::make_error_code(boost::system::errc::success)));
            }

            /**
             * JMS subscription:
             * 'selector' is SQL-like expression on properties...
             *   "slotInstanceIds LIKE '%|" + m_instanceId + "|%' OR slotInstanceIds LIKE '%|*|%'"
             *                             specific subscription            global subscription
             *
             * @param handler       - success handler
             * @param errorNotifier - error handler
             */
            void startReading(const consumer::MessageHandler& handler,
                              const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override;

            void stopReading() override;

            /**
             * Heartbeat is used for tracking instances (tracking all instances or no tracking at all)
             *
             * JMS subscription
             * 'selector' is SQL-like logical expression on properties...
             *   "signalFunction = 'signalHeartbeat'"
             *
             * @param handler       - success handler
             * @param errorNotifier - error handler
             */
            void startReadingHeartbeats(const consumer::MessageHandler& handler,
                                const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override;

            void stopReadingHeartbeats() override;

            /**
             * JMS subscription.
             * 'selector' is SQL-like expression on properties (in header)
             *   "target = 'log'"
             *
             * @param messageHandler - message handler
             * @param errorNotifier - error handler
             */
            void startReadingLogs(const consumer::MessageHandler& messageHandler,
                          const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override;

            void stopReadingLogs() override;

            void write(const std::string& topic,
                       const karabo::util::Hash::Pointer& header,
                       const karabo::util::Hash::Pointer& body,
                       const int priority,
                       const int timeToLive) override;

            void writeLocal(const consumer::MessageHandler& handler,
                            const karabo::util::Hash::Pointer& header,
                            const karabo::util::Hash::Pointer& body) override;

        private:

            karabo::net::JmsConnection::Pointer m_connection;
            karabo::net::JmsProducer::Pointer m_producerChannel;
            karabo::net::JmsConsumer::Pointer m_consumerChannel;
            karabo::net::JmsConsumer::Pointer m_heartbeatConsumerChannel;
            karabo::net::JmsConsumer::Pointer m_logConsumerChannel;
        };

    }
}

#endif	/* KARABO_NET_OPENMQBROKER_HH */

