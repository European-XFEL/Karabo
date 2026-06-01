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
#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>

#include "Connection.hh"
#include "karabo/data/types/Hash.hh"


namespace karabo::net {

    class Channel;
    class WebSocketChannel;
    class WebSocketServer;
    class WebSocketClient;

    /**
     * @class WebSocketConnection
     * @brief a class for handling websocket connections
     *
     */
    class WebSocketConnection : public Connection {
        friend class WebSocketChannel;

       public:
        KARABO_CLASSINFO(WebSocketConnection, "WebSocket", "1.0")

        virtual ~WebSocketConnection();

        static void expectedParameters(karabo::data::Schema& expected);

        WebSocketConnection(const karabo::data::Hash& input);

        /**
         * Starts the connection
         */
        std::shared_ptr<Channel> start() override {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Synchronous start to connect is not supported method");
        }

        /**
         * Starts the connection asynchronously providing a slot
         */
        int startAsync(const ConnectionHandler& slot) override;

        /**
         * Closes the connection
         */
        void stop() override;

        /**
         * This function creates a "channel" for the given connection.
         * @return Pointer to Channel
         */
        std::shared_ptr<Channel> createChannel() override {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("\"createChannel\" is not supported for this transport layer");
        }

       private:
        void initServer(boost::asio::ip::tcp::endpoint endpoint);

       private:
        boost::asio::io_context& m_ioc;
        boost::asio::ip::tcp::acceptor m_acceptor;
        boost::asio::ip::tcp::resolver m_resolver;
        Connection::WeakPointer m_connectionPointer;
        std::string m_connectionType;
        std::string m_hostname;
        unsigned int m_port;
    };

} // namespace karabo::net
