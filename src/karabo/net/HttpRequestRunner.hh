/*
 * HttpRequestRunner.hh
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

#ifndef KARABO_NET_HTTPREQUESTRUNNER_HH
#define KARABO_NET_HTTPREQUESTRUNNER_HH

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>

#include "HttpCommon.hh"

namespace karabo {
    namespace net {

        class HttpRequestRunner : public std::enable_shared_from_this<HttpRequestRunner> {
           public:
            HttpRequestRunner(boost::asio::io_context& ioc, verb method, int httpVersion);

            void run(const std::string& host, unsigned short port, const std::string& route,
                     const HttpHeaders& reqHeaders, const std::string& reqBody, const HttpResponseHandler& respHandler);

           private:
            resolver m_resolver;
            tcp_stream m_stream;
            verb m_method;
            int m_httpVersion;
            flatBuffer m_buffer;
            getRequest m_getRequest;
            postRequest m_postRequest;
            response m_response;
            HttpResponseHandler m_responseHandler;

            void throwIfUnsupportedMethod();

            void on_resolve(errorCode ec, const results_type& results);

            void on_connect(errorCode ec, const results_type::endpoint_type&);

            void on_write(errorCode ec, std::size_t bytesTransferred);

            void on_read(errorCode ec, std::size_t bytesTransferred);

            void fail(errorCode ec, char const* what);

        }; // HttpRequestRunner

    } // namespace net
} // namespace karabo

#endif
