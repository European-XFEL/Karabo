/*
 * $Id$
 *
 * File:   AmqpBroker.cc
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 * Created on May 19, 2020, 16:08 PM
 */

#include "karabo/net/AmqpBroker.hh"

#include <chrono>

#include "karabo/log/Logger.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/SimpleElement.hh"


using namespace karabo::util;


#define AMQP_CLIENT_CLASS "AmqpClient"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Broker, karabo::net::AmqpBroker)

namespace karabo {
    namespace net {


        void AmqpBroker::expectedParameters(karabo::util::Schema& s) {}


        AmqpBroker::AmqpBroker(const karabo::util::Hash& config)
            : Broker(config),
              m_client(),
              m_heartbeatConsumerChannel(),
              m_logConsumerChannel(),
              m_handlerStrand(boost::make_shared<Strand>(EventLoop::getIOService())),
              m_timestamp(double(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count())) {
            Hash amqpConfig("brokers", m_availableBrokerUrls);
            amqpConfig.set("instanceId", m_instanceId);
            amqpConfig.set("domain", m_topic);

            m_client = Configurator<AmqpClient>::create(AMQP_CLIENT_CLASS, amqpConfig);
        }


        AmqpBroker::~AmqpBroker() {
            stopReading();
            m_client.reset();
            m_heartbeatConsumerChannel.reset();
            m_logConsumerChannel.reset();
        }


        AmqpBroker::AmqpBroker(const AmqpBroker& o, const std::string& newInstanceId)
            : Broker(o, newInstanceId),
              m_client(Configurator<AmqpClient>::create(
                    AMQP_CLIENT_CLASS,
                    Hash("brokers", m_availableBrokerUrls, "instanceId", newInstanceId, "domain", m_topic))),
              m_handlerStrand(boost::make_shared<Strand>(EventLoop::getIOService())),
              m_timestamp(double(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count())) {}


        Broker::Pointer AmqpBroker::clone(const std::string& instanceId) {
            return Broker::Pointer(new AmqpBroker(*this, instanceId));
        }


        void AmqpBroker::connect() {
            if (!m_client->isConnected()) {
                boost::system::error_code ec = m_client->connect();
                if (ec) {
                    std::ostringstream oss;
                    oss << "Failed to connect to AMQP broker: code #" << ec.value() << " -- " << ec.message();
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }
        }


        void AmqpBroker::disconnect() {
            if (m_logConsumerChannel && m_logConsumerChannel->isConnected()) m_logConsumerChannel->disconnect();
            if (m_heartbeatConsumerChannel && m_heartbeatConsumerChannel->isConnected())
                m_heartbeatConsumerChannel->disconnect();
            if (m_client && m_client->isConnected()) m_client->disconnect();
        }


        bool AmqpBroker::isConnected() const {
            return (m_client && m_client->isConnected());
        }


        std::string AmqpBroker::getBrokerUrl() const {
            return m_client->getBrokerUrl();
        }


        void AmqpBroker::amqpReadHashHandler(const boost::system::error_code& ec,
                                             const std::string& exchange,   // publisher's exchange
                                             const std::string& routingkey, // publisher's routingkey
                                             const Hash::Pointer& msg, const consumer::MessageHandler& handler,
                                             const consumer::ErrorNotifier& errorNotifier) {
            if (m_client && m_client->isConnected() && !ec) {
                auto header = boost::make_shared<Hash>(msg->get<Hash>("header"));
                header->set("exchange", exchange);
                header->set("routingkey", routingkey);
                auto body = boost::make_shared<Hash>(msg->get<Hash>("body"));
                handler(header, body);
            } else {
                // Error ...
                std::ostringstream oss;
                oss << "Exchange -> \"" << exchange << "\", routing key -> \"" << routingkey << "\" : Error code #"
                    << ec.value() << " -- " << ec.message();
                if (errorNotifier) {
                    errorNotifier(consumer::Error::drop, oss.str()); // call error handler
                } else {
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }
        }


        boost::system::error_code AmqpBroker::subscribeToRemoteSignal(const std::string& signalInstanceId,
                                                                      const std::string& signalFunction) {
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            subscribeToRemoteSignalAsync(signalInstanceId, signalFunction,
                                         [prom](const boost::system::error_code& ec) { prom->set_value(ec); });
            return fut.get();
        }


        void AmqpBroker::subscribeToRemoteSignalAsync(const std::string& signalInstanceId,
                                                      const std::string& signalFunction,
                                                      const AsyncHandler& completionHandler) {
            const std::string exchange = m_topic + ".signals";
            const std::string bindingkey = signalInstanceId + "." + signalFunction;
            m_client->subscribeAsync(exchange, bindingkey, completionHandler);
        }


        boost::system::error_code AmqpBroker::unsubscribeFromRemoteSignal(const std::string& signalInstanceId,
                                                                          const std::string& signalFunction) {
            auto prom = std::make_shared<std::promise<boost::system::error_code> >();
            auto fut = prom->get_future();
            unsubscribeFromRemoteSignalAsync(signalInstanceId, signalFunction,
                                             [prom](const boost::system::error_code& ec) { prom->set_value(ec); });
            return fut.get();
        }


        void AmqpBroker::unsubscribeFromRemoteSignalAsync(const std::string& signalInstanceId,
                                                          const std::string& signalFunction,
                                                          const AsyncHandler& completionHandler) {
            if (!m_client || !m_client->isConnected()) {
                if (completionHandler) {
                    m_handlerStrand->post(boost::bind(completionHandler, KARABO_ERROR_CODE_NOT_CONNECTED));
                }
                return;
            }
            const std::string exchange = m_topic + ".signals";
            const std::string bindingkey = signalInstanceId + "." + signalFunction;
            m_client->unsubscribeAsync(exchange, bindingkey, completionHandler);
        }


        void AmqpBroker::write(const std::string& target, const karabo::util::Hash::Pointer& header,
                               const karabo::util::Hash::Pointer& body, const int priority, const int timeToLive) {
            if (!m_client || !m_client->isConnected()) {
                std::ostringstream oss;
                oss << "AmqpBroker.write: no broker connection.";
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
            if (!header) {
                throw KARABO_PARAMETER_EXCEPTION("AmqpBroker.write: header pointer is null");
            }

            KARABO_LOG_FRAMEWORK_TRACE << "*** write TARGET = \"" << target << "\", topic=\"" << m_topic
                                       << "\"...\n... and HEADER is \n"
                                       << *header;

            std::string exchange = "";
            std::string routingkey = "";

            if (target == m_topic + "_beats") {
                exchange = m_topic + ".signals";
                routingkey = m_instanceId + ".signalHeartbeat";

            } else if (target == "karaboGuiDebug") {
                exchange = "karaboGuiDebug";

            } else if (target == m_topic && header->has("target") && header->get<std::string>("target") == "log") {
                exchange = m_topic + ".log";

            } else if (target == m_topic) {
                if (!header->has("signalFunction")) {
                    throw KARABO_LOGIC_EXCEPTION("Header has to define \"signalFunction\"");
                }
                if (!header->has("slotInstanceIds")) {
                    throw KARABO_LOGIC_EXCEPTION("Header has to define \"slotInstanceIds\"");
                }

                const std::string& signalInstanceId = header->get<std::string>("signalInstanceId");
                const std::string& signalFunction = header->get<std::string>("signalFunction");
                if (signalInstanceId != m_instanceId) {
                    std::ostringstream oss;
                    oss << "Cannot publish \"" << signalFunction << "\" from \"" << m_instanceId << "\": "
                        << "the signalInstanceId should be \"" << signalInstanceId << "\"!";
                    throw KARABO_LOGIC_EXCEPTION(oss.str());
                }
                std::string slotInstanceIds = header->get<std::string>("slotInstanceIds");
                // strip possible vertical lines ...   ("__none__" is without '|')
                if (slotInstanceIds.at(0) == '|' && slotInstanceIds.at(slotInstanceIds.size() - 1) == '|') {
                    slotInstanceIds = slotInstanceIds.substr(1, slotInstanceIds.size() - 2); // trim | (vertical line)
                }

                if (signalFunction == "__call__" && slotInstanceIds == "*") {
                    // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                    // 'signalFunction' => __call__ STRING
                    // 'slotInstanceIds' => |*| STRING
                    // 'slotFunctions' => |*:slotInstanceNew| STRING

                    // NOTE: broadcast messages are not used for serial number counting...
                    exchange = m_topic + ".global_slots";

                } else if (signalFunction == "__request__" ||
                           signalFunction == "__requestNoWait__"
                           // ************************** request **************************
                           // 'replyTo' => 38184c31-6a5a-4f9d-bc81-4d9ae754a16c STRING
                           // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                           // 'signalFunction' => __request__ STRING
                           // 'slotInstanceIds' => |Karabo_GuiServer_0| STRING
                           // 'slotFunctions' => |Karabo_GuiServer_0:slotPing| STRING
                           // ...  or ...
                           // 'replyInstanceIds' => |Karabo_GuiServer_0| STRING
                           // 'replyFunctions' => |Karabo_GuiServer_0:slotLoggerMap| STRING
                           // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                           // 'signalFunction' => __requestNoWait__ STRING
                           // 'slotInstanceIds' => |Karabo_DataLoggerManager_0| STRING
                           // 'slotFunctions' => |Karabo_DataLoggerManager_0:slotGetLoggerMap| STRING

                           || signalFunction == "__reply__" ||
                           signalFunction == "__replyNoWait__"
                           // ************************** reply **************************
                           // 'replyFrom' => 10c91a8f-abbf-47bd-82f5-b8201057e0e2 STRING
                           // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                           // 'signalFunction' => __reply__ STRING
                           // 'slotInstanceIds' => |Karabo_AlarmService| STRING
                           // ... or ...
                           // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                           // 'signalFunction' => __replyNoWait__ STRING
                           // 'slotInstanceIds' => |DataLogger-karabo/dataLogger| STRING
                           // 'slotFunctions' => |DataLogger-karabo/dataLogger:slotChanged| STRING

                           || signalFunction == "__call__") {
                    // ************************** call **************************
                    // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                    // 'signalFunction' => __call__ STRING
                    // 'slotInstanceIds' => |Karabo_AlarmService| STRING
                    // 'slotFunctions' => |Karabo_AlarmService:slotPingAnswer| STRING

                    const std::string& slotInstanceId = slotInstanceIds;
                    if (signalFunction == "__call__" && slotInstanceId.find("|") != std::string::npos) {
                        throw KARABO_LOGIC_EXCEPTION("Unexpected vertical line(|) in slotInstanceId=" + slotInstanceId);
                    }
                    exchange = m_topic + ".slots";
                    routingkey = slotInstanceId;


                } else {
                    // ************************** emit **************************
                    // signalFunction == "signalSomething"
                    // Example:
                    // 'signalInstanceId' => Karabo_GuiServer_0 STRING
                    // 'signalFunction' => signalChanged STRING
                    // 'slotInstanceIds' => |DataLogger-karabo/dataLogger||dataAggregator1| STRING
                    // 'slotFunctions' => |DataLogger-karabo/dataLogger:slotChanged||dataAggregator1:slotData| STRING //
                    // 'slotInstanceIds' => |DataLogger-karabo/dataLogger| STRING
                    // ...

                    exchange = m_topic + ".signals";
                    routingkey = signalInstanceId + "." + signalFunction;
                }
            }

            if (exchange.empty()) {
                throw KARABO_LOGIC_EXCEPTION("Attempt to 'write' to unknown target: \"" + target + "\"");
            }

            auto msg = boost::make_shared<Hash>("header", *header, "body", *body);

            try {
                this->publish(exchange, routingkey, msg);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        // This method is protected and virtual ... can be overridden in derived class
        void AmqpBroker::publish(const std::string& exchange, const std::string& routingkey,
                                 const karabo::util::Hash::Pointer& message) {
            boost::system::error_code ec;
            ec = m_client->publish(exchange, routingkey, message);
            if (ec) {
                std::ostringstream oss;
                oss << "\"" << m_instanceId << "\" : Failed to publish to \"" << exchange
                    << "\" exchange with routing key = \"" << routingkey << "\" : code #" << ec.value() << " -- "
                    << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void AmqpBroker::startReading(const consumer::MessageHandler& handler,
                                      const consumer::ErrorNotifier& errorNotifier) {
            std::string exchange = m_topic + ".slots";
            std::string bindingkey = m_instanceId;

            auto readHandler =
                  bind_weak(&AmqpBroker::amqpReadHashHandler, this, _1, _2, _3, _4, handler, errorNotifier);
            m_client->registerConsumerHandler(readHandler);

            boost::system::error_code ec = m_client->subscribe(exchange, bindingkey);
            if (!ec && m_consumeBroadcasts) {
                exchange = m_topic + ".global_slots";
                bindingkey = "";
                ec = m_client->subscribe(exchange, bindingkey);
            }
            if (ec) {
                std::ostringstream oss;
                oss << "Subscription to exchange -> \"" << exchange << "\", binding key -> \"" << bindingkey
                    << "\" failed: #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void AmqpBroker::stopReading() {
            if (m_topic.empty() || m_instanceId.empty()) return;
            m_client->close();
            if (m_heartbeatConsumerChannel) m_heartbeatConsumerChannel->close();
            if (m_logConsumerChannel) m_logConsumerChannel->close();
        }


        void AmqpBroker::startReadingHeartbeats(const consumer::MessageHandler& handler,
                                                const consumer::ErrorNotifier& errorNotifier) {
            if (!m_heartbeatConsumerChannel) {
                Hash amqpConfig("brokers", m_availableBrokerUrls);
                amqpConfig.set("instanceId", m_instanceId);
                amqpConfig.set("domain", m_topic);
                m_heartbeatConsumerChannel = Configurator<AmqpClient>::create(AMQP_CLIENT_CLASS, amqpConfig);
                boost::system::error_code ec = m_heartbeatConsumerChannel->connect();
                if (ec) {
                    std::ostringstream oss;
                    oss << "Failed to connect to AMQP broker: code #" << ec.value() << " -- " << ec.message();
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }

            const std::string exchange = m_topic + ".signals";
            const std::string bindingkey = "*.signalHeartbeat";

            auto readHandler =
                  bind_weak(&AmqpBroker::amqpReadHashHandler, this, _1, _2, _3, _4, handler, errorNotifier);
            m_heartbeatConsumerChannel->registerConsumerHandler(readHandler);

            boost::system::error_code ec = m_heartbeatConsumerChannel->subscribe(exchange, bindingkey);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to exchange -> \"" << exchange << "\", bindingkey->\"" << bindingkey
                    << "\" : code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void AmqpBroker::startReadingLogs(const consumer::MessageHandler& handler,
                                          const consumer::ErrorNotifier& errorNotifier) {
            if (!m_logConsumerChannel) {
                Hash amqpConfig("brokers", m_availableBrokerUrls);
                amqpConfig.set("instanceId", m_instanceId);
                amqpConfig.set("domain", m_topic);
                m_logConsumerChannel = Configurator<AmqpClient>::create(AMQP_CLIENT_CLASS, amqpConfig);
                boost::system::error_code ec = m_logConsumerChannel->connect();
                if (ec) {
                    std::ostringstream oss;
                    oss << "Failed to connect to AMQP broker: code #" << ec.value() << " -- " << ec.message();
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }

            const std::string exchange = m_topic + ".log";
            const std::string bindingkey = "";

            auto readHandler =
                  bind_weak(&AmqpBroker::amqpReadHashHandler, this, _1, _2, _3, _4, handler, errorNotifier);
            m_logConsumerChannel->registerConsumerHandler(readHandler);

            boost::system::error_code ec = m_logConsumerChannel->subscribe(exchange, bindingkey);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to exchange -> \"" << exchange << "\", bindingkey->\"" << bindingkey
                    << "\" : code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }

    } // namespace net
} // namespace karabo
