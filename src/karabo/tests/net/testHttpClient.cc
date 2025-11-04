/*
 * HttpClient_Test.cc
 *
 * Unit tests for the HttpClient class.
 * Complements the tests in the integration test GuiServerDevice_Test, which
 * issues a POST request for an HTTP server over a plain-text connection.
 *
 * Created on August, 21, 2023.
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

#include <gtest/gtest.h>

#include "karabo/net/HttpClient.hh"

#define TEST_URL "http://www.xfel.eu"
#define TEST_URL_SSL "https://www.xfel.eu"

namespace http = boost::beast::http;
using namespace karabo::net;

TEST(TestHttpClient, testHttpsGet) {
    // TODO: Change the second argument passed to the HttpClient (verifyCert) to
    //       true as soon as the openssl dependency of the Framework is updated
    //       to 1.1.1(*). The current openssl version, 1.0.2t, specified in
    //       "extern/conanfile.txt" fails the verification, while tests done with
    //       openssl 1.1.1t were able to perform the certificate verification
    //       successfully.
    HttpClient cli{TEST_URL_SSL, false};

    HttpHeaders reqHeaders;
    reqHeaders.set(HttpHeader::user_agent, "Karabo HttpClient_Test");
    reqHeaders.set(HttpHeader::content_type, "text/html");

    cli.asyncGet("/", reqHeaders, "", [](const http::response<http::string_body>& resp) {
        // The page is retrieved over the secure connection.
        ASSERT_TRUE(resp.result_int() == 200);
        ASSERT_TRUE(resp.body().size() > 0);
        ASSERT_TRUE(resp.base()["Content-Type"] == "text/html; charset=utf-8");
    });
}

TEST(TestHttpClient, testHttpGet) {
    HttpClient cli{TEST_URL};

    HttpHeaders reqHeaders;
    reqHeaders.set(HttpHeader::user_agent, "Karabo HttpClient_Test");
    reqHeaders.set(HttpHeader::content_type, "text/html");

    cli.asyncGet("/", reqHeaders, "", [](const http::response<http::string_body>& resp) {
        // The non-secure version of the site redirects to the secure version.
        ASSERT_TRUE(resp.result_int() == 302);
        ASSERT_TRUE(resp.body().size() == 0);
        ASSERT_TRUE(resp.base()["Location"].starts_with(TEST_URL_SSL));
    });
}
