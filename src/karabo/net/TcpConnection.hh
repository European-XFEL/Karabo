/*
 * $Id: Connection.hh 3602 2011-05-31 21:09:02Z esenov@DESY.DE $
 *
 * File:   AConnection.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_NET_ATCPCONNECTION_HH
#define	KARABO_NET_ATCPCONNECTION_HH

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <string>

#include "Connection.hh"
#include "IOService.hh"


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
        typedef boost::shared_ptr<boost::asio::io_service> BoostIOServicePointer;
        typedef boost::shared_ptr<boost::asio::ip::tcp::socket> BoostTcpSocket;
        typedef boost::shared_ptr<boost::asio::ip::tcp::resolver> BoostTcpResolver;
        typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor> BoostTcpAcceptor;
        
        /**
         * The Connection class.
         * This class serves as the interface for all connections.
         * A connection is only established upon call of the start() function.
         */
        class TcpConnection : public Connection {
            friend class TcpChannel;

        public:

            KARABO_CLASSINFO(TcpConnection, "Tcp", "1.0")

            TcpConnection();

            virtual ~TcpConnection() {
            }

            static void expectedParameters(karabo::util::Schema& expected);

            void configure(const karabo::util::Hash& input);

            /**
             * Starts the connection
             */
            virtual ChannelPointer start();

            /**
             * Starts the connection asynchronously providing a slot
             */
            virtual void startAsync(const ConnectionHandler& slot);

            /**
             * Stops the connection
             */
            virtual void stop();

            /**
             * Closes the connection
             */
            virtual void close();

            /**
             * This function creates a "channel" for the given connection.
             * @return Pointer to Channel
             */
            virtual ChannelPointer createChannel();
            
        private:
            void resolveHandler(const ConnectionHandler&, const ErrorCode&, boost::asio::ip::tcp::resolver::iterator);
            void acceptHandler(ChannelPointer, const ConnectionHandler&, const ErrorCode&);
            void connectHandler(ChannelPointer, const ConnectionHandler&, const ErrorCode&);
            ChannelPointer startServer();
            ChannelPointer startClient();
            void startServer(const ConnectionHandler&);
            void startClient(const ConnectionHandler&);
            
            
        private:
          
            //TcpIOServicePointer m_ioService;
            BoostIOServicePointer m_boost_io_service;
            BoostTcpResolver m_resolver;
            BoostTcpAcceptor m_acceptor;
            std::string m_connectionType;
            std::string m_hostname;
            unsigned int m_port;
            bool m_isAsyncConnect;
        };
    }
}

#endif	/* KARABO_NET_ATCPCONNECTION_HH */
