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
 * File:   ParseUrl_Test.hh
 * Author: parenti
 *
 * Created on January  7, 2022,  4:00 PM
 */

#include <gtest/gtest.h>

#include <tuple>

#include "karabo/net/utils.hh"


using namespace karabo::net;


TEST(TestParseUrl, testGenericParse) {
    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("invalid-url");
        EXPECT_EQ("", std::get<0>(parsed_url));
        EXPECT_EQ("", std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("invalid-url:");
        EXPECT_EQ("", std::get<0>(parsed_url));
        EXPECT_EQ("", std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl(":invalid-url");
        EXPECT_EQ("", std::get<0>(parsed_url));
        EXPECT_EQ("", std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("mailto:john.smith@example.com");
        EXPECT_EQ("mailto", std::get<0>(parsed_url));
        EXPECT_EQ("john.smith@example.com", std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("mac://0A:0B:0C:0D:10:11");
        EXPECT_EQ("mac", std::get<0>(parsed_url));
        EXPECT_EQ("0A:0B:0C:0D:10:11", std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("sn://s123456");
        EXPECT_EQ("sn", std::get<0>(parsed_url));
        EXPECT_EQ("s123456", std::get<1>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string> parsed_url = parseGenericUrl("file:///tmp/file.txt");
        EXPECT_EQ("file", std::get<0>(parsed_url));
        EXPECT_EQ("/tmp/file.txt", std::get<1>(parsed_url));
    }
}


TEST(TestParseUrl, testHttpParse) {
    {
        const std::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("tcp://host1:1234");
        EXPECT_EQ("tcp", std::get<0>(parsed_url));
        EXPECT_EQ("host1", std::get<1>(parsed_url));
        EXPECT_EQ("1234", std::get<2>(parsed_url));
        EXPECT_EQ("", std::get<3>(parsed_url));
        EXPECT_EQ("", std::get<4>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("socket://host2/path1");
        EXPECT_EQ("socket", std::get<0>(parsed_url));
        EXPECT_EQ("host2", std::get<1>(parsed_url));
        EXPECT_EQ("", std::get<2>(parsed_url));
        EXPECT_EQ("/path1", std::get<3>(parsed_url));
        EXPECT_EQ("", std::get<4>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("http://host3:2345/path2");
        EXPECT_EQ("http", std::get<0>(parsed_url));
        EXPECT_EQ("host3", std::get<1>(parsed_url));
        EXPECT_EQ("2345", std::get<2>(parsed_url));
        EXPECT_EQ("/path2", std::get<3>(parsed_url));
        EXPECT_EQ("", std::get<4>(parsed_url));
    }

    {
        const std::tuple<std::string, std::string, std::string, std::string, std::string> parsed_url =
              parseUrl("https://host4:3456/path3?some-query");
        EXPECT_EQ("https", std::get<0>(parsed_url));
        EXPECT_EQ("host4", std::get<1>(parsed_url));
        EXPECT_EQ("3456", std::get<2>(parsed_url));
        EXPECT_EQ("/path3", std::get<3>(parsed_url));
        EXPECT_EQ("some-query", std::get<4>(parsed_url));
    }
}
