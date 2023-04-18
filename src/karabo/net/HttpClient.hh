/*
 * HttpClient.hh
 *
 * An HttpClient supporting simple GET and POST assynchronous requests over
 * TCP or SSL.
 *
 * Note: this is actually an interface class, that internally uses a wrapping
 * library for Boost Beast as its implementation. The goal is to be able to
 * replace the underlying Boost Beast wrapping library without affecting users
 * of the HttpClient class.
 *
 * Created on February, 09, 2023.
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_NET_HTTPCLIENT_HH
#define KARABO_NET_HTTPCLIENT_HH

#include <boost/beast/http.hpp>
#include <functional>
#include <memory>
#include <string>

namespace karabo {
    namespace net {

        namespace http = boost::beast::http;

        using HttpHeader = boost::beast::http::field;
        using HttpHeaders = boost::beast::http::fields;

        using ResponseHandler = std::function<void(const http::response<http::string_body>)>;

        class HttpClient {
           public:
            explicit HttpClient(const std::string& baseURL);

            // The destructor has to be declared even if it is the default
            // to guarantee that the implementation class is fully defined
            // by the time the destructor is defined.
            // The destructor is defined in "HttpClient.cc" after the definition of
            // the "Impl" implementation class. More details about this at:
            // https://stackoverflow.com/questions/34072862/why-is-error-invalid-application-of-sizeof-to-an-incomplete-type-using-uniqu
            ~HttpClient();

            void asyncPost(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                           const ResponseHandler& respHandler);
            void asyncGet(const std::string& route, const HttpHeaders& reqHeaders, const std::string& reqBody,
                          const ResponseHandler& respHandler);

           private:
            class Impl; // forward declaration of the implementation class
            std::unique_ptr<Impl> m_impl;
        };

    } // namespace net
} // namespace karabo


#endif