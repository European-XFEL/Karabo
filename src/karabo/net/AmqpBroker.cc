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
              m_clientConsumerHandler(),
              m_clientErrorNotifier(),
              m_heartbeatConsumerHandler(),
              m_heartbeatErrorNotifier(),
              m_logConsumerHandler(),
              m_logErrorNotifier(),
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
            if (m_client && m_client->isConnected()) m_client->disconnect();
            if (m_heartbeatClient && m_heartbeatClient->isConnected()) m_heartbeatClient->disconnect();
        }


        bool AmqpBroker::isConnected() const {
            return (m_client && m_client->isConnected());
        }


        std::string AmqpBroker::getBrokerUrl() const {
            return m_client->getBrokerUrl();
        }


        void AmqpBroker::amqpReadHashHandler(const boost::system::error_code& ec, const Hash::Pointer& msg,
                                             const consumer::MessageHandler& handler,
                                             const consumer::ErrorNotifier& errorNotifier) {
            if (!ec) {
                auto header = boost::make_shared<Hash>(msg->get<Hash>("header"));
                auto body = boost::make_shared<Hash>(msg->get<Hash>("body"));
                m_handlerStrand->post(boost::bind(handler, header, body));
            } else {
                // Error ...
                std::ostringstream oss;
                oss << "Message -> ...\n" << *msg << ": Error code #" << ec.value() << " -- " << ec.message();
                if (errorNotifier) {
                    errorNotifier(consumer::Error::drop, oss.str()); // call error handler
                } else {
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }
        }


        boost::system::error_code AmqpBroker::subscribeToRemoteSignal(const std::string& signalInstanceId,
                                                                      const std::string& signalFunction) {
            const std::string exchange = m_topic + ".signals";
            const std::string bindingKey = signalInstanceId + "." + signalFunction;
            return m_client->subscribe(exchange, bindingKey);
        }


        void AmqpBroker::subscribeToRemoteSignalAsync(const std::string& signalInstanceId,
                                                      const std::string& signalFunction,
                                                      const AsyncHandler& completionHandler) {
            const std::string exchange = m_topic + ".signals";
            const std::string bindingKey = signalInstanceId + "." + signalFunction;
            m_client->subscribeAsync(exchange, bindingKey, completionHandler);
        }


        boost::system::error_code AmqpBroker::unsubscribeFromRemoteSignal(const std::string& signalInstanceId,
                                                                          const std::string& signalFunction) {
            if (!m_client || !m_client->isConnected()) return KARABO_ERROR_CODE_SUCCESS;
            const std::string exchange = m_topic + ".signals";
            const std::string bindingKey = signalInstanceId + "." + signalFunction;
            return m_client->unsubscribe(exchange, bindingKey);
        }


        void AmqpBroker::unsubscribeFromRemoteSignalAsync(const std::string& signalInstanceId,
                                                          const std::string& signalFunction,
                                                          const AsyncHandler& completionHandler) {
            if (!m_client || !m_client->isConnected()) {
                if (completionHandler) {
                    m_handlerStrand->post(boost::bind(completionHandler, KARABO_ERROR_CODE_SUCCESS));
                }
                return;
            }
            const std::string exchange = m_topic + ".signals";
            const std::string bindingkey = signalInstanceId + "." + signalFunction;
            m_client->unsubscribeAsync(exchange, bindingkey, completionHandler);
        }


        void AmqpBroker::write(const std::string& target, const karabo::util::Hash::Pointer& header,
                               const karabo::util::Hash::Pointer& body, const int priority, const int timeToLive) {
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
                // Use m_heartbeatClient if exists ...
                if (m_heartbeatClient) {
                    auto msg = boost::make_shared<Hash>("header", *header, "body", *body);
                    boost::system::error_code ec = m_heartbeatClient->publish(exchange, routingkey, msg);
                    if (ec) {
                        std::ostringstream oss;
                        oss << "\"" << m_instanceId << "\" : Failed to publish heartbeat msg to \"" << exchange
                            << "\" exchange with routing key = \"" << routingkey << "\" : code #" << ec.value()
                            << " -- " << ec.message();
                        throw KARABO_NETWORK_EXCEPTION(oss.str());
                    }
                    return;
                }
            } else if (target == "karaboGuiDebug") {
                exchange = "karaboGuiDebug";

            } else if (target == m_topic && header->has("target") && header->get<std::string>("target") == "log") {
                exchange = m_topic + ".log";
                // Use m_logClient if exists ...
                if (m_logClient) {
                    auto msg = boost::make_shared<Hash>("header", *header, "body", *body);
                    boost::system::error_code ec = m_logClient->publish(exchange, routingkey, msg);
                    if (ec) {
                        std::ostringstream oss;
                        oss << "\"" << m_instanceId << "\" : Failed to publish log msg to \"" << exchange
                            << "\" exchange with routing key = \"" << routingkey << "\" : code #" << ec.value()
                            << " -- " << ec.message();
                        throw KARABO_NETWORK_EXCEPTION(oss.str());
                    }
                    return;
                }

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
            m_clientConsumerHandler = handler;
            m_clientErrorNotifier = errorNotifier;

            std::string exchange = m_topic + ".slots";
            std::string bindingKey = m_instanceId;

            auto readHandler = bind_weak(&AmqpBroker::amqpReadHashHandler, this, _1, _2, handler, errorNotifier);

            if (!m_client) {
                throw KARABO_LOGIC_EXCEPTION("startReading: client is not initialized");
            }
            if (!m_client->isConnected()) {
                throw KARABO_LOGIC_EXCEPTION("startReading: client is not connected");
            }
            m_client->registerConsumerHandler(readHandler);

            boost::system::error_code ec = m_client->subscribe(exchange, bindingKey);
            if (!ec && m_consumeBroadcasts) {
                exchange = m_topic + ".global_slots";
                bindingKey = "";
                ec = m_client->subscribe(exchange, bindingKey);
            }
            if (ec) {
                std::ostringstream oss;
                oss << "Subscription to exchange -> \"" << exchange << "\", binding key -> \"" << bindingKey
                    << "\" failed: #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void AmqpBroker::stopReading() {
            if (m_topic.empty() || m_instanceId.empty()) return;
            m_client->close();
            if (m_heartbeatClient) m_heartbeatClient->close();
            if (m_logClient) m_logClient->close();
            // clear handlers as well
            m_clientConsumerHandler = karabo::net::consumer::MessageHandler();
            m_clientErrorNotifier = karabo::net::consumer::ErrorNotifier();
            m_heartbeatConsumerHandler = karabo::net::consumer::MessageHandler();
            m_heartbeatErrorNotifier = karabo::net::consumer::ErrorNotifier();
        }


        void AmqpBroker::startReadingHeartbeats(const consumer::MessageHandler& handler,
                                                const consumer::ErrorNotifier& errorNotifier) {
            m_heartbeatConsumerHandler = handler;
            m_heartbeatErrorNotifier = errorNotifier;

            if (!m_heartbeatClient) {
                Hash config("brokers", m_availableBrokerUrls);
                config.set("instanceId", m_instanceId + ":beats");
                config.set("domain", m_topic);
                m_heartbeatClient = Configurator<AmqpClient>::create(AMQP_CLIENT_CLASS, config);
                boost::system::error_code ec = m_heartbeatClient->connect();
                if (ec) {
                    std::ostringstream oss;
                    oss << "Failed to connect to AMQP broker: code #" << ec.value() << " -- " << ec.message();
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }

            auto readHandler = bind_weak(&AmqpBroker::amqpReadHashHandler, this, _1, _2, handler, errorNotifier);
            m_heartbeatClient->registerConsumerHandler(readHandler);

            const std::string exchange = m_topic + ".signals";
            const std::string bindingKey = "*.signalHeartbeat";
            boost::system::error_code ec = m_heartbeatClient->subscribe(exchange, bindingKey);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to exchange -> \"" << exchange << "\", bindingkey->\"" << bindingKey
                    << "\" : code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void AmqpBroker::startReadingLogs(const consumer::MessageHandler& handler,
                                          const consumer::ErrorNotifier& errorNotifier) {
            m_logConsumerHandler = handler;
            m_logErrorNotifier = errorNotifier;

            if (!m_logClient) {
                Hash config("brokers", m_availableBrokerUrls);
                config.set("instanceId", m_instanceId + ":rdlog");
                config.set("domain", m_topic);
                m_logClient = Configurator<AmqpClient>::create(AMQP_CLIENT_CLASS, config);
                boost::system::error_code ec = m_logClient->connect();
                if (ec) {
                    std::ostringstream oss;
                    oss << "Failed to connect to AMQP broker: code #" << ec.value() << " -- " << ec.message();
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
            }

            auto readHandler = bind_weak(&AmqpBroker::amqpReadHashHandler, this, _1, _2, handler, errorNotifier);
            m_logClient->registerConsumerHandler(readHandler);

            const std::string exchange = m_topic + ".log";
            const std::string bindingKey = "";
            boost::system::error_code ec = m_logClient->subscribe(exchange, bindingKey);
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to exchange -> \"" << exchange << "\", bindingkey->\"" << bindingKey
                    << "\" : code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }

    } // namespace net
} // namespace karabo
