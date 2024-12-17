/*
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
/*
 * File:   ParseUrl_Test.cc
 * Author: parenti
 *
 * Created on Januar  7, 2022,  4:00 PM
 */

#include "ParseUrl_Test.hh"

#include <karabo/net/utils.hh>
#include <tuple>

using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(ParseUrl_Test);


ParseUrl_Test::ParseUrl_Test() {}


ParseUrl_Test::~ParseUrl_Test() {}


void ParseUrl_Test::setUp() {}


void ParseUrl_Test::tearDown() {}


void ParseUrl_Test::testGenericParse() {
    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("invalid-url");
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("invalid-url:");
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl(":invalid-url");
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("mailto:john.smith@example.com");
        CPPUNIT_ASSERT_EQUAL(std::string("mailto"), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("john.smith@example.com"), std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("mac://0A:0B:0C:0D:10:11");
        CPPUNIT_ASSERT_EQUAL(std::string("mac"), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("0A:0B:0C:0D:10:11"), std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("sn://s123456");
        CPPUNIT_ASSERT_EQUAL(std::string("sn"), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("s123456"), std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("file:///tmp/file.txt");
        CPPUNIT_ASSERT_EQUAL(std::string("file"), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("/tmp/file.txt"), std::get<1>(parsed_url));
    }
}


void ParseUrl_Test::testHttpParse() {
    {
        const std::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("tcp://host1:1234");
        CPPUNIT_ASSERT_EQUAL(std::string("tcp"), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("host1"), std::get<1>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("1234"), std::get<2>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<3>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<4>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("socket://host2/path1");
        CPPUNIT_ASSERT_EQUAL(std::string("socket"), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("host2"), std::get<1>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<2>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("/path1"), std::get<3>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<4>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("http://host3:2345/path2");
        CPPUNIT_ASSERT_EQUAL(std::string("http"), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("host3"), std::get<1>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("2345"), std::get<2>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("/path2"), std::get<3>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string(""), std::get<4>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("https://host4:3456/path3?some-query");
        CPPUNIT_ASSERT_EQUAL(std::string("https"), std::get<0>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("host4"), std::get<1>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("3456"), std::get<2>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("/path3"), std::get<3>(parsed_url));
        CPPUNIT_ASSERT_EQUAL(std::string("some-query"), std::get<4>(parsed_url));
    }
}
