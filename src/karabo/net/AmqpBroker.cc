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
#include "array"
#include "karabo/data/types/Hash.hh"
#include "karabo/log/Logger.hh"
#include "karabo/util/MetaTools.hh"
#include "utils.hh"


using namespace karabo::data;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

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
              m_slotExchange(m_topic + ".slots"),
              m_globalSlotExchange(m_topic + ".global_slots") {}


        AmqpBroker::AmqpBroker(const AmqpBroker& o, const std::string& newInstanceId)
            : Broker(o, newInstanceId),
              m_connection(o.m_connection),
              m_client(),
              m_handlerStrand(Configurator<Strand>::create("Strand", Hash("maxInARow", 10u))),
              m_slotExchange(o.m_slotExchange),
              m_globalSlotExchange(o.m_globalSlotExchange) {}


        Broker::Pointer AmqpBroker::clone(const std::string& instanceId) {
            return Broker::Pointer(new AmqpBroker(*this, instanceId));
        }

        AmqpBroker::~AmqpBroker() {
            // Resets not strictly needed.
            // We could add someting like client->disable() here if clients outlive their broker for some reason
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
                // We do not repeat again until a broker
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
                                              bind_weak(&AmqpBroker::amqpReadHandler, this, _1, _2, _3, _4),
                                              bind_weak(&AmqpBroker::amqpErrorNotifier, this, _1));
        }


        void AmqpBroker::disconnect() {
            // Note: m_connection is kept alive and connected
            m_client.reset();
        }


        bool AmqpBroker::isConnected() const {
            return (m_connection->isConnected() && m_client);
        }


        std::string AmqpBroker::getBrokerUrl() const {
            return m_connection->getCurrentUrl();
        }


        void AmqpBroker::amqpReadHandler(const Hash::Pointer& header, const Hash::Pointer& body,
                                         const std::string& exchange, const std::string& key) {
            auto callReadHandler = [weakSelf{weak_from_this()}, header, body, exchange, key] {
                if (auto self = std::static_pointer_cast<Self>(weakSelf.lock())) {
                    if (self->m_readHandler) {
                        const size_t posSep = key.find('.');
                        const bool isOneToOne = (exchange == self->m_slotExchange);
                        if (isOneToOne || exchange == self->m_globalSlotExchange) {
                            const std::string slot = key.substr(posSep + 1); // 2nd part of routing key is slot
                            self->m_readHandler(slot, !isOneToOne, header, body);
                        } else {
                            // exchange == m_topic + ".signals", so routing key maps to slots
                            for (const std::string& slot : self->m_slotsForSignals[key]) {
                                self->m_readHandler(slot, false, header, body);
                            }
                        }
                    } else {
                        KARABO_LOG_FRAMEWORK_ERROR << "Lack read handler for message with header " << *header;
                    }
                }
            };
            m_handlerStrand->post(callReadHandler);
        }


        void AmqpBroker::amqpErrorNotifier(const std::string& msg) {
            if (m_errorNotifier) {
                m_handlerStrand->post(std::bind(m_errorNotifier, net::consumer::Error::type, msg));
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Lack error notifier for error message " << msg;
            }
        }


        boost::system::error_code AmqpBroker::subscribeToRemoteSignal(const std::string& slot,
                                                                      const std::string& signalInstanceId,
                                                                      const std::string& signalFunction) {
            std::promise<boost::system::error_code> subDone;
            auto fut = subDone.get_future();
            subscribeToRemoteSignalAsync(slot, signalInstanceId, signalFunction,
                                         [&subDone](const boost::system::error_code ec) { subDone.set_value(ec); });
            return fut.get();
        }


        void AmqpBroker::subscribeToRemoteSignalAsync(const std::string& slot, const std::string& signalInstanceId,
                                                      const std::string& signalFunction,
                                                      const AsyncHandler& completionHandler) {
            if (!m_client) {
                karabo::net::EventLoop::post(std::bind(completionHandler, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }

            const std::string exchange = m_topic + ".signals";
            const std::string bindingKey = (signalInstanceId + ".") += signalFunction;
            auto wrapHandler = [weakSelf{weak_from_this()}, slot, bindingKey,
                                completionHandler](const boost::system::error_code& ec) {
                if (auto self = std::static_pointer_cast<Self>(
                          weakSelf.lock())) { // weakSelf points to type Broker, not AmqpBroker
                    auto wrap2 = [weak2Self{WeakPointer(self)}, slot, bindingKey, completionHandler, ec]() {
                        if (!ec) {
                            // If subscribed on broker, add slot to book-keeping before reporting success
                            if (auto self = weak2Self.lock()) {
                                self->m_slotsForSignals[bindingKey].insert(slot);
                            }
                        }
                        if (completionHandler) completionHandler(ec);
                    };
                    // The wrap2 handler will be called in same strand as our readHandler (that protects
                    // m_slotsForSignals)
                    self->m_handlerStrand->post(std::move(wrap2));
                } else if (completionHandler) {
                    // We are dead, but better call handler nevertheless. Ensure that it is not running in the AMQP
                    // event loop - it may contain synchronous writing to the broker that would then block that event
                    // loop
                    karabo::net::EventLoop::post(std::bind(std::move(completionHandler), ec));
                }
            };
            // Our wrapHandler will be called in event loop of AmqpConnection
            m_client->asyncSubscribe(exchange, bindingKey, std::move(wrapHandler));
        }

        boost::system::error_code AmqpBroker::unsubscribeFromRemoteSignal(const std::string& slot,
                                                                          const std::string& signalInstanceId,
                                                                          const std::string& signalFunction) {
            std::promise<boost::system::error_code> unsubDone;
            auto fut = unsubDone.get_future();
            unsubscribeFromRemoteSignalAsync(
                  slot, signalInstanceId, signalFunction,
                  [&unsubDone](const boost::system::error_code ec) { unsubDone.set_value(ec); });
            return fut.get();
        }


        void AmqpBroker::unsubscribeFromRemoteSignalAsync(const std::string& slot, const std::string& signalInstanceId,
                                                          const std::string& signalFunction,
                                                          const AsyncHandler& completionHandler) {
            if (!m_client) {
                karabo::net::EventLoop::post(std::bind(completionHandler, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }

            const std::string exchange = m_topic + ".signals";
            const std::string bindingKey = (signalInstanceId + ".") += signalFunction;
            // Wrap handler - see comment in subscribeToRemoteSignalAsync
            auto wrapHandler = [weakSelf{weak_from_this()}, slot, bindingKey,
                                completionHandler](const boost::system::error_code& ec) {
                if (auto self = std::static_pointer_cast<Self>(
                          weakSelf.lock())) { // weakSelf points to type Broker, not AmqpBroker
                    auto wrap2 = [weak2Self{WeakPointer(self)}, slot, bindingKey, completionHandler, ec]() {
                        if (!ec) {
                            // If unsubscribed on broker, remove slot from book-keeping before reporting success
                            if (auto self = weak2Self.lock()) {
                                std::set<std::string>& slots = self->m_slotsForSignals[bindingKey];
                                if (0 == slots.erase(slot)) {
                                    KARABO_LOG_FRAMEWORK_WARN << "Slot " << slot << " not registered for " << bindingKey
                                                              << ", but trying to unsubscribe";
                                }
                                if (slots.empty()) {
                                    self->m_slotsForSignals.erase(bindingKey);
                                }
                            }
                        }
                        if (completionHandler) completionHandler(ec);
                    };
                    // The wrap2 handler will be called in same strand as m_readHandler (that protects
                    // m_slotsForSignals)
                    self->m_handlerStrand->post(std::move(wrap2));
                } else if (completionHandler) {
                    // We are dead, but better call handler nevertheless. Ensure that it is not running in the AMQP
                    // event loop - it may contain synchronous writing to the broker that would then block that event
                    // loop
                    karabo::net::EventLoop::post(std::bind(std::move(completionHandler), ec));
                }
            };
            // Handler will be called in event loop of AmqpConnection
            m_client->asyncUnsubscribe(exchange, bindingKey, std::move(wrapHandler));
        }


        void AmqpBroker::sendSignal(const std::string& signal, const karabo::data::Hash::Pointer& header,
                                    const karabo::data::Hash::Pointer& body) {
            const std::string exchange = m_topic + ".signals";
            const std::string routingkey = (m_instanceId + ".") += signal;

            publish(exchange, routingkey, header, body);
        }


        void AmqpBroker::sendBroadcast(const std::string& slot, const karabo::data::Hash::Pointer& header,
                                       const karabo::data::Hash::Pointer& body) {
            auto it = std::find(m_broadcastSlots.begin(), m_broadcastSlots.end(), slot);
            if (it == m_broadcastSlots.end() && slot != "slotHeartbeat") {
                throw KARABO_PARAMETER_EXCEPTION(slot + " is not known broadcast slot");
            }
            const std::string routingkey = (m_instanceId + ".") += slot;
            publish(m_globalSlotExchange, routingkey, header, body);
        }


        void AmqpBroker::sendOneToOne(const std::string& receiverId, const std::string& slot,
                                      const karabo::data::Hash::Pointer& header,
                                      const karabo::data::Hash::Pointer& body) {
            const std::string routingkey = (receiverId + ".") += slot;
            publish(m_slotExchange, routingkey, header, body);
        }


        void AmqpBroker::publish(const std::string& exchange, const std::string& routingKey,
                                 const karabo::data::Hash::Pointer& header, const karabo::data::Hash::Pointer& body) {
            std::promise<boost::system::error_code> pubDone;
            auto pubFut = pubDone.get_future();
            m_client->asyncPublish(exchange, routingKey, header, body,
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

            // Figure out which subscriptions are needed
            std::vector<std::array<std::string, 2>> subscriptions; // exchange and routingKey
            // Subscribe to all 1-to-1 slots (.#, not .* for slots with dots, i.e. under node)...
            subscriptions.push_back({m_topic + ".slots", m_instanceId + ".#"});
            if (m_consumeBroadcasts) {
                // ...and to all known (!) broadcast slots
                const std::string broadcastExchange(m_topic + ".global_slots");
                for (const std::string& broadcastSlot : m_broadcastSlots) {
                    subscriptions.push_back({broadcastExchange, "*." + broadcastSlot});
                }
            }

            // Asynchronously subscribe to them in parallel
            std::vector<std::future<boost::system::error_code>> futs;
            for (const auto& subscription : subscriptions) {
                auto promise = std::make_shared<std::promise<boost::system::error_code>>();
                futs.push_back(promise->get_future());
                m_client->asyncSubscribe(subscription[0], subscription[1],
                                         [promise](const boost::system::error_code ec) { promise->set_value(ec); });
            }

            // Finally wait for one subscription after another
            for (auto itFut = futs.begin(); itFut != futs.end(); ++itFut) {
                const boost::system::error_code ec = itFut->get();
                if (ec) {
                    // Note: Device instantiation fails here if we fail due to broker connection loss.
                    //       Probably, without the throw all would go fine on reconnection _except_ that the
                    //       uniqueness check for the instanceId would be not effective.
                    const size_t subIndex = itFut - futs.begin();
                    const auto& subscription = subscriptions[subIndex];
                    std::ostringstream oss;
                    oss << "Subscription to exchange -> \"" << subscription[0] << "\", binding key -> \""
                        << subscription[1] << "\" failed: #" << ec.value() << " -- " << ec.message();
                    throw KARABO_NETWORK_EXCEPTION(oss.str());
                }
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
        }


        void AmqpBroker::startReadingHeartbeats() {
            // Check whether we are already reading by checking whether  read handler exists:
            std::promise<bool> readerIsSet;
            auto fut = readerIsSet.get_future();
            m_handlerStrand->post([weakThis{weak_from_this()}, &readerIsSet]() {
                if (auto self = std::static_pointer_cast<Self>(weakThis.lock())) {
                    const bool result = static_cast<bool>(self->m_readHandler);
                    readerIsSet.set_value(result);
                }
            });
            if (!fut.get()) {
                throw KARABO_LOGIC_EXCEPTION("Cannot startReadingHeartbeats before startReading");
            }

            // Subscribe client to (all) heartbeats
            const std::string exchange(m_topic + ".global_slots");
            const std::string bindingKey("*.slotHeartbeat");
            std::promise<boost::system::error_code> subDone;
            auto subFut = subDone.get_future();
            m_client->asyncSubscribe(exchange, bindingKey,
                                     [&subDone](const boost::system::error_code ec) { subDone.set_value(ec); });
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
