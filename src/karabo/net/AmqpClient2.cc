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

// Open questions/TODO:
// - need for m_channel->onError(nullptr); at several places? (See old AmqpClient code about where.)

namespace karabo::net {

    AmqpClient2::AmqpClient2(AmqpConnection::Pointer connection, std::string instanceId, AMQP::Table queueArgs,
                             ReadHandler readHandler)
        : m_connection(std::move(connection)),
          m_instanceId(std::move(instanceId)),
          m_queue(m_instanceId),
          m_queueArgs(std::move(queueArgs)),
          m_readHandler(std::move(readHandler)),
          m_channel(),
          m_channelStatus(ChannelStatus::REQUEST) {}

    AmqpClient2::~AmqpClient2() {
        // Call remaining handlers with operation_cancelled indicator - in io context as promised.
        // Better use that io context also for AMQP::Channel since AMQP library is not thread safe.
        // (AMQP::Table seems to be safe. If not, we should use shared_ptr and reset that.)

        std::promise<void> promise;
        auto future = promise.get_future();
        m_connection->dispatch([this, &promise]() {
            const auto cancelledError = KARABO_ERROR_CODE_OP_CANCELLED;

            if (m_channelPreparationCallback) {
                m_channelPreparationCallback(cancelledError);
                m_channelPreparationCallback = AsyncHandler();
            }

            for (auto& [dummy, statusAndHandler] : m_subscriptions) {
                if (statusAndHandler.onSubscription) {
                    statusAndHandler.onSubscription(cancelledError);
                }
            }
            m_subscriptions.clear();

            while (!m_postponedPubMessages.empty()) {
                m_postponedPubMessages.front().onPublishDone(cancelledError);
                m_postponedPubMessages.pop();
            }

            m_channel.reset();

            promise.set_value();
        });
        future.wait();
    }

    void AmqpClient2::setReadHandler(ReadHandler readHandler) {
        if (!readHandler) throw KARABO_PARAMETER_EXCEPTION("Read handler must be valid");

        // To avoid concurrency with incoming messages, setting the read handler has to detour via io context thread
        std::promise<void> prom;
        auto fut = prom.get_future();
        m_connection->dispatch([this, &prom, readHandler{std::move(readHandler)}]() mutable {
            m_readHandler = std::move(readHandler);
            prom.set_value();
        });
        fut.wait();
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
                onSubscriptionDone(KARABO_ERROR_CODE_OP_CANCELLED);
                return;
            }

            // Is there already a subscription? Or an ongoing unsubscription?
            auto it = m_subscriptions.find({exchange, routingKey});
            if (it != m_subscriptions.end()) {
                if (it->second.status < SubscriptionStatus::READY) {
                    // There is an ongoing subscription. Hijack that and attach onSubscriptionDone
                    auto combinedOnSubDone = [newOnSubDone{std::move(onSubscriptionDone)},
                                              previousOnSubDone{std::move(it->second.onSubscription)}](
                                                   const boost::system::error_code ec) {
                        previousOnSubDone(ec);
                        newOnSubDone(ec);
                    };
                    it->second.onSubscription = combinedOnSubDone;
                } else if (it->second.status == SubscriptionStatus::READY) {
                    // We are already subscribed, claim that as success.
                    onSubscriptionDone(KARABO_ERROR_CODE_SUCCESS);
                } else if (it->second.status > SubscriptionStatus::READY) {
                    // There is an ongoing unsubscription. Attach a repost of this to its handler
                    auto subscribeAgain = util::bind_weak(&AmqpClient2::asyncSubscribe, this, exchange, routingKey,
                                                          std::move(onSubscriptionDone));
                    auto patchedOnUnsubDone =
                          [previousOnSubDone{std::move(it->second.onSubscription)},
                           subscribeAgain{std::move(subscribeAgain)}](const boost::system::error_code ec) {
                              previousOnSubDone(ec);
                              subscribeAgain();
                          };
                    it->second.onSubscription = std::move(patchedOnUnsubDone);
                }
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
                    doSubscribePending(KARABO_ERROR_CODE_SUCCESS);
                    break;
            }
        });
    }

    void AmqpClient2::asyncUnsubscribe(const std::string& exchange, const std::string& routingKey,
                                       AsyncHandler onUnsubscriptionDone) {
        // Ensure to run in single threaded io context - no concurrency problems!
        m_connection->post([weakThis{weak_from_this()}, this, exchange, routingKey,
                            onUnsubscriptionDone{std::move(onUnsubscriptionDone)}]() mutable {
            auto self(weakThis.lock());
            if (!self) {
                onUnsubscriptionDone(KARABO_ERROR_CODE_OP_CANCELLED);
                return;
            }

            auto it = m_subscriptions.find({exchange, routingKey});
            if (it == m_subscriptions.end()) {
                // Unsubscribing something not subscribed is called success (since afterwards we are not subscribed)
                onUnsubscriptionDone(KARABO_ERROR_CODE_SUCCESS);
                return;
            }

            if (it->second.status != SubscriptionStatus::READY) {
                // Not yet connected or being unsubscribed: try again once READY or removed from subscriptions
                auto unsubscribeAgain = util::bind_weak(&AmqpClient2::asyncUnsubscribe, this, exchange, routingKey,
                                                        std::move(onUnsubscriptionDone));
                auto patchedOnDone =
                      [previousOnDone{std::move(it->second.onSubscription)},
                       unsubscribeAgain{std::move(unsubscribeAgain)}](const boost::system::error_code ec) {
                          previousOnDone(ec);
                          unsubscribeAgain();
                      };
                it->second.onSubscription = std::move(patchedOnDone);
                return;
            }

            // Finally real work to do - we store the handler and move further with our subscription status
            it->second.status = SubscriptionStatus::UNBIND_QUEUE;
            it->second.onSubscription = std::move(onUnsubscriptionDone);
            moveSubscriptionState(exchange, routingKey);
        });
    }

    void AmqpClient2::asyncUnsubscribeAll(AsyncHandler onUnsubscriptionsDone) {
        m_connection->dispatch([weakThis{weak_from_this()}, onUnsubscriptionsDone{std::move(onUnsubscriptionsDone)}]() {
            if (auto self = weakThis.lock()) {
                // Prepare handler for each subscription 'i': If last unsubscriptions is handled (all done flags are
                // true), call common handler, either with success or failure code of last failing unsubscription.
                const size_t numSubscriptions = self->m_subscriptions.size();
                if (numSubscriptions == 0) {
                    onUnsubscriptionsDone(KARABO_ERROR_CODE_SUCCESS);
                } else {
                    // Capture flags and common ec as pointers to be shared among all lambda instances
                    auto singleUnsubDone =
                          [doneFlags{std::make_shared<std::vector<bool>>(numSubscriptions, false)},
                           onUnsubscriptionsDone{std::move(onUnsubscriptionsDone)},
                           commonEc{std::make_shared<boost::system::error_code>(KARABO_ERROR_CODE_SUCCESS)}](
                                size_t i, const boost::system::error_code ec) mutable {
                              (*doneFlags)[i] = true;
                              // Track last failure
                              if (ec) *commonEc = ec;
                              // Since in single threaded io context, no need for concurrency protection of doneFlags
                              for (bool flag : *doneFlags) {
                                  if (!flag) return; // Still at least one to go
                              }
                              onUnsubscriptionsDone(*commonEc);
                          };

                    // Now call 'asyncUnsubscribe' for each subscription
                    size_t i = 0;
                    for (auto& [exchangeRoutingKey, dummy] : self->m_subscriptions) {
                        self->asyncUnsubscribe(
                              exchangeRoutingKey.first, exchangeRoutingKey.second,
                              // AsyncHandler is an std::function, so use bind and placeholder from std:
                              std::bind(singleUnsubDone, i++, std::placeholders::_1)); // postfix!
                    }
                }

            } else {
                onUnsubscriptionsDone(KARABO_ERROR_CODE_OP_CANCELLED);
            }
        });
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
                onPublishDone(KARABO_ERROR_CODE_OP_CANCELLED);
                return;
            }
            switch (m_channelStatus) {
                case ChannelStatus::REQUEST: {
                    m_channelStatus = ChannelStatus::CREATE;
                    // Postpone message and trigger channel creation - if that done, publish and call callbacks
                    m_postponedPubMessages.push(PostponedMessage(std::move(exchange), std::move(routingKey),
                                                                 std::move(data), std::move(onPublishDone)));
                    asyncPrepareChannel([weakThis](const boost::system::error_code& ec) {
                        if (auto self = weakThis.lock()) {
                            if (ec) {
                                KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                      << "Channel preparation failed (" << ec.message() << "), so "
                                      << self->m_postponedPubMessages.size()
                                      << " postponed messaged cannot be published";
                            }
                            // First use of m_postponedPubMessages: Collect messages till channel is ready
                            std::queue<PostponedMessage> postponedMessages;
                            postponedMessages.swap(self->m_postponedPubMessages); // doPublish may add again
                            while (!postponedMessages.empty()) {
                                PostponedMessage& m = postponedMessages.front();
                                if (ec) {
                                    m.onPublishDone(ec);
                                } else {
                                    self->doPublish(m.exchange, m.routingKey, m.data, m.onPublishDone);
                                }
                                postponedMessages.pop();
                            }
                        } // else: onPublishDones should be called in destructor
                    });
                    break;
                }
                case ChannelStatus::CREATE:
                case ChannelStatus::CREATE_QUEUE:
                case ChannelStatus::CREATE_CONSUMER:
                    // Channel being prepared, so just postpone message. If channel ready, that will publish messages
                    m_postponedPubMessages.push(PostponedMessage(std::move(exchange), std::move(routingKey),
                                                                 std::move(data), std::move(onPublishDone)));
                    break;
                case ChannelStatus::READY:
                    // Channel ready, so directly send
                    doPublish(exchange, routingKey, data, onPublishDone);
                    break;
            }
        });
    }

    void AmqpClient2::doPublish(const std::string& exchange, const std::string& routingKey,
                                const std::shared_ptr<std::vector<char>>& data, const AsyncHandler& onPublishDone) {
        auto itExchange = m_exchanges.find(exchange);
        const bool needToDeclareExch = (itExchange == m_exchanges.end());
        if (needToDeclareExch) {
            // Take care that 'exchange' is declared and thus exists. If it does not exist, channel->publish(..)
            // returns true, but the channel is not usable afterwards (slightly silly library interface, I'd say).
            m_exchanges[exchange] = ExchangeStatus::DECLARING;
            auto wSelf(weak_from_this());
            m_channel
                  ->declareExchange(exchange, AMQP::topic, 0) // Karabo 3: switch from '0' to AMQP::autodelete
                  .onSuccess([wSelf, exchange]() {
                      if (auto self = wSelf.lock()) {
                          KARABO_LOG_FRAMEWORK_DEBUG_C("AmqpClient") << self->m_instanceId << ": Declaring exchange "
                                                                     << exchange << " to publish to succeeded!";
                          self->m_exchanges[exchange] = ExchangeStatus::READY;
                          self->publishPostponedIfExchange();
                      }
                  })
                  .onError([wSelf, exchange](const char* message) {
                      KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                            << "Failed to to declare exchange '" << exchange << "' to publish to: " << message;
                      if (auto self = wSelf.lock()) {
                          self->m_exchanges.erase(exchange);
                          self->publishPostponedIfExchange();
                      }
                  });
        }

        // First declare all exchanges, then start publishing
        for (const auto& exchStatusPair : m_exchanges) {
            if (exchStatusPair.second != ExchangeStatus::READY) { // e.g. the one added above
                // Second use of m_postponedPubMessages: collect messages for which the exchanges need to be declared
                m_postponedPubMessages.push(PostponedMessage(exchange, routingKey, data, onPublishDone));
                return;
            }
        }

        // The envelope just stores pointer and size so one might wonder about data lifetime (and there is no
        // callback!). But since there is a publish method that takes the data as a 'const std::string&' that internally
        // creates an envelope as we do here from vector<char>, one can assume that (unfortunately) the data is copied
        // here (and our 'onPublishDone(success)' is called a bit too soon).
        AMQP::Envelope dataWrap(data->data(), data->size());
        if (m_channel->publish(exchange, routingKey, dataWrap)) {
            onPublishDone(KARABO_ERROR_CODE_SUCCESS);
        } else {
            KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                  << m_instanceId << ": publish failed. Channel " << (m_channel->usable() ? "" : "not ") << "usable. "
                  << m_connection->connectionInfo() << " (Use count: " << m_connection.use_count() << ")";
            onPublishDone(KARABO_ERROR_CODE_IO_ERROR);
        }
    }

    void AmqpClient2::publishPostponedIfExchange() {
        while (!m_postponedPubMessages.empty()) {
            PostponedMessage& postponedMsg = m_postponedPubMessages.front();
            auto itExchange = m_exchanges.find(postponedMsg.exchange);
            if (itExchange == m_exchanges.end()) { // Failed to declare
                postponedMsg.onPublishDone(make_error_code(AmqpCppErrc::eCreateExchangeError));
                m_postponedPubMessages.pop();
            } else if (itExchange->second == ExchangeStatus::READY) {
                AMQP::Envelope dataWrap(postponedMsg.data->data(), postponedMsg.data->size());
                if (m_channel->publish(postponedMsg.exchange, postponedMsg.routingKey, dataWrap)) {
                    postponedMsg.onPublishDone(KARABO_ERROR_CODE_SUCCESS);
                } else {
                    KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                          << m_instanceId << ": publish postponed message failed. Channel "
                          << (m_channel->usable() ? "" : "not ") << "usable. " << m_connection->connectionInfo()
                          << " (Use count: " << m_connection.use_count() << ")";
                    postponedMsg.onPublishDone(KARABO_ERROR_CODE_IO_ERROR);
                }
                m_postponedPubMessages.pop();
            } else {
                // exchange is in DECLARING state, all messages have to wait further
                return;
            }
        }
    }

    void AmqpClient2::asyncPrepareChannel(AsyncHandler onChannelPrepared) {
        if (m_channelStatus != ChannelStatus::CREATE) {
            KARABO_LOG_FRAMEWORK_WARN << m_instanceId << ".asyncPrepareChannel called in status "
                                      << static_cast<int>(m_channelStatus);
        }
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
                KARABO_LOG_FRAMEWORK_WARN << "Inconsistent channel state in moveChannelState: REQUEST or CREATE: "
                                          << static_cast<int>(m_channelStatus);
                break;
            case ChannelStatus::CREATE_QUEUE: {
                m_channel->declareQueue(m_queue, AMQP::autodelete, m_queueArgs)
                      .onSuccess([wSelf](const std::string& name, int msgCount, int consumerCount) {
                          if (auto self = wSelf.lock()) {
                              if (consumerCount > 0) {
                                  // Queue already exists, but we need a unique one for us - attach timestamp
                                  KARABO_LOG_FRAMEWORK_INFO_C("AmqpClient")
                                        << "Queue " << self->m_queue
                                        << " already has a consumer, append some bytes from clock and try again.";
                                  std::ostringstream oss;
                                  oss << ":" << std::hex << std::chrono::steady_clock::now().time_since_epoch().count();
                                  self->m_queue += oss.str();
                                  self->moveChannelState(); // simply try again with new queue name
                              } else {
                                  const std::string queue(self->m_instanceId == self->m_queue ? ""
                                                                                              : self->m_queue + " ");
                                  KARABO_LOG_FRAMEWORK_DEBUG_C("AmqpClient")
                                        << "Queue " << queue << "declared for id " << self->m_instanceId
                                        << " (message/consumer cout: " << msgCount << "/" << consumerCount << ")";
                                  self->m_channelStatus = ChannelStatus::CREATE_CONSUMER;
                                  self->moveChannelState();
                              }
                          }
                      }) // end of success handler
                      .onError([wSelf](const char* message) {
                          if (auto self = wSelf.lock()) {
                              const std::string queue(self->m_instanceId == self->m_queue ? "" : self->m_queue + " ");
                              KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                    << self->m_instanceId << ": Declaring queue " << queue << "failed: " << message;
                              // reset channel
                              self->m_channel.reset();
                              self->m_channelStatus = ChannelStatus::REQUEST;
                              AsyncHandler callback;
                              callback.swap(self->m_channelPreparationCallback);
                              callback(make_error_code(AmqpCppErrc::eCreateQueueError));
                          }
                      }); // end of failure handler
            } break;
            case ChannelStatus::CREATE_CONSUMER:
                // Use m_queue instead of m_instance since it is unique.
                // And we want automatic acknowledgement and must be the only consumer on that queue
                m_channel->consume(m_queue, AMQP::noack + AMQP::exclusive)
                      .onReceived([wSelf](const AMQP::Message& msg, uint64_t deliveryTag, bool redelivered) {
                          if (auto self = wSelf.lock()) {
                              if (redelivered) {
                                  KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                        << "Redelivered message from exchange '" << msg.exchange()
                                        << "' on routing key '" << msg.routingkey() << "', tag " << deliveryTag
                                        << ", size " << msg.bodySize();
                              }
                              // Copy of message body not avoidable although in AMQP io context here: AMQP::Message
                              // better be destructed in io context event loop and deserialisation better done elsewhere
                              auto vec = std::make_shared<std::vector<char>>(msg.body(), msg.body() + msg.bodySize());
                              if (!self->m_readHandler) { // bail out (exception won't be caught, but crash the program)
                                  throw KARABO_LOGIC_EXCEPTION(
                                        "Coding bug: AmqpClient lacks read handler, set it before subscribing!");
                              }
                              self->m_readHandler(vec, msg.exchange(), msg.routingkey());
                          }
                      })
                      .onSuccess([wSelf](const std::string& consumerTag) {
                          if (auto self = wSelf.lock()) {
                              const std::string queue(
                                    self->m_instanceId == self->m_queue ? "" : " (queue " + self->m_queue + ")");
                              KARABO_LOG_FRAMEWORK_DEBUG_C("AmqpClient") << "Consumer for id " << self->m_instanceId
                                                                         << queue << " ready, tag: " << consumerTag;
                              self->m_channelStatus = ChannelStatus::READY;
                              // TODO: Overwrite error handler that will notice if channel has a problem
                              // self->m_channel->onError([](const char* msg){});
                              AsyncHandler callback;
                              callback.swap(self->m_channelPreparationCallback);
                              callback(KARABO_ERROR_CODE_SUCCESS);
                          }
                      })
                      .onError([this, wSelf](const char* message) {
                          if (auto self = wSelf.lock()) {
                              // We may have failed because in parallel to us another instance started with the same id
                              // and we both created the queue with our id before the other one could create the
                              // consumer. The 2nd one that creates the consumer will fail here with a message like
                              //     "ACCESS_REFUSED - queue 'XXXX' in vhost '/yyyy' in exclusive use"
                              // and the channel is then not valid anymore, so start again
                              //
                              // Another case seen when a device is shutdown and quickly re-instantiated is
                              //     "NOT_FOUND - no queue 'XXXX' in vhost '/yyyy'"
                              // Maybe the queue of the previous incarnation was not yet removed from the broker
                              // (but its consumer, otherwise we would have failed in declareQueue(..).onSuccess())
                              // when we declared the queue, but now that we want to consume, the ("old") queue is gone
                              // due to its 'autodelete' flag. We start again as well.
                              self->m_channel.reset();
                              const std::string msg(message);
                              constexpr auto np = std::string::npos;
                              if ((msg.find("ACCESS_REFUSED") != np && msg.find("in exclusive use") != np) ||
                                  (msg.find("NOT_FOUND") != np && msg.find("no queue") != np)) {
                                  KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                        << "Queue " << self->m_queue << ": Consumer creation failed: '" << message
                                        << "'. Need to recreate the channel.";
                                  self->m_channelStatus = ChannelStatus::CREATE;
                                  AsyncHandler callback;
                                  callback.swap(self->m_channelPreparationCallback);
                                  self->asyncPrepareChannel(std::move(callback));
                              } else {
                                  KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                        << "Queue " << self->m_queue << ": Consumer creation failed: " << message;
                                  self->m_channelStatus = ChannelStatus::REQUEST;
                                  AsyncHandler callback;
                                  callback.swap(self->m_channelPreparationCallback);
                                  callback(make_error_code(AmqpCppErrc::eCreateConsumerError));
                              }
                          }
                      });
                break;
            case ChannelStatus::READY:
                break;
        }
    }

    void AmqpClient2::doSubscribePending(const boost::system::error_code& ec) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_ERROR << m_instanceId
                                       << ": Subscribing failed since channel preparation failed: " << ec.message();
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
                    statusAndHandler.status = SubscriptionStatus::CHECK_EXCHANGE;
                    moveSubscriptionState(exchange, routingKey);
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
            case SubscriptionStatus::PENDING: // How can this call happen?
                KARABO_LOG_FRAMEWORK_ERROR << "Nothing to do for pending subscription of '" << m_instanceId
                                           << "' to exchange '" << exchange << "' and routing key '" << routingKey
                                           << "'.";
                break;
            case SubscriptionStatus::CHECK_EXCHANGE: {
                auto exchangeIt = m_exchanges.find(exchange);
                if (exchangeIt != m_exchanges.end() && exchangeIt->second == ExchangeStatus::READY) {
                    // Exchange is known and ready, so jump directly to BIND_QUEUE step
                    it->second.status = SubscriptionStatus::BIND_QUEUE;
                } else { // If m_exchanges[exchange] == ExchangeStatus::DECLARING, we declare once more - so what?
                    it->second.status = SubscriptionStatus::DECLARE_EXCHANGE;
                }
                moveSubscriptionState(exchange, routingKey);
            } break;
            case SubscriptionStatus::DECLARE_EXCHANGE: {
                const int flags = 0; // Karabo 3: switch to AMQP::autodelete (not AMQP::durable!)
                m_channel->declareExchange(exchange, AMQP::topic, flags)
                      .onSuccess([wSelf, exchange, routingKey]() {
                          if (auto self = wSelf.lock()) {
                              self->m_exchanges[exchange] = ExchangeStatus::READY;
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << self->m_instanceId << ": Declaring exchange " << exchange << " for routing"
                                        << " key " << routingKey << " succeeded, but subscription gone!";
                              } else {
                                  KARABO_LOG_FRAMEWORK_DEBUG_C("AmqpClient")
                                        << self->m_instanceId << ": Declared exchange " << exchange;
                                  it->second.status = SubscriptionStatus::BIND_QUEUE;
                                  self->moveSubscriptionState(exchange, routingKey);
                              }
                          }
                      })
                      .onError([wSelf, exchange, routingKey](const char* message) {
                          if (auto self = wSelf.lock()) {
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << self->m_instanceId << ": Declaring exchange " << exchange
                                        << " for routing key " << routingKey << " failed, but subscription gone!";
                              } else {
                                  KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                        << self->m_instanceId << ": Declaring exchange " << exchange
                                        << " for routing key " << routingKey << " failed: " << message;
                                  self->m_exchanges.erase(exchange);
                                  const AsyncHandler callback(std::move(it->second.onSubscription));
                                  self->m_subscriptions.erase(it);
                                  callback(make_error_code(AmqpCppErrc::eCreateExchangeError));
                              }
                          }
                      });
            } break;
            case SubscriptionStatus::BIND_QUEUE: {
                m_channel->bindQueue(exchange, m_queue, routingKey)
                      .onSuccess([wSelf, exchange, routingKey]() {
                          if (auto self = wSelf.lock()) {
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << "Binding queue " << self->m_queue << " to exchange " << exchange
                                        << " with routing key " << routingKey << " succeeded, but subscription gone!";
                              } else {
                                  it->second.status = SubscriptionStatus::READY;
                                  AsyncHandler callback;
                                  callback.swap(it->second.onSubscription);
                                  callback(KARABO_ERROR_CODE_SUCCESS);
                              }
                          }
                      })
                      .onError([wSelf, exchange, routingKey](const char* message) {
                          if (auto self = wSelf.lock()) {
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << "Binding queue " << self->m_queue << " to exchange " << exchange
                                        << " with routing key " << routingKey << " failed and subscription gone!";
                              } else { // Call handler with failure
                                  KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                        << "Binding queue " << self->m_queue << " to exchange " << exchange
                                        << " with routing key " << routingKey << " failed: " << message;
                                  const AsyncHandler callback(std::move(it->second.onSubscription));
                                  self->m_subscriptions.erase(it);
                                  callback(make_error_code(AmqpCppErrc::eBindQueueError));
                              }
                          }
                      });
            } break;
            case SubscriptionStatus::READY: // Nothing anymore to do - how can this call happen?
                KARABO_LOG_FRAMEWORK_WARN << "Nothing to do for subscription of '" << m_queue << "' to exchange '"
                                          << exchange << "' and routing key '" << routingKey << "' since ready.";
                break;
            case SubscriptionStatus::UNBIND_QUEUE:
                m_channel->unbindQueue(exchange, m_queue, routingKey)
                      .onSuccess([wSelf, exchange, routingKey]() {
                          if (auto self = wSelf.lock()) {
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << "Unbinding queue " << self->m_queue << " from exchange " << exchange
                                        << " with routing key " << routingKey << " succeeded, but subscription gone!";
                              } else {
                                  const AsyncHandler callback(std::move(it->second.onSubscription));
                                  self->m_subscriptions.erase(it);
                                  callback(KARABO_ERROR_CODE_SUCCESS);
                              }
                          }
                      })
                      .onError([wSelf, exchange, routingKey](const char* message) {
                          if (auto self = wSelf.lock()) {
                              auto it = self->m_subscriptions.find({exchange, routingKey});
                              if (it == self->m_subscriptions.end()) { // Should not happen!
                                  KARABO_LOG_FRAMEWORK_ERROR_C("AmqpClient")
                                        << "Unbinding queue " << self->m_queue << " from exchange " << exchange
                                        << " with routing key " << routingKey << " failed and subscription gone!";
                              } else { // Call handler with failure, but keep subscription (but erase handler)
                                  KARABO_LOG_FRAMEWORK_WARN_C("AmqpClient")
                                        << "Unbinding queue " << self->m_queue << " from exchange " << exchange
                                        << " with routing key " << routingKey
                                        << " failed, consider subscription alive!";
                                  it->second.status = SubscriptionStatus::READY;
                                  AsyncHandler callback;
                                  callback.swap(it->second.onSubscription);
                                  callback(make_error_code(AmqpCppErrc::eUnbindQueueError));
                              }
                          }
                      });
                break;
        } // end switch

    } // end moveSubscriptionState

} // namespace karabo::net
