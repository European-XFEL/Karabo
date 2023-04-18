/*
 * $Id: Connection.hh 3602 2011-05-31 21:09:02Z esenov@DESY.DE $
 *
 * File:   AConnection.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_NET_ATCPCONNECTION_HH
#define KARABO_NET_ATCPCONNECTION_HH

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

#include "Connection.hh"

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
        typedef boost::shared_ptr<Channel> ChannelPointer;
        typedef boost::function<void()> TimeoutHandler;

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

            static void expectedParameters(karabo::util::Schema& expected);

            TcpConnection(const karabo::util::Hash& input);

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
            void resolveHandler(const ErrorCode&, boost::asio::ip::tcp::resolver::iterator, const ConnectionHandler&);
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
            bool m_isAsyncConnect;
            unsigned int m_sizeofLength;
            bool m_lengthIsTextFlag;
            bool m_manageAsyncData;
        };
    } // namespace net
} // namespace karabo

#endif /* KARABO_NET_ATCPCONNECTION_HH */
