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

// #include <belle.hh>
#include <boost/algorithm/string.hpp>

#include "karabo/net/utils.hh"
#include "karabo/util/Exception.hh"
#include "karabo/util/StringTools.hh"

namespace karabo {
    namespace net {

        class HttpClient::Impl {
           public:
            Impl(const std::string& baseURL) : m_baseURL(baseURL) {
                if (!baseURL.empty()) {
                    const auto& urlParts = parseUrl(baseURL);
                    const std::string& protocol = boost::algorithm::to_lower_copy(urlParts.get<0>());
                    if (protocol != "http" && protocol != "https") {
                        throw KARABO_PARAMETER_EXCEPTION("Unsupported protocol, '" + protocol +
                                                         "' in baseURL argument, '" + baseURL + "'.");
                    }
                    const std::string host = urlParts.get<1>();
                    if (host.empty()) {
                        throw KARABO_PARAMETER_EXCEPTION("No host specified in baseURL argument, '" + baseURL + "'.");
                    }
                    const std::string portStr = urlParts.get<2>();
                    unsigned short port = 0;
                    if (portStr.empty()) {
                        port = (protocol == "http" ? 80 : 443);
                    } else {
                        port = karabo::util::fromString<unsigned short>(portStr);
                        if (port == 0) {
                            // As fromString ends up using strtoul, a zero is interpreted as "no valid conversion could
                            // be performed". A port 0 is invalid anyway; the valid range for ports is 1 through 65535.
                            throw KARABO_PARAMETER_EXCEPTION("Invalid port '" + portStr + "' in baseURL argument, '" +
                                                             baseURL + "'.");
                        }
                    }

                    /*
                    m_cli.address(host);
                    m_cli.port(port);
                    if (protocol == "https") {
                        m_cli.ssl(true);
                        // NOTE: Once none of the certificates used in test and production environments are self-signed,
                        //       remove the line below and rely on the default, "boost::asio::ssl::verify_peer".
                        m_cli.ssl_context().set_verify_mode(boost::asio::ssl::verify_none);
                    } else {
                        m_cli.ssl(false);
                    }
                    */
                }
            }

            void asyncPost(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                           const ResponseHandler& respHandler) {
                // asyncRequest(OB::Belle::Method::post, route, reqHeaders, reqBody, respHandler);
            }

            void asyncGet(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                          const ResponseHandler& respHandler) {
                // asyncRequest(OB::Belle::Method::get, route, reqHeaders, reqBody, respHandler);
            }

           private:
            // OB::Belle::Client m_cli;
            const std::string m_baseURL;

            /*
            void asyncRequest(OB::Belle::Method method, const std::string& route, const HttpHeaders& reqHeaders,
                              const std::string& reqBody, const ResponseHandler& respHandler) {
                if (m_baseURL.empty()) {
                    throw KARABO_PARAMETER_EXCEPTION(
                          "Invalid use of karabo::net::HttpClient: baseURL provided for HttpClient is empty");
                }
                m_cli.on_http(method, route, reqHeaders, reqBody,
                              [respHandler](const OB::Belle::Client::Http_Ctx& ctx) {
                                  // Calls the supplied response handler passing the boost::beast::http::response part
                                  // of the Belle Http_Ctx.
                                  if (respHandler) {
                                      respHandler(ctx.res);
                                  }
                              });
                // Submit the request
                m_cli.connect();
            }
            */
        };

        HttpClient::HttpClient(const std::string& baseURL) {
            m_impl = std::make_unique<Impl>(baseURL);
        }

        HttpClient::~HttpClient() = default;

        void HttpClient::asyncPost(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                                   const ResponseHandler& respHandler) {
            m_impl->asyncPost(route, reqHeaders, reqBody, respHandler);
        }

        void HttpClient::asyncGet(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                                  const ResponseHandler& respHandler) {
            m_impl->asyncGet(route, reqHeaders, reqBody, respHandler);
        }
    } // namespace net
} // namespace karabo
