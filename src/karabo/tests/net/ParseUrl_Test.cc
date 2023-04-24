/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   ParseUrl_Test.cc
 * Author: parenti
 *
 * Created on Januar  7, 2022,  4:00 PM
 */

#include "ParseUrl_Test.hh"

#include <boost/tuple/tuple.hpp>
#include <karabo/net/utils.hh>

using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(ParseUrl_Test);


ParseUrl_Test::ParseUrl_Test() {}


ParseUrl_Test::~ParseUrl_Test() {}


void ParseUrl_Test::setUp() {}


void ParseUrl_Test::tearDown() {}


void ParseUrl_Test::testGenericParse() {
    {
        const boost::tuple<std::string, std::string> parsed_url = parseGenericUrl("invalid-url");
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<1>());
    }

    {
        const boost::tuple<std::string, std::string> parsed_url = parseGenericUrl("invalid-url:");
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<1>());
    }

    {
        const boost::tuple<std::string, std::string> parsed_url = parseGenericUrl(":invalid-url");
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<1>());
    }

    {
        const boost::tuple<std::string, std::string> parsed_url = parseGenericUrl("mailto:john.smith@example.com");
        CPPUNIT_ASSERT_EQUAL(std::string("mailto"), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string("john.smith@example.com"), parsed_url.get<1>());
    }

    {
        const boost::tuple<std::string, std::string> parsed_url = parseGenericUrl("mac://0A:0B:0C:0D:10:11");
        CPPUNIT_ASSERT_EQUAL(std::string("mac"), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string("0A:0B:0C:0D:10:11"), parsed_url.get<1>());
    }

    {
        const boost::tuple<std::string, std::string> parsed_url = parseGenericUrl("sn://s123456");
        CPPUNIT_ASSERT_EQUAL(std::string("sn"), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string("s123456"), parsed_url.get<1>());
    }

    {
        const boost::tuple<std::string, std::string> parsed_url = parseGenericUrl("file:///tmp/file.txt");
        CPPUNIT_ASSERT_EQUAL(std::string("file"), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string("/tmp/file.txt"), parsed_url.get<1>());
    }
}


void ParseUrl_Test::testHttpParse() {
    {
        const boost::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("tcp://host1:1234");
        CPPUNIT_ASSERT_EQUAL(std::string("tcp"), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string("host1"), parsed_url.get<1>());
        CPPUNIT_ASSERT_EQUAL(std::string("1234"), parsed_url.get<2>());
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<3>());
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<4>());
    }

    {
        const boost::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("socket://host2/path1");
        CPPUNIT_ASSERT_EQUAL(std::string("socket"), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string("host2"), parsed_url.get<1>());
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<2>());
        CPPUNIT_ASSERT_EQUAL(std::string("/path1"), parsed_url.get<3>());
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<4>());
    }

    {
        const boost::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("http://host3:2345/path2");
        CPPUNIT_ASSERT_EQUAL(std::string("http"), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string("host3"), parsed_url.get<1>());
        CPPUNIT_ASSERT_EQUAL(std::string("2345"), parsed_url.get<2>());
        CPPUNIT_ASSERT_EQUAL(std::string("/path2"), parsed_url.get<3>());
        CPPUNIT_ASSERT_EQUAL(std::string(""), parsed_url.get<4>());
    }

    {
        const boost::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("https://host4:3456/path3?some-query");
        CPPUNIT_ASSERT_EQUAL(std::string("https"), parsed_url.get<0>());
        CPPUNIT_ASSERT_EQUAL(std::string("host4"), parsed_url.get<1>());
        CPPUNIT_ASSERT_EQUAL(std::string("3456"), parsed_url.get<2>());
        CPPUNIT_ASSERT_EQUAL(std::string("/path3"), parsed_url.get<3>());
        CPPUNIT_ASSERT_EQUAL(std::string("some-query"), parsed_url.get<4>());
    }
}
