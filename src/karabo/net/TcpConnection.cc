/*
 * $Id: Connection.hh 3602 2011-05-31 21:09:02Z esenov@DESY.DE $
 *
 * File:   Connection.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 9:47 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "TcpConnection.hh"
#include "TcpChannel.hh"
#include "EventLoop.hh"
#include "utils.hh"

#include "karabo/log/Logger.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/MetaTools.hh"

#include <boost/any.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>

using namespace std;
using namespace boost::asio;
using namespace karabo::util;

namespace karabo {
    namespace net {


        KARABO_REGISTER_FOR_CONFIGURATION(Connection, TcpConnection)

        void TcpConnection::expectedParameters(karabo::util::Schema& expected) {
            STRING_ELEMENT(expected)
                    .key("type")
                    .displayedName("Connection Type")
                    .description("Decide whether the connection is used to implement a TCP Server or TCP Client")
                    .assignmentOptional().defaultValue("client")
                    .options("server,client")
                    .commit();

            STRING_ELEMENT(expected)
                    .key("hostname")
                    .displayedName("Hostname")
                    .description("Hostname of a peer (used only for client)")
                    .assignmentOptional().defaultValue("localhost")
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("port")
                    .displayedName("Hostport")
                    .description("Hostport of a peer for type 'client' and local port for type 'server'")
                    .assignmentOptional().defaultValue(0)
                    .commit();

            STRING_ELEMENT(expected)
                    .key("url")
                    .displayedName("URL")
                    .description("URL format is tcp://hostname:port. This style has precedence.")
                    .assignmentOptional().defaultValue("")
                    .commit();

            UINT32_ELEMENT(expected)
                    .key("sizeofLength")
                    .displayedName("Size of Message Length")
                    .description("The size of messageLength field in communication protocol")
                    .assignmentOptional().defaultValue(4)
                    .init()
                    .expertAccess()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("messageTagIsText")
                    .displayedName("Message Tag is Text")
                    .description("The length field in communication protocol is considered as text string")
                    .assignmentOptional().defaultValue(false)
                    .init()
                    .expertAccess()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("manageAsyncData")
                    .displayedName("Manage Async Data")
                    .description("If set to true, asynchronous write handlers will copy the data to be written. The user does not "
                                 "have to make sure that the to-be-written data has a long-enough life time.")
                    .assignmentOptional().defaultValue(true)
                    .init()
                    .expertAccess()
                    .commit();

            INT32_ELEMENT(expected)
                    .key("compressionUsageThreshold")
                    .displayedName("Compression Usage Threshold")
                    .description("The limit size to decide about applying a compression to the message.")
                    .reconfigurable()
                    .unit(Unit::BYTE)
                    .assignmentOptional().defaultValue(-1)
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("compression")
                    .displayedName("Compression")
                    .description("Compression library used")
                    .init()
                    .assignmentOptional().defaultValue(string("snappy"))
                    .options("snappy")
                    .expertAccess()
                    .commit();
        }


        TcpConnection::TcpConnection(const karabo::util::Hash& input)
            : Connection(input)
            , m_resolver(EventLoop::getIOService())
            , m_acceptor(EventLoop::getIOService()) {

            string url;
            input.get("url", url);
            if (url.empty()) {
                input.get("hostname", m_hostname);
                input.get("port", m_port);
            } else {
                const boost::tuple<std::string, std::string,
                                   std::string, std::string, std::string> parts = parseUrl(url);
                assert(parts.get<0>() == "tcp");
                m_hostname = parts.get<1>();
                m_port = fromString<unsigned int>(parts.get<2>());
            }
            input.get("type", m_connectionType);
            input.get("sizeofLength", m_sizeofLength);
            input.get("messageTagIsText", m_lengthIsTextFlag);
            input.get("manageAsyncData", m_manageAsyncData);
            input.get("compressionUsageThreshold", m_compressionUsageThreshold);
            input.get("compression", m_compression);
        }


        TcpConnection::~TcpConnection() {
            stop();
        }


        Channel::Pointer TcpConnection::start() {

            m_isAsyncConnect = false;

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
                TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel > (channel);
                tcpChannel->acceptSocket(m_acceptor);
            } catch (...) {
                KARABO_RETHROW
            }
            return channel;
        }


        Channel::Pointer TcpConnection::startClient() {
            Channel::Pointer channel;
            try {
                ip::tcp::resolver::query query(ip::tcp::v4(), m_hostname, karabo::util::toString(m_port));
                ip::tcp::resolver::iterator endpoint_iterator = m_resolver.resolve(query);
                channel = this->createChannel();
                TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel > (channel);
                tcpChannel->socketConnect(*endpoint_iterator);
            } catch (...) {
                KARABO_RETHROW
            }
            return channel;
        }


        int TcpConnection::startAsync(const ConnectionHandler& handler) {

            m_isAsyncConnect = true;
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
                                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("bind with port "
                                        + toString(m_port) + " failed. OS: '" + e.what() + "'"));
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
                TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel > (channel);
                tcpChannel->asyncAcceptSocket(m_acceptor, boost::bind(handler, boost::asio::placeholders::error, channel));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::startClient(const ConnectionHandler& handler) {
            try {
                ip::tcp::resolver::query query(ip::tcp::v4(), m_hostname, karabo::util::toString(m_port));
                m_resolver.async_resolve(query, bind_weak(&TcpConnection::resolveHandler,
                                                          this, boost::asio::placeholders::error,
                                                          boost::asio::placeholders::iterator, handler));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::resolveHandler(const ErrorCode& e, ip::tcp::resolver::iterator it, const ConnectionHandler& handler) {
            try {
                if (!e) {
                    // No errors... create channel
                    Channel::Pointer channel = createChannel();
                    // ... cast it to the TcpChannel
                    TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel > (channel);
                    // ... get peer endpoint ...
                    const boost::asio::ip::tcp::endpoint& peerEndpoint = *it;
                    // ... and let the tcpChannel connect its socket asynchronously to the endpoint
                    tcpChannel->asyncSocketConnect(peerEndpoint,
                                                   boost::bind(handler, boost::asio::placeholders::error, channel));
                } else {
                    Channel::Pointer c;
                    handler(e, c);
                }
            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW
            } catch (const std::exception& ex) {
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION(string("Standard exception caught by TcpConnection::resolveHandler: ") + ex.what()))
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
            ChannelPointer channel(new TcpChannel(this->getConnectionPointer()));
            return channel;
        }
    }
}
