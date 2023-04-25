/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP SSL client, asynchronous
//
//------------------------------------------------------------------------------
#ifndef KARABO_NET_BEASTCLIENTBASE_1_68_HH
#define KARABO_NET_BEASTCLIENTBASE_1_68_HH

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>


namespace karabo {
    namespace net {

        using tcp = boost::asio::ip::tcp;    // from <boost/asio/ip/tcp.hpp>
        namespace ssl = boost::asio::ssl;    // from <boost/asio/ssl.hpp>
        namespace http = boost::beast::http; // from <boost/beast/http.hpp>

        //------------------------------------------------------------------------------

        // Performs an HTTP POST and prints the response
        class HttpsSession : public std::enable_shared_from_this<HttpsSession> {
            tcp::resolver m_resolver;
            ssl::stream<tcp::socket> m_stream;
            boost::beast::flat_buffer m_buffer; // (Must persist between reads)
            http::request<http::string_body> m_request;
            http::response<http::string_body> m_response;
            std::function<void(bool, std::string)> m_handler;

           public:
            // Resolver and stream require an io_context
            explicit HttpsSession(boost::asio::io_context& ioc, ssl::context& ctx)
                : m_resolver(ioc), m_stream(ioc, ctx) {}

            // Start the asynchronous operation
            void run(const std::string& host, const std::string& port, const std::string& target,
                     const std::string& body, int version, const std::function<void(bool, std::string)>& handler) {
                m_handler = handler;
                // Set SNI Hostname (many hosts need this to handshake successfully)
                if (!SSL_set_tlsext_host_name(m_stream.native_handle(), host.c_str())) {
                    boost::system::error_code ec{static_cast<int>(::ERR_get_error()),
                                                 boost::asio::error::get_ssl_category()};
                    std::cerr << ec.message() << "\n";
                    m_handler(false, ec.message());
                    return;
                }

                // Set up an HTTP POST request message
                m_request.version(version);
                m_request.method(http::verb::post);
                m_request.target(target);
                m_request.set(http::field::host, host);
                m_request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
                m_request.set(http::field::content_type, "application/json");
                m_request.body() = body;
                m_request.prepare_payload();

                // Look up the domain name
                m_resolver.async_resolve(host, port,
                                         std::bind(&HttpsSession::on_resolve, shared_from_this(), std::placeholders::_1,
                                                   std::placeholders::_2));
            }

            void on_resolve(boost::system::error_code ec, tcp::resolver::results_type results) {
                if (ec) {
                    fail(ec, "resolve");
                    return;
                }

                // Make the connection on the IP address we get from a lookup
                boost::asio::async_connect(
                      m_stream.next_layer(), results.begin(), results.end(),
                      std::bind(&HttpsSession::on_connect, shared_from_this(), std::placeholders::_1));
            }

            void on_connect(boost::system::error_code ec) {
                if (ec) {
                    fail(ec, "connect");
                    return;
                }

                // Perform the SSL handshake
                m_stream.async_handshake(
                      ssl::stream_base::client,
                      std::bind(&HttpsSession::on_handshake, shared_from_this(), std::placeholders::_1));
            }

            void on_handshake(boost::system::error_code ec) {
                if (ec) {
                    fail(ec, "handshake");
                    return;
                }

                // Send the HTTP request to the remote host
                http::async_write(m_stream, m_request,
                                  std::bind(&HttpsSession::on_write, shared_from_this(), std::placeholders::_1,
                                            std::placeholders::_2));
            }

            void on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if (ec) {
                    fail(ec, "write");
                    return;
                }

                // Receive the HTTP response
                http::async_read(m_stream, m_buffer, m_response,
                                 std::bind(&HttpsSession::on_read, shared_from_this(), std::placeholders::_1,
                                           std::placeholders::_2));
            }

            void on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if (ec) {
                    fail(ec, "read");
                    return;
                }

                // Call handler or wrtie the message to the standard output
                std::ostringstream oss;
                oss << m_response.body();
                if (!m_handler) {
                    std::cout << "No handler defined: " << oss.str() << std::endl;
                } else {
                    m_handler(true, oss.str());
                }

                // Gracefully close the stream
                m_stream.async_shutdown(
                      std::bind(&HttpsSession::on_shutdown, shared_from_this(), std::placeholders::_1));
            }

            void on_shutdown(boost::system::error_code ec) {
                if (ec == boost::asio::error::eof || ec == ssl::error::stream_errors::stream_truncated) {
                    // Rationale:
                    // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
                    ec.assign(0, ec.category());
                }
                if (ec) {
                    std::cout << "WARN shutdown: " << ec.message() << std::endl;
                    return;
                }

                // If we get here then the connection is closed gracefully
            }

            // Report a failure
            void fail(boost::system::error_code ec, char const* what) {
                std::ostringstream oss;
                oss << what << ": " << ec.message();
                m_handler(false, oss.str());
            }
        };
    } // namespace net
} // namespace karabo
#endif /* KARABO_NET_BEASTCLIENTBASE_1_68_HH */
