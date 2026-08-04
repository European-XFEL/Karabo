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

#include "WebSocketConnection.hh"

#include <any>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>

#include "EventLoop.hh"
#include "WebSocketChannel.hh"
#include "karabo/data/io/BinarySerializer.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/log/Logger.hh"
#include "karabo/util/MetaTools.hh"
#include "utils.hh"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
using asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

using namespace std;
using namespace karabo::data;
using namespace karabo::util;


KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Connection, karabo::net::WebSocketConnection)


namespace karabo::net {

    static void fail(beast::error_code ec, char const* what);

    class WebSocketListener : public std::enable_shared_from_this<WebSocketListener> {
       private:
        asio::io_context& m_ioc;
        tcp::acceptor& m_acceptor;
        Connection::WeakPointer m_connectionPointer;
        Connection::ConnectionHandler m_handler;
        std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>> m_ws; // websocket ptr


       public:
        explicit WebSocketListener(tcp::acceptor& acctor, Connection::Pointer conn,
                                   const Connection::ConnectionHandler& handler)
            : m_ioc(EventLoop::getIOService()),
              m_acceptor(acctor),
              m_connectionPointer(conn),
              m_handler(handler),
              m_ws(nullptr) {}

        ~WebSocketListener() {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketListener") << "Destructor is called";
        }

        void start() {
            doAccept();
        }

       private:
        void doAccept() {
            // The new connection gets its own strand
            m_acceptor.async_accept(asio::make_strand(m_ioc),
                                    beast::bind_front_handler(&WebSocketListener::onAccept, shared_from_this()));
        }

        void onAccept(beast::error_code ec, tcp::socket socket) {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketListener") << "onAccept : ec=" << ec;

            if (ec == beast::errc::operation_canceled) return; // 'stop()' is called?
            if (ec) {
                recycleAcceptor();
                fail(ec, "accept");
            }
            // Create websocket pointer
            m_ws = std::make_shared<websocket::stream<beast::tcp_stream>>(std::move(socket));
            run();
        }

        void recycleAcceptor() {
            if (m_acceptor.is_open()) {
                m_acceptor.cancel();
                m_acceptor.close();
            }
        }

        // Get on the correct executor
        void run() {
            // We need to be executing within a strand to perform async operations
            // on the I/O objects in this channel. Although not strictly necessary
            // for single-threaded contexts, this code is written to be
            // thread-safe by default.
            asio::dispatch(m_ws->get_executor(),
                           beast::bind_front_handler(&WebSocketListener::onRun, shared_from_this()));
        }

        void onRun() {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketListener") << "onRun ...";

            m_ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
            m_ws->set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
                res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async");
            }));
            // Waiting for handshake request
            m_ws->async_accept(beast::bind_front_handler(&WebSocketListener::onHandshake, shared_from_this()));
        }

        void onHandshake(beast::error_code ec) {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketListener") << "onHandshake : ec=" << ec;

            if (ec) {
                recycleAcceptor();
                fail(ec, "accept-handshake");
            }

            // For testing purposes we activate control callback...
            m_ws->control_callback([](boost::beast::websocket::frame_type kind, boost::beast::string_view payload) {
                if (kind == boost::beast::websocket::frame_type::ping) {
                    KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketListener") << "Ping received!";
                } else if (kind == boost::beast::websocket::frame_type::close) {
                    KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketListener") << "Close frame received.";
                }
            });

            // Create WebSocket channel pointer ...
            auto wschannel = std::make_shared<WebSocketChannel>(std::move(m_ws), m_connectionPointer);
            // From derived to base
            Channel::Pointer channel = std::static_pointer_cast<Channel>(wschannel);
            constexpr auto success = boost::system::errc::make_error_code(boost::system::errc::success);
            asio::post(wschannel->get_executor(), beast::bind_handler(std::move(m_handler), success, channel));
            // After leaving this function the WebSocketListener destructor is called
        }
    };


    class WebSocketConnector : public std::enable_shared_from_this<WebSocketConnector> {
       private:
        boost::asio::io_context& m_ioc;
        std::shared_ptr<websocket::stream<boost::beast::tcp_stream>> m_ws;
        boost::asio::ip::tcp::resolver& m_resolver;
        Connection::WeakPointer m_connectionPointer;
        Connection::ConnectionHandler m_handler;
        std::string m_host;


       public:
        WebSocketConnector(boost::asio::ip::tcp::resolver& resver, Connection::Pointer conn,
                           const Connection::ConnectionHandler& handler)
            : m_ioc(EventLoop::getIOService()),
              m_ws(std::make_shared<websocket::stream<beast::tcp_stream>>(asio::make_strand(m_ioc))),
              m_resolver(resver),
              m_connectionPointer(conn),
              m_handler(handler) {}

        ~WebSocketConnector() {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketConnector") << "Destructor is called.";
        }

        void start(const std::string& host, const std::string& port) {
            m_host = host;

            // Look up the domain name
            m_resolver.async_resolve(host, port,
                                     beast::bind_front_handler(&WebSocketConnector::onResolve, shared_from_this()));
        }

        void onResolve(beast::error_code ec, tcp::resolver::results_type results) {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketConnector") << "onResolve: ec=" << ec;
            if (ec) return fail(ec, "resolve");
            // Set the timeout for the operation
            beast::get_lowest_layer(*m_ws).expires_after(std::chrono::seconds(30));
            // Make the connection on the IP address we get from a lookup
            beast::get_lowest_layer(*m_ws).async_connect(
                  results, beast::bind_front_handler(&WebSocketConnector::onConnect, shared_from_this()));
        }

        void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketConnector") << "onConnect: ec=" << ec;
            if (ec) return fail(ec, "connect");

            // Turn off the timeout on the tcp_stream, because
            // the websocket stream has its own timeout system.
            beast::get_lowest_layer(*m_ws).expires_never();

            // Set suggested timeout settings for the websocket
            m_ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

            // Set a decorator to change the User-Agent of the handshake
            m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
                req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
            }));

            // Update the host_ string. This will provide the value of the
            // Host HTTP header during the WebSocket handshake.
            // See https://tools.ietf.org/html/rfc7230#section-5.4
            m_host += ':' + std::to_string(ep.port());

            // Perform the websocket handshake
            m_ws->async_handshake(m_host, "/",
                                  beast::bind_front_handler(&WebSocketConnector::onHandshake, shared_from_this()));
        }

        void onHandshake(beast::error_code ec) {
            KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketConnector") << "onHandshake: ec=" << ec;
            if (ec) return fail(ec, "handshake");

            // For testing purposes we activate control callback...
            m_ws->control_callback([](boost::beast::websocket::frame_type kind, boost::beast::string_view payload) {
                if (kind == boost::beast::websocket::frame_type::ping) {
                    KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketListener") << "Ping received!";
                } else if (kind == boost::beast::websocket::frame_type::close) {
                    KARABO_LOG_FRAMEWORK_DEBUG_C("karabo::net::WebSocketListener") << "Close frame received.";
                }
            });

            auto wschannel = std::make_shared<WebSocketChannel>(std::move(m_ws), m_connectionPointer);
            Channel::Pointer channel = std::static_pointer_cast<Channel>(wschannel);
            constexpr auto success = boost::system::errc::make_error_code(boost::system::errc::success);
            asio::post(wschannel->get_executor(), beast::bind_handler(std::move(m_handler), success, channel));
            // After leaving this function the WebSocketListener destructor is called
        }
    };


    // Report a failure
    void fail(beast::error_code ec, char const* what) {
        std::ostringstream oss;
        oss << what << ": " << ec.message();
        throw KARABO_NETWORK_EXCEPTION(oss.str());
    }


    void WebSocketConnection::expectedParameters(karabo::data::Schema& expected) {
        STRING_ELEMENT(expected)
              .key("type")
              .displayedName("Connection Type")
              .description("Decide whether the connection is used to implement a WebSocket Server or WebSocket Client")
              .options(std::vector<std::string>({"server", "client"}))
              .assignmentOptional()
              .defaultValue("server")
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
              .defaultValue(8888)
              .maxInc(65535) // ports are 16-bit
              .commit();

        STRING_ELEMENT(expected)
              .key("url")
              .displayedName("URL")
              .description(
                    "URL format is ws://hostname:port. This style has precedence over specifying hostname and "
                    "port.")
              .assignmentOptional()
              .defaultValue("")
              .commit();
    }


    WebSocketConnection::WebSocketConnection(const karabo::data::Hash& input)
        : Connection(input),
          m_ioc(EventLoop::getIOService()),
          m_acceptor(m_ioc),
          m_resolver(asio::make_strand(m_ioc)),
          m_connectionType("server"),
          m_hostname("0.0.0.0"),
          m_port(8090) {
        string url;
        input.get("url", url);
        if (url.empty()) {
            input.get("hostname", m_hostname);
            input.get("port", m_port);
        } else {
            const std::tuple<std::string, std::string, std::string, std::string, std::string> parts = parseUrl(url);
            if (std::get<0>(parts) != "ws") {
                throw KARABO_NETWORK_EXCEPTION(("url '" + url) += "' does not start with 'ws'");
            }
            m_hostname = std::get<1>(parts);
            m_port = fromString<unsigned int>(std::get<2>(parts));
        }
        input.get("type", m_connectionType);
    }


    WebSocketConnection::~WebSocketConnection() {
        stop();
    }


    void WebSocketConnection::initServer(boost::asio::ip::tcp::endpoint endpoint) {
        try {
            beast::error_code ec;

            // Open the acceptor
            m_acceptor.open(endpoint.protocol(), ec);
            if (ec) fail(ec, "open");
            // Allow address reuse
            m_acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
            if (ec) fail(ec, "set_option 1");
            // Enable connection to be aborted
            m_acceptor.set_option(tcp::acceptor::enable_connection_aborted(true), ec);
            if (ec) fail(ec, "set_option 2");
            // Bind to the server address
            m_acceptor.bind(endpoint, ec);
            if (ec) fail(ec, "bind");
            // Start listening for incoming connections
            m_acceptor.listen(asio::socket_base::max_listen_connections, ec);
            if (ec) fail(ec, "listen");
        } catch (const std::exception& e) {
            if (m_acceptor.is_open()) {
                m_acceptor.cancel();
                m_acceptor.close();
            }
            KARABO_RETHROW_AS(KARABO_NETWORK_EXCEPTION("bind with port " + std::to_string(m_port) + " failed."));
        }
    }


    int WebSocketConnection::startAsync(const Connection::ConnectionHandler& handler) {
        if (!handler) throw KARABO_PARAMETER_EXCEPTION("Empty handler");
        beast::error_code ec;
        if (m_connectionType == "server") {
            tcp::endpoint endpoint(tcp::v4(), m_port);
            if (!m_acceptor.is_open()) initServer(endpoint);
            std::make_shared<WebSocketListener>(m_acceptor, shared_from_this(), handler)->start();
        } else if (m_connectionType == "client") {
            auto port = std::to_string(m_port);
            std::make_shared<WebSocketConnector>(m_resolver, shared_from_this(), handler)->start(m_hostname, port);
        }
        return m_port;
    }


    void WebSocketConnection::stop() {
        try {
            m_resolver.cancel();
        } catch (const boost::system::system_error& e) {
        }

        try {
            m_acceptor.cancel();
        } catch (const boost::system::system_error& e) {
        }

        try {
            m_acceptor.close();
        } catch (const boost::system::system_error& e) {
        }
    }

} // namespace karabo::net
