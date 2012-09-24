/* 
 * File:   AUdpConnection.cc
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 17, 2011, 12:32 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/asio/ip/udp.hpp>
#include "IOService.hh"
#include "AsioIOService.hh"
#include "UdpConnection.hh"
#include "UdpChannel.hh"


using namespace std;
using namespace boost::asio;
using namespace exfel::util;
using boost::asio::ip::udp;

namespace exfel {
    namespace net {

        EXFEL_REGISTER_FACTORY_CC(Connection, UdpConnection)

        UdpConnection::UdpConnection() {
        }

        void UdpConnection::expectedParameters(exfel::util::Schema& expected) {
            STRING_ELEMENT(expected)
                    .key("type")
                    .displayedName("Connection Type")
                    .description("Decide whether the connection is used to implement a UDP Server or UDP Client")
                    .assignmentOptional().defaultValue("client")
                    .options("server,client")
                    .commit();

            STRING_ELEMENT(expected)
                    .key("hostname")
                    .displayedName("Hostname")
                    .description("Hostname of a peer (used only for client)")
                    .assignmentOptional().defaultValue("localhost")
                    .commit();

            UINT16_ELEMENT(expected)
                    .key("port")
                    .displayedName("Hostport")
                    .description("Hostport of a peer for type 'client' and local port for type 'server'")
                    .assignmentOptional().defaultValue(11111)
                    .commit();

            UINT32_ELEMENT(expected)
                .key("maxlen")
                .displayedName("UDP Maxlen")
                .description("Maximal message length used in datagram (UDP protocol)")
                .assignmentOptional().defaultValue(1024)
                .commit();
        }

        void UdpConnection::configure(const exfel::util::Hash& input) {
            // Create a private IOService in case the user has not given us an external one
            if (!m_service) {
                m_service = IOService::Pointer(new IOService);
            }
            setIOServiceType("Asio");

            // Get the specific boost::asio::io_service object
            m_boost_io_service = getIOService()->castTo<AsioIOService > ()->getBoostIOService();

            input.get("type", m_connectionType);
            input.get("hostname", m_hostname);
            input.get("port", m_port);
            input.get("maxlen", m_MaxLength);
        }

        Channel::Pointer UdpConnection::start() {
            if (m_connectionType == "server") return startServer();
            return startClient();
        }

        Channel::Pointer UdpConnection::startServer() {
            try {
                BoostUdpSocketPointer sock(new udp::socket(*m_boost_io_service, udp::endpoint(udp::v4(), m_port)));
                m_sock = sock;
                createChannel();
            } catch (...) {
                RETHROW
            }
            return m_channel;
        }

        Channel::Pointer UdpConnection::startClient() {
            try {
                BoostUdpSocketPointer sock(new udp::socket(*m_boost_io_service, udp::endpoint(udp::v4(), 0)));
                m_sock = sock;
                udp::resolver resolver(*m_boost_io_service);
                udp::resolver::query query(udp::v4(), m_hostname, String::toString(m_port));
                udp::resolver::iterator it = resolver.resolve(query);
                m_remoteEndpoint = *it;
                createChannel();
            } catch (...) {
                RETHROW
            }
            return m_channel;
        }

        void UdpConnection::stop() {
            close();
            m_boost_io_service->stop();
        }

        void UdpConnection::close() {
            m_sock->close();
        }

        ChannelPointer UdpConnection::createChannel() {
            UdpChannel* ch = new UdpChannel(*this);
//            ch->setChannelEndpoint(m_remoteEndpoint);
            ChannelPointer cp(ch);
            m_channel = cp;
            return m_channel;
        }
    }
}