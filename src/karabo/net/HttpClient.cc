/*
 * HttpClient.cc
 *
 * Created on February, 09, 2023.
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

#include "HttpClient.hh"

#include <boost/algorithm/string.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/ssl.hpp>

#include "karabo/net/HttpRequestRunner.hh"
#include "karabo/net/HttpsRequestRunner.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/Exception.hh"
#include "karabo/util/StringTools.hh"

namespace karabo {
    namespace net {

        namespace beast = boost::beast;   // from <boost/beast.hpp>
        namespace http = beast::http;     // from <boost/beast/http.hpp>
        namespace asio = boost::asio;     // from <boost/asio.hpp>
        namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
        using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

        /**
         * @brief Implementation of the WebClient class.
         */
        class HttpClient::Impl {
           public:
            Impl(const std::string& baseURL, bool verifyCerts)
                : m_baseURL{baseURL}, m_sslCtx{ssl::context::tlsv12_client} {
                if (!m_baseURL.empty()) {
                    const auto& urlParts = parseUrl(baseURL);
                    const std::string& protocol = boost::algorithm::to_lower_copy(std::get<0>(urlParts));
                    if (protocol != "http" && protocol != "https") {
                        throw KARABO_PARAMETER_EXCEPTION("Unsupported protocol, '" + protocol +
                                                         "' in baseURL argument, '" + baseURL + "'.");
                    }
                    m_ssl = (protocol == "https");
                    m_host = std::get<1>(urlParts);
                    if (m_host.empty()) {
                        throw KARABO_PARAMETER_EXCEPTION("No host specified in baseURL argument, '" + baseURL + "'.");
                    }
                    const std::string portStr = std::get<2>(urlParts);
                    m_port = 0;
                    if (portStr.empty()) {
                        m_port = (m_ssl ? 443 : 80);
                    } else {
                        m_port = karabo::util::fromString<unsigned short>(portStr);
                        if (m_port == 0) {
                            // As fromString ends up using strtoul, a zero is interpreted as "no valid conversion could
                            // be performed". A port 0 is invalid anyway; the valid range for ports is 1 through 65535.
                            throw KARABO_PARAMETER_EXCEPTION("Invalid port '" + portStr + "' in baseURL argument, '" +
                                                             baseURL + "'.");
                        }
                    }
                    if (m_ssl) {
                        if (verifyCerts) {
                            m_sslCtx.set_verify_mode(ssl::verify_peer | ssl::context::verify_fail_if_no_peer_cert);
                            m_sslCtx.set_default_verify_paths();
                        }
                        m_sslCtx.set_verify_mode(verifyCerts ? ssl::verify_peer : ssl::verify_none);
                    }
                }
            }

            void asyncPost(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                           const HttpResponseHandler& respHandler) {
                asyncRequest(http::verb::post, route, reqHeaders, reqBody, respHandler);
            }

            void asyncGet(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                          const HttpResponseHandler& respHandler) {
                asyncRequest(http::verb::get, route, reqHeaders, reqBody, respHandler);
            }

            void asyncRequest(http::verb method, const std::string& route, const HttpHeaders& reqHeaders,
                              const std::string& reqBody, const HttpResponseHandler& respHandler) {
                if (m_baseURL.empty()) {
                    throw KARABO_PARAMETER_EXCEPTION(
                          "A non-empty base URL with protocol, host and optional port specification is required.");
                }
                // NOTE: The request runner objects can currently handle a single request at a time.
                //       To comply with this limitation, the HttpClient instantiates a request runner per
                //       request.
                asio::io_context ioc;
                if (m_ssl) {
                    std::make_shared<HttpsRequestRunner>(boost::asio::make_strand(ioc), m_sslCtx, method, HTTP_VERSION)
                          ->run(m_host, m_port, route, reqHeaders, reqBody, respHandler);
                } else {
                    std::make_shared<HttpRequestRunner>(ioc, method, HTTP_VERSION)
                          ->run(m_host, m_port, route, reqHeaders, reqBody, respHandler);
                }
                ioc.run();
            }

           private:
            const std::string m_baseURL;
            bool m_ssl;
            std::string m_host;
            unsigned short m_port;
            ssl::context m_sslCtx;

        }; // class WebClient::Impl

        HttpClient::HttpClient(const std::string& baseURL, bool verifyCerts) {
            m_impl = std::make_unique<Impl>(baseURL, verifyCerts);
        }

        HttpClient::~HttpClient() = default;

        void HttpClient::asyncPost(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                                   const HttpResponseHandler& respHandler) {
            m_impl->asyncPost(route, reqHeaders, reqBody, respHandler);
        }

        void HttpClient::asyncGet(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                                  const HttpResponseHandler& respHandler) {
            m_impl->asyncGet(route, reqHeaders, reqBody, respHandler);
        }
    } // namespace net
} // namespace karabo
