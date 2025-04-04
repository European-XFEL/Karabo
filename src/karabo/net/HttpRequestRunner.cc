/*
 * HttpRequestRunner.cc
 *
 * Runs an http GET or POST request over a plain connection.
 *
 * Created on August, 02, 2023.
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

// TODO: Keep a queue of HTTP requests and reuse the network connection for
//       all the requests in the queue. The runner should run while there are
//       requests to be processed.

#include "HttpRequestRunner.hh"

#include <iostream>
#include <memory>

#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/StringTools.hh"

namespace karabo {
    namespace net {
        HttpRequestRunner::HttpRequestRunner(boost::asio::io_context& ioc, verb method, int httpVersion)
            : m_resolver(boost::asio::make_strand(ioc)),
              m_stream(boost::asio::make_strand(ioc)),
              m_method(method),
              m_httpVersion(httpVersion) {
            throwIfUnsupportedMethod();
        }

        void HttpRequestRunner::run(const std::string& host, unsigned short port, const std::string& route,
                                    const HttpHeaders& reqHeaders, const std::string& reqBody,
                                    const HttpResponseHandler& respHandler) {
            if (m_method == verb::get) {
                m_getRequest.version(m_httpVersion);
                m_getRequest.method(verb::get);
                m_getRequest.target(route);
                m_getRequest.set(HttpHeader::host, host);
                for (const auto& req : reqHeaders) {
                    m_getRequest.set(req.name_string(), req.value());
                }
            } else {
                m_postRequest.version(m_httpVersion);
                m_postRequest.method(verb::post);
                m_postRequest.target(route);
                m_postRequest.set(HttpHeader::host, host);
                for (const auto& req : reqHeaders) {
                    m_postRequest.set(req.name_string(), req.value());
                }
                m_postRequest.body() = reqBody;
                m_postRequest.prepare_payload();
            }

            m_responseHandler = respHandler;

            m_resolver.async_resolve(
                  host, karabo::data::toString(port),
                  boost::beast::bind_front_handler(&HttpRequestRunner::on_resolve, shared_from_this()));
        }

        void HttpRequestRunner::on_resolve(errorCode ec, const results_type& results) {
            if (ec) {
                fail(ec, "resolve");
                return;
            }

            // Set a timeout on the operation
            m_stream.expires_after(std::chrono::seconds(NET_OP_TIMEOUT_SECS));

            // Make the connection on the IP address we get from a lookup
            boost::beast::get_lowest_layer(m_stream).async_connect(
                  results, boost::beast::bind_front_handler(&HttpRequestRunner::on_connect, this->shared_from_this()));
        }

        void HttpRequestRunner::on_connect(errorCode ec, const results_type::endpoint_type&) {
            if (ec) {
                fail(ec, "connect");
                return;
            }

            // Sets a timeout
            m_stream.expires_after(std::chrono::seconds(NET_OP_TIMEOUT_SECS));

            // Sends the HTTP request to the remote host
            if (m_method == verb::get) {
                boost::beast::http::async_write(
                      m_stream, m_getRequest,
                      boost::beast::bind_front_handler(&HttpRequestRunner::on_write, shared_from_this()));
            } else {
                boost::beast::http::async_write(
                      m_stream, m_postRequest,
                      boost::beast::bind_front_handler(&HttpRequestRunner::on_write, shared_from_this()));
            }
        }

        void HttpRequestRunner::on_write(errorCode ec, std::size_t bytesTransferred) {
            boost::ignore_unused(bytesTransferred);
            if (ec) {
                return fail(ec, "write");
            }

            // Receives the HTTP response
            boost::beast::http::async_read(
                  m_stream, m_buffer, m_response,
                  boost::beast::bind_front_handler(&HttpRequestRunner::on_read, shared_from_this()));
        }

        void HttpRequestRunner::on_read(errorCode ec, std::size_t bytesTransferred) {
            boost::ignore_unused(bytesTransferred);

            if (ec) {
                return fail(ec, "read");
            }

            // Sends the received HTTP response to the handler.
            m_responseHandler(m_response);

            // Closes the socket gracefully
            m_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

            // Ignore not_connected as it can happen sometimes
            if (ec && ec != boost::beast::errc::not_connected) {
                return fail(ec, "shutdown");
            }

            // Reaching this means the connection has been closed gracefully.
        }

        void HttpRequestRunner::throwIfUnsupportedMethod() {
            if (m_method != verb::get && m_method != verb::post) {
                throw KARABO_PARAMETER_EXCEPTION("Only GET and POST methods are supported.");
            }
        }

        void HttpRequestRunner::fail(errorCode ec, char const* what) {
            std::cerr << "ERROR on HttpRequestRunner  - " << what << ": " << ec.message() << "\n";
        }

    } // namespace net
} // namespace karabo
