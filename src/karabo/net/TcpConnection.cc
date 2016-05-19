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

#include <boost/any.hpp>
#include <boost/pointer_cast.hpp>
#include "IOService.hh"
#include "TcpConnection.hh"
#include "TcpChannel.hh"
#include "AsioIOService.hh"
#include "karabo/log/Logger.hh"
#include <karabo/util/SimpleElement.hh>

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
                    .assignmentOptional().defaultValue(11111)
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
        , m_boostIoServicePointer()
        , m_resolver()
        , m_acceptor() {

            input.get("hostname", m_hostname);
            input.get("port", m_port);
            input.get("type", m_connectionType);
            input.get("sizeofLength", m_sizeofLength);
            input.get("messageTagIsText", m_lengthIsTextFlag);
            input.get("manageAsyncData", m_manageAsyncData);
            input.get("compressionUsageThreshold", m_compressionUsageThreshold);
            input.get("compression", m_compression);
        }


        TcpConnection::~TcpConnection() {
            m_acceptor.reset();
            m_resolver.reset();
            m_boostIoServicePointer.reset();
        }


        Channel::Pointer TcpConnection::start() {

            this->setIOServiceType("Asio");
            // Get the specific boost::asio::io_service object
            m_boostIoServicePointer = getIOService()->castTo<AsioIOService > ()->getBoostIOService();

            m_isAsyncConnect = false;

            if (m_connectionType == "server") {
                if (!m_acceptor) {
                    try {
                        BoostTcpAcceptor acceptor(new ip::tcp::acceptor(*m_boostIoServicePointer));
                        ip::tcp::endpoint endpoint(ip::tcp::v4(), m_port);
                        acceptor->open(endpoint.protocol());
                        acceptor->set_option(ip::tcp::acceptor::reuse_address(true));
                        acceptor->bind(endpoint); // <=== here exception possible: port in use
                        acceptor->listen();
                        m_acceptor = acceptor;
                        if (m_port != endpoint.port()) {
                            m_port = endpoint.port(); // if m_port was == 0 then the OS assigns free port number.
                        }
                    } catch (...) {
                        KARABO_RETHROW
                    }
                }
                return startServer();
            } else {
                if (!m_resolver) {
                    BoostTcpResolver resolv(new ip::tcp::resolver(*m_boostIoServicePointer));
                    m_resolver = resolv;
                }
                return startClient();
            }
        }


        Channel::Pointer TcpConnection::startServer() {
            Channel::Pointer new_channel;
            try {
                new_channel = this->createChannel();
                TcpChannel::Pointer ch = boost::static_pointer_cast<TcpChannel > (new_channel);
                m_acceptor->accept(ch->socket());
                //KARABO_LOG_FRAMEWORK_DEBUG << "Accepted new connection: " << ch->socket().remote_endpoint().address() << ":" << ch->socket().remote_endpoint().port();
            } catch (...) {
                KARABO_RETHROW
            }
            return new_channel;
        }


        Channel::Pointer TcpConnection::startClient() {
            Channel::Pointer new_channel;
            try {
                ip::tcp::resolver::query query(ip::tcp::v4(), m_hostname, karabo::util::toString(m_port));
                ip::tcp::resolver::iterator endpoint_iterator = m_resolver->resolve(query);
                new_channel = this->createChannel();
                TcpChannel::Pointer ch = boost::static_pointer_cast<TcpChannel > (new_channel);
                ch->socket().connect(*endpoint_iterator);
                //KARABO_LOG_FRAMEWORK_DEBUG << "Connected to: " << ch->socket().remote_endpoint().address() << ":" << ch->socket().remote_endpoint().port();
            } catch (...) {
                KARABO_RETHROW
            }
            return new_channel;
        }


        int TcpConnection::startAsync(const ConnectionHandler& handler) {

            this->setIOServiceType("Asio");
            // Get the specific boost::asio::io_service object
            m_boostIoServicePointer = getIOService()->castTo<AsioIOService > ()->getBoostIOService();


            m_isAsyncConnect = true;
            if (m_connectionType == "server") {
                if (!m_acceptor) {
                    try {
                        BoostTcpAcceptor acceptor(new ip::tcp::acceptor(*m_boostIoServicePointer));
                        ip::tcp::endpoint endpoint(ip::tcp::v4(), m_port);
                        acceptor->open(endpoint.protocol());
                        acceptor->set_option(ip::tcp::acceptor::reuse_address(true));
                        acceptor->set_option(ip::tcp::acceptor::enable_connection_aborted(true));
                        acceptor->bind(endpoint); // <=== here exception possible: port in use
                        acceptor->listen();
                        if (m_port == 0) {
                            ip::tcp::endpoint le = acceptor->local_endpoint();
                            m_port = le.port();
                        }
                        m_acceptor = acceptor;
//                        if (m_port != endpoint.port()) {
//                            m_port = endpoint.port(); // if m_port was == 0 then the OS assigns free port number.
//                        }
                    } catch (...) {
                        KARABO_RETHROW
                    }
                }
                startServer(handler);
            } else if (m_connectionType == "client") {
                if (!m_resolver) {
                    BoostTcpResolver resolv(new ip::tcp::resolver(*m_boostIoServicePointer));
                    m_resolver = resolv;
                }
                startClient(handler);
            }
            return m_port;
        }


        void TcpConnection::startServer(const ConnectionHandler& handler) {
            try {
                Channel::Pointer new_channel = this->createChannel();
                TcpChannel::Pointer ch = boost::static_pointer_cast<TcpChannel > (new_channel);
                m_acceptor->async_accept(ch->socket(), boost::bind(&TcpConnection::acceptHandler,
                                                                   this, new_channel, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::acceptHandler(Channel::Pointer channel, const ConnectionHandler& handler, const boost::system::error_code& e) {
            try {
                if (!e) {
                    {
                        TcpChannel::Pointer tc = boost::static_pointer_cast<TcpChannel > (channel);
                        KARABO_LOG_FRAMEWORK_DEBUG << "Accepted new connection: " << tc->socket().remote_endpoint().address() << ":" << tc->socket().remote_endpoint().port();
                    }
                    handler(channel);
                } else {
                    if (m_errorHandler)
                        m_errorHandler(e);
                    else if (e.value() != 125) {   // 125 -- Operation canceled
                        KARABO_LOG_FRAMEWORK_WARN << "TCP : Accept handler got code #" << e.value() << " -- " << e.message();
                    }
                }
            } catch (...) {
                // probably must not throw?
                KARABO_RETHROW
            }
        }


        void TcpConnection::startClient(const ConnectionHandler& handler) {
            try {
                ip::tcp::resolver::query query(ip::tcp::v4(), m_hostname, karabo::util::toString(m_port));
                m_resolver->async_resolve(query, boost::bind(&TcpConnection::resolveHandler,
                                                             this, handler, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::resolveHandler(const ConnectionHandler& handler, const ErrorCode& e, ip::tcp::resolver::iterator it) {
            try {
                if (!e) {
                    // No errors... create channel first to get access to the socket
                    Channel::Pointer new_channel = createChannel();
                    // ... cast it to the TcpChannel
                    TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel > (new_channel);
                    // ... get tcp channel socket ...
                    boost::asio::ip::tcp::socket& tcpSocket = tcpChannel->socket();
                    // ... and peer endpoint ...
                    const boost::asio::ip::tcp::endpoint& peerEndpoint = *it;
                    // ... and, finally, try to connect asynchronously ...
                    tcpSocket.async_connect(peerEndpoint,
                                            boost::bind(&TcpConnection::connectHandler, this,
                                                        new_channel, handler, boost::asio::placeholders::error));
                } else {
                    if (m_errorHandler)
                        m_errorHandler(e);
                    else
                        throw KARABO_NETWORK_EXCEPTION(e.message());
                }
            } catch (const std::exception& ex) {
                KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION(string("Standard exception caught by TcpConnection::resolveHandler: ") + ex.what()))
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::connectHandler(const Channel::Pointer& channel, const ConnectionHandler& handler, const boost::system::error_code& e) {
            try {
                if (!e) {
                    {
                        TcpChannel::Pointer tc = boost::static_pointer_cast<TcpChannel > (channel);
                        KARABO_LOG_FRAMEWORK_DEBUG << "Connected to: " << tc->socket().remote_endpoint().address() << ":" << tc->socket().remote_endpoint().port();
                    }
                    handler(channel);
                } else {
                    if (m_errorHandler)
                        m_errorHandler(e);
                    else
                        throw KARABO_NETWORK_EXCEPTION(e.message());
                }
            } catch (const std::exception& ex) {
                throw KARABO_NETWORK_EXCEPTION(string("Standard exception caught by TcpConnection::connectHandler: ") + ex.what());
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpConnection::stop() {
            boost::system::error_code ec;
            {
                boost::mutex::scoped_lock lock(m_boostTcpMutex);
                if (m_connectionType == "server" && m_acceptor) {
                    m_acceptor->cancel(ec);
                    if (ec) cout << "WARN  :  Acceptor cancellation failed: #" << ec.value() << " -- " << ec.message() << endl;
                    ec.clear();
                    m_acceptor->close(ec);
                    if (ec) cout << "WARN  :  Acceptor closing failed: #" << ec.value() << " -- " << ec.message() << endl;
                    m_acceptor.reset();
                } else if (m_resolver) {
                    m_resolver->cancel();
                    m_resolver.reset();
                }
            }
            m_boostIoServicePointer.reset();
            m_service.reset();
        }


        ChannelPointer TcpConnection::createChannel() {
            ChannelPointer channel(new TcpChannel(this->getConnectionPointer()));
            return channel;
        }
    }
}
