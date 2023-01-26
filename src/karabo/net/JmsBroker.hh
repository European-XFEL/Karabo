/*
 * File:   JmsBroker.hh
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 * Created on June 27, 2020, 9:10 PM
 */

#ifndef KARABO_NET_JMSBROKER_HH
#define KARABO_NET_JMSBROKER_HH

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <list>
#include <vector>

#include "EventLoop.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/JmsConnection.hh"
#include "karabo/net/JmsConsumer.hh"
#include "karabo/net/JmsProducer.hh"
#include "karabo/util/ClassInfo.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/Schema.hh"
#include "utils.hh"


namespace karabo {
    namespace net {

        class JmsBroker : public Broker {
           public:
            KARABO_CLASSINFO(JmsBroker, "jms", "1.0")

            static void expectedParameters(karabo::util::Schema& s);

            explicit JmsBroker(const karabo::util::Hash& configuration);

            virtual ~JmsBroker();

            Broker::Pointer clone(const std::string& instanceId) override;

            void connect() override;

            void disconnect() override;

            bool isConnected() const override;

            std::string getBrokerUrl() const override;

            std::string getBrokerType() const override {
                return getClassInfo().getClassId();
            }


            /**
             * There is no need to subscribe in OpenMQBroker case.  "Subscription"
             * (message filtering on the broker) happens via "properties" settings
             * in message header.
             *
             * @param signalInstanceId
             * @param signalFunction
             * @return boost error code
             */
            boost::system::error_code subscribeToRemoteSignal(const std::string& signalInstanceId,
                                                              const std::string& signalFunction) override;

            /**
             * There is no need to un-subscribe in OpenMQBroker case.
             *
             * @param signalInstanceId
             * @param signalFunction
             * @return boost error code
             */
            boost::system::error_code unsubscribeFromRemoteSignal(const std::string& signalInstanceId,
                                                                  const std::string& signalFunction) override;

            /**
             * There is no need to subscribe in OpenMQBroker case.  "Subscription"
             * (message filtering on the broker) happens via "properties" settings
             * in message header.
             *
             * @param signalInstanceId
             * @param signalFunction
             * @param completionHandler  called when subscribing is done
             */
            void subscribeToRemoteSignalAsync(const std::string& signalInstanceId, const std::string& signalFunction,
                                              const AsyncHandler& completionHandler) override {
                EventLoop::getIOService().post(boost::bind(
                      completionHandler, boost::system::errc::make_error_code(boost::system::errc::success)));
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
                                                  const AsyncHandler& completionHandler) override {
                EventLoop::getIOService().post(boost::bind(
                      completionHandler, boost::system::errc::make_error_code(boost::system::errc::success)));
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
            void startReadingHeartbeats(
                  const consumer::MessageHandler& handler,
                  const consumer::ErrorNotifier& errorNotifier = consumer::ErrorNotifier()) override;

            void write(const std::string& topic, const karabo::util::Hash::Pointer& header,
                       const karabo::util::Hash::Pointer& body, const int priority, const int timeToLive) override;

           private:
            JmsBroker(const JmsBroker& o) = delete;
            JmsBroker(const JmsBroker& o, const std::string& newInstanceId);

            karabo::net::JmsConnection::Pointer m_connection;
            karabo::net::JmsProducer::Pointer m_producerChannel;
            karabo::net::JmsConsumer::Pointer m_consumerChannel;
            karabo::net::JmsConsumer::Pointer m_heartbeatConsumerChannel;
        };

    } // namespace net
} // namespace karabo

#endif /* KARABO_NET_JMSBROKER_HH */
