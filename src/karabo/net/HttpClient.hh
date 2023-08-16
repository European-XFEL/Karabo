/*
 * HttpClient.hh
 *
 * An HttpClient supporting simple GET and POST assynchronous requests over
 * secure and plain connections.
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

#ifndef KARABO_NET_HTTPCLIENT_HH
#define KARABO_NET_HTTPCLIENT_HH

#include <functional>
#include <memory>
#include <string>

#include "karabo/net/HttpCommon.hh"

namespace karabo {
    namespace net {

        class HttpClient {
           public:
            /**
             * @brief Creates a web client capable of submitting GET and POST requests to a given URL, over a secure or
             * a plain connection.
             *
             * @param baseURL the base URL to be prepended to each request path.
             * @param verifyCerts when set to false, allows self generated server certificates when connecting securely
             * (by bypassing certificate origin verification).
             */
            explicit HttpClient(const std::string& baseURL, bool verifyCerts = false);

            // The destructor has to be declared even if it is the default
            // to guarantee that the implementation class is fully defined
            // by the time the destructor is defined.
            // The destructor is defined in "HttpClient.cc" after the definition of
            // the "Impl" implementation class. More details about this at:
            // https://stackoverflow.com/questions/34072862/why-is-error-invalid-application-of-sizeof-to-an-incomplete-type-using-uniqu
            ~HttpClient();

            void asyncPost(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                           const HttpResponseHandler& respHandler);

            void asyncGet(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                          const HttpResponseHandler& respHandler);

           private:
            class Impl; // forward declaration of the implementation class
            std::unique_ptr<Impl> m_impl;
        };

    } // namespace net
} // namespace karabo


#endif
