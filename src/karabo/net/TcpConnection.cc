/*
 * $Id: Connection.hh 3602 2011-05-31 21:09:02Z esenov@DESY.DE $
 *
 * File:   Connection.hh
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

#include "TcpConnection.hh"

#include <any>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <memory>

#include "EventLoop.hh"
#include "TcpChannel.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/log/Logger.hh"
#include "karabo/util/MetaTools.hh"
#include "utils.hh"

using namespace std;
using namespace boost::asio;
using namespace karabo::data;
using namespace karabo::util;


KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Connection, karabo::net::TcpConnection)

namespace karabo {
    namespace net {


        void TcpConnection::expectedParameters(karabo::data::Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("type")
                  .displayedName("Connection Type")
                  .description("Decide whether the connection is used to implement a TCP Server or TCP Client")
                  .options(std::vector<std::string>({"server", "client"}))
                  .assignmentOptional()
                  .defaultValue("client")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("hostname")
                  .displayedName("Hostname")
                  .description("Hostname of a peer (used only for client)")
                  .assignmentOptional()
                  .defaultValue("localhost")
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("port")
                  .displayedName("Hostport")
                  .description("Hostport of a peer for type 'client' and local port for type 'server'")
                  .assignmentOptional()
                  .defaultValue(0)
                  .maxInc(65535) // ports are 16-bit
                  .commit();

            STRING_ELEMENT(expected)
                  .key("url")
                  .displayedName("URL")
                  .description(
                        "URL format is tcp://hostname:port. This style has precedence over specifying hostname and "
                        "port.")
                  .assignmentOptional()
                  .defaultValue("")
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("sizeofLength")
                  .displayedName("Size of Message Length")
                  .description("The size of messageLength field in communication protocol")
                  .assignmentOptional()
                  .defaultValue(4)
                  .init()
                  .expertAccess()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("messageTagIsText")
                  .displayedName("Message Tag is Text")
                  .description("The length field in communication protocol is considered as text string")
                  .assignmentOptional()
                  .defaultValue(false)
                  .init()
                  .expertAccess()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("manageAsyncData")
                  .displayedName("Manage Async Data")
                  .description(
                        "If set to true, asynchronous write handlers will copy the data to be written. The user does "
                        "not "
                        "have to make sure that the to-be-written data has a long-enough life time.")
                  .assignmentOptional()
                  .defaultValue(true)
                  .init()
                  .expertAccess()
                  .commit();

            NODE_ELEMENT(expected).key("keepalive").displayedName("Tcp Keep Alive").expertAccess().commit();

            BOOL_ELEMENT(expected)
                  .key("keepalive.enabled")
                  .displayedName("Enabled")
                  .assignmentOptional()
                  .defaultValue(false)
                  .commit();

            INT32_ELEMENT(expected)
                  .key("keepalive.toleratedSilence")
                  .displayedName("Tolerated Silence")
                  .description(
                        "Idle time after which keep-alive mechanism start checking the connection (TCP_KEEPIDLE)")
                  .unit(karabo::data::Unit::SECOND)
                  .assignmentOptional()
                  .defaultValue(30) // Linux default is 7200
                  .minInc(5)
                  .commit();

            INT32_ELEMENT(expected)
                  .key("keepalive.interval")
                  .displayedName("Interval")
                  .description("Interval between probes keep-alive probes (TCP_KEEPINTVL)")
                  .unit(karabo::data::Unit::SECOND)
                  .assignmentOptional()
                  .defaultValue(5) // Linux default is 75
                  .minInc(1)
                  .commit();

            INT32_ELEMENT(expected)
                  .key("keepalive.numProbes")
                  .displayedName("Number of Probes")
                  .description(
                        "Number of ot acknowledged probes after that the connection is considered dead (TCP_KEEPCNT)")
                  .unit(karabo::data::Unit::COUNT)
                  .assignmentOptional()
                  .defaultValue(5) // Linux default is 9
                  .minInc(2)
                  .commit();
        }


        TcpConnection::TcpConnection(const karabo::data::Hash& input)
            : Connection(input),
              m_resolver(EventLoop::getIOService()),
              m_acceptor(EventLoop::getIOService()),
              m_keepAliveSettings(input.get<karabo::data::Hash>("keepalive")) {
            string url;
            input.get("url", url);
            if (url.empty()) {
                input.get("hostname", m_hostname);
                input.get("port", m_port);
            } else {
                const std::tuple<std::string, std::string, std::string, std::string, std::string> parts = parseUrl(url);
                if (std::get<0>(parts) != "tcp") {
                    throw KARABO_NETWORK_EXCEPTION(("url '" + url) += "' does not start with 'tcp'");
                }
                m_hostname = std::get<1>(parts);
                m_port = fromString<unsigned int>(std::get<2>(parts));
            }
            input.get("type", m_connectionType);
            input.get("sizeofLength", m_sizeofLength);
            input.get("messageTagIsText", m_lengthIsTextFlag);
            input.get("manageAsyncData", m_manageAsyncData);
        }


        TcpConnection::~TcpConnection() {
            stop();
        }


        Channel::Pointer TcpConnection::start() {
            if (m_connectionType == "server") {
                try {
                    if (m_acceptor.is_open()) {
                        m_acceptor.cancel();
                        m_acceptor.close();
                    }
                    ip::tcp::endpoint endpoint(ip::tcp::v4(), m_port);
                    m_acceptor.open(endpoint.protocol());
                    m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
                    m_acceptor.bind(endpoint); // <=== here exception possible: port in use
                    m_acceptor.listen();
                    if (m_port != endpoint.port()) {
                        m_port = endpoint.port(); // if m_port was == 0 then the OS assigns free port number.
                    }
                } catch (...) {
                    KARABO_RETHROW
                }
                return startServer();
            } else {
                return startClient();
            }
        }


        Channel::Pointer TcpConnection::startServer() {
            Channel::Pointer channel;
            try {
                channel = this->createChannel();
                TcpChannel::Pointer tcpChannel = std::static_pointer_cast<TcpChannel>(channel);
                tcpChannel->acceptSocket(m_acceptor);
            } catch (...) {
                KARABO_RETHROW
            }
            return channel;
        }


        Channel::Pointer TcpConnection::startClient() {
            Channel::Pointer channel;
            try {
                ip::tcp::resolver::query query(ip::tcp::v4(), m_hostname, karabo::data::toString(m_port));
                ip::tcp::resolver::iterator endpoint_iterator = m_resolver.resolve(query);
                channel = this->createChannel();
                TcpChannel::Pointer tcpChannel = std::static_pointer_cast<TcpChannel>(channel);
                tcpChannel->socketConnect(*endpoint_iterator);
            } catch (...) {
                KARABO_RETHROW
            }
            return channel;
        }


        int TcpConnection::startAsync(const ConnectionHandler& handler) {
            if (m_connectionType == "server") {
                if (!m_acceptor.is_open()) {
                    do {
                        try {
                            ip::tcp::endpoint endpoint(ip::tcp::v4(), m_port);
                            m_acceptor.open(endpoint.protocol());
                            if (m_port > 0) {
                                m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
                            }
                            m_acceptor.set_option(ip::tcp::acceptor::enable_connection_aborted(true));
                            m_acceptor.bind(endpoint); // <=== here exception possible: port in use
                            m_acceptor.listen();
                            if (m_port == 0) {
                                ip::tcp::endpoint le = m_acceptor.local_endpoint();
                                m_port = le.port();
                            }
                        } catch (const std::exception& e) {
                            if (m_acceptor.is_open()) {
                                m_acceptor.cancel();
                                m_acceptor.close();
                            }
                            if (m_port != 0) {
                                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("bind with port " + toString(m_port) +
                                                                           " failed. OS: '" + e.what() + "'"));
                            }
                        }
                    } while (m_port == 0);
                }
                startServer(handler);
            } else if (m_connectionType == "client") {
                startClient(handler);
            }
            return m_port;
        }


        void TcpConnection::startServer(const ConnectionHandler& handler) {
            try {
                Channel::Pointer channel = this->createChannel();
                TcpChannel::Pointer tcpChannel = std::static_pointer_cast<TcpChannel>(channel);
                // Caveat - cyclic shared_ptr to 'this' TcpConnection if 'channel' would have a shared_ptr to its
                // TcpConnection: Then 'channel' is bound to the callback created by std::bind and asyncAcceptSocket
                // will call 'm_acceptor.async_accept(aSocket, callback)' and that certainly has to store the callback
                // somewhere. So the datamember 'm_acceptor' holds a shared_ptr to this - if now the callback is never
                // called (why?), the TcpConnection lives forever even if nothing outside keeps a pointer to it.
                tcpChannel->asyncAcceptSocket(m_acceptor,
                                              std::bind(handler, boost::asio::placeholders::error, channel));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::startClient(const ConnectionHandler& handler) {
            try {
                ip::tcp::resolver::query query(ip::tcp::v4(), m_hostname, karabo::data::toString(m_port));
                m_resolver.async_resolve(
                      query, bind_weak(&TcpConnection::resolveHandler, this, boost::asio::placeholders::error,
                                       boost::asio::placeholders::iterator, handler));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::resolveHandler(const ErrorCode& e, ip::tcp::resolver::iterator it,
                                           const ConnectionHandler& handler) {
            try {
                if (!e) {
                    // No errors... create channel
                    Channel::Pointer channel = createChannel();
                    // ... cast it to the TcpChannel
                    TcpChannel::Pointer tcpChannel = std::static_pointer_cast<TcpChannel>(channel);
                    // ... get peer endpoint ...
                    const boost::asio::ip::tcp::endpoint& peerEndpoint = *it;
                    // ... and let the tcpChannel connect its socket asynchronously to the endpoint
                    tcpChannel->asyncSocketConnect(peerEndpoint,
                                                   std::bind(handler, boost::asio::placeholders::error, channel));
                } else {
                    Channel::Pointer c;
                    handler(e, c);
                }
            } catch (const karabo::data::Exception& e) {
                KARABO_RETHROW
            } catch (const std::exception& ex) {
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION(
                      string("Standard exception caught by TcpConnection::resolveHandler: ") + ex.what()))
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::stop() {
            m_resolver.cancel();
            try {
                m_acceptor.cancel();
            } catch (const boost::system::system_error& e) {
            }
            try {
                m_acceptor.close();
            } catch (const boost::system::system_error& e) {
            }
        }


        ChannelPointer TcpConnection::createChannel() {
            ChannelPointer channel(
                  new TcpChannel(std::dynamic_pointer_cast<TcpConnection>(this->getConnectionPointer())));
            return channel;
        }
    } // namespace net
} // namespace karabo
