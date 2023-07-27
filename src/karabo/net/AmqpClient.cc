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
#include "AmqpClient.hh"

#include <boost/algorithm/string.hpp>
#include <boost/core/null_deleter.hpp>
#include <cassert>

#include "karabo/log/Logger.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/VectorElement.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::log;
using namespace boost::placeholders;


KARABO_REGISTER_FOR_CONFIGURATION_ADDON(AMQP::Table, karabo::net::AmqpClient)


//----- https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/ -----v

namespace { // anonymous namespace

    struct AmqpCppErrCategory : boost::system::error_category {
        const char* name() const noexcept override;
        std::string message(int ev) const override;
    };


    const char* AmqpCppErrCategory::name() const noexcept {
        return "amqpcpp";
    }


    std::string AmqpCppErrCategory::message(int ev) const {
        switch (static_cast<AmqpCppErrc>(ev)) {
            case AmqpCppErrc::eCreateChannelError:
                return "error creating channel";

            case AmqpCppErrc::eCreateExchangeError:
                return "error creating exchange";

            case AmqpCppErrc::eCreateQueueError:
                return "error creating queue";

            case AmqpCppErrc::eBindQueueError:
                return "error binding queue";

            case AmqpCppErrc::eCreateConsumerError:
                return "error creating consumer";

            case AmqpCppErrc::eConsumerCancelError:
                return "error cancelling consumer";

            case AmqpCppErrc::eUnbindQueueError:
                return "error unbinding queue";

            case AmqpCppErrc::eDrop:
                return "channel dropped error";

            default:
                return "(unrecognized error)";
        }
    }

    const AmqpCppErrCategory theAmqpCppErrCategory{};

} // anonymous namespace


boost::system::error_code make_error_code(AmqpCppErrc e) {
    return {static_cast<int>(e), theAmqpCppErrCategory};
}

//----- https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/ -----^


namespace karabo {
    namespace net {


        const int AmqpTransceiver::s_exchangeCreationFlags = 0; // AMQP::autodelete + AMQP::durable;
        std::unordered_map<AmqpTransceiver::State, AmqpTransceiver::State> AmqpTransceiver::s_stopTransit = {
              {eCreateChannel, eEnd},       {eCreateExchange, eCloseChannel},
              {eCheckQueue, eCloseChannel}, {eCreateQueue, eCloseChannel},
              {eBindQueue, eCloseChannel},  {eCreateConsumer, eUnbindQueue},
              {eReady, eShutdown}};


        AmqpTransceiver::AmqpTransceiver(const std::string& exchange, const std::string& queue_,
                                         const std::string& route_in, bool listener, const AMQP::Table& queueArgs)
            : m_state(eEnd),
              m_connector(),
              m_exchange(exchange),
              m_queue(queue_),
              m_route(route_in),
              m_listener(listener),
              m_queueExist(false),
              m_onMessage(nullptr),
              m_connection(nullptr),
              m_channel(),
              m_ec(KARABO_ERROR_CODE_SUCCESS),
              m_completeHandlers(),
              m_completeHandlersMutex(),
              m_queueArgs(queueArgs) {}


        AmqpTransceiver::~AmqpTransceiver() {}


        void AmqpTransceiver::onMessage(AmqpCppMessageCallback&& callback) {
            m_onMessage = std::move(callback);
        }


        void AmqpTransceiver::sendAsync(const std::shared_ptr<std::vector<char>>& payload, const std::string& route,
                                        const AsyncHandler& onComplete) {
            AmqpConnector::Pointer ptr(m_connector.lock());
            if (ptr) ptr->post(bind_weak(&AmqpTransceiver::_sendAsync, this, payload, route, onComplete));
        }


        void AmqpTransceiver::_sendAsync(const std::shared_ptr<std::vector<char>>& payload, const std::string& route,
                                         const AsyncHandler& onComplete) {
            if (m_state != eReady) {
                onComplete(KARABO_ERROR_CODE_NOT_CONNECTED);
                return;
            }
            if (!payload || !payload->size()) {
                onComplete(KARABO_ERROR_CODE_IO_ERROR);
                return;
            }

            AMQP::Envelope envelope(payload->data(), payload->size());
            bool res = m_channel->publish(m_exchange, route, envelope);
            if (res) {
                onComplete(KARABO_ERROR_CODE_SUCCESS);
            } else {
                onComplete(KARABO_ERROR_CODE_IO_ERROR);
            }
        }


        void AmqpTransceiver::startAsync(const AmqpConnector::Pointer& connector, const std::string& recvQueue,
                                         const AsyncHandler& onComplete) {
            connector->post(bind_weak(&AmqpTransceiver::_startAsync, this, connector, recvQueue, onComplete));
        }


        void AmqpTransceiver::_startAsync(const AmqpConnector::Pointer& connector, const std::string& recvQueue,
                                          const AsyncHandler& onComplete) {
            m_connector = connector;
            auto single = connector->getSingleton();
            AMQP::TcpConnection* tcpConnection = single->getConnection().get();

            if (m_connection != tcpConnection) {
                // New connection just established. Therefore we have to start transceiver from the beginning
                m_state = eEnd;
                m_channel.reset();
                m_connection = nullptr;
                m_recvQueue.clear();
                m_consumerTag.clear();
                m_queueExist = false;
                std::lock_guard<std::mutex> lock(m_completeHandlersMutex);
                m_completeHandlers.clear();
            }
            if (m_state == eReady) {
                onComplete(KARABO_ERROR_CODE_SUCCESS);
                return;
            } else {
                std::lock_guard<std::mutex> lock(m_completeHandlersMutex);
                m_completeHandlers.push_back(onComplete);
            }
            if (m_state == eEnd) {
                m_channel.reset();
                m_connection = tcpConnection;
                if (m_listener && !recvQueue.empty()) {
                    m_queue = recvQueue;
                    m_recvQueue = recvQueue;
                    m_queueExist = true;
                }
                m_error.clear();
                m_ec = KARABO_ERROR_CODE_SUCCESS;
                m_state = eCreateChannel;
                moveStateMachine();
            }
        }


        void AmqpTransceiver::stopAsync(const AsyncHandler& onComplete) {
            AmqpConnector::Pointer ptr(m_connector.lock());
            if (ptr) ptr->post(bind_weak(&AmqpTransceiver::_stopAsync, this, onComplete));
        }


        void AmqpTransceiver::_stopAsync(const AsyncHandler& onComplete) {
            m_ec = KARABO_ERROR_CODE_SUCCESS;
            {
                std::lock_guard<std::mutex> lock(m_completeHandlersMutex);
                m_completeHandlers.push_back(onComplete);
                if (m_state == eEnd) {
                    while (!m_completeHandlers.empty()) {
                        m_completeHandlers.front()(KARABO_ERROR_CODE_SUCCESS);
                        m_completeHandlers.pop_front();
                    }
                    return;
                }
            }
            if ((m_state >= eCreateChannel) && (m_state <= eReady)) {
                m_state = s_stopTransit[m_state];
                moveStateMachine();
            }
        }


        boost::system::error_code AmqpTransceiver::drop() {
            m_ec = KARABO_ERROR_CODE_SUCCESS;
            m_state = eEnd;
            moveStateMachine();
            return AmqpCppErrc::eDrop;
        }


        void AmqpTransceiver::moveStateMachine() {
            auto wSelf(weak_from_this());

            // Sequence of states for ...
            // no listener:   eCreateChannel -> eCreateExchange -> eReady
            // listener:      eCreateChannel -> eCheckQueue -> (eCreateQueue) -> eCreateExchange
            //                 -> eBindQueue -> eCreateConsumer -> eReady
            switch (m_state) {
                case eCreateChannel: {
                    auto successCb = [this, wSelf]() {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eCreateChannel) return; // obsolete callback
                        // drop unnecessary callback
                        m_channel->onError(nullptr);
                        m_state = m_listener ? eCheckQueue : eCreateExchange;
                        moveStateMachine();
                    };
                    auto failureCb = [this, wSelf](const char* message) {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eCreateChannel) return; // obsolete callback
                        m_error = "Channel creation: ";
                        m_error += message;
                        m_ec = AmqpCppErrc::eCreateChannelError;
                        m_state = eEnd;
                        moveStateMachine();
                    };
                    if (m_connection && m_connection->usable()) {
                        std::shared_ptr<AMQP::TcpChannel> channel = std::make_shared<AMQP::TcpChannel>(m_connection);
                        m_channel.swap(channel);
                        m_channel->onReady(successCb);
                        m_channel->onError(failureCb);
                    } else {
                        failureCb("no AMQP connection");
                    }
                } break;

                case eCheckQueue:
                    if (m_queueExist) {
                        m_state = eCreateExchange;
                        moveStateMachine();
                    } else if (m_queue.empty()) {
                        // Rely on unique queue name provided by broker ...
                        m_state = eCreateQueue;
                        moveStateMachine();
                    } else {
                        auto successCb = [this, wSelf](const std::string& name, int /*msgcount*/, int /*conscount*/) {
                            auto self = wSelf.lock();
                            if (!self) return;
                            if (m_state != eCheckQueue) return;
                            // queue exist!  Normally we should NOT be here. Some other application uses this name
                            // Solution: try to follow MDL solution adding suffix...
                            auto dur = std::chrono::steady_clock::now().time_since_epoch().count();
                            std::ostringstream oss;
                            oss << std::hex << dur;
                            m_queue += ":" + oss.str();
                            m_queueExist = false;
                            m_recvQueue.clear();
                            m_state = eCreateQueue;
                            moveStateMachine();
                        };
                        auto failureCb = [this, wSelf](const char* message) {
                            auto self = wSelf.lock();
                            if (!self) return;
                            if (m_state != eCheckQueue) return;
                            // queue does not exist. Logically this is not an error, but the channel is not valid.
                            // Create the new channel
                            m_state = eRecreateChannel;
                            m_queueExist = false;
                            m_recvQueue.clear();
                            moveStateMachine();
                        };
                        m_channel->declareQueue(m_queue, AMQP::passive).onSuccess(successCb).onError(failureCb);
                    }
                    break;

                case eRecreateChannel: {
                    // we are here since we checked queue on the broker and fail...
                    // ... so we cannot work with current channel ...
                    m_channel->close();
                    auto channel = std::make_shared<AMQP::TcpChannel>(m_connection);
                    m_channel.swap(channel);
                    auto successCb = [this, wSelf]() {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eRecreateChannel) return;
                        // drop unnecessary callback
                        m_channel->onError(nullptr);
                        // new channel is ready
                        m_state = m_listener ? eCreateQueue : eCreateExchange;
                        moveStateMachine();
                    };
                    auto failureCb = [this, wSelf](const char* message) {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eRecreateChannel) return;
                        m_error = "Channel creation: ";
                        m_error += message;
                        m_ec = AmqpCppErrc::eCreateChannelError;
                        m_state = eEnd;
                        moveStateMachine();
                    };
                    m_channel->onReady(successCb);
                    m_channel->onError(failureCb);
                } break;

                case eCreateExchange: {
                    auto successCb = [this, wSelf]() {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eCreateExchange) return;
                        m_state = m_listener ? eBindQueue : eReady;
                        moveStateMachine();
                    };
                    auto failureCb = [this, wSelf](const char* message) {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eCreateExchange) return;
                        m_error = "Exchange creation: ";
                        m_error += message;
                        m_ec = AmqpCppErrc::eCreateExchangeError;
                        m_state = eCloseChannel;
                        moveStateMachine();
                    };
                    m_channel->declareExchange(m_exchange, AMQP::topic, s_exchangeCreationFlags)
                          .onSuccess(successCb)
                          .onError(failureCb);
                } break;

                case eCreateQueue:
                    if (m_queueExist) {
                        m_state = eCreateExchange;
                        moveStateMachine();
                    } else {
                        auto successCb = [this, wSelf](const std::string& name, int /*msgcount*/,
                                                       int /*consumercount*/) {
                            auto self = wSelf.lock();
                            if (!self) return;
                            if (m_state != eCreateQueue) return; // obsolete callback
                            m_recvQueue = name;
                            m_queueExist = false;
                            m_state = eCreateExchange;
                            moveStateMachine();
                        };
                        auto failureCb = [this, wSelf](const char* message) {
                            auto self = wSelf.lock();
                            if (!self) return;
                            if (m_state != eCreateQueue) return;
                            m_error = "Queue creation: ";
                            m_error += message;
                            m_ec = AmqpCppErrc::eCreateQueueError;
                            m_state = eCloseChannel;
                            moveStateMachine();
                        };
                        m_channel->declareQueue(m_queue, AMQP::autodelete, m_queueArgs)
                              .onSuccess(successCb)
                              .onError(failureCb);
                    }
                    break;

                case eBindQueue: {
                    auto successCb = [this, wSelf]() {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eBindQueue) return;
                        // Create consumer if queue name is not known yet.
                        // Otherwise the consumer is running
                        m_state = !m_queueExist ? eCreateConsumer : eReady;
                        moveStateMachine();
                    };
                    auto failureCb = [this, wSelf](const char* message) {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eBindQueue) return;
                        m_error = "Queue binding: ";
                        m_error += message;
                        m_ec = AmqpCppErrc::eBindQueueError;
                        m_state = eCloseChannel;
                        moveStateMachine();
                    };
                    m_channel->bindQueue(m_exchange, m_recvQueue, m_route).onSuccess(successCb).onError(failureCb);
                } break;

                case eCreateConsumer: {
                    auto successCb = [this, wSelf](const std::string& consumer) {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eCreateConsumer) return;
                        m_queueExist = true;
                        m_consumerTag = consumer;
                        m_state = eReady;
                        moveStateMachine();
                    };
                    auto receivedCb = [this, wSelf](const AMQP::Message& message, uint64_t deliveryTag,
                                                    bool redelivered) {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_channel) {
                            if (m_state >= eCreateConsumer && m_onMessage) {
                                // PROCESS INCOMING MESSAGES
                                m_onMessage(message, deliveryTag, redelivered);
                            }
                        }
                    };
                    auto failureCb = [this, wSelf](const char* message) {
                        auto self = wSelf.lock();
                        if (!self) return;
                        if (m_state != eCreateConsumer) return;
                        m_error = "Consumer creation: ";
                        m_error += message;
                        m_ec = AmqpCppErrc::eCreateConsumerError;
                        m_state = eShutdown;
                        moveStateMachine();
                    };
                    // Automatic acknowledgement
                    m_channel->consume(m_recvQueue, AMQP::noack)
                          .onSuccess(successCb)
                          .onReceived(receivedCb)
                          .onError(failureCb);
                } break;

                case eReady: {
                    AmqpConnector::Pointer connector(m_connector.lock());
                    if (!connector) return; // parent (being) destructed
                    auto publisher = connector->getPublisher();
                    if (!m_listener) {
                        // Connection problem should be caught and handled by ConnectionHandler object
                        // so remove callback below
                        m_channel->onError(nullptr);
                        // Check and possibly assign shared publisher to use single channel
                        if (!publisher || !publisher->usable()) {
                            connector->setPublisher(m_channel);
                        } else {
                            m_channel->close();
                            m_channel = publisher;
                        }
                    } else {
                        // m_listener == True
                        // Keep only those channels that have created a consumer
                        // In current design we create one consumer (one queue) per connector!
                        if (m_consumerTag.empty()) {
                            if (!publisher || !publisher->usable()) {
                                connector->setPublisher(m_channel);
                            } else {
                                m_channel->close();
                                m_channel = publisher;
                            }
                        }
                    }
                    {
                        // Call completion handlers registered so far...
                        std::lock_guard<std::mutex> lock(m_completeHandlersMutex);
                        while (!m_completeHandlers.empty()) {
                            connector->post(std::bind(m_completeHandlers.front(), KARABO_ERROR_CODE_SUCCESS));
                            m_completeHandlers.pop_front();
                        }
                    }
                } break;

                case eShutdown: {
                    AmqpConnector::Pointer connector(m_connector.lock());
                    if (!connector) return; // parent (being) destructed
                    auto publisher = connector->getPublisher();
                    if (m_channel == publisher) {
                        m_channel.reset();
                        // ... create a channel
                        auto successCb = [this, wSelf]() {
                            auto self = wSelf.lock();
                            if (!self) return;
                            if (m_state != eShutdown) return; // obsolete callback
                            // call eShutdown again but with proper m_channel, so no cycling
                            m_state = eShutdown;
                            moveStateMachine();
                        };
                        auto failureCb = [this, wSelf](const char* message) {
                            auto self = wSelf.lock();
                            if (!self) return;
                            // if (m_state != eShutdown) return; // obsolete callback
                            m_error = "eShutdown: channel re-creation: ";
                            m_error += message;
                            m_ec = AmqpCppErrc::eCreateChannelError;
                            m_state = eEnd;
                            moveStateMachine();
                        };
                        if (m_connection && m_connection->usable()) {
                            std::shared_ptr<AMQP::TcpChannel> channel =
                                  std::make_shared<AMQP::TcpChannel>(m_connection);
                            m_channel.swap(channel);
                            m_channel->onReady(successCb);
                            m_channel->onError(failureCb);
                        } else {
                            failureCb("eShutdown: no AMQP connection");
                        }
                    }
                    if (m_listener) {
                        if (m_consumerTag.empty()) {
                            m_state = eUnbindQueue;
                            moveStateMachine();
                        } else {
                            auto successCb = [this, wSelf](const std::string& consumer) {
                                auto self = wSelf.lock();
                                if (!self) return;
                                if (m_consumerTag != consumer) return;
                                m_state = eUnbindQueue;
                                moveStateMachine();
                            };
                            auto failureCb = [this, wSelf](const char* message) {
                                auto self = wSelf.lock();
                                if (!self) return;
                                m_error = "Consumer cancel: ";
                                m_error += message;
                                m_ec = AmqpCppErrc::eConsumerCancelError;
                                m_state = eUnbindQueue;
                                moveStateMachine();
                            };
                            m_channel->cancel(m_consumerTag).onSuccess(successCb).onError(failureCb);
                        }
                    } else {
                        m_state = eCloseChannel;
                        moveStateMachine();
                    }
                } break;

                case eUnbindQueue:
                    if (m_recvQueue.empty()) {
                        // TODO: impossible???
                        m_state = eCloseChannel;
                        moveStateMachine();
                    } else {
                        // replace broken channel and unbind queue
                        m_channel->close();
                        std::shared_ptr<AMQP::TcpChannel> channel = std::make_shared<AMQP::TcpChannel>(m_connection);
                        m_channel.swap(channel);
                        auto successCb = [this, wSelf]() {
                            auto self = wSelf.lock();
                            if (!self) return;
                            if (m_state != eUnbindQueue) return;
                            // drop unnecessary callback
                            m_channel->onError(nullptr);
                            auto successCbForUnbind = [this, wSelf]() {
                                auto self = wSelf.lock();
                                if (!self) return;
                                m_state = eCloseChannel;
                                moveStateMachine();
                            };
                            auto failureCbForUnbind = [this, wSelf](const char* message) {
                                auto self = wSelf.lock();
                                if (!self) return;
                                m_error = "Queue unbinding: ";
                                m_error += message;
                                m_ec = AmqpCppErrc::eUnbindQueueError;
                                m_state = eCloseChannel;
                                moveStateMachine();
                            };
                            m_channel->unbindQueue(m_exchange, m_recvQueue, m_route)
                                  .onSuccess(successCbForUnbind)
                                  .onError(failureCbForUnbind);
                        };
                        auto failureCb = [this, wSelf](const char* message) {
                            auto self = wSelf.lock();
                            if (!self) return;
                            if (m_state != eUnbindQueue) return;
                            m_error = "Channel restoration: ";
                            m_error += message;
                            m_ec = AmqpCppErrc::eCreateChannelError;
                            m_state = eEnd;
                            moveStateMachine();
                        };
                        m_channel->onReady(successCb);
                        m_channel->onError(failureCb);
                    }
                    break;

                case eCloseChannel:
                    // Don't call channel close() directly...
                    // Channel destructor calls close()
                    m_state = eEnd;
                    moveStateMachine();
                    break;

                case eEnd:
                    m_connection = nullptr;
                    m_recvQueue.clear();
                    m_consumerTag.clear();
                    m_queueExist = false;
                    m_channel.reset();
                    {
                        AmqpConnector::Pointer connector(m_connector.lock());
                        if (!connector) return; // parent (being) destructed, bail out
                        std::lock_guard<std::mutex> lock(m_completeHandlersMutex);
                        while (!m_completeHandlers.empty()) {
                            connector->post(std::bind(m_completeHandlers.front(), KARABO_ERROR_CODE_SUCCESS));
                            m_completeHandlers.pop_front();
                        }
                    }
                    break;

                case eMax: // to please the compiler and still do not have to add 'default:'
                    break;
            }
        }


        //------------------------------------------------------------------------------------ ConnectionHandler


        ConnectionHandler::ConnectionHandler(boost::asio::io_context& ctx) : AMQP::LibBoostAsioHandler(ctx) {}


        //-------------------------------------------------------------------------------- AmqpSingletonImpl


        AmqpSingletonImpl::AmqpSingletonImpl(boost::asio::io_context& iocontext)
            : m_ioctx(iocontext),
              m_handler(),
              m_connection(),
              m_connectors(),
              m_connectorsMutex(),
              m_urls(),
              m_state(eNotConnected),
              m_error(""),
              m_attemptCounter(0),
              m_onComplete(),
              m_stopWaiting(true),
              m_waitingMutex(),
              m_activateCondition() {}


        AmqpSingletonImpl::~AmqpSingletonImpl() {
            try {
                if (m_connection) m_connection->close();
            } catch (...) {
            }
            m_connection.reset();
            m_handler.reset();
        }


        void AmqpSingletonImpl::autoReconnect(const std::list<std::string>& urls) {
            if (!urls.empty()) m_urls = urls;
            auto promi = std::make_shared<std::promise<boost::system::error_code>>();
            auto futur = promi->get_future();
            m_attemptCounter = 12;
            connectAsync([promi](const boost::system::error_code& ec) { promi->set_value(ec); });
            futur.get();
        }


        void AmqpSingletonImpl::onAttachedCallback(AMQP::TcpConnection*, const std::string& url) {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo.net.AmqpSingletonImpl")
                  << "ConnectionHandler::onAttached. url=" << url;
            m_state = eNotConnected;
            m_error = "";
        }

        void AmqpSingletonImpl::onConnectedCallback(AMQP::TcpConnection*, const std::string& url) {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo.net.AmqpSingletonImpl")
                  << "ConnectionHandler::onConnected : phys. connection OK. url=" << url;
            m_state = eConnectionDone;
        }


        void AmqpSingletonImpl::onReadyCallback(AMQP::TcpConnection* connection, const std::string& url) {
            if (!connection) return;
            // At this point, the m_connection is initialized and ready
            m_state = eConnectionReady;
            KARABO_LOG_FRAMEWORK_INFO_C("karabo.net.AmqpSingletonImpl")
                  << "Successful connection (onReady) to \"" << url << "\"";
            assert(m_connection.get() == connection);
            // arm counter
            m_attemptCounter = 12;
            notifyConnectorsToStart();
            if (!m_onComplete) return;
            // Call onComplete handler that probably sets value to promise
            post(boost::bind(m_onComplete, KARABO_ERROR_CODE_SUCCESS));
            // onComplete should be called only once ... so in case of reconnection
            // we will have null handler
            m_onComplete = AsyncHandler();
        }


        void AmqpSingletonImpl::onErrorCallback(AMQP::TcpConnection* connection, const char* message,
                                                const std::string& url) {
            KARABO_LOG_FRAMEWORK_INFO_C("karabo.net.AmqpSingletonImpl")
                  << "ConnectionHandler::onError: \"" << message << "\". url=" << url;
            m_state = eConnectionError;
            m_error = message;
        }


        void AmqpSingletonImpl::onClosedCallback(AMQP::TcpConnection*, const std::string& url) {
            KARABO_LOG_FRAMEWORK_INFO_C("karabo.net.AmqpSingletonImpl") << "ConnectionHandler::onClosed. url=" << url;
            m_state = eConnectionClosed;
            m_error = "";
        }


        void AmqpSingletonImpl::onLostCallback(AMQP::TcpConnection*) {
            KARABO_LOG_FRAMEWORK_INFO_C("karabo.net.AmqpSingletonImpl") << "ConnectionHandler::onLost";
            m_state = eConnectionLost;
        }


        void AmqpSingletonImpl::onDetachedCallback(AMQP::TcpConnection* connection) {
            KARABO_LOG_FRAMEWORK_INFO_C("karabo.net.AmqpSingletonImpl")
                  << "ConnectionHandler::onDetached for node \"" << this->url() << "\"";
            notifyConnectorsToDrop(m_error);
            if (m_state == eConnectionClosed && !m_onComplete) {
                return;
            } else {
                // Update list to the next url candidate on the top...F
                m_urls.push_back(m_urls.front());
                m_urls.pop_front();
                KARABO_LOG_FRAMEWORK_DEBUG_C("karabo.net.AmqpSingletonImpl")
                      << "ConnectionHandler::onDetached : try to re-connect to \"" << this->url() << "\"";
                if (--m_attemptCounter > 0) {
                    // ... and re-connect asynchronously ...
                    post(bind_weak(&AmqpSingletonImpl::connectAsync, this, m_onComplete));
                } else {
                    // ... or re-connect later ...
                    KARABO_LOG_FRAMEWORK_INFO_C("karabo.net.AmqpSingletonImpl") << "Reconnecting after 5 seconds.";
                    auto timer = std::make_shared<boost::asio::deadline_timer>(m_ioctx);
                    timer->expires_from_now(boost::posix_time::seconds(5));
                    timer->async_wait(bind_weak(&AmqpSingletonImpl::onWaitCallback, this, _1, timer));
                }
            }
        }


        void AmqpSingletonImpl::onWaitCallback(const boost::system::error_code& ec,
                                               const std::shared_ptr<boost::asio::deadline_timer>&) {
            if (ec) return;
            m_attemptCounter = 12;
            connectAsync(m_onComplete);
        }


        void AmqpSingletonImpl::connectAsync(const AsyncHandler& onComplete) {
            // Use first url in the list ...
            const std::string& url = m_urls.front();
            m_error.clear();
            // save complete handler ...
            m_onComplete = onComplete;
            AMQP::Address address(url);

            // Create and setup with callbacks a new ConnectionHandler ...
            m_handler = std::make_shared<ConnectionHandler>(m_ioctx);
            m_handler->setOnAttachedHandler(bind_weak(&AmqpSingletonImpl::onAttachedCallback, this, _1, url));
            m_handler->setOnConnectedHandler(bind_weak(&AmqpSingletonImpl::onConnectedCallback, this, _1, url));
            m_handler->setOnReadyHandler(bind_weak(&AmqpSingletonImpl::onReadyCallback, this, _1, url));
            m_handler->setOnErrorHandler(bind_weak(&AmqpSingletonImpl::onErrorCallback, this, _1, _2, url));
            m_handler->setOnClosedHandler(bind_weak(&AmqpSingletonImpl::onClosedCallback, this, _1, url));
            m_handler->setOnLostHandler(bind_weak(&AmqpSingletonImpl::onLostCallback, this, _1));
            m_handler->setOnDetachedHandler(bind_weak(&AmqpSingletonImpl::onDetachedCallback, this, _1));

            // Create connection using ConnectionHandler above and address ...
            m_connection = std::make_shared<AMQP::TcpConnection>(m_handler.get(), address);
        }


        void AmqpSingletonImpl::registerConnector(const boost::shared_ptr<AmqpConnector>& ptr) {
            std::lock_guard<std::mutex> lk(m_connectorsMutex);
            m_connectors.insert(ptr);
        }


        void AmqpSingletonImpl::unregisterConnector(const boost::shared_ptr<AmqpConnector>& ptr) {
            std::lock_guard<std::mutex> lk(m_connectorsMutex);
            m_connectors.erase(ptr);
        }


        void AmqpSingletonImpl::notifyConnectorsToDrop(const std::string& message) {
            KARABO_LOG_FRAMEWORK_ERROR_C("karabo.net.AmqpSingletonImpl")
                  << "Connection error for node \"" << this->url() << "\" ...\n\t\t\t\t" << message;
            // Inform users that connection and channels cannot be used
            {
                std::lock_guard<std::mutex> lk(m_waitingMutex);
                m_stopWaiting = false;
            }
            std::lock_guard<std::mutex> lk(m_connectorsMutex);
            for (auto it = m_connectors.begin(); it != m_connectors.end();) {
                auto connector = it->lock();
                // Keep only "alive" connectors and drop "dead" ones ...
                if (connector) {
                    connector->onDropConnectorCallback(message);
                    ++it;
                } else {
                    it = m_connectors.erase(it);
                }
            }
        }


        void AmqpSingletonImpl::notifyConnectorsToStart() {
            std::lock_guard<std::mutex> lk(m_connectorsMutex);
            for (auto i = m_connectors.begin(); i != m_connectors.end(); ++i) {
                auto connector = i->lock();
                if (connector) post(bind_weak(&AmqpConnector::onStartConnectorCallback, connector.get()));
            }
        }


        bool AmqpSingletonImpl::allConnectorsStarted() {
            std::lock_guard<std::mutex> lk(m_connectorsMutex);
            for (auto i = m_connectors.begin(); i != m_connectors.end(); ++i) {
                auto connector = i->lock();
                if (connector && !connector->isActivated()) return false;
            }
            return true;
        }


        void AmqpSingletonImpl::showConnectorsStatus() {
            std::lock_guard<std::mutex> lk(m_connectorsMutex);
            for (auto i = m_connectors.begin(); i != m_connectors.end(); ++i) {
                auto connector = i->lock();
                if (!connector) {
                    KARABO_LOG_FRAMEWORK_WARN_C("karabo.net.AmqpSingletonImpl") << "!!! Dead entry !!!";
                } else if (!connector->isActivated()) {
                    KARABO_LOG_FRAMEWORK_WARN_C("karabo.net.AmqpSingletonImpl")
                          << std::left << std::setw(40) << connector->id() << std::right << std::setw(15)
                          << "not activated";
                } else {
                    KARABO_LOG_FRAMEWORK_INFO_C("karabo.net.AmqpSingletonImpl")
                          << std::left << std::setw(40) << connector->id() << std::right << std::setw(15)
                          << "activated";
                }
            }
        }


        void AmqpSingletonImpl::waits() {
            using namespace std::chrono_literals;
            if (!allConnectorsStarted()) {
                std::unique_lock<std::mutex> lck(m_waitingMutex);
                auto interval = 30s;
                if (!m_stopWaiting) {
                    // if wait_for returns true the stopWaiting is getting true
                    auto self(shared_from_this());
                    auto cb = [this, self] { return m_stopWaiting; };
                    if (m_activateCondition.wait_for(lck, interval, cb)) return;
                    m_stopWaiting = true;
                    lck.unlock();
                    KARABO_LOG_FRAMEWORK_WARN_C("karabo.net.AmqpSingletonImpl")
                          << "Reconnection time period (" << interval.count()
                          << " seconds) is over but not all connectors are activated";
                    showConnectorsStatus();
                }
            }
        }


        void AmqpSingletonImpl::notifyAll() {
            {
                std::lock_guard<std::mutex> lock(m_waitingMutex);
                m_stopWaiting = true;
            }
            m_activateCondition.notify_all();
        }


        void AmqpSingletonImpl::close() {
            if (m_connection) {
                m_connection->close();
                m_connection.reset();
            }
        }


        //------------------------------------------------------------------------------------ AmqpConnector


        AmqpConnector::AmqpConnector(const std::vector<std::string>& urls, const std::string& id,
                                     const AMQP::Table& queueArgs)
            : m_urls(urls),
              m_instanceId(id),
              m_publisher(),
              m_transceivers(),
              m_connectorActivated(true),
              m_activateMutex(),
              m_amqp(std::make_shared<AmqpSingleton>(EventLoop::getIOService().get_executor().context(), urls)),
              m_queueArgs(queueArgs) {}


        AmqpConnector::~AmqpConnector() {}


        void AmqpConnector::onDropConnectorCallback(const std::string& message) {
            // drop all transceivers
            dropAndEraseIfShutdown();
        }


        void AmqpConnector::onStartConnectorCallback() {
            // start all transceivers
            // TODO: Check whether posting is needed - we should already be on the single threaded event loop
            m_amqp->post([this, wSelf{weak_from_this()}]() {
                auto self = wSelf.lock();
                if (!self) return;
                startAsync([this, wSelf](const boost::system::error_code& ec) {
                    auto self = wSelf.lock();
                    if (!self) return;
                    if (!ec) {
                        std::lock_guard<std::mutex> lock(m_activateMutex);
                        m_connectorActivated = true;
                    } else {
                        KARABO_LOG_FRAMEWORK_WARN_C("karabo.net.AmqpConnector")
                              << "\"" << m_instanceId << "\"\t: transceivers' recovery resulted in: #" << ec.value()
                              << " -- " << ec.message();
                    }
                    if (m_amqp->allConnectorsStarted()) {
                        KARABO_LOG_FRAMEWORK_INFO_C("karabo.net.AmqpConnector")
                              << "All connectors activated successfully";
                        m_amqp->notifyAll();
                    }
                });
            });
        }


        const std::string AmqpConnector::getRecvQueueName() {
            std::lock_guard<std::mutex> lock(m_transceiversMutex);
            for (const auto& item : m_transceivers) {
                const auto& t = item.second;
                if (t->isListener() && t->ready()) return t->recvQueueName();
            }
            return "";
        }


        AmqpTransceiver::Pointer AmqpConnector::transceiver(const std::string& exchange, const std::string& queue,
                                                            const std::string& bindingKey, bool listener) {
            std::lock_guard<std::mutex> lock(m_transceiversMutex);
            if (m_transceivers.empty()) m_amqp->registerConnector(shared_from_this());
            const std::string key = transceiverKey(exchange, bindingKey, listener);
            auto it = m_transceivers.find(key);
            if (it != m_transceivers.end()) {
                auto t = it->second;
                if (!listener || t->isListener()) return t;
            }
            // Create new transceiver
            auto t = boost::make_shared<AmqpTransceiver>(exchange, queue, bindingKey, listener, m_queueArgs);
            // Update existing entry or insert new one
            m_transceivers[key] = t;
            return t;
        }


        bool AmqpConnector::has(const std::string& exchange, const std::string& bindingKey, bool listener) {
            std::string key = transceiverKey(exchange, bindingKey, listener);
            std::lock_guard<std::mutex> lock(m_transceiversMutex);
            if (m_transceivers.find(key) == m_transceivers.end()) return false;
            return true;
        }


        void AmqpConnector::open(const AmqpTransceiver::Pointer& t, const AsyncHandler& onComplete) {
            if (!this->ready()) {
                // Connection is broken ...
                post(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
            } else {
                std::string recvQueue;
                if (t->isListener()) recvQueue = getRecvQueueName(); // existing queue name
                // Connection OK and transceiver is in eEnd state ... start transceiver ...
                t->startAsync(shared_from_this(), recvQueue, onComplete);
            }
        }


        void AmqpConnector::close(const AmqpTransceiver::Pointer& t, const AsyncHandler& onComplete) {
            t->stopAsync(onComplete);
        }


        void AmqpConnector::sendDelayedCallback(const boost::system::error_code& ec, const std::string& exchange,
                                                const std::string& routingKey,
                                                const std::shared_ptr<std::vector<char>>& data,
                                                const AsyncHandler& onComplete,
                                                const std::shared_ptr<boost::asio::deadline_timer>& timer) {
            if (!ec) {
                if (this->ready()) {
                    // Connection is established
                    sendAsync(exchange, routingKey, data, onComplete);
                } else {
                    // No coneection yet ...
                    sendAsyncDelayed(exchange, routingKey, data, onComplete); // TODO: re-use timer?
                }
            } else {
                // Waiting was cancelled?
                onComplete(ec);
            }
        }


        void AmqpConnector::sendAsyncDelayed(const std::string& exchange, const std::string& routingKey,
                                             const std::shared_ptr<std::vector<char>>& data,
                                             const AsyncHandler& onComplete) {
            {
                std::lock_guard<std::mutex> lock(m_activateMutex);
                m_connectorActivated = false; // block sending for all connectors
            }
            auto timer = std::make_shared<boost::asio::deadline_timer>(m_amqp->ioContext());
            timer->expires_from_now(boost::posix_time::seconds(1));
            timer->async_wait(bind_weak(&AmqpConnector::sendDelayedCallback, this, _1, exchange, routingKey, data,
                                        onComplete, timer));
        }


        void AmqpConnector::sendAsync(const std::string& exchange, const std::string& routingKey,
                                      const std::shared_ptr<std::vector<char>>& data, const AsyncHandler& onComplete) {
            auto self(shared_from_this());
            // Wait until all connectors are activated
            m_amqp->waits();

            // Ignore
            auto t = transceiver(exchange, "", "", false);
            if (t->isBusy()) {
                // transeceiver is trying to reach eReady or eEnd state
                sendAsyncDelayed(exchange, routingKey, data, onComplete);
                return;
            }

            // Callback for 'open' ...
            auto cb = [this, self, t, exchange, routingKey, data, onComplete](const boost::system::error_code& ec) {
                // Callback for sending ...
                auto sendComplete = [this, self, t, exchange, routingKey, data,
                                     onComplete](const boost::system::error_code& ec) {
                    if (!ec) {
                        // Transceiver 'send' is OK
                        onComplete(KARABO_ERROR_CODE_SUCCESS);
                    } else {
                        KARABO_LOG_FRAMEWORK_ERROR_C("karabo.net.AmqpConnector")
                              << m_instanceId << ": transceiver's sendAsync failed to send a message to exchange: \""
                              << exchange << "\" and routingKey: \"" << routingKey
                              << "\" due to error code=" << ec.value() << " -- " << ec.message() << " (\"" << t->error()
                              << "\")";
                        // Lost connection?
                        sendAsyncDelayed(exchange, routingKey, data, onComplete);
                    }
                };

                if (!ec) {
                    // 'open' succeeded === t->ready() is true...
                    t->sendAsync(data, routingKey, sendComplete);
                } else {
                    // 'open' failed ... either connection broken or some problem in activation process.
                    // t is in eEnd state...
                    KARABO_LOG_FRAMEWORK_ERROR_C("karabo.net.AmqpConnector")
                          << m_instanceId << ": transceiver's open failed to send a message to exchange: \"" << exchange
                          << "\" and routingKey: \"" << routingKey << "\" due to error code=" << ec.value() << " -- "
                          << ec.message() << " (\"" << t->error() << "\")";
                    // Lost connection while declaring exchange?
                    sendAsyncDelayed(exchange, routingKey, data, onComplete);
                }
            };
            if (t->ready()) {
                // Directly call the "callback" - not the most elegant way, but little overhead
                cb(KARABO_ERROR_CODE_SUCCESS);
            } else {
                open(t, cb);
            }
        }


        void AmqpConnector::_onOpen(const boost::system::error_code& ec, std::queue<std::string> paths,
                                    const AsyncHandler& onComplete) {
            AmqpTransceiver::Pointer t(nullptr);
            while (!t) {
                if (paths.size() == 0 || ec) {
                    onComplete(ec);
                    return;
                }
                const std::string& key = paths.front();
                std::lock_guard<std::mutex> lock(m_transceiversMutex);
                auto it = m_transceivers.find(key);
                if (it != m_transceivers.end()) t = it->second;
                paths.pop();
            }
            open(t, bind_weak(&AmqpConnector::_onOpen, this, _1, paths, onComplete));
        }


        void AmqpConnector::removeSubscription(const std::string& exchange, const std::string& routingKey,
                                               const AsyncHandler& onComplete) {
            const std::string key = transceiverKey(exchange, routingKey, true);
            std::lock_guard<std::mutex> lock(m_transceiversMutex);
            auto it = m_transceivers.find(key);
            if (it != m_transceivers.end()) {
                auto& t = it->second;
                auto weakSelf(weak_from_this());
                t->stopAsync([this, weakSelf, onComplete, key](const boost::system::error_code ec) {
                    auto self = weakSelf.lock();
                    if (self) {
                        std::lock_guard<std::mutex> lock(m_transceiversMutex);
                        m_transceivers.erase(key);
                    }
                    onComplete(ec);
                });
            } else {
                // no subscription found, so report sucess that now unsubscribed (logic like elsewhere in Amqp classes)
                post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
            }
        }


        /**
         * @brief Activate ('open') all transceivers known in connector
         *
         * @param onComplete callback
         */
        void AmqpConnector::startAsync(const AsyncHandler& onComplete) {
            std::queue<std::string> paths;
            AmqpTransceiver::Pointer t;
            {
                std::lock_guard<std::mutex> lock(m_transceiversMutex);
                for (auto it = m_transceivers.begin(); it != m_transceivers.end(); ++it) paths.push(it->first);
                if (paths.size() == 0) {
                    post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                    return;
                }
                t = m_transceivers.at(paths.front());
                paths.pop();
            }
            open(t, bind_weak(&AmqpConnector::_onOpen, this, _1, paths, onComplete));
        }


        void AmqpConnector::_onClose(const boost::system::error_code& ec, std::queue<std::string> paths,
                                     const AsyncHandler& onComplete) {
            AmqpTransceiver::Pointer t(nullptr);
            while (!t) {
                if (paths.size() == 0 || ec) {
                    if (paths.size() == 0) {
                        m_publisher.reset();
                    }
                    post(boost::bind(onComplete, ec));
                    return;
                }
                const std::string& key = paths.front();
                std::lock_guard<std::mutex> lock(m_transceiversMutex);
                if (m_transceivers.find(key) != m_transceivers.end()) t = m_transceivers.at(key);
                paths.pop();
            }
            close(t, bind_weak(&AmqpConnector::_onClose, this, _1, paths, onComplete));
        }


        void AmqpConnector::closeAsync(const AsyncHandler& onComplete) {
            std::queue<std::string> paths;
            AmqpTransceiver::Pointer t;
            {
                std::lock_guard<std::mutex> lock(m_transceiversMutex);
                for (auto it = m_transceivers.begin(); it != m_transceivers.end(); ++it) paths.push(it->first);
                if (paths.size() == 0) {
                    m_publisher.reset();
                    post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                    return;
                }
                t = m_transceivers.at(paths.front());
                paths.pop();
            }
            close(t, bind_weak(&AmqpConnector::_onClose, this, _1, paths, onComplete));
        }


        void AmqpConnector::stopAsync(const AsyncHandler& onComplete) {
            auto self(shared_from_this());
            closeAsync([this, self, onComplete](const boost::system::error_code& ec) {
                dropAndEraseIfShutdown(true);
                post(boost::bind(onComplete, ec));
            });
        }


        boost::system::error_code AmqpConnector::dropAndEraseIfShutdown(bool eraseFlag) {
            {
                std::lock_guard<std::mutex> lk(m_activateMutex);
                m_connectorActivated = false;
            }
            std::lock_guard<std::mutex> lock(m_transceiversMutex);
            for (auto it = m_transceivers.begin(); it != m_transceivers.end();) {
                auto t = it->second;
                t->drop();
                if (eraseFlag) {
                    it = m_transceivers.erase(it);
                } else {
                    ++it;
                }
            }
            m_publisher.reset();
            if (m_transceivers.empty()) m_amqp->unregisterConnector(shared_from_this());
            return KARABO_ERROR_CODE_SUCCESS;
        }


        //------------------------------------------------------------------------------------ AmqpClient


        void AmqpClient::expectedParameters(Schema& expected) {
            VECTOR_STRING_ELEMENT(expected)
                  .key("brokers")
                  .displayedName("Broker URLs")
                  .description("Vector of URLs")
                  .assignmentMandatory()
                  .minSize(1)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("instanceId")
                  .displayedName("Instance ID")
                  .description("Instance ID")
                  .assignmentOptional()
                  .defaultValue("none")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("domain")
                  .displayedName("Domain")
                  .description("Domain is root topic (former JMS topic)")
                  .assignmentMandatory()
                  .commit();

            unsigned int defTimeout = 10;
            const char* env = getenv("KARABO_AMQP_TIMEOUT");
            if (env) {
                const unsigned int envInt = util::fromString<unsigned int>(env);
                defTimeout = (envInt > 0 ? envInt : defTimeout);
                KARABO_LOG_FRAMEWORK_INFO << "AMQP timeout from environment: " << defTimeout;
            }

            UINT32_ELEMENT(expected)
                  .key("amqpRequestTimeout")
                  .displayedName("AMQP request timeout")
                  .description("AMQP request timeout in seconds")
                  .assignmentOptional()
                  .defaultValue(defTimeout)
                  .unit(Unit::SECOND)
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("skipFlag")
                  .displayedName("Skip body deserialization")
                  .description("Skip body deserialization, i.e. keep message body as a binary blob")
                  .assignmentOptional()
                  .defaultValue(false)
                  .commit();
        }


        AmqpClient::AmqpClient(const karabo::util::Hash& input, const AMQP::Table& queueArgs)
            : m_brokerUrls(input.get<std::vector<std::string>>("brokers")),
              m_binarySerializer(karabo::io::BinarySerializer<karabo::util::Hash>::create("Bin")),
              m_domain(input.get<std::string>("domain")),
              m_instanceId(input.get<std::string>("instanceId")),
              m_amqpRequestTimeout(input.get<unsigned int>("amqpRequestTimeout")),
              m_connector(),
              m_serializerStrand(boost::make_shared<Strand>(EventLoop::getIOService())),
              m_strand(boost::make_shared<Strand>(EventLoop::getIOService())),
              m_onRead(),
              m_skipFlag(input.get<bool>("skipFlag")),
              m_queueArgs(queueArgs) {}


        AmqpClient::~AmqpClient() {}


        boost::system::error_code AmqpClient::connect() {
            // Create connector object which blocks only if no connection is established before.
            // Otherwise it doesn't block and uses beforehand established connection
            std::string connectorId = m_domain + "." + m_instanceId;
            m_connector = boost::make_shared<AmqpConnector>(m_brokerUrls, connectorId, m_queueArgs);
            // At this point we always have successfully established connection.
            // If m_brokerUrls contains no valid AMQP brokers, the previous statement blocks forever.
            return KARABO_ERROR_CODE_SUCCESS;
        }


        void AmqpClient::onMessageReceived(const AMQP::Message& m, uint64_t deliveryTag, bool /*redelivered*/) {
            // Check if we have handler registered for this message...
            if (!m_onRead) return;
            const auto& exchange = m.exchange();
            const auto& key = m.routingkey();
            auto vec = std::make_shared<std::vector<char>>(m.body(), m.body() + m.bodySize());
            m_serializerStrand->post(bind_weak(&AmqpClient::deserialize, this, exchange, key, vec));
        }


        void AmqpClient::deserialize(const std::string& exch, const std::string& key,
                                     const std::shared_ptr<std::vector<char>>& vec) {
            karabo::util::Hash::Pointer msg = boost::make_shared<Hash>();
            Hash::Pointer header(boost::make_shared<Hash>());
            size_t bytes = m_binarySerializer->load(*header, vec->data(), vec->size());
            header->set<std::string>("exchange", exch);
            header->set<std::string>("routingkey", key);
            msg->set("header", header);
            if (m_skipFlag) {
                std::vector<char>& raw = msg->bindReference<std::vector<char>>("raw");
                std::copy(vec->data() + bytes, vec->data() + vec->size(), std::back_inserter(raw));
            } else {
                Hash::Pointer body(boost::make_shared<Hash>());
                m_binarySerializer->load(*body, vec->data() + bytes, vec->size() - bytes);
                msg->set("body", body);
            }
            m_strand->post(boost::bind(m_onRead, KARABO_ERROR_CODE_SUCCESS, msg));
        }


        bool AmqpClient::isConnected() const {
            return (m_connector && m_connector->ready());
        }


        boost::system::error_code AmqpClient::disconnect() {
            auto promi = std::make_shared<std::promise<boost::system::error_code>>();
            auto futur = promi->get_future();
            auto cb = [promi](const boost::system::error_code& ec) { promi->set_value(ec); };

            disconnectAsync(cb);

            auto status = futur.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return futur.get();
        }


        void AmqpClient::disconnectAsync(const AsyncHandler& onComplete) {
            if (!m_connector) post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
            m_connector->stopAsync(onComplete);
        }


        boost::system::error_code AmqpClient::close() {
            auto promi = std::make_shared<std::promise<boost::system::error_code>>();
            auto futur = promi->get_future();
            auto cb = [promi](const boost::system::error_code& ec) { promi->set_value(ec); };

            closeAsync(cb);

            auto status = futur.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return futur.get();
        }


        void AmqpClient::closeAsync(const AsyncHandler& onComplete) {
            if (m_connector && m_connector->ready()) {
                m_connector->closeAsync(onComplete);
            } else {
                post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
            }
        }


        boost::system::error_code AmqpClient::subscribe(const std::string& exchange, const std::string& routingKey) {
            auto promi = std::make_shared<std::promise<boost::system::error_code>>();
            auto futur = promi->get_future();
            auto cb = [promi](const boost::system::error_code& ec) { promi->set_value(ec); };

            subscribeAsync(exchange, routingKey, cb);

            auto status = futur.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return futur.get();
        }


        void AmqpClient::subscribeAsync(const std::string& exchange, const std::string& routingKey,
                                        const AsyncHandler& onComplete) {
            // Check if we have valid AmqpConnector::Pointer object ...
            if (!m_connector) {
                m_strand->post(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }
            // ... and it is in "ready" state ...
            if (!m_connector->ready()) {
                m_strand->post(boost::bind(onComplete, KARABO_ERROR_CODE_NOT_CONNECTED));
                return;
            }
            try {
                // Add new subscription parameters ...
                m_connector->addSubscription(exchange, routingKey,
                                             bind_weak(&AmqpClient::onMessageReceived, this, _1, _2, _3), onComplete);
            } catch (const std::exception& e) {
                throw;
            }
        }


        boost::system::error_code AmqpClient::unsubscribe(const std::string& exchange, const std::string& routingKey) {
            auto promi = std::make_shared<std::promise<boost::system::error_code>>();
            auto futur = promi->get_future();
            auto cb = [promi](const boost::system::error_code& ec) { promi->set_value(ec); };

            unsubscribeAsync(exchange, routingKey, cb);

            auto status = futur.wait_for(std::chrono::seconds(m_amqpRequestTimeout));
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            return futur.get();
        }


        void AmqpClient::unsubscribeAsync(const std::string& exchange, const std::string& routingKey,
                                          const AsyncHandler& onComplete) {
            // Check that AmqpConnector::Pointer exists
            if (!m_connector) {
                post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                return;
            }
            // ... and still has "ready" connection ...
            if (!m_connector->ready()) {
                post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                return;
            }
            // ... and not yet unsubscribed ...
            if (!m_connector->has(exchange, routingKey, true)) {
                post(boost::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                return;
            }
            m_connector->removeSubscription(exchange, routingKey, onComplete);
        }


        bool AmqpClient::isSubscribed(const std::string& exchange, const std::string& routingKey) {
            return m_connector->has(exchange, routingKey, true);
        }


        boost::system::error_code AmqpClient::publish(const std::string& exchange, const std::string& routingKey,
                                                      const karabo::util::Hash::Pointer& msg) {
            auto payload = std::make_shared<std::vector<char>>();
            if (msg) {
                m_binarySerializer->save2(*(msg->get<Hash::Pointer>("header")), *payload); // header -> payload
                m_binarySerializer->save2(*(msg->get<Hash::Pointer>("body")), *payload);   // body   -> payload
            }

            auto promi = std::make_shared<std::promise<boost::system::error_code>>();
            auto futur = promi->get_future();
            auto cb = [promi](const boost::system::error_code& ec) { promi->set_value(ec); };

            m_connector->sendAsync(exchange, routingKey, payload, cb);

            // Set timeout avoiding to block forever ... but still with the hope for recovering ...
            int timeoutCount = m_amqpRequestTimeout * 10;
            std::future_status status;
            do {
                status = futur.wait_for(std::chrono::milliseconds(100));
                --timeoutCount;
            } while (status != std::future_status::ready && timeoutCount > 0 && isConnected());
            if (status == std::future_status::timeout) {
                return KARABO_ERROR_CODE_TIMED_OUT;
            }
            boost::system::error_code ec = futur.get();
            if (!isConnected()) return ec;
            // Destroy payload later
            auto timer = std::make_shared<boost::asio::deadline_timer>(m_connector->getContext());
            timer->expires_from_now(boost::posix_time::milliseconds(100));
            timer->async_wait([payload, timer](const boost::system::error_code& ec) mutable { payload.reset(); });
            return ec;
        }

    } // namespace net
} // namespace karabo
