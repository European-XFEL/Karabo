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

#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "karabo/util/ClassInfo.hh"
#include "utils.hh" // for AsyncHandler

namespace AMQP {
    class TcpConnection;
    class Channel;
} // namespace AMQP

namespace karabo::net {

    class ConnectionHandler; // forward declare handler that keeps status of AmqpConnection

    /**
     * AmqpConnection
     *
     * @brief
     * Wraps the AMQP::TcpConnection and the single threaded io context where all calls to the amqp library must run
     *
     */
    class AmqpConnection : public boost::enable_shared_from_this<AmqpConnection> { // FIXME: Why inheriting?
       public:
        KARABO_CLASSINFO(AmqpConnection, "AmqpConnection", "1.0")

        /**
         * Handler for asyncCreateChannel
         *
         * Either returns the channel or (if returned channel pointer is empty) state the failure reason.
         */
        using ChannelCreationHandler = std::function<void(const std::shared_ptr<AMQP::Channel>&, const char* errMsg)>;

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
         * Whether connected or in good progress of being connected
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
        void asyncConnect(AsyncHandler&& onComplete);

        /**
         * Trigger creation of an amqp channel and return it via the handler.
         *
         * If not connected yet, try to connect first.
         *
         * Can be called from any thread.
         *
         * @param onComplete A valid (!) ChannelCreationHandler that will be called from within the internal io context,
         *                   but not within the scope of asyncCreateChannel.
         */
        void asyncCreateChannel(ChannelCreationHandler onComplete);


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

        // Broker urls from input
        const std::vector<std::string> m_urls;
        size_t m_urlIndex;

        // For internal event loop for all AMQP communication
        mutable boost::asio::io_context m_ioContext;
        std::unique_ptr<boost::asio::io_context::work> m_work;
        std::thread m_thread;

        // Connection and its state:
        std::shared_ptr<AMQP::TcpConnection> m_connection;
        enum class State {
            eUnknown = 2000,   // At the beginning
            eNotConnected,     // No connection
            eConnectionDone,   // Phys.connection done
            eConnectionReady,  // Logical connection (phys. + login)
            eConnectionClosed, // Connection just closed
            eConnectionError,  // Connection error (TCP?) with error message
            eConnectionLost,   // Connection lost  (cluster node is shut down)
        };
        State m_state;

        // Completion handler
        AsyncHandler m_onConnectionComplete;
        std::vector<ChannelCreationHandler> m_pendingOnChannelCreations;
    };
} // namespace karabo::net

#endif /* KARABO_NET_AMQPCONNECTION_HH */
