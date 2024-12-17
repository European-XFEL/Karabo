/*
 * UserAuthClient.hh
 *
 * Client for the KaraboAuthServer web api. The KaraboAuthServer allows
 * authenticating Karabo users based on their (userId, password) credentials
 * and authorizing authenticated users on a given topic.
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

#ifndef KARABO_NET_USERAUTHCLIENT_HH
#define KARABO_NET_USERAUTHCLIENT_HH

#include <functional>
#include <string>

#include "karabo/net/HttpClient.hh"
#include "karabo/util/Schema.hh"

namespace karabo {
    namespace net {

        /**
         * @brief The results of a one-time token validation / authorization.
         *
         */
        struct OneTimeTokenAuthorizeResult {
            // Has the token been validated?
            bool success;
            // The user associated to the valid token - blank if token is invalid or an error occurred.
            std::string userId;
            // The access level in the topic for the user linked to the token
            karabo::util::Schema::AccessLevel accessLevel;
            // An error description for a failed token validation.
            std::string errMsg;
        };

        using AuthOneTimeTokenHandler = std::function<void(const OneTimeTokenAuthorizeResult&)>;

        class UserAuthClient {
           public:
            explicit UserAuthClient(const std::string& authServerUrl);

            /**
             * @brief Validate and authorize, asynchronously, a given one-time token against a given topic.
             *
             * @param token the token to be validated and authorized.
             * @param topic the topic against which the user linked to a valid token will be authorized.
             * @param handler the handler to be called when the token is processed.
             */
            void authorizeOneTimeToken(const std::string& token, const std::string& topic,
                                       const AuthOneTimeTokenHandler& handler);

           private:
            karabo::net::HttpClient m_cli;
        };
    } // namespace net
} // namespace karabo

#endif
