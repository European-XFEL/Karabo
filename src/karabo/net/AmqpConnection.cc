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

#include "AmqpUtils.hh" // for ConnectionHandler, KARABO_ERROR_CODE_XXX
#include "karabo/log/Logger.hh"
#include "karabo/util/Exception.hh"
#include "karabo/util/MetaTools.hh" // for bind_weak

using boost::placeholders::_1;
using boost::placeholders::_2;

using karabo::util::bind_weak;

namespace karabo {

    namespace net {

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
                    // TODO: Each created AMQP::Channel also carries a shared_ptr to our connection!
                    //       So we have to take care by some (future) logic that all these channels are gone before this
                    //       destructor is called (or at least within the join() below?)
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
            if (m_thread.get_id() == std::this_thread::get_id()) {
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
            dispatch([this, &connectedDone]() {
                // For now, treat being in the connection process as already connected
                const bool result = (m_state > State::eUnknown && m_state <= State::eConnectionReady);
                connectedDone.set_value(result);
            });
            return connectedFut.get();
        }

        void AmqpConnection::asyncConnect(AsyncHandler&& onComplete) {
            // Jump to the internal thread (if not yet in it)
            dispatch([weakThis{weak_from_this()}, onComplete{std::move(onComplete)}]() {
                if (auto self = weakThis.lock()) {
                    // TODO: Here we could check the connection status and attach 'onComplete' to
                    //       'm_onConnectionComplete' in case the process has already been started.
                    self->m_onConnectionComplete = std::move(onComplete);
                    self->doAsyncConnect();
                } else {
                    // To guarantee that 'onComplete' is not executed in the calling thread, we would have to post.
                    // But we can't since we are already (being) destructed, so member function 'post' not available.
                    onComplete(KARABO_ERROR_CODE_OP_CANCELLED);
                }
            });
        }

        void AmqpConnection::doAsyncConnect() {
            m_state = State::eUnknown;
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
            if (m_state == State::eUnknown) {
                KARABO_LOG_FRAMEWORK_DEBUG << "AmqpConnection attached. url=" << url;
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "AmqpConnection attached called, but in state "
                                          << static_cast<int>(m_state) << ", " << url;
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
                                          << static_cast<int>(m_state) << ", url = " << url;
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
                                          << static_cast<int>(m_state);
            }
            m_state = State::eConnectionReady;
            callOnComplete(KARABO_ERROR_CODE_SUCCESS);
        }


        void AmqpConnection::onError(AMQP::TcpConnection* connection, const char* message, const std::string& url) {
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_WARN << "Ignore 'onError' for wrong url: " << url << " != " << m_urls[m_urlIndex];
                return;
            }
            KARABO_LOG_FRAMEWORK_WARN << "AMQP error: '" << message << "', state " << static_cast<int>(m_state)
                                      << ". url=" << url;
            // This is e.g. called
            // - for an invalid tcp address (then we are eNotConnected)
            // - a valid tcp address, but invalid credentials with the url (eConnectionDone)
            // What is weird is that in the former case onDetached is called afterwards, in the latter not.
            if (m_state == State::eNotConnected) {
                // Invalid Tcp address: onDetached will be called afterwards
                return; // Do not set m_state, see onDetached
            } else if (m_state == State::eConnectionDone) {
                // Connected on Tcp level, but invalid credentials in the url:
                // onDetached will not be called (bug in AMQP lib?), so call m_onConnectionComplete
                callOnComplete(KARABO_ERROR_CODE_CONNECT_REFUSED);
                m_state = State::eUnknown;
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


        void AmqpConnection::onLost(AMQP::TcpConnection*, const std::string& url) {
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_WARN << "Ignore 'onLost' for wrong url: " << url << " != " << m_urls[m_urlIndex];
                return;
            }
            KARABO_LOG_FRAMEWORK_WARN << "Connection lost in state " << static_cast<int>(m_state) << ", url=" << url;
            m_state = State::eConnectionLost;
        }


        void AmqpConnection::onDetached(AMQP::TcpConnection* connection, const std::string& url) {
            if (url != m_urls[m_urlIndex]) {
                KARABO_LOG_FRAMEWORK_WARN << "Ignore 'onDetached' for wrong url: " << url
                                          << " != " << m_urls[m_urlIndex];
                return;
            }

            KARABO_LOG_FRAMEWORK_DEBUG << "Connection detached in state " << static_cast<int>(m_state)
                                       << ", url=" << url;

            if (m_state == State::eNotConnected) {
                // We come here after onError if connection failed due to invalid credentials
                callOnComplete(KARABO_ERROR_CODE_NOT_CONNECTED);
                m_state = State::eUnknown;
            }
        }

        void AmqpConnection::callOnComplete(const boost::system::error_code& ec) {
            if (ec) {
                m_connection.reset();
                if (++m_urlIndex < m_urls.size()) { // So far failed, but there are further urls to try
                    // Posting needed when invalid host port was the last url tried, otherwise handler times out
                    // (posting puts doAsyncConnect after the expected onDetached)
                    post(bind_weak(&AmqpConnection::doAsyncConnect, this));
                    return; // no call to m_onConnectionComplete yet
                } else {
                    m_urlIndex = 0; // if asyncConnect is called again, start from first url
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
                    std::string errMsg("Connection could not be established: ");
                    errMsg += ec.message();
                    handler(std::shared_ptr<AMQP::Channel>(), errMsg.data());
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
                    if (m_state == State::eUnknown) doAsyncConnect(); // no m_onConnectionComplete needed
                } else {                                              // closed, lost or error are not (yet?) treated
                    // Have to post since it was guaranteed that handler is not called from same scope as
                    // asyncCreateChannel
                    post(boost::bind(onComplete, nullptr, "Connection in bad state"));
                    // In future might downgrade to DEBUG - let's see...
                    KARABO_LOG_FRAMEWORK_INFO << "Channel creation failed: connection in bad state.";
                }
                return;
            }
            // Create channel: Since it takes a raw pointer to the connection, we use a deleter that takes care that the
            //                 connection outlives the channel.
            std::shared_ptr<AMQP::Channel> channelPtr(new AMQP::TcpChannel(m_connection.get()),
                                                      [connection{m_connection}](AMQP::Channel* p) mutable {
                                                          delete p;
                                                          connection.reset();
                                                      });

            // Attach success and failure handlers to channel - since we run in single threaded event loop that is OK
            // after channel creation since any action can only run after this function
            // Capturing channelPtr here keeps channel alive (via a temporary [!] circular reference...)
            channelPtr->onReady([onComplete, channelPtr]() mutable {
                // Reset error handler: Previous one indicates creation failure. When will the new one be called?
                // E.g. "Channel reports: ACCESS_REFUSED - queue '<name>' in vhost '/xxx' in exclusive use"
                // when a channel creates a consumer for a queue that already has an AMQP::exclusive consumer.
                // Note that text after "...reports: " is also sent to the method registered with
                // channel->consume(...).onError(..) and that that channel is disabled afterwards.
                channelPtr->onError([](const char* errMsg) {
                    KARABO_LOG_FRAMEWORK_ERROR_C("AmqpConnection") << "Channel reports: " << errMsg;
                });
                // Reset also 'onReady' handler to get rid of circular reference
                // To be able to call 'onComplete' at the very end (i.e. it sees the channel after reset of handler) we
                // have to safe the captured objects since they go away when overwriting the handler object we are in.
                auto callback = std::move(onComplete);
                auto channel = std::move(channelPtr);
                channel->onReady(AMQP::SuccessCallback());
                callback(channel, nullptr);
            });
            channelPtr->onError([onComplete, channelPtr](const char* errMsg) {
                // Reset both handlers to get rid of circular reference
                channelPtr->onReady(AMQP::SuccessCallback());
                onComplete(nullptr, errMsg);
                channelPtr->onError(AMQP::ErrorCallback()); // Can be after 'onComplete' channel not passed
                // At least for now WARN despite of handler that has to take care - later may use DEBUG
                KARABO_LOG_FRAMEWORK_WARN_C("AmqpConnection") << "Channel creation failed: " << errMsg;
            });
        }
    } // namespace net
} // namespace karabo
