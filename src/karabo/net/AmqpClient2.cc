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

#include "AmqpClient2.hh"

#include <amqpcpp.h>

#include "AmqpUtils.hh"
#include "karabo/log/Logger.hh"
#include "karabo/util/Exception.hh"


using boost::placeholders::_1;

namespace bse = boost::system::errc;

// Open questions/TODO:
// - handle case that queue name is already used
//   * request queue with some suffix?
//   * need to recreate channel as well?
// - need for m_channel->onError(nullptr); at several places? (See old AmqpClient code about where.)

namespace karabo::net {

    AmqpClient2::AmqpClient2(AmqpConnection::Pointer connection, std::string instanceId, AMQP::Table queueArgs,
                             ReadHandler readHandler)
        : m_connection(std::move(connection)),
          m_instanceId(std::move(instanceId)),
          m_queueArgs(std::move(queueArgs)),
          m_readHandler(std::move(readHandler)),
          m_channelStatus(ChannelStatus::REQUEST) {}

    AmqpClient2::~AmqpClient2() {
        // Call remaining handlers with operation_cancelled indicator - in io context as promised
        const auto cancelledError = bse::make_error_code(bse::operation_canceled);
        if (m_channelPreparationCallback) {
            m_connection->dispatch(boost::bind(m_channelPreparationCallback, cancelledError));
        }

        for (auto& [dummy, statusAndHandler] : m_subscriptions) {
            if (statusAndHandler.onSubscription) {
                m_connection->dispatch(boost::bind(statusAndHandler.onSubscription, cancelledError));
            }
        }
        for (PostponedMessage& m : m_postponedPubMessages) {
            m_connection->dispatch(boost::bind(m.onPublishDone, cancelledError));
        }
    }

    void AmqpClient2::asyncSubscribe(const std::string& exchange, const std::string& routingKey,
                                     AsyncHandler onSubscriptionDone) {
        // Ensure to run in single threaded io context - no concurrency problems!
        // We post and do not dispatch to ensure that also in the most normal case (ChannelStatus::READY below) the
        // onSubscriptionDone inside doSubscribePending does not need posting and still the guarantee holds that it
        // is not called from within asyncSubscribe(..).
        m_connection->post([weakThis{weak_from_this()}, this, exchange, routingKey,
                            onSubscriptionDone{std::move(onSubscriptionDone)}]() mutable {
            auto self(weakThis.lock());
            if (!self) {
                onSubscriptionDone(bse::make_error_code(bse::operation_canceled));
                return;
            }
            // Store requested subscription
            SubscriptionStatusHandler statusHandler(SubscriptionStatus::PENDING, std::move(onSubscriptionDone));
            m_subscriptions.insert_or_assign({exchange, routingKey}, std::move(statusHandler));

            switch (m_channelStatus) {
                case ChannelStatus::REQUEST: {
                    // request preparation of channel and then subscribe all (then) pending subscriptions
                    m_channelStatus = ChannelStatus::CREATE;
                    asyncPrepareChannel(util::bind_weak(&AmqpClient2::doSubscribePending, this, _1));
                    break;
                }
                case ChannelStatus::CREATE:
                case ChannelStatus::CREATE_QUEUE:
                case ChannelStatus::CREATE_CONSUMER:
                    // Channel preparation is requested earlier - let that do the job
                    break;
                case ChannelStatus::READY:
                    // Channel ready, so directly subscribe
                    doSubscribePending(bse::make_error_code(bse::success));
                    break;
            }
        });
    }

    void AmqpClient2::asyncUnsubscribe(const std::string& exchange, const std::string& routingKey,
                                       AsyncHandler onUnsubscriptionDone) {
        throw KARABO_NOT_IMPLEMENTED_EXCEPTION("To be done");
    }


    void AmqpClient2::asyncPublish(const std::string& exchange, const std::string& routingKey,
                                   const std::shared_ptr<std::vector<char>>& data, AsyncHandler onPublishDone) {
        // Ensure to run in single threaded io context (=> no concurrency problems!).
        // We post and do not dispatch to ensure that also in the most normal case (ChannelStatus::READY below)
        // the onPublishDone does not need posting and still the guarantee holds that it is not called from within
        // asyncPublish(..).
        m_connection->post([weakThis{weak_from_this()}, this, exchange, routingKey, data{std::move(data)},
                            onPublishDone{std::move(onPublishDone)}]() mutable {
            auto self(weakThis.lock());
            if (!self) {
                onPublishDone(bse::make_error_code(bse::operation_canceled));
                return;
            }
            switch (m_channelStatus) {
                case ChannelStatus::REQUEST: {
                    m_channelStatus = ChannelStatus::CREATE;
                    // Postpone message and trigger channel creation - if that done, publish and call callbacks
                    m_postponedPubMessages.push_back(PostponedMessage(std::move(exchange), std::move(routingKey),
                                                                      std::move(data), std::move(onPublishDone)));
                    asyncPrepareChannel([weakThis](const boost::system::error_code& ec) {
                        if (auto self = weakThis.lock()) {
                            for (PostponedMessage& m : self->m_postponedPubMessages) {
                                if (ec) {
                                    m.onPublishDone(ec);
                                } else {
                                    AMQP::Envelope dataWrap(m.data->data(), m.data->size());
                                    if (self->m_channel->publish(m.exchange, m.routingKey, dataWrap)) {
                                        m.onPublishDone(bse::make_error_code(bse::success));
                                    } else {
                                        m.onPublishDone(bse::make_error_code(bse::io_error));
                                    }
                                }
                            }
                            self->m_postponedPubMessages.clear();
                        } // else: onPublishDones should be called in destructor
                    });
                    break;
                }
                case ChannelStatus::CREATE:
                case ChannelStatus::CREATE_QUEUE:
                case ChannelStatus::CREATE_CONSUMER:
                    // Channel being prepared, so just postpone message. If channel ready, that will publish messages
                    m_postponedPubMessages.push_back(PostponedMessage(std::move(exchange), std::move(routingKey),
                                                                      std::move(data), std::move(onPublishDone)));
                    break;
                case ChannelStatus::READY: {
                    // Channel ready, so directly send
                    AMQP::Envelope dataWrap(data->data(), data->size());
                    if (m_channel->publish(exchange, routingKey, dataWrap)) {
                        onPublishDone(bse::make_error_code(bse::success));
                    } else {
                        onPublishDone(bse::make_error_code(bse::io_error));
                    }
                } break;
            }
        });
    }


    void AmqpClient2::asyncPrepareChannel(AsyncHandler onChannelPrepared) {
        // TODO:
        // Check whether we can safely do this or channel is in an unforeseen state
        m_channelPreparationCallback = std::move(onChannelPrepared);
        m_connection->asyncCreateChannel(
              [weakThis{weak_from_this()}](const std::shared_ptr<AMQP::Channel>& channel, const char* errMsg) {
                  if (auto self = weakThis.lock()) {
                      if (channel) {
                          KARABO_LOG_FRAMEWORK_DEBUG_C("AmqpClient") << "Channel created for id " << self->m_instanceId;
                          self->m_channel = channel;
                          self->m_channelStatus = ChannelStatus::CREATE_QUEUE;
                          self->moveChannelState();
                      } else {
                          KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                << "Failed to create channel for id " << self->m_instanceId << ": " << errMsg;
                          self->m_channelStatus = ChannelStatus::REQUEST; // need start from scratch
                          AsyncHandler callback; // Better first swap, handler might reset m_channelPreparationCallback
                          callback.swap(self->m_channelPreparationCallback);
                          callback(make_error_code(AmqpCppErrc::eCreateChannelError));
                      }
                  }
              });
    }

    void AmqpClient2::moveChannelState() {
        auto wSelf(weak_from_this());

        switch (m_channelStatus) {
            case ChannelStatus::REQUEST:
            case ChannelStatus::CREATE:
                KARABO_LOG_FRAMEWORK_WARN << "Inconsistent channel state in moveChannelState: REQUEST or CREATE "
                                          << static_cast<int>(m_channelStatus);
                break;
            case ChannelStatus::CREATE_QUEUE:
                m_channel->declareQueue(m_instanceId, AMQP::autodelete, m_queueArgs)
                      .onSuccess([wSelf](const std::string& name, int /*msgcount*/, int /*consumercount*/) {
                          if (auto self = wSelf.lock()) {
                              KARABO_LOG_FRAMEWORK_DEBUG_C("AmqpClient")
                                    << "Queue created for id " << self->m_instanceId;
                              self->m_channelStatus = ChannelStatus::CREATE_CONSUMER;
                              self->moveChannelState();
                          }
                      }) // end of success handler
                      .onError([wSelf](const char* message) {
                          if (auto self = wSelf.lock()) {
                              KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                    << self->m_instanceId << ": Queue creation failed: " << message;
                              // reset channel
                              self->m_channel.reset();
                              self->m_channelStatus = ChannelStatus::REQUEST;
                              AsyncHandler callback;
                              callback.swap(self->m_channelPreparationCallback);
                              callback(make_error_code(AmqpCppErrc::eCreateQueueError));
                          }
                      }); // end of failure handler
                break;
            case ChannelStatus::CREATE_CONSUMER:
                // Automatic acknowledgement  FIXME later: + AMQP::exclusive?
                m_channel->consume(m_instanceId, AMQP::noack)
                      .onReceived([wSelf](const AMQP::Message& msg, uint64_t deliveryTag, bool redelivered) {
                          if (auto self = wSelf.lock()) {
                              // Copy of data not avoidable although in AMQP io context here: AMQP::Message better be
                              // destructed in io context event loop and deserialisation not done there
                              auto vec = std::make_shared<std::vector<char>>(msg.body(), msg.body() + msg.bodySize());
                              self->m_readHandler(vec, msg.exchange(), msg.routingkey());
                          }
                      })
                      .onSuccess([wSelf](const std::string& consumerTag) {
                          if (auto self = wSelf.lock()) {
                              KARABO_LOG_FRAMEWORK_DEBUG_C("AmqpClient")
                                    << "Consumer for id " << self->m_instanceId << " ready, tag: " << consumerTag;
                              self->m_channelStatus = ChannelStatus::READY;
                              AsyncHandler callback;
                              callback.swap(self->m_channelPreparationCallback);
                              callback(bse::make_error_code(bse::success));
                          }
                      })
                      .onError([this, wSelf](const char* message) {
                          if (auto self = wSelf.lock()) {
                              KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                    << self->m_instanceId << ": Consumer creation failed: " << message;
                              self->m_channel.reset();
                              self->m_channelStatus = ChannelStatus::REQUEST;
                              AsyncHandler callback;
                              callback.swap(self->m_channelPreparationCallback);
                              callback(make_error_code(AmqpCppErrc::eCreateConsumerError));
                          }
                      });
                break;
            case ChannelStatus::READY:
                break;
        }
    }
    void AmqpClient2::doSubscribePending(const boost::system::error_code& ec) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_ERROR << "Subscribing failed since channel preparation failed: " << ec.message();
        }
        for (auto it = m_subscriptions.begin(); it != m_subscriptions.end();) {
            SubscriptionStatusHandler& statusAndHandler = it->second;
            if (statusAndHandler.status == SubscriptionStatus::PENDING) {
                const std::string& exchange = it->first.first;
                const std::string& routingKey = it->first.second;
                if (ec) {
                    KARABO_LOG_FRAMEWORK_ERROR << m_instanceId << " failed to subscribe for exchange '" << exchange
                                               << "' and routing key '" << routingKey << "': " << ec.message();
                    statusAndHandler.onSubscription(ec);
                    m_subscriptions.erase(it++);
                    continue;
                } else {
                    KARABO_LOG_FRAMEWORK_DEBUG << m_instanceId << " subscribed for exchange '" << exchange
                                               << "' and routing key '" << routingKey << "'";
                    statusAndHandler.status = SubscriptionStatus::CREATE_EXCHANGE;
                    moveSubscriptionState(exchange, routingKey); // it as arg?
                }
            }
            ++it;
        }
    }

    void AmqpClient2::moveSubscriptionState(const std::string& exchange, const std::string& routingKey) {
        auto it = m_subscriptions.find({exchange, routingKey});
        if (it == m_subscriptions.end()) { // Should not happen!
            KARABO_LOG_FRAMEWORK_WARN << "Moving subscription state for exchange " << exchange << " and routingKey "
                                      << routingKey << " fails since not in subscription map.";
            return;
        }

        auto wSelf = weak_from_this();
        SubscriptionStatus& status = it->second.status;
        switch (status) {
            case SubscriptionStatus::PENDING:
                break; // Nothing yet to do - how can this call happen?
            case SubscriptionStatus::CREATE_EXCHANGE: {
                const int flags = 0; // Karabo 3: switch to AMQP::autodelete (not AMQP::durable!)
                m_channel->declareExchange(exchange, AMQP::topic, flags)
                      .onSuccess([wSelf, exchange, routingKey]() {
                          if (auto self = wSelf.lock()) {
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << "Creating exchange " << exchange << " for routing key " << routingKey
                                        << " succeeded, but subscription gone!";
                              } else {
                                  it->second.status = SubscriptionStatus::BIND_QUEUE;
                                  self->moveSubscriptionState(exchange, routingKey);
                              }
                          }
                      })
                      .onError([wSelf, exchange, routingKey](const char* message) {
                          if (auto self = wSelf.lock()) {
                              KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                    << "Creating exchange " << exchange << " for routing key " << routingKey
                                    << " failed: " << message;
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << "Creating exchange " << exchange << " for routing key " << routingKey
                                        << " failed, but subscription gone!";
                              } else {
                                  it->second.onSubscription(make_error_code(AmqpCppErrc::eCreateExchangeError));
                                  self->m_subscriptions.erase(it);
                              }
                          }
                      });
            } break;
            case SubscriptionStatus::BIND_QUEUE: {
                m_channel->bindQueue(exchange, m_instanceId, routingKey)
                      .onSuccess([wSelf, exchange, routingKey]() {
                          if (auto self = wSelf.lock()) {
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << "Binding queue " << self->m_instanceId << " to exchange " << exchange
                                        << " with routing key " << routingKey << " succeeded, but subscription gone!";
                              } else {
                                  it->second.status = SubscriptionStatus::READY;
                                  AsyncHandler callback;
                                  callback.swap(it->second.onSubscription);
                                  callback(bse::make_error_code(bse::success));
                              }
                          }
                      })
                      .onError([wSelf, exchange, routingKey](const char* message) {
                          if (auto self = wSelf.lock()) {
                              KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                    << "Binding queue " << self->m_instanceId << " to exchange " << exchange
                                    << " with routing key " << routingKey << " failed: " << message;
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << "Binding queue " << self->m_instanceId << " to exchange " << exchange
                                        << " with routing key " << routingKey << " failed and subscription gone!";
                              } else { // Call handler with failure
                                  it->second.onSubscription(make_error_code(AmqpCppErrc::eBindQueueError));
                                  self->m_subscriptions.erase(it);
                              }
                          }
                      });
            } break;
            case SubscriptionStatus::READY:
                // Nothing anymore to do - how can this call happen?
                break;
        } // end switch
    }     // end

} // namespace karabo::net
