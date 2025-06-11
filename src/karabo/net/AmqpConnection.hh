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

#ifndef KARABO_NET_AMQPCONNECTION_HH
#define KARABO_NET_AMQPCONNECTION_HH

#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <functional>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "karabo/data/types/ClassInfo.hh"
#include "utils.hh" // for AsyncHandler

namespace AMQP {
    class TcpConnection;
    class Channel;
} // namespace AMQP

namespace karabo::net {

    /**
     * @brief Declare our custom ConnectionHandler to implement their callbacks.
     * Every callback delegates processing to externally assigned callback if it was set.
     * ConnectionHandler object calls its callbacks as follows...
     * while connecting...
     *     onAttached, onConnected, onReady
     * in case of error ...
     *     onError, (onLost), onDetached
     * in case of normal closure ...
     *     onClosed, onDetached
     *
     * onAttached is called if new connection is associated with the handler
     * onConnected is called if physical (TCP) connection is successfully established.
     * onReady is called if login is successful
     * onClosed is called in case of normal connection closure
     * onError is called if errors encountered on connection
     * onLost is called if onError was called and earlier onConnected was called.
     * onDetached id called as a last step of connection loss
     */
    class ConnectionHandler : public virtual AMQP::LibBoostAsioHandler {
       public:
        typedef std::function<void(AMQP::TcpConnection*)> ConnectionHandlerCallback;
        typedef std::function<void(AMQP::TcpConnection*, const char*)> ConnectionErrorCallback;

       private:
        ConnectionHandlerCallback m_onAttached;
        ConnectionHandlerCallback m_onConnected;
        ConnectionHandlerCallback m_onReady;
        ConnectionErrorCallback m_onError;
        ConnectionHandlerCallback m_onClosed;
        ConnectionHandlerCallback m_onLost;
        ConnectionHandlerCallback m_onDetached;

       private:
        ConnectionHandler() = delete;

        ConnectionHandler(ConnectionHandler&&) = delete;
        ConnectionHandler(const ConnectionHandler&) = delete;

       public:
        explicit ConnectionHandler(boost::asio::io_context& ctx);

        virtual ~ConnectionHandler() = default;

        void onAttached(AMQP::TcpConnection* connection) override {
            if (m_onAttached) m_onAttached(connection);
        }

        void setOnAttachedHandler(ConnectionHandlerCallback&& cb) {
            m_onAttached = std::move(cb);
        }

        void onConnected(AMQP::TcpConnection* connection) override {
            if (m_onConnected) m_onConnected(connection);
        }

        void setOnConnectedHandler(ConnectionHandlerCallback&& cb) {
            m_onConnected = std::move(cb);
        }

        void onReady(AMQP::TcpConnection* connection) override {
            if (m_onReady) m_onReady(connection);
        }

        void setOnReadyHandler(ConnectionHandlerCallback&& cb) {
            m_onReady = std::move(cb);
        }

        void onError(AMQP::TcpConnection* connection, const char* message) override {
            if (m_onError) m_onError(connection, message);
        }

        void setOnErrorHandler(ConnectionErrorCallback&& cb) {
            m_onError = std::move(cb);
        }

        void onClosed(AMQP::TcpConnection* connection) override {
            if (m_onClosed) m_onClosed(connection);
        }

        void setOnClosedHandler(ConnectionHandlerCallback&& cb) {
            m_onClosed = std::move(cb);
        }

        void onLost(AMQP::TcpConnection* connection) override {
            if (m_onLost) m_onLost(connection);
        }

        void setOnLostHandler(ConnectionHandlerCallback&& cb) {
            m_onLost = std::move(cb);
        }

        void onDetached(AMQP::TcpConnection* connection) override {
            if (m_onDetached) m_onDetached(connection);
        }

        void setOnDetachedHandler(ConnectionHandlerCallback&& cb) {
            m_onDetached = std::move(cb);
        }
    };

    class AmqpClient;
    /**
     * AmqpConnection
     *
     * @brief
     * Wraps the AMQP::TcpConnection and the single threaded io context where all calls to the amqp library must run
     *
     */
    class AmqpConnection : public std::enable_shared_from_this<AmqpConnection> {
       public:
        KARABO_CLASSINFO(AmqpConnection, "AmqpConnection", "1.0")

        /**
         * Connection states
         */
        enum class State {
            eUnknown = 2000,   // At the beginning and after a failed connection attempt
            eStarted,          // Connection attempt is started
            eNotConnected,     // No connection
            eConnectionDone,   // Phys.connection done
            eConnectionReady,  // Logical connection (phys. + login)
            eConnectionClosed, // Connection just closed
            eConnectionError,  // Connection error (TCP?) with error message
            eConnectionLost,   // Connection lost  (cluster node is shut down)
        };

        /**
         * Handler for asyncCreateChannel
         *
         * Either returns the channel or (if returned channel pointer is empty) state the failure reason.
         */
        using ChannelCreationHandler =
              std::function<void(const std::shared_ptr<AMQP::Channel>&, const std::string& errMsg)>;

        /**
         * Constructing a connection and starting the thread of the io context
         *
         * @param urls vector of broker urls to try to connect to in asyncConnect
         *             (throws karabo::util::NetworkException if vector is empty)
         */
        AmqpConnection(std::vector<std::string> urls);

        virtual ~AmqpConnection();

        /**
         * Return currently used broker URL (either already connected to it or the currently/next tried one)
         */
        std::string getCurrentUrl() const;

        /**
         * Whether connection established
         */
        bool isConnected() const;

        /**
         * Various info about internal connection (for debug logs)
         */
        std::string connectionInfo() const;

        /**
         * Asynchronously connect to any of the broker addresses passed to the constructor.
         *
         * Addresses will be tried in the order they have been passed.
         * Can be called from any thread.
         *
         * @param onComplete AsyncHAndler called (not from within asyncConnect) about success or failure of connection
         *                   attempt. If all addresses failed, the error code passed is the one of the last address
         *                   passed to the constructor.
         *                   The handler (if valid) will be called from within the internal io context,
         *                   but not within the scope of asyncConnect.
         */
        void asyncConnect(AsyncHandler onComplete);

        /**
         * Trigger creation of an amqp channel and return it via the handler.
         *
         * If not connected yet, try to connect first.
         * Note that if connection lost, channel creation will not be tried again, but failure is reported.
         *
         * Can be called from any thread.
         *
         * @param onComplete A valid (!) ChannelCreationHandler that will be called from within the internal io context,
         *                   but not within the scope of asyncCreateChannel.
         */
        void asyncCreateChannel(ChannelCreationHandler onComplete);

        /**
         * Register client to be informed about re-established connection after connection loss
         */
        void registerForReconnectInfo(std::weak_ptr<AmqpClient> client);

        /**
         * Clean clients registered to receive reconnect info, i.e. remove all dangling weak pointers
         *
         * Can e.g. be called in the destructor of a client that registered before.
         */
        void cleanReconnectRegistrations();

        /**
         * Post a task to the io context
         *
         * The task must not contain blocking code since otherwise the thread running the AMQP communication is blocked.
         */
        template <typename CompletionToken>
        void post(CompletionToken&& task) const {
            boost::asio::post(m_ioContext, std::forward<CompletionToken>(task));
        }

        /**
         * Detach a task to the io context, i.e. run it now or later depending on which thread we are in
         *
         * The task must not contain blocking code since otherwise the thread running the AMQP communication is blocked.
         */
        template <typename CompletionToken>
        void dispatch(CompletionToken&& task) const {
            boost::asio::dispatch(m_ioContext, std::forward<CompletionToken>(task));
        }


       private:
        void onAttached(AMQP::TcpConnection*, const std::string& url);
        void onConnected(AMQP::TcpConnection*, const std::string& url);
        void onReady(AMQP::TcpConnection*, const std::string& url);
        void onError(AMQP::TcpConnection* connection, const char* message, const std::string& url);
        void onClosed(AMQP::TcpConnection* connection, const std::string& url);
        void onLost(AMQP::TcpConnection* connection, const std::string& url);
        void onDetached(AMQP::TcpConnection* connection, const std::string& url);

        /**
         * Helper to asyncConnect iterating over urls until success or all urls tried.
         * Then calls m_onConnectionComplete.
         *
         * Prerequisite: m_urlIndex < m_urls.size()
         */
        void doAsyncConnect();
        // Helper to call and reset m_onConnectionComplete (if set)
        void callOnComplete(const boost::system::error_code& ec);
        // Helper to run content of asyncCreateChannel in our io context
        void doCreateChannel(ChannelCreationHandler onComplete);

        /**
         * Must run in io context
         */
        void informReconnection(const boost::system::error_code& ec);

        /**
         * Must run in io context
         */
        void triggerReconnection();

        /**
         * Convert State to a string (or rather const char*)
         */
        const char* stateString(AmqpConnection::State state);

        // Broker urls from input
        const std::vector<std::string> m_urls;
        size_t m_urlIndex;

        // For internal event loop for all AMQP communication
        mutable boost::asio::io_context m_ioContext;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work;
        std::jthread m_thread;

        // Connection and its state:
        std::shared_ptr<AMQP::TcpConnection> m_connection;
        State m_state;

        // Completion handler
        AsyncHandler m_onConnectionComplete;
        std::vector<ChannelCreationHandler> m_pendingOnChannelCreations;

        /// Track clients to inform about reconnections
        std::set<std::weak_ptr<AmqpClient>, std::owner_less<std::weak_ptr<AmqpClient>>>
              m_registeredClients; // registered to get informed about reconenctions
        std::unique_ptr<std::minstd_rand0> m_random;
    };
} // namespace karabo::net

#endif /* KARABO_NET_AMQPCONNECTION_HH */
