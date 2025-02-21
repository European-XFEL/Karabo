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
/*
 * File:   AmqpConnection.hh
 *
 * Created on Feb 29, 2024
 */

#include "AmqpConnection.hh"

#include <boost/asio.hpp>
#include <chrono>

#include "AmqpClient.hh"
#include "AmqpUtils.hh" // for ConnectionHandler, KARABO_ERROR_CODE_XXX
#include "karabo/log/Logger.hh"
#include "karabo/util/Exception.hh"
#include "karabo/util/MetaTools.hh" // for bind_weak

using namespace std::chrono;
using std::placeholders::_1;
using std::placeholders::_2;

using karabo::util::bind_weak;

namespace karabo {

    namespace net {

        ConnectionHandler::ConnectionHandler(boost::asio::io_context& ctx) : AMQP::LibBoostAsioHandler(ctx) {}

        AmqpConnection::AmqpConnection(std::vector<std::string> urls)
            : m_urls(std::move(urls)),
              m_urlIndex(0),
              m_ioContext(),
              m_work(std::make_unique<boost::asio::io_context::work>(m_ioContext)),
              m_thread([this]() { m_ioContext.run(); }),
              m_state(State::eUnknown) {
            if (m_urls.empty()) {
                throw KARABO_NETWORK_EXCEPTION("Need at least one broker address");
            }
        }

        AmqpConnection::~AmqpConnection() {
            // Call remaining handlers and also clean-up AMQP::Connection in io context
            std::promise<void> promise;
            auto future = promise.get_future();
            dispatch([this, &promise]() {
                if (m_onConnectionComplete) {
                    m_onConnectionComplete(KARABO_ERROR_CODE_OP_CANCELLED);
                    m_onConnectionComplete = AsyncHandler();
                }

                for (ChannelCreationHandler& handler : m_pendingOnChannelCreations) {
                    if (handler) {
                        handler(std::shared_ptr<AMQP::Channel>(), "Connection destructed");
                    }
                }
                m_pendingOnChannelCreations.clear();

                if (m_connection) {
                    m_connection->close(false); // true will be without proper AMQP hand shakes
                    // Caveat: Each created AMQP::Channel also carries a shared_ptr to our connection!
                    //         They should be gone by now, but at least log a warning if not.
                    if (m_connection.use_count() > 1) {
                        KARABO_LOG_FRAMEWORK_WARN << "Underlying AMQP::Connection will not be destroyed, use count is "
                                                  << m_connection.use_count();
                        m_connection.reset();
                    }
                }
                promise.set_value();
            });
            future.get();

            // remove protection that keeps thread alive even without actual work
            m_work.reset();

            // Join thread except if running in that thread.
            if (m_thread.get_id() == boost::this_thread::get_id()) {
                // Happened while development when onError and onDetached handlers where both called in a connection
                // attempt that failed due to invalid address. At that stage, onError called the AsyncHandler that then
                // removed all external reference counts to the connection. Likely the bind_weak mechanism for
                // onDetached kept it alive a while and, since posted to the thread, triggers destruction in the thread.
                KARABO_LOG_FRAMEWORK_WARN
                      << "Cannot join thread since running in it: stop io context and detach thread";
                m_ioContext.stop(); // Take care no further tasks can run after this one
                m_thread.detach();
            } else {
                m_thread.join();
            }
        }

        std::string AmqpConnection::getCurrentUrl() const {
            // Better go via io context to avoid concurrent access - m_urlIndex might be changing...
            std::promise<std::string> prom;
            auto fut = prom.get_future();
            dispatch([this, &prom]() { prom.set_value(m_urls[m_urlIndex]); });
            return fut.get();
        }

        bool AmqpConnection::isConnected() const {
            std::promise<bool> connectedDone;
            auto connectedFut = connectedDone.get_future();
            // Also check m_connection->usable();?
            dispatch([this, &connectedDone]() { connectedDone.set_value(m_state == State::eConnectionReady); });
            return connectedFut.get();
        }

        std::string AmqpConnection::connectionInfo() const {
            // For concurrency reasons detour via io context
            std::promise<std::string> resultDone;
            auto resultFut = resultDone.get_future();
            dispatch([this, &resultDone]() {
                std::ostringstream str;
                str << "AMQP::Connection is ";
                if (m_connection) {
                    str << (m_connection->usable() ? "" : "not ") << "usable, " // can send further messages
                        << (m_connection->ready() ? "" : "not ") << "ready, " // passed login handshake, not yet closed
                        << (m_connection->initialized() ? "" : "not ")
                        << "initialized, " // passed login handshake (maybe closing/closed)
                        << (m_connection->closed() ? "" : "not ") << "closed and has " // full tcp closed
                        << m_connection->channels() << " channels.";
                } else {
                    str << "not yet created!";
                }
                resultDone.set_value(str.str());
            });
            return resultFut.get();
        }

        void AmqpConnection::asyncConnect(AsyncHandler onComplete) {
            // Jump to the internal thread (if not yet in it)
            dispatch([weakThis{weak_from_this()}, onComplete{std::move(onComplete)}]() {
                if (auto self = weakThis.lock()) {
                    if (self->m_state == State::eConnectionReady) {
                        // Already connected is treated as success
                        self->post(std::bind(onComplete, KARABO_ERROR_CODE_SUCCESS));
                    } else if (self->m_state == State::eUnknown || self->m_state > State::eConnectionReady) {
                        // Not yet tried or after connection loss: Try again
                        self->m_onConnectionComplete = std::move(onComplete);
                        self->m_state = State::eStarted;
                        self->doAsyncConnect();
                    } else {
                        // Connection process has been started, just hook the handler into the existing one
                        if (self->m_onConnectionComplete) {
                            auto combinedOnComplete = [newOnComplete{std::move(onComplete)},
                                                       previousOnComplete{std::move(self->m_onConnectionComplete)}](
                                                            const boost::system::error_code ec) {
                                previousOnComplete(ec);
                                newOnComplete(ec);
                            };
                            self->m_onConnectionComplete = combinedOnComplete;
                        } else {
                            self->m_onConnectionComplete = std::move(onComplete);
                        }
                    }
                } else {
                    // To guarantee that 'onComplete' is not executed in the calling thread, we would have to post.
                    // But we can't since we are already (being) destructed, so member function 'post' not available.
                    onComplete(KARABO_ERROR_CODE_OP_CANCELLED);
                }
            });
        }

        void AmqpConnection::doAsyncConnect() {
            const std::string& url = m_urls[m_urlIndex];
            try {
                AMQP::Address address(url);

                // Create and setup with callbacks a new ConnectionHandler ...
                auto handler = std::make_shared<ConnectionHandler>(m_ioContext);
                handler->setOnAttachedHandler(bind_weak(&AmqpConnection::onAttached, this, _1, url));
                handler->setOnConnectedHandler(bind_weak(&AmqpConnection::onConnected, this, _1, url));
                handler->setOnReadyHandler(bind_weak(&AmqpConnection::onReady, this, _1, url));
                handler->setOnErrorHandler(bind_weak(&AmqpConnection::onError, this, _1, _2, url));
                handler->setOnClosedHandler(bind_weak(&AmqpConnection::onClosed, this, _1, url));
                handler->setOnLostHandler(bind_weak(&AmqpConnection::onLost, this, _1, url));
                handler->setOnDetachedHandler(bind_weak(&AmqpConnection::onDetached, this, _1, url));

                // Create connection and bind lifetime of handler to destruction of connection
                ConnectionHandler* bareHandler = handler.get(); // before move(handler)!
                auto deleteConn = [handler = std::move(handler)](AMQP::TcpConnection* p) mutable {
                    // First delete connection, then handler:
                    // So connection can make use of the handler until its very end
                    delete p;
                    handler.reset();
                };
                m_connection = decltype(m_connection)(new AMQP::TcpConnection(bareHandler, address), deleteConn);
            } catch (const std::runtime_error& e) {
                // AMQP::Address throws runtime_error on invalid protocol in AMQP::Address
                KARABO_LOG_FRAMEWORK_WARN << "Invalid url: " << e.what();
                // Have to post since it was guaranteed that handler is not called from same scope as asyncConnect
                post(bind_weak(&AmqpConnection::callOnComplete, this, KARABO_ERROR_CODE_WRONG_PROTOCOL));
            }
        }


        void AmqpConnection::onAttached(AMQP::TcpConnection*, const std::string& url) {
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_WARN << "Ignore 'onAttached' for wrong url: " << url
                                          << " != " << m_urls[m_urlIndex];
                return;
            }
            if (m_state == State::eStarted) {
                KARABO_LOG_FRAMEWORK_DEBUG << "AmqpConnection attached, url=" << url;
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "AmqpConnection attached called, but in state " << stateString(m_state)
                                          << ", " << url;
            }
            m_state = State::eNotConnected;
        }

        void AmqpConnection::onConnected(AMQP::TcpConnection*, const std::string& url) {
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_WARN << "Ignore 'onConnected' for wrong url: " << url
                                          << " != " << m_urls[m_urlIndex];
                return;
            }
            if (m_state == State::eNotConnected) {
                KARABO_LOG_FRAMEWORK_DEBUG << "AmqpConnection connected (Tcp). url=" << url;
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "AmqpConnection connected (Tcp) called, but in state "
                                          << stateString(m_state) << ", url = " << url;
            }
            m_state = State::eConnectionDone;
        }


        void AmqpConnection::onReady(AMQP::TcpConnection* connection, const std::string& url) {
            // At this point, the m_connection is initialized and ready
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_WARN << "Ignore 'onReady' for wrong url: " << url << " != " << m_urls[m_urlIndex];
                return;
            }
            if (m_state == State::eConnectionDone) {
                KARABO_LOG_FRAMEWORK_DEBUG << "Established connection to '" << url << "'";
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Established connection to '" << url << "', but state was "
                                          << stateString(m_state);
            }
            m_state = State::eConnectionReady;
            callOnComplete(KARABO_ERROR_CODE_SUCCESS);
        }


        void AmqpConnection::onError(AMQP::TcpConnection* connection, const char* message, const std::string& url) {
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_WARN << "Ignore 'onError' for wrong url: " << url << " != " << m_urls[m_urlIndex];
                return;
            }
            KARABO_LOG_FRAMEWORK_WARN << "AMQP error: '" << message << "', state " << stateString(m_state)
                                      << ", url=" << url;
            // This is e.g. called
            // - for an invalid tcp address (then we are eNotConnected)
            // - a valid tcp address, but invalid credentials with the url (eConnectionDone)
            // What is weird is that in the former case onDetached is called afterwards, in the latter not.
            if (m_state == State::eNotConnected) {
                // Invalid Tcp address: onDetached will be called afterwards
                // (Since no connection yet, could not yet be 'connection lost' scenario...
                //   ==> No need to check that message contains "Name or service not known".)
                return; // Do not set m_state, see onDetached
            } else if (m_state == State::eConnectionDone) {
                // Connected on Tcp level, but invalid credentials in the url:
                // onDetached will not be called (bug in AMQP lib?), so call m_onConnectionComplete
                callOnComplete(KARABO_ERROR_CODE_CONNECT_REFUSED);
                return;
            }

            m_state = State::eConnectionError;
        }


        void AmqpConnection::onClosed(AMQP::TcpConnection*, const std::string& url) {
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_INFO << "Ignore 'onClosed' for wrong url: " << url << " != " << m_urls[m_urlIndex];
                return;
            }
            KARABO_LOG_FRAMEWORK_INFO << "Connection cosed. url=" << url;
            m_state = State::eConnectionClosed;
        }

        void AmqpConnection::onLost(AMQP::TcpConnection* connection, const std::string& url) {
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_WARN << "Ignore 'onLost' for wrong url: " << url << " != " << m_urls[m_urlIndex];
                return;
            }
            if (connection != m_connection.get()) {
                KARABO_LOG_FRAMEWORK_WARN << "Loss of unknown connection (claimed url '" << url << "') ignored.";
                return;
            }
            // Saw this print 114 ms after first sign of connection loss in channelPtr->onError!
            KARABO_LOG_FRAMEWORK_WARN << "Connection lost in state " << stateString(m_state) << ", url=" << url
                                      << ". Now try to reconnect, recreate channels and subscriptions.";
            m_state = State::eConnectionLost;

            m_connection.reset(); // use_count should still be > 0: channels are not yet destroyed...

            triggerReconnection();
        }


        void AmqpConnection::triggerReconnection() {
            if (!m_random) {
                // A simple random generator: since disconnections happen at the same time, seed from 'this', not time
                const auto seed = reinterpret_cast<unsigned long long>(this); // Could also use process id.
                m_random = std::make_unique<std::minstd_rand0>(static_cast<unsigned int>(seed));
            }
            // Do not let all connections bombard any now available broker at once (that is why we need different
            // seeds!):
            std::uniform_int_distribution<int> distribution(2000, 15000); // 2 to 15 seconds in ms
            size_t delay(distribution(*m_random));
            KARABO_LOG_FRAMEWORK_INFO << "Delay reconnection a bit: " << delay;
            auto timer = std::make_shared<boost::asio::steady_timer>(m_ioContext);
            timer->expires_after(milliseconds(delay));
            timer->async_wait([weakThis{weak_from_this()}, timer](const boost::system::error_code& e) {
                if (e) return;
                if (auto self = weakThis.lock()) {
                    self->asyncConnect(
                          bind_weak(&AmqpConnection::informReconnection, self.get(), boost::asio::placeholders::error));
                }
            });
        }


        void AmqpConnection::onDetached(AMQP::TcpConnection* connection, const std::string& url) {
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_WARN << "Ignore 'onDetached' for wrong url: " << url
                                          << " != " << m_urls[m_urlIndex];
                return;
            }

            KARABO_LOG_FRAMEWORK_DEBUG << "Connection detached in state " << stateString(m_state) << ", url=" << url;

            if (m_state == State::eNotConnected) {
                // We come here after onError if connection failed due to invalid credentials
                callOnComplete(KARABO_ERROR_CODE_NOT_CONNECTED);
            }
        }

        void AmqpConnection::callOnComplete(const boost::system::error_code& ec) {
            if (ec) {
                m_connection.reset();
                if (++m_urlIndex < m_urls.size()) { // So far failed, but there are further urls to try
                    m_state = State::eStarted;
                    // Posting needed when invalid host port was the last url tried, otherwise handler times out
                    // (posting puts doAsyncConnect after the expected onDetached)
                    post(bind_weak(&AmqpConnection::doAsyncConnect, this));
                    return; // no call to m_onConnectionComplete yet
                } else {
                    m_urlIndex = 0; // if asyncConnect is called again, start from first url
                    m_state = State::eUnknown;
                }
            }
            // Succeeded or finally failed
            if (m_onConnectionComplete) {
                // Reset handler before calling it to avoid cases where the handler calls a function that
                // sets it to another value
                AsyncHandler onComplete;
                onComplete.swap(m_onConnectionComplete);
                onComplete(ec);
            }
            // Trigger pending channel requests if there are some
            for (ChannelCreationHandler& handler : m_pendingOnChannelCreations) {
                if (ec) {
                    handler(std::shared_ptr<AMQP::Channel>(), "Connection could not be established: " + ec.message());
                } else {
                    doCreateChannel(std::move(handler));
                }
            }
            m_pendingOnChannelCreations.clear();
        }

        void AmqpConnection::asyncCreateChannel(AmqpConnection::ChannelCreationHandler onComplete) {
            // ensure we are in our AMQP thread
            dispatch([weakThis{weak_from_this()}, onComplete{std::move(onComplete)}]() {
                if (auto self = weakThis.lock()) {
                    self->doCreateChannel(std::move(onComplete));
                } else {
                    onComplete(std::shared_ptr<AMQP::Channel>(), "Operation cancelled");
                }
            });
        }

        void AmqpConnection::doCreateChannel(AmqpConnection::ChannelCreationHandler onComplete) {
            if (m_state != State::eConnectionReady) {
                if (m_state < State::eConnectionReady) {
                    KARABO_LOG_FRAMEWORK_INFO
                          << "Channel creation requested, but not yet connected. Postpone until connected.";
                    m_pendingOnChannelCreations.push_back(std::move(onComplete));
                    if (m_state == State::eUnknown) {
                        // no need to set m_onConnectionComplete
                        m_state = State::eStarted;
                        doAsyncConnect();
                    }
                } else { // closed, lost or error are not (yet?) treated
                    // Have to post since it was guaranteed that handler is not called from same scope as
                    // asyncCreateChannel
                    post(std::bind(onComplete, nullptr, "Connection in bad state"));
                    // In future might downgrade to DEBUG - let's see...
                    KARABO_LOG_FRAMEWORK_INFO << "Channel creation failed: connection in bad state.";
                }
                return;
            }
            // Create channel: Since it takes a raw pointer to the connection, we use a deleter that takes care that the
            //                 connection outlives the channel.
            std::shared_ptr<AMQP::Channel> channelPtr(nullptr, [connection{m_connection}](AMQP::Channel* p) mutable {
                delete p;
                connection.reset();
            });
            try {
                channelPtr.reset(new AMQP::TcpChannel(m_connection.get()));
            } catch (const std::runtime_error& e) {
                // As documented for TcpChannel constructor if connection closed or max number of channels reached
                KARABO_LOG_FRAMEWORK_ERROR << "Cannot create channel due to: " << e.what();
                // Need to post to guarantee not to call handler in calling thread
                // Note: onComplete takes const char*, so cannot bind a temporary
                std::string msg("Runtime exception creating channel: ");
                post(std::bind(onComplete, nullptr, msg += e.what()));
                return;
            }

            // Attach success and failure handlers to channel - since we run in single threaded event loop that is OK
            // after channel creation since any action can only run after this function
            // Capturing channelPtr here keeps channel alive (via a temporary [!] circular reference...)
            channelPtr->onReady([onComplete, channelPtr]() mutable {
                // Reset error handler: Previous one indicates creation failure. When will the new one be called?
                // - "ACCESS_REFUSED - queue '<name>' in vhost '/xxx' in exclusive use"
                //   when a channel creates a consumer for a queue that already has an AMQP::exclusive consumer.
                //   Note that same is also sent to the method registered with
                //   channel->consume(...).onError(..) and that channel is disabled afterwards.
                // - When channel tried to create a consumer for a non-existing queue
                //   "NOT_FOUND - no queue '<name>' in vhost '/'xxx""
                // - When the broker goes down: "connection lost"
                //   (Here is some duplication since also AmqpConnection::onLost is called
                // - "PRECONDITION_FAILED - message size <NNN> is larger than configured max size 134217728"
                //   a while after publishing a message bigger than the configured maximum of the broker,
                //   the channel wis ususable afterwards
                // NOTE: Once its channel status is READY, AmqpClient sets a new error handler.
                channelPtr->onError([](const char* errMsg) {
                    KARABO_LOG_FRAMEWORK_WARN_C("AmqpConnection") << "Channel reports: " << errMsg;
                });
                // Reset also 'onReady' handler to get rid of circular reference
                // To be able to call 'onComplete' at the very end (i.e. it sees the channel after reset of handler) we
                // have to safe the captured objects since they go away when overwriting the handler object we are in.
                auto callback = std::move(onComplete);
                auto channel = std::move(channelPtr);
                channel->onReady(AMQP::SuccessCallback());
                callback(channel, std::string());
            });
            channelPtr->onError([onComplete, channelPtr](const char* errMsg) {
                // Also error "connection lost" just forwarded as failure here, calling code has to take care.
                // Reset both handlers to get rid of circular reference
                channelPtr->onReady(AMQP::SuccessCallback());
                onComplete(nullptr, errMsg);
                channelPtr->onError(AMQP::ErrorCallback()); // Can be after 'onComplete' channel not passed
                // At least for now WARN despite of handler that has to take care - later may use DEBUG
                KARABO_LOG_FRAMEWORK_WARN_C("AmqpConnection") << "Channel creation failed: " << errMsg;
            });
        }

        void AmqpConnection::informReconnection(const boost::system::error_code& ec) {
            if (ec) {
                KARABO_LOG_FRAMEWORK_WARN << "Reconnection failed (" << ec.message() << "), try again.";
                triggerReconnection();
                return;
            }

            KARABO_LOG_FRAMEWORK_INFO << "Successfully reconnected, now inform up to " << m_registeredClients.size()
                                      << " registered clients";

            for (auto weakClientIt = m_registeredClients.begin(); weakClientIt != m_registeredClients.end();) {
                if (auto client = weakClientIt->lock()) {
                    client->reviveIfReconnected();
                    ++weakClientIt;
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "AmqpConnection::informReconnection: a client is gone!";
                    weakClientIt = m_registeredClients.erase(weakClientIt);
                }
            }
        }

        void AmqpConnection::registerForReconnectInfo(std::weak_ptr<AmqpClient> client) {
            dispatch([weakThis{weak_from_this()}, client{std::move(client)}]() {
                // std::set takes care that client is registered only once
                if (auto self = weakThis.lock()) self->m_registeredClients.emplace(std::move(client));
            });
        }

        void AmqpConnection::cleanReconnectRegistrations() {
            dispatch([weakThis{weak_from_this()}]() {
                if (auto self = weakThis.lock()) {
                    for (auto it = self->m_registeredClients.begin(); it != self->m_registeredClients.end();) {
                        if (it->use_count() == 0) { // weak_ptr with no shared_ptr left
                            it = self->m_registeredClients.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            });
        }

        const char* AmqpConnection::stateString(AmqpConnection::State state) {
            switch (state) {
                case State::eUnknown:
                    return "'unknown'";
                case State::eStarted:
                    return "'started'";
                case State::eNotConnected:
                    return "'not connected'";
                case State::eConnectionDone:
                    return "'connection done'";
                case State::eConnectionReady:
                    return "'connection ready'";
                case State::eConnectionClosed:
                    return "'connection closed'";
                case State::eConnectionError:
                    return "'connection error'";
                case State::eConnectionLost:
                    return "'connection lost'";
            }
            return "''"; // Please compiler warning
        }

    } // namespace net
} // namespace karabo
