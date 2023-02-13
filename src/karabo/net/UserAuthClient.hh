/*
 * UserAuthClient.hh
 *
 * Client for the KaraboAuthServer web api. The KaraboAuthServer allows
 * authenticating Karabo users based on their (userId, password) credentials
 * and authorizing authenticated users on a given topic.
 *
 * Created on August, 11, 2022.
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_NET_USERAUTHCLIENT_HH
#define KARABO_NET_USERAUTHCLIENT_HH

#include <boost/function.hpp>
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

        using AuthOneTimeTokenHandler = boost::function<void(const OneTimeTokenAuthorizeResult&)>;

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