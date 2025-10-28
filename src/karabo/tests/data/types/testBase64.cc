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
 * File:   Base64_Test.cc
 * Author: parenti
 *
 * Created on October 2, 2013, 4:18 PM
 */

#include <gtest/gtest.h>

#include "karabo/data/types/Base64.hh"

using namespace karabo::data;
using namespace std;


TEST(TestBase64, testEncode) {
    const unsigned char in1[] = "1234567890";
    EXPECT_EQ(base64Encode(in1, 10), "MTIzNDU2Nzg5MA==");
    EXPECT_EQ(base64Encode(in1, 9), "MTIzNDU2Nzg5");
    EXPECT_EQ(base64Encode(in1, 8), "MTIzNDU2Nzg=");

    const unsigned char in2[] = "abcdefghijklmnopqrstuvxwyz";
    EXPECT_EQ(base64Encode(in2, 26), "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnh3eXo=");

    const unsigned char in3[] = "ABCDEFGHIJKLMNOPQRSTUVXWYZ";
    EXPECT_EQ(base64Encode(in3, 26), "QUJDREVGR0hJSktMTU5PUFFSU1RVVlhXWVo=");
}


TEST(TestBase64, testDecode) {
    std::vector<unsigned char> out;

    out.clear();
    base64Decode("MTIzNDU2Nzg5MA==", out);
    EXPECT_EQ(memcmp(out.data(), "1234567890", out.size()), 0);
    EXPECT_EQ(out.size(), 10);

    out.clear();
    base64Decode("MTIzNDU2Nzg5", out);
    EXPECT_EQ(memcmp(out.data(), "123456789", out.size()), 0);
    EXPECT_EQ(out.size(), 9);

    out.clear();
    base64Decode("MTIzNDU2Nzg=", out);
    EXPECT_EQ(memcmp(out.data(), "12345678", out.size()), 0);
    EXPECT_EQ(out.size(), 8);

    out.clear();
    base64Decode("YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnh3eXo=", out);
    EXPECT_EQ(memcmp(out.data(), "abcdefghijklmnopqrstuvxwyz", out.size()), 0);
    EXPECT_EQ(out.size(), 26);

    out.clear();
    base64Decode("QUJDREVGR0hJSktMTU5PUFFSU1RVVlhXWVo=", out);
    EXPECT_EQ(memcmp(out.data(), "ABCDEFGHIJKLMNOPQRSTUVXWYZ", out.size()), 0);
    EXPECT_EQ(out.size(), 26);
}
