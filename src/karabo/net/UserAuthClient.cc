/*
 * UserAuthClient.cc
 *
 * Created on August, 11, 2022.
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

#include "UserAuthClient.hh"

#include <nlohmann/json.hpp>

namespace karabo {
    namespace net {

        namespace nl = nlohmann;
        namespace http = boost::beast::http;

        UserAuthClient::UserAuthClient(const std::string& authServerUrl) : m_cli(authServerUrl) {}

        void UserAuthClient::authorizeOneTimeToken(const std::string& token, const std::string& topic,
                                                   const AuthOneTimeTokenHandler& authHandler) {
            using karabo::data::Schema;

            HttpHeaders reqHeaders;
            reqHeaders.set(HttpHeader::user_agent, "Karabo User Auth Client");
            reqHeaders.set(HttpHeader::content_type, "application/json");

            const std::string reqBody{R"({"tk": ")" + token + R"(", "topic": ")" + topic + "\"}"};

            m_cli.asyncPost("/authorize_once_tk", reqHeaders, reqBody,
                            [authHandler](const http::response<http::string_body>& resp) {
                                if (resp.result() != http::status::ok) {
                                    // An error occurred at the http level
                                    authHandler(OneTimeTokenAuthorizeResult{
                                          .success = false,
                                          .userId = "",
                                          .accessLevel = Schema::OBSERVER,
                                          .errMsg = karabo::data::toString(resp.result_int()) + " - " +
                                                    std::string(resp.reason().data(), resp.reason().size())});
                                } else {
                                    try {
                                        nl::json respObj = nl::json::parse(resp.body());
                                        // The AuthServer processed the request and generated a valid response - pass
                                        // the response to the handler.
                                        Schema::AccessLevel level = Schema::OBSERVER;
                                        if (!respObj["visibility"].is_null()) {
                                            level = respObj["visibility"];
                                        }
                                        authHandler(OneTimeTokenAuthorizeResult{
                                              .success = static_cast<bool>(respObj["success"]),
                                              .userId = respObj["username"].is_null() ? "" : respObj["username"],
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
