/*
 * UserAuthClient.cc
 *
 * Created on August, 11, 2022.
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "UserAuthClient.hh"

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

#include "karabo/net/utils.hh"
#include "karabo/util/Exception.hh"
#include "karabo/util/StringTools.hh"

namespace karabo {
    namespace net {

        namespace nl = nlohmann;

        UserAuthClient::UserAuthClient(const std::string& authServerUrl) : m_cli(authServerUrl) {}

        void UserAuthClient::authorizeOneTimeToken(const std::string& token, const std::string& topic,
                                                   const AuthOneTimeTokenHandler& authHandler) {
            using karabo::util::Schema;

            HttpHeaders reqHeaders;
            reqHeaders.set(HttpHeader::user_agent, "Karabo User Auth Client");
            reqHeaders.set(HttpHeader::content_type, "application/json");

            const std::string reqBody{"{\"tk\": \"" + token + "\", \"topic\": \"" + topic + "\"}"};

            m_cli.asyncPost("/authorize_once_tk", reqHeaders, reqBody,
                            [authHandler](const http::response<http::string_body>& resp) {
                                if (resp.result() != http::status::ok) {
                                    // An error occurred at the http level
                                    authHandler(OneTimeTokenAuthorizeResult{
                                          .success = false,
                                          .userId = "",
                                          .accessLevel = Schema::OBSERVER,
                                          .errMsg = std::string(resp.reason().data(), resp.reason().size())});
                                } else {
                                    try {
                                        nl::json respObj = nl::json::parse(resp.body());
                                        // The AuthServer processed the request and generated a valid response - pass
                                        // the response to the handler.
                                        Schema::AccessLevel level = Schema::OBSERVER;
                                        if (!respObj["visib_level"].is_null()) {
                                            level = respObj["visib_level"];
                                        }
                                        authHandler(OneTimeTokenAuthorizeResult{
                                              .success = static_cast<bool>(respObj["success"]),
                                              .userId = respObj["user"].is_null() ? "" : respObj["user"],
                                              .accessLevel = static_cast<Schema::AccessLevel>(level),
                                              .errMsg = respObj["error_msg"].is_null() ? "" : respObj["error_msg"]});

                                    } catch (const std::exception& e) {
                                        // Problem parsing the JSON response (probably invalid JSON)
                                        authHandler(OneTimeTokenAuthorizeResult{
                                              .success = false,
                                              .userId = "",
                                              .accessLevel = Schema::OBSERVER,
                                              .errMsg = "Error parsing JSON response: " + std::string(e.what()) +
                                                        "\nResponse:\n" + resp.body()});
                                    }
                                }
                            }); // m_cli.asyncPost
        }
    } // namespace net
} // namespace karabo
