/*
 * $Id: Connection.hh 3602 2011-05-31 21:09:02Z esenov@DESY.DE $
 *
 * File:   AConnection.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 9:47 AM
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
 */


#ifndef KARABO_NET_ATCPCONNECTION_HH
#define KARABO_NET_ATCPCONNECTION_HH

#include <boost/asio.hpp>
#include <memory>
#include <string>

#include "Connection.hh"
#include "karabo/data/types/Hash.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        class Channel;
        class TcpChannel;
        typedef boost::system::error_code ErrorCode;
        typedef std::shared_ptr<Channel> ChannelPointer;

        /**
         * @class TcpConnection
         * @brief a class for handling tcp connections
         *
         * This class serves as the interface for all connections.
         * A connection is only established upon call of the start() function.
         * It is a factory class and thus can be configured using its expected
         * parameters
         */
        class TcpConnection : public Connection {
            friend class TcpChannel;

           public:
            KARABO_CLASSINFO(TcpConnection, "Tcp", "1.0")

            virtual ~TcpConnection();

            static void expectedParameters(karabo::data::Schema& expected);

            TcpConnection(const karabo::data::Hash& input);

            /**
             * Starts the connection
             */
            virtual ChannelPointer start();

            /**
             * Starts the connection asynchronously providing a slot
             */
            virtual int startAsync(const ConnectionHandler& slot);

            /**
             * Closes the connection
             */
            virtual void stop();

            /**
             * This function creates a "channel" for the given connection.
             * @return Pointer to Channel
             */
            virtual ChannelPointer createChannel();

            size_t getSizeofLength() const {
                return m_sizeofLength;
            }

            bool lengthIsText() const {
                return m_lengthIsTextFlag;
            }


           private:
            void resolveHandler(const ErrorCode&, const boost::asio::ip::tcp::resolver::results_type&,
                                const ConnectionHandler&);
            ChannelPointer startServer();
            ChannelPointer startClient();
            void startServer(const ConnectionHandler&);
            void startClient(const ConnectionHandler&);


           private:
            boost::asio::ip::tcp::resolver m_resolver;
            boost::asio::ip::tcp::acceptor m_acceptor;
            std::string m_connectionType;
            std::string m_hostname;
            unsigned int m_port;
            unsigned int m_sizeofLength;
            bool m_lengthIsTextFlag;
            bool m_manageAsyncData;
            const karabo::data::Hash m_keepAliveSettings;
        };
    } // namespace net
} // namespace karabo

#endif /* KARABO_NET_ATCPCONNECTION_HH */
