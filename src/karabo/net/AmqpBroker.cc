/*
 * File:   AmqpBroker.cc
 *
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
 * Created on May 19, 2020, 16:08 PM
 */

#include "AmqpBroker.hh"

#include "AmqpUtils.hh"
#include "EventLoop.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/log/Logger.hh"
#include "karabo/util/MetaTools.hh"
#include "utils.hh"


using namespace karabo::data;
using std::placeholders::_1;
using std::placeholders::_2;

using karabo::util::bind_weak;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Broker, karabo::net::AmqpBroker)

namespace karabo {
    namespace net {


        void AmqpBroker::expectedParameters(karabo::data::Schema& s) {}


        void AmqpBroker::defaultQueueArgs(AMQP::Table& args) {
            args.set("x-max-length", 10'000)      // Queue limit
                  .set("x-overflow", "drop-head") // drop oldest if limit reached
                  .set("x-message-ttl", 120'000); // message time-to-live in ms
        }


        AmqpBroker::AmqpBroker(const karabo::data::Hash& config)
            : Broker(config),
              m_connection(std::make_shared<AmqpConnection>(m_availableBrokerUrls)),
              m_client(),
              m_handlerStrand(Configurator<Strand>::create("Strand", Hash("maxInARow", 10u))),
              m_heartbeatClient() {}


        AmqpBroker::AmqpBroker(const AmqpBroker& o, const std::string& newInstanceId)
            : Broker(o, newInstanceId),
              m_connection(o.m_connection),
              m_client(),
              m_handlerStrand(Configurator<Strand>::create("Strand", Hash("maxInARow", 10u))),
              m_heartbeatClient() {}


        Broker::Pointer AmqpBroker::clone(const std::string& instanceId) {
            return Broker::Pointer(new AmqpBroker(*this, instanceId));
        }

        AmqpBroker::~AmqpBroker() {
            // Resets not strictly needed.
            // We could add someting like client->disable() here if clients outlive their broker for some reason
            m_heartbeatClient.reset();
            m_client.reset();
            m_connection.reset();
        }

        void AmqpBroker::connect() {
            // The connection would be created asynchronously in the background when the client needs it.
            // To match the Broker interface, we block here until connected.
            // That also eases diagnosis in case of problems.
            // Note that it does not matter for asyncConnect whether we are already connected or not or
            // in the process of connecting - we will get success or failure reported properly.
            std::promise<boost::system::error_code> prom;
            auto fut = prom.get_future();
            m_connection->asyncConnect([&prom](const boost::system::error_code ec) { prom.set_value(ec); });
            const boost::system::error_code ec = fut.get();
            if (ec) {
                // In contrast to what is implemented for the JmsBorker, we do not repeat again until a broker
                // behind one of the urls gets available. The exception here will terminate the process.
                // Deployment action might be to restart it.
                // But this happens also if a device is instantiated after a connection loss before successful
                // reconnection. Then this exception will lead to instantiation failure.
                std::ostringstream oss;
                oss << "Failed to connect to AMQP broker: code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }

            // Create client already here - since no subscriptions yet, read handler will not yet be called
            AMQP::Table queueArgs;
            defaultQueueArgs(queueArgs);
            m_client = AmqpHashClient::create(m_connection, (m_topic + ".") += m_instanceId, queueArgs,
                                              bind_weak(&AmqpBroker::amqpReadHandler, this, _1, _2),
                                              bind_weak(&AmqpBroker::amqpErrorNotifier, this, _1));
        }


        void AmqpBroker::disconnect() {
            // Note: m_connection is kept alive and connected
            m_client.reset();
            m_heartbeatClient.reset();
        }


        bool AmqpBroker::isConnected() const {
            return (m_connection->isConnected() && m_client);
        }


        std::string AmqpBroker::getBrokerUrl() const {
            return m_connection->getCurrentUrl();
        }


        void AmqpBroker::amqpReadHandler(const Hash::Pointer& header, const Hash::Pointer& body) {
            if (m_readHandler) {
                m_handlerStrand->post(std::bind(m_readHandler, header, body));
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Lack read handler for message with header " << *header;
            }
        }

        void AmqpBroker::amqpReadHandlerBeats(const Hash::Pointer& header, const Hash::Pointer& body) {
            if (m_readHandlerBeats) {
                m_handlerStrandBeats->post(std::bind(m_readHandlerBeats, header, body));
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Lack read handler for beats with header " << *header;
            }
        }

        void AmqpBroker::amqpErrorNotifier(const std::string& msg) {
            if (m_errorNotifier) {
                m_handlerStrand->post(std::bind(m_errorNotifier, net::consumer::Error::type, msg));
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Lack error notifier for error message " << msg;
            }
        }

        void AmqpBroker::amqpErrorNotifierBeats(const std::string& msg) {
            if (m_errorNotifierBeats) {
                m_handlerStrandBeats->post(std::bind(m_errorNotifierBeats, net::consumer::Error::type, msg));
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Lack error notifier for beats error message " << msg;
            }
        }

        boost::system::error_code AmqpBroker::subscribeToRemoteSignal(const std::string& signalInstanceId,
                                                                      const std::string& signalFunction) {
            std::promise<boost::system::error_code> subDone;
            auto fut = subDone.get_future();
            subscribeToRemoteSignalAsync(signalInstanceId, signalFunction,
                                         [&subDone](const boost::system::error_code ec) { subDone.set_value(ec); });
            return fut.get();
        }


        void AmqpBroker::subscribeToRemoteSignalAsync(const std::string& signalInstanceId,
                                                      const std::string& signalFunction,
                                                      const AsyncHandler& completionHandler) {
            if (!m_client) {
                karabo::net::EventLoop::post(std::bind(completionHandler, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }

            const std::string exchange = m_topic + ".signals";
            const std::string bindingKey = (signalInstanceId + ".") += signalFunction;
            auto wrapHandler = [completionHandler](const boost::system::error_code& ec) {
                if (completionHandler) {
                    // Ensure that the passed handler is not running in the AMQP event loop - it may contain synchronous
                    // writing to the broker that would then block that event loop
                    karabo::net::EventLoop::post(std::bind(std::move(completionHandler), ec));
                } else if (ec) {
                    KARABO_LOG_FRAMEWORK_WARN << "Some subscription to remote signal failed: " << ec.message();
                }
            };
            m_client->asyncSubscribe(exchange, bindingKey, std::move(wrapHandler));
        }

        boost::system::error_code AmqpBroker::unsubscribeFromRemoteSignal(const std::string& signalInstanceId,
                                                                          const std::string& signalFunction) {
            std::promise<boost::system::error_code> unsubDone;
            auto fut = unsubDone.get_future();
            unsubscribeFromRemoteSignalAsync(
                  signalInstanceId, signalFunction,
                  [&unsubDone](const boost::system::error_code ec) { unsubDone.set_value(ec); });
            return fut.get();
        }


        void AmqpBroker::unsubscribeFromRemoteSignalAsync(const std::string& signalInstanceId,
                                                          const std::string& signalFunction,
                                                          const AsyncHandler& completionHandler) {
            if (!m_client) {
                karabo::net::EventLoop::post(std::bind(completionHandler, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }

            const std::string exchange = m_topic + ".signals";
            const std::string bindingKey = (signalInstanceId + ".") += signalFunction;
            // Wrap handler - see comment in subscribeToRemoteSignalAsync
            auto wrapHandler = [completionHandler](const boost::system::error_code& ec) {
                if (completionHandler) {
                    EventLoop::post(std::bind(std::move(completionHandler), ec));
                } else if (ec) {
                    KARABO_LOG_FRAMEWORK_WARN << "Some unsubscription from remote signal failed: " << ec.message();
                }
            };
            m_client->asyncUnsubscribe(exchange, bindingKey, std::move(wrapHandler));
        }


        void AmqpBroker::write(const std::string& target, const karabo::data::Hash::Pointer& header,
                               const karabo::data::Hash::Pointer& body) {
            if (!header) {
                throw KARABO_PARAMETER_EXCEPTION("AmqpBroker.write: header pointer is null");
            }
            if (!m_client) {
                KARABO_LOG_FRAMEWORK_WARN << getInstanceId()
                                          << ": Skip 'write' since not connected, header: " << *header;
                return;
            }

            std::string exchange = "";
            std::string routingkey = "";
            bool useHeartbeatClient = false;

            if (target == "karaboGuiDebug") {
                exchange = m_topic + ".karaboGuiDebug";
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
                    oss << "Cannot publish \"" << signalFunction << "\" from \"" << m_instanceId
                        << "\": the signalInstanceId should be \"" << signalInstanceId << "\"!";
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
                    exchange = m_topic + ".global_slots";

                    auto slotFunctionsNode = header->find("slotFunctions");
                    if (slotFunctionsNode && slotFunctionsNode->getValue<std::string>() == "|*:slotHeartbeat|") {
                        routingkey = signalInstanceId + ".slotHeartbeat";
                        // Use m_heartbeatClient if exists ...
                        useHeartbeatClient = true;
                    }
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
                    // 'slotFunctions' => |DataLogger-karabo/dataLogger:slotChanged||dataAggregator1:slotData| STRING
                    // 'slotInstanceIds' => |DataLogger-karabo/dataLogger| STRING
                    // ...
                    exchange = m_topic + ".signals";
                    routingkey = signalInstanceId + "." + signalFunction;
                }
            }

            if (exchange.empty()) {
                throw KARABO_LOGIC_EXCEPTION("Attempt to 'write' to unknown target: \"" + target + "\"");
            }

            std::promise<boost::system::error_code> pubDone;
            auto pubFut = pubDone.get_future();
            (useHeartbeatClient && m_heartbeatClient ? m_heartbeatClient : m_client)
                  ->asyncPublish(exchange, routingkey, header, body,
                                 [&pubDone](const boost::system::error_code ec) { pubDone.set_value(ec); });
            const boost::system::error_code ec = pubFut.get();
            if (ec) {
                if (ec == AmqpCppErrc::eMessageDrop) {
                    KARABO_LOG_FRAMEWORK_WARN << "Publishing failed since client dropped voluntarily";
                } else {
                    KARABO_LOG_FRAMEWORK_ERROR << "Publishing message failed (" << ec.message()
                                               << "), header: " << *header;
                    throw KARABO_NETWORK_EXCEPTION("Publishing failed: " + ec.message());
                }
            }
        }


        void AmqpBroker::startReading(const consumer::MessageHandler& handler,
                                      const consumer::ErrorNotifier& errorNotifier) {
            if (!m_client) {
                throw KARABO_LOGIC_EXCEPTION("Cannot startReading before connected");
            }

            // All access to handler on strand, so post there.
            // Capture 'this' as well since weakThis is Broker, not AmqpBroker.
            m_handlerStrand->post([this, weakThis{weak_from_this()}, handler, errorNotifier]() {
                if (auto self = weakThis.lock()) {
                    this->m_readHandler = std::move(handler);
                    this->m_errorNotifier = std::move(errorNotifier);
                }
            });

            // Subscribe to "slots" exchange with instanceId as routing key
            std::string exchange(m_topic + ".slots");
            std::string bindingKey(m_instanceId);
            std::promise<boost::system::error_code> subDone;
            auto subFut = subDone.get_future();

            m_client->asyncSubscribe(exchange, bindingKey,
                                     [&subDone](const boost::system::error_code ec) { subDone.set_value(ec); });
            boost::system::error_code ec = subFut.get();
            if (!ec && m_consumeBroadcasts) {
                // ...and potentially subscribe to "global_slots" without routing key
                exchange = (m_topic + ".global_slots");
                bindingKey.clear();
                std::promise<boost::system::error_code> subGlobDone;
                auto subGlobFut = subGlobDone.get_future();
                m_client->asyncSubscribe(exchange, bindingKey, [&subGlobDone](const boost::system::error_code ec) {
                    subGlobDone.set_value(ec);
                });
                ec = subGlobFut.get();
            }
            if (ec) {
                // Note: Device instantiation fails here if we fail due to broker connection loss.
                //       Probably, without the throw all would go fine on reconnection _except_ that subscription
                //       to "global_slots" will miss if already the subscription to "slots" failed.
                std::ostringstream oss;
                oss << "Subscription to exchange -> \"" << exchange << "\", binding key -> \"" << bindingKey
                    << "\" failed: #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }


        void AmqpBroker::stopReading() {
            if (!m_client) return; // Not yet connected...
            // Simply unsubscribe from all subscriptions, i.e. slots, global_slots and any signals we have subscribed
            std::promise<boost::system::error_code> unsubDone;
            auto fut = unsubDone.get_future();
            m_client->asyncUnsubscribeAll(
                  [&unsubDone](const boost::system::error_code ec) { unsubDone.set_value(ec); });
            const boost::system::error_code ec = fut.get();
            if (ec) {
                KARABO_LOG_FRAMEWORK_WARN
                      << "Failed to unsubscribe from all subscriptions when stopping to read: " << ec.message() << " ("
                      << ec.value() << ").";
            }

            // Post erasure of handlers on the handler strands, see startReading
            m_handlerStrand->post([this, weakThis{weak_from_this()}]() {
                if (auto self = weakThis.lock()) {
                    this->m_readHandler = nullptr;
                    this->m_errorNotifier = nullptr;
                }
            });

            // Same now also for heartbeat client if it exists
            if (m_heartbeatClient) {
                std::promise<boost::system::error_code> unsubBeatsDone;
                auto futBeats = unsubBeatsDone.get_future();
                m_heartbeatClient->asyncUnsubscribeAll(
                      [&unsubBeatsDone](const boost::system::error_code ec) { unsubBeatsDone.set_value(ec); });
                const boost::system::error_code ec = futBeats.get();
                if (ec) {
                    KARABO_LOG_FRAMEWORK_WARN
                          << "Failed to unsubscribe from heartbeat subscriptions when stopping to read: "
                          << ec.message() << " (" << ec.value() << ").";
                }
                m_handlerStrandBeats->post([this, weakThis{weak_from_this()}]() {
                    if (auto self = weakThis.lock()) {
                        this->m_readHandlerBeats = nullptr;
                        this->m_errorNotifierBeats = nullptr;
                    }
                });
                // Erase heartbeat client and strand: Next round might not startReadingHeartbeats.
                // Strand with "guaranteeToRun", so above posting of handler erasure will be executed
                m_handlerStrandBeats.reset();
                m_heartbeatClient.reset();
            }
        }


        void AmqpBroker::startReadingHeartbeats(const consumer::MessageHandler& handler,
                                                const consumer::ErrorNotifier& errorNotifier) {
            // Create handler strand - guaranteeToRun ensures handlers are reset in stopReading
            m_handlerStrandBeats =
                  Configurator<Strand>::create("Strand", Hash("maxInARow", 10u, "guaranteeToRun", true));
            m_handlerStrandBeats->post([this, weakThis{weak_from_this()}, handler, errorNotifier]() {
                if (auto self = weakThis.lock()) { // self is ptr to base class Broker, not to AmqpBroker
                    this->m_readHandlerBeats = std::move(handler);
                    this->m_errorNotifierBeats = std::move(errorNotifier);
                }
            });

            // Create special client
            AMQP::Table queueArgs;
            defaultQueueArgs(queueArgs);
            // overwrite queue limit to be shorter
            queueArgs.set("x-max-length", 10'000);
            std::ostringstream osstr;
            osstr << m_topic << "." << m_instanceId << ":beats";
            m_heartbeatClient = AmqpHashClient::create(m_connection, osstr.str(), queueArgs,
                                                       bind_weak(&AmqpBroker::amqpReadHandlerBeats, this, _1, _2),
                                                       bind_weak(&AmqpBroker::amqpErrorNotifierBeats, this, _1));

            // Subscribe client to (all) heartbeats
            const std::string exchange(m_topic + ".global_slots");
            const std::string bindingKey("*.slotHeartbeat");
            std::promise<boost::system::error_code> subDone;
            auto subFut = subDone.get_future();
            m_heartbeatClient->asyncSubscribe(
                  exchange, bindingKey, [&subDone](const boost::system::error_code ec) { subDone.set_value(ec); });
            const boost::system::error_code ec = subFut.get();
            if (ec) {
                std::ostringstream oss;
                oss << "Failed to subscribe to exchange -> '" << exchange << "', bindingkey->'" << bindingKey
                    << "' for heartbeats: code #" << ec.value() << " -- " << ec.message();
                throw KARABO_NETWORK_EXCEPTION(oss.str());
            }
        }

    } // namespace net
} // namespace karabo
