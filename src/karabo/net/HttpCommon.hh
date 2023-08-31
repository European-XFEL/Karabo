/*
 * HttpCommon.hh
 *
 * Declarations used by multiple files using Boost Beast to handle http(s) requests.
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

#ifndef KARABO_NET_HTTPCOMMON_HH
#define KARABO_NET_HTTPCOMMON_HH

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <functional>

#define HTTP_VERSION 11
#define NET_OP_TIMEOUT_SECS 30

namespace karabo {
    namespace net {
        using errorCode = boost::beast::error_code;
        using flatBuffer = boost::beast::flat_buffer;
        using getRequest = boost::beast::http::request<boost::beast::http::empty_body>;
        using HttpHeader = boost::beast::http::field;
        using HttpHeaders = boost::beast::http::fields;
        using HttpResponseHandler =
              std::function<void(const boost::beast::http::response<boost::beast::http::string_body>&)>;
        using postRequest = boost::beast::http::request<boost::beast::http::string_body>;
        using response = boost::beast::http::response<boost::beast::http::string_body>;
        using resolver = boost::asio::ip::tcp::resolver;
        using results_type = boost::asio::ip::tcp::resolver::results_type;
        using tcp_stream = boost::beast::tcp_stream;
        using ssl_stream = boost::beast::ssl_stream<boost::beast::tcp_stream>;
        using ssl_context = boost::asio::ssl::context;
        using verb = boost::beast::http::verb;
    } // namespace net
} // namespace karabo


#endif
