/* 
 * File:   AUdpConnection.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 17, 2011, 12:32 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_NET_AUDPCONNECTION_HH
#define	EXFEL_NET_AUDPCONNECTION_HH

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <string>

#include "Connection.hh"
#include "Channel.hh"
#include "IOService.hh"

#define START_PORT 50001
#define END_PORT   60000
#define MAX_UDP_BUFFER 1450

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Name space for package net
     */
    namespace net {

        class UdpChannel;
        typedef boost::system::error_code ErrorCode;
        typedef boost::shared_ptr<Channel> ChannelPointer;
        typedef boost::shared_ptr<boost::asio::ip::udp::socket> BoostUdpSocketPointer;
        typedef boost::shared_ptr<boost::asio::ip::udp::resolver> BoostUdpResolverPointer;

        class UdpConnection : public Connection {
            friend class exfel::net::UdpChannel;

        public:

            EXFEL_CLASSINFO(UdpConnection, "Udp", "1.0")

            UdpConnection();

            virtual ~UdpConnection() {
            }
            static void expectedParameters(exfel::util::Schema& expected);
            void configure(const exfel::util::Hash& input);
            virtual ChannelPointer start();
            virtual void stop();
            virtual void close();
            virtual ChannelPointer createChannel();

            virtual IOService::Pointer getService() {
                return m_service;
            }

        private:
            
            Channel::Pointer startServer();
            Channel::Pointer startClient();
            
        private:
            boost::shared_ptr<boost::asio::io_service> m_boost_io_service;
            BoostUdpSocketPointer m_sock;
            boost::asio::ip::udp::endpoint m_remoteEndpoint;
            ChannelPointer m_channel;
            std::string m_connectionType;
            std::string m_hostname;
            unsigned short m_port;
            unsigned int m_MaxLength;
            std::string m_headerFormat;
            char m_data[MAX_UDP_BUFFER];
        };
    }
}


#endif	/* EXFEL_NET_AUDPCONNECTION_HH */

