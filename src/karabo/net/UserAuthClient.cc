/*
 * UserAuthClient.cc
 *
 * Created on August, 11, 2022.
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "UserAuthClient.hh"

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

#include "BeastClient.hh"
#include "karabo/net/utils.hh"
#include "karabo/util/Exception.hh"
#include "karabo/util/StringTools.hh"

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace net {

        namespace nl = nlohmann;

        UserAuthClient::UserAuthClient(const string& authServerUrl) : m_authServerUrl(authServerUrl) {
            if (!m_authServerUrl.empty()) {
                const auto& urlParts = parseUrl(authServerUrl);
                const string& protocol = boost::algorithm::to_lower_copy(urlParts.get<0>());
                if (protocol != "https") {
                    throw KARABO_PARAMETER_EXCEPTION("Unsupported protocol, '" + protocol + "' in authServerUrl, '" +
                                                     authServerUrl + "'.");
                }
                m_authServerHost = urlParts.get<1>();
                if (m_authServerHost.empty()) {
                    throw KARABO_PARAMETER_EXCEPTION("No host specified in authServerUrl, '" + authServerUrl + "'.");
                }
                const string& port = urlParts.get<2>();
                if (port.empty()) {
                    m_authServerPort = (protocol == "http" ? 80 : 443);
                } else {
                    m_authServerPort = fromString<unsigned short>(port);
                    if (m_authServerPort == 0) {
                        // As fromString ends up using strtoul, a zero is interpreted as "no valid conversion could be
                        // performed". A port 0 is invalid anyway; the valid range for ports is 1 through 65535.
                        throw KARABO_PARAMETER_EXCEPTION("Invalid port '" + port + "' in authServerUrl, '" +
                                                         authServerUrl + "'.");
                    }
                }
                const string& path = urlParts.get<3>();
                if (!path.empty() && path.at(path.size() - 1) == '/') {
                    // Make sure the path doesn't end with a '/' - the forward slash will be added when accessing the
                    // auth server endpoints.
                    m_authServerPath = path.substr(0, path.size() - 1);
                } else {
                    m_authServerPath = path;
                }
                m_useSSL = (protocol == "https");
            }
        }

        void UserAuthClient::authorizeOneTimeToken(const string& token, const string& topic,
                                                   const AuthOneTimeTokenHandler& handler) {
            if (m_authServerUrl.empty()) {
                handler(OneTimeTokenAuthorizeResult{
                      .success = false,
                      .userId = "",
                      .accessLevel = Schema::OBSERVER,
                      .errMsg = "No Authentication Server specified.",
                });
            }

            const std::string host = m_authServerHost;
            const std::string port = toString(m_authServerPort);
            assert(m_useSSL);
            const std::string target("/authorize_once_tk");
            const string reqBody{"{\"tk\": \"" + token + "\", \"topic\": \"" + topic + "\"}"};
            int version = 11; // HTTP 1.1

            httpsPost(host, port, target, reqBody, version, [handler](bool success, const std::string& response) {
                if (!success) {
                    handler(OneTimeTokenAuthorizeResult{
                          .success = false, .userId = "", .accessLevel = Schema::OBSERVER, .errMsg = response});
                } else {
                    try {
                        nl::json respObj = nl::json::parse(response);
                        // The AuthServer processed the request and generated a valid response - pass the
                        // response to the handler.
                        Schema::AccessLevel level = Schema::OBSERVER;
                        if (!respObj["visib_level"].is_null()) {
                            level = respObj["visib_level"];
                        }
                        handler(OneTimeTokenAuthorizeResult{
                              .success = static_cast<bool>(respObj["success"]),
                              .userId = respObj["user"].is_null() ? "" : respObj["user"],
                              .accessLevel = static_cast<Schema::AccessLevel>(level),
                              .errMsg = respObj["error_msg"].is_null() ? "" : respObj["error_msg"]});

                    } catch (const std::exception& e) {
                        // Problem parsing the JSON response (probably invalid JSON)
                        handler(OneTimeTokenAuthorizeResult{.success = false,
                                                            .userId = "",
                                                            .accessLevel = Schema::OBSERVER,
                                                            .errMsg = "Error parsing JSON response: " +
                                                                      string(e.what()) + "\nResponse:\n" + response});
                    }
                }
            });
        }
    } // namespace net
} // namespace karabo
