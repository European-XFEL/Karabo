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

#include "Base64_Test.hh"

#include <cstring>

#include "karabo/data/types/Base64.hh"

using namespace karabo::data;
using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION(Base64_Test);


Base64_Test::Base64_Test() {}


Base64_Test::~Base64_Test() {}


void Base64_Test::setUp() {}


void Base64_Test::tearDown() {}


void Base64_Test::testEncode() {
    const unsigned char in1[] = "1234567890";
    CPPUNIT_ASSERT(base64Encode(in1, 10) == "MTIzNDU2Nzg5MA==");
    CPPUNIT_ASSERT(base64Encode(in1, 9) == "MTIzNDU2Nzg5");
    CPPUNIT_ASSERT(base64Encode(in1, 8) == "MTIzNDU2Nzg=");

    const unsigned char in2[] = "abcdefghijklmnopqrstuvxwyz";
    CPPUNIT_ASSERT(base64Encode(in2, 26) == "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnh3eXo=");

    const unsigned char in3[] = "ABCDEFGHIJKLMNOPQRSTUVXWYZ";
    CPPUNIT_ASSERT(base64Encode(in3, 26) == "QUJDREVGR0hJSktMTU5PUFFSU1RVVlhXWVo=");
}


void Base64_Test::testDecode() {
    std::vector<unsigned char> out;

    out.clear();
    base64Decode("MTIzNDU2Nzg5MA==", out);
    CPPUNIT_ASSERT(memcmp(out.data(), "1234567890", out.size()) == 0);
    CPPUNIT_ASSERT(out.size() == 10);

    out.clear();
    base64Decode("MTIzNDU2Nzg5", out);
    CPPUNIT_ASSERT(memcmp(out.data(), "123456789", out.size()) == 0);
    CPPUNIT_ASSERT(out.size() == 9);

    out.clear();
    base64Decode("MTIzNDU2Nzg=", out);
    CPPUNIT_ASSERT(memcmp(out.data(), "12345678", out.size()) == 0);
    CPPUNIT_ASSERT(out.size() == 8);

    out.clear();
    base64Decode("YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnh3eXo=", out);
    CPPUNIT_ASSERT(memcmp(out.data(), "abcdefghijklmnopqrstuvxwyz", out.size()) == 0);
    CPPUNIT_ASSERT(out.size() == 26);

    out.clear();
    base64Decode("QUJDREVGR0hJSktMTU5PUFFSU1RVVlhXWVo=", out);
    CPPUNIT_ASSERT(memcmp(out.data(), "ABCDEFGHIJKLMNOPQRSTUVXWYZ", out.size()) == 0);
    CPPUNIT_ASSERT(out.size() == 26);
}
