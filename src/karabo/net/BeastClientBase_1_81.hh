//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
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

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>


namespace karabo {
    namespace net {

        namespace beast = boost::beast;   // from <boost/beast.hpp>
        namespace http = beast::http;     // from <boost/beast/http.hpp>
        namespace net = boost::asio;      // from <boost/asio.hpp>
        namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
        using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

        //------------------------------------------------------------------------------

        // Performs an HTTP POST and prints the response
        class HttpsSession : public std::enable_shared_from_this<HttpsSession> {
            tcp::resolver m_resolver;
            beast::ssl_stream<beast::tcp_stream> m_stream;
            beast::flat_buffer m_buffer; // (Must persist between reads)
            http::request<http::string_body> m_request;
            http::response<http::string_body> m_response;
            std::function<void(bool, std::string)> m_handler;

           public:
            explicit HttpsSession(net::any_io_executor ex, ssl::context& ctx) : m_resolver(ex), m_stream(ex, ctx) {}

            // Start the asynchronous operation
            void run(const std::string& host, const std::string& port, const std::string& target,
                     const std::string& body, int version, const std::function<void(bool, std::string)>& handler) {
                m_handler = std::move(handler);
                // Set SNI Hostname (many hosts need this to handshake successfully)
                if (!SSL_set_tlsext_host_name(m_stream.native_handle(), host.c_str())) {
                    beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
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
                                         beast::bind_front_handler(&HttpsSession::on_resolve, shared_from_this()));
            }

           private:
            void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
                if (ec) {
                    fail(ec, "resolve");
                    return;
                }

                // Set a timeout on the operation
                beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

                // Make the connection on the IP address we get from a lookup
                beast::get_lowest_layer(m_stream).async_connect(
                      results, beast::bind_front_handler(&HttpsSession::on_connect, shared_from_this()));
            }

            void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
                if (ec) {
                    fail(ec, "connect");
                    return;
                }

                // Perform the SSL handshake
                m_stream.async_handshake(ssl::stream_base::client,
                                         beast::bind_front_handler(&HttpsSession::on_handshake, shared_from_this()));
            }

            void on_handshake(beast::error_code ec) {
                if (ec) {
                    fail(ec, "handshake");
                    return;
                }

                // Set a timeout on the operation
                beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

                // Send the HTTP request to the remote host
                http::async_write(m_stream, m_request,
                                  beast::bind_front_handler(&HttpsSession::on_write, shared_from_this()));
            }

            void on_write(beast::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if (ec) {
                    fail(ec, "write");
                    return;
                }

                // Receive the HTTP response
                http::async_read(m_stream, m_buffer, m_response,
                                 beast::bind_front_handler(&HttpsSession::on_read, shared_from_this()));
            }

            void on_read(beast::error_code ec, std::size_t bytes_transferred) {
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

                // Set a timeout on the operation
                beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

                // Gracefully close the stream
                m_stream.async_shutdown(beast::bind_front_handler(&HttpsSession::on_shutdown, shared_from_this()));
            }

            void on_shutdown(beast::error_code ec) {
                if (ec == net::error::eof || ec == ssl::error::stream_errors::stream_truncated) {
                    // Rationale:
                    // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
                    ec = {};
                }
                if (ec) {
                    std::cout << "WARN shutdown: " << ec.message() << std::endl;
                    return;
                }

                // If we get here then the connection is closed gracefully
            }

            // Report a failure
            void fail(beast::error_code ec, char const* what) {
                std::ostringstream oss;
                oss << what << ": " << ec.message();
                m_handler(false, oss.str());
            }
        };
    } // namespace net
} // namespace karabo
