/*
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
 */
#include "karabo/net/JmsBroker.hh"

#include <karabo/log/Logger.hh>

#include "karabo/net/JmsConnection.hh"
#include "karabo/net/utils.hh"


using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Broker, karabo::net::JmsBroker)

namespace karabo {
    namespace net {


        void JmsBroker::expectedParameters(karabo::util::Schema& s) {}


        JmsBroker::JmsBroker(const karabo::util::Hash& config)
            : Broker(config), m_connection(), m_producerChannel(), m_consumerChannel(), m_heartbeatConsumerChannel() {
            Hash jmsConfig("brokers", m_availableBrokerUrls);
            m_connection = Configurator<JmsConnection>::create("JmsConnection", jmsConfig);
        }


        JmsBroker::~JmsBroker() {}


        JmsBroker::JmsBroker(const JmsBroker& o, const std::string& newInstanceId)
            : Broker(o, newInstanceId),
              m_connection(o.m_connection),
              m_producerChannel(),
              m_consumerChannel(),
              m_heartbeatConsumerChannel() {}


        Broker::Pointer JmsBroker::clone(const std::string& instanceId) {
            return Broker::Pointer(new JmsBroker(*this, instanceId));
        }


        void JmsBroker::connect() {
            if (!m_connection) {
                std::ostringstream oss;
                oss << "Broker::connect : JMS connection pointer is not initialized";
                throw KARABO_OPENMQ_EXCEPTION(oss.str());
            }
            if (!m_connection->isConnected()) m_connection->connect();
        }


        void JmsBroker::disconnect() {}


        bool JmsBroker::isConnected() const {
            return (m_connection && m_connection->isConnected());
        }


        std::string JmsBroker::getBrokerUrl() const {
            return m_connection->getBrokerUrl();
        }


        boost::system::error_code JmsBroker::subscribeToRemoteSignal(const std::string& signalInstanceId,
                                                                     const std::string& signalFunction) {
            return boost::system::errc::make_error_code(boost::system::errc::success);
        }


        boost::system::error_code JmsBroker::unsubscribeFromRemoteSignal(const std::string& signalInstanceId,
                                                                         const std::string& signalFunction) {
            return boost::system::errc::make_error_code(boost::system::errc::success);
        }


        void JmsBroker::write(const std::string& target, const karabo::util::Hash::Pointer& header,
                              const karabo::util::Hash::Pointer& body, const int priority, const int timeToLive) {
            KARABO_LOG_FRAMEWORK_TRACE << "*** write TARGET = \"" << target << "\"...\n... and HEADER is \n" << *header;

            if (!m_producerChannel) m_producerChannel = m_connection->createProducer();

            m_producerChannel->write(target, header, body, priority, timeToLive);
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
        void JmsBroker::startReading(const consumer::MessageHandler& handler,
                                     const consumer::ErrorNotifier& errorNotifier) {
            if (!m_consumerChannel) {
                std::string selector("slotInstanceIds LIKE '%|" + m_instanceId + "|%'");
                if (m_consumeBroadcasts) {
                    selector += " OR slotInstanceIds LIKE '%|*|%'";
                }
                m_consumerChannel = m_connection->createConsumer(m_topic, selector);
            }
            m_messageHandler = handler;
            m_errorNotifier = errorNotifier;
            m_consumerChannel->startReading(m_messageHandler, m_errorNotifier);
        }


        void JmsBroker::stopReading() {
            if (m_consumerChannel) m_consumerChannel->stopReading();
            if (m_heartbeatConsumerChannel) m_heartbeatConsumerChannel->stopReading();
        }


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
        void JmsBroker::startReadingHeartbeats(const consumer::MessageHandler& handler,
                                               const consumer::ErrorNotifier& errorNotifier) {
            if (!m_heartbeatConsumerChannel) {
                std::string selector = "signalFunction = 'signalHeartbeat'";
                m_heartbeatConsumerChannel = m_connection->createConsumer(m_topic + "_beats", selector);
            }
            m_heartbeatConsumerChannel->startReading(handler, errorNotifier);
        }

    } // namespace net
} // namespace karabo
