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
 * File:   StringTools_Test.cc
 * Author: parenti
 *
 * Created on December 15, 2014, 12:16 PM
 */

#include "StringTools_Test.hh"

#include <boost/core/null_deleter.hpp>
#include <set>
#include <unordered_set>

#include "karabo/data/types/StringTools.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(StringTools_Test);

using namespace karabo::data;
using namespace std;


StringTools_Test::StringTools_Test() {}


StringTools_Test::~StringTools_Test() {}


void StringTools_Test::setUp() {}


void StringTools_Test::tearDown() {}


void StringTools_Test::testFromString() {
    // Unsigned integer
    CPPUNIT_ASSERT((unsigned char)234 == fromString<unsigned char>("234"));
    CPPUNIT_ASSERT((unsigned short)34567 == fromString<unsigned short>("34567"));
    CPPUNIT_ASSERT((unsigned int)2345678901 == fromString<unsigned int>("2345678901"));
    CPPUNIT_ASSERT((unsigned long long)123456789012 == fromString<unsigned long long>("123456789012"));
    CPPUNIT_ASSERT((unsigned int)1 == fromString<unsigned int>("1.23"));
    CPPUNIT_ASSERT((unsigned int)0 == fromString<unsigned int>("1.23e-10"));
    CPPUNIT_ASSERT((unsigned int)123 == fromString<unsigned int>("1.23e2"));

    // Signed integer
    CPPUNIT_ASSERT((signed char)123 == fromString<signed char>("123"));
    CPPUNIT_ASSERT((short)23456 == fromString<short>("23456"));
    CPPUNIT_ASSERT((int)1234567890 == fromString<int>("1234567890"));
    CPPUNIT_ASSERT((int)1 == fromString<int>("1.23"));
    CPPUNIT_ASSERT((int)0 == fromString<int>("1.23e-10"));
    CPPUNIT_ASSERT((long long)123456789012 == fromString<long long>("123456789012"));
    CPPUNIT_ASSERT((unsigned long long)1 == fromString<unsigned long long>("1.23"));
    CPPUNIT_ASSERT((unsigned long long)0 == fromString<unsigned long long>("1.23e-10"));
    CPPUNIT_ASSERT((unsigned long long)123 == fromString<unsigned long long>("1.23e2"));

    // Boolean
    CPPUNIT_ASSERT(false == fromString<bool>("false"));
    CPPUNIT_ASSERT(false == fromString<bool>("no"));
    CPPUNIT_ASSERT(false == fromString<bool>("n"));
    CPPUNIT_ASSERT(false == fromString<bool>("0"));

    CPPUNIT_ASSERT(true == fromString<bool>("true"));
    CPPUNIT_ASSERT(true == fromString<bool>("yes"));
    CPPUNIT_ASSERT(true == fromString<bool>("y"));
    CPPUNIT_ASSERT(true == fromString<bool>("1"));

    // Floating point
    CPPUNIT_ASSERT((float)1.234567 == fromString<float>("1.234567"));
    CPPUNIT_ASSERT((double)1.23456789012345 == fromString<double>("1.23456789012345"));

    // Complex
    CPPUNIT_ASSERT(std::complex<float>(1.234, 5.678) == fromString<std::complex<float> >("(1.234,5.678)"));
    CPPUNIT_ASSERT(std::complex<double>(1.234, 5.678) == fromString<std::complex<double> >("(1.234,5.678)"));

    // Test from vector
    // Vector bool is always special
    {
        std::vector<bool> aVector = fromString<bool, std::vector>("true", ",");
        CPPUNIT_ASSERT_EQUAL(1ul, aVector.size());
        CPPUNIT_ASSERT(true == aVector[0]);

        aVector = fromString<bool, std::vector>("", ",");
        CPPUNIT_ASSERT_EQUAL(0ul, aVector.size());

        aVector = fromString<bool, std::vector>("y,0,false,1", ",");
        CPPUNIT_ASSERT_EQUAL(4ul, aVector.size());
        CPPUNIT_ASSERT(true == aVector[0]);
        CPPUNIT_ASSERT(false == aVector[1]);
        CPPUNIT_ASSERT(false == aVector[2]);
        CPPUNIT_ASSERT(true == aVector[3]);
    }
    // Vector short is using the templated version
    {
        std::vector<short> aVector = fromString<short, std::vector>("77", ",");
        CPPUNIT_ASSERT_EQUAL(1ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(static_cast<short>(77), aVector[0]);

        aVector = fromString<short, std::vector>("", ",");
        CPPUNIT_ASSERT_EQUAL(0ul, aVector.size());

        aVector = fromString<short, std::vector>("-32768, -77, 32767", ",");
        CPPUNIT_ASSERT_EQUAL(3ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(static_cast<short>(-32768), aVector[0]);
        CPPUNIT_ASSERT_EQUAL(static_cast<short>(-77), aVector[1]);
        CPPUNIT_ASSERT_EQUAL(static_cast<short>(32767), aVector[2]);
    }

    // Vector unsigned char has a special implementation for backward compatibility
    // (base64 encoding applied before Karabo 3)
    {
        std::vector<unsigned char> aVector = fromString<unsigned char, std::vector>("13", ",");
        CPPUNIT_ASSERT_EQUAL(1ul, aVector.size());
        CPPUNIT_ASSERT(13u == aVector[0]);

        // Corner case: empty string and thus vector
        aVector = fromString<unsigned char, std::vector>("", ",");
        CPPUNIT_ASSERT_EQUAL(0ul, aVector.size());

        // Corner case: longest string without separator
        aVector = fromString<unsigned char, std::vector>("222", ",");
        CPPUNIT_ASSERT_EQUAL(1ul, aVector.size());
        CPPUNIT_ASSERT(222u == aVector[0]);

        // Corner case: longest string with a separator
        aVector = fromString<unsigned char, std::vector>("1,8", ",");
        CPPUNIT_ASSERT_EQUAL(2ul, aVector.size());
        CPPUNIT_ASSERT(1u == aVector[0]);
        CPPUNIT_ASSERT(8u == aVector[1]);

        // All values smaller than max signed (!) char
        CPPUNIT_ASSERT_NO_THROW((aVector = fromString<unsigned char, std::vector>("1, 127", ",")));
        CPPUNIT_ASSERT_EQUAL(2ul, aVector.size());
        CPPUNIT_ASSERT(1u == aVector[0]);
        CPPUNIT_ASSERT(127u == aVector[1]);

        // Now also bigger ones
        CPPUNIT_ASSERT_NO_THROW((aVector = fromString<unsigned char, std::vector>("0, 200, 255", ",")));
        CPPUNIT_ASSERT_EQUAL(3ul, aVector.size());
        CPPUNIT_ASSERT(0u == aVector[0]);
        CPPUNIT_ASSERT(200u == aVector[1]);
        CPPUNIT_ASSERT(255u == aVector[2]);

        // An old, base64 encoded string (from before Karabo 3)
        aVector = fromString<unsigned char, std::vector>("KSorLC0u", ",");
        CPPUNIT_ASSERT_EQUAL(6ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(static_cast<unsigned char>(41), aVector[0]);
        CPPUNIT_ASSERT_EQUAL(static_cast<unsigned char>(42), aVector[1]);
        CPPUNIT_ASSERT_EQUAL(static_cast<unsigned char>(43), aVector[2]);
        CPPUNIT_ASSERT_EQUAL(static_cast<unsigned char>(44), aVector[3]);
        CPPUNIT_ASSERT_EQUAL(static_cast<unsigned char>(45), aVector[4]);
        CPPUNIT_ASSERT_EQUAL(static_cast<unsigned char>(46), aVector[5]);
    }
    // Vector int has a template specialisation
    {
        std::vector<int> aVector = fromString<int, std::vector>("77", ",");
        CPPUNIT_ASSERT_EQUAL(1ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(77, aVector[0]);

        aVector = fromString<int, std::vector>("", ",");
        CPPUNIT_ASSERT_EQUAL(0ul, aVector.size());

        aVector = fromString<int, std::vector>("-32768, -77, 32767", ",");
        CPPUNIT_ASSERT_EQUAL(3ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(-32768, aVector[0]);
        CPPUNIT_ASSERT_EQUAL(-77, aVector[1]);
        CPPUNIT_ASSERT_EQUAL(32767, aVector[2]);
    }

    // Vector unsigned int has another template specialisation
    {
        std::vector<unsigned int> aVector = fromString<unsigned int, std::vector>("77", ",");
        CPPUNIT_ASSERT_EQUAL(1ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(77u, aVector[0]);

        aVector = fromString<unsigned int, std::vector>("", ",");
        CPPUNIT_ASSERT_EQUAL(0ul, aVector.size());

        aVector = fromString<unsigned int, std::vector>(" [0, 77, 65535] ", ",");
        CPPUNIT_ASSERT_EQUAL(3ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(0u, aVector[0]);
        CPPUNIT_ASSERT_EQUAL(77u, aVector[1]);
        CPPUNIT_ASSERT_EQUAL(65535u, aVector[2]);
    }

    // Vector long long has yet another template specialisation
    {
        std::vector<long long> aVector = fromString<long long, std::vector>(" 77 ", ",");
        CPPUNIT_ASSERT_EQUAL(1ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(77ll, aVector[0]);

        aVector = fromString<long long, std::vector>("", ",");
        CPPUNIT_ASSERT_EQUAL(0ul, aVector.size());

        aVector = fromString<long long, std::vector>(" -2147483648, -77 , 2147483647", ",");
        CPPUNIT_ASSERT_EQUAL(3ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(-2147483648ll, aVector[0]);
        CPPUNIT_ASSERT_EQUAL(-77ll, aVector[1]);
        CPPUNIT_ASSERT_EQUAL(2147483647ll, aVector[2]);
    }

    // Vector unsigned long long has yet another template specialisation
    {
        std::vector<unsigned long long> aVector = fromString<unsigned long long, std::vector>("77", ",");
        CPPUNIT_ASSERT_EQUAL(1ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(77ull, aVector[0]);

        aVector = fromString<unsigned long long, std::vector>("", ",");
        CPPUNIT_ASSERT_EQUAL(0ul, aVector.size());

        aVector = fromString<unsigned long long, std::vector>(" 0, 77, 4294967295      ", ",");
        CPPUNIT_ASSERT_EQUAL(3ul, aVector.size());
        CPPUNIT_ASSERT_EQUAL(0ull, aVector[0]);
        CPPUNIT_ASSERT_EQUAL(77ull, aVector[1]);
        CPPUNIT_ASSERT_EQUAL(4294967295ull, aVector[2]);
    }

    // tests for set
    {
        auto aSet = fromStringToSortedCont<int, std::set>("77", ",");
        CPPUNIT_ASSERT_EQUAL(1ul, aSet.size());
        CPPUNIT_ASSERT_EQUAL(77, *(aSet.begin()));

        aSet = fromStringToSortedCont<int, std::set>("", ",");
        CPPUNIT_ASSERT_EQUAL(0ul, aSet.size());

        aSet = fromStringToSortedCont<int, std::set>("-32768, -77, 32767, -77", ",");
        CPPUNIT_ASSERT_EQUAL(3ul, aSet.size());
        auto it = aSet.begin();
        CPPUNIT_ASSERT_EQUAL(-32768, *it);
        ++it;
        CPPUNIT_ASSERT_EQUAL(-77, *it);
        ++it;
        CPPUNIT_ASSERT_EQUAL(32767, *it);
    }
}


void StringTools_Test::testToString() {
    // Unsigned int types
    CPPUNIT_ASSERT(toString((unsigned char)234) == "234");
    CPPUNIT_ASSERT(toString((unsigned short)34567) == "34567");
    CPPUNIT_ASSERT(toString((unsigned int)2345678901) == "2345678901");
    CPPUNIT_ASSERT(toString((unsigned long long)123456789012) == "123456789012");

    // Signed int types
    CPPUNIT_ASSERT(toString((signed char)123) == "123");
    CPPUNIT_ASSERT(toString((short)23456) == "23456");
    CPPUNIT_ASSERT(toString((int)1234567890) == "1234567890");
    CPPUNIT_ASSERT(toString((long long)123456789012) == "123456789012");

    // Boolean
    CPPUNIT_ASSERT(toString(false) == "0");
    CPPUNIT_ASSERT(toString(true) == "1");

    // Floating point types
    CPPUNIT_ASSERT(toString(1.2345678f) == "1.234568");
    CPPUNIT_ASSERT(toString(0.1f) == "0.1");
    CPPUNIT_ASSERT(toString(-0.1f) == "-0.1");
    CPPUNIT_ASSERT(toString(0.000000099999987f) == "9.999999e-08");
    CPPUNIT_ASSERT(toString(-99999987198.0f) == "-9.999999e+10");
    CPPUNIT_ASSERT(toString(3333332187236.0f) == "3.333332e+12");

    CPPUNIT_ASSERT(toString(1.234567890123456) == "1.23456789012346");
    CPPUNIT_ASSERT(toString(0.123456789012345) == "0.123456789012345");
    CPPUNIT_ASSERT(toString(0.01234567890123456) == "0.0123456789012346");

    // Border(?) cases ...
    CPPUNIT_ASSERT(toString(0.0001f) == "0.0001");
    CPPUNIT_ASSERT(toString(0.00001f) == "1e-05");
    CPPUNIT_ASSERT(toString(1000000.f) == "1000000");
    CPPUNIT_ASSERT(toString(10000000.f) == "1e+07");

    // Complex types
    CPPUNIT_ASSERT(toString(std::complex<float>(1.2345678f, 1.3456789f)) == "(1.234568,1.345679)");
    CPPUNIT_ASSERT(toString(std::complex<float>(123456700000.0f, 0.00000000013456780f)) ==
                   "(1.234567e+11,1.345678e-10)");
    CPPUNIT_ASSERT(toString(std::complex<double>(1.234567890123456, 6.543210987654321)) ==
                   "(1.23456789012346,6.54321098765432)");

    // Vectors
    std::vector<unsigned int> uint32Vector(4, 12345);
    CPPUNIT_ASSERT(toString(uint32Vector) == "12345,12345,12345,12345");
    std::vector<float> floatVector{1.f,         0.5f,       3.1415956f,  0.1f,
                                   0.09999878f, 2.8790123f, 99.8765411f, -0.00000000003456789f};
    CPPUNIT_ASSERT(toString(floatVector) == "1,0.5,3.141596,0.1,0.09999878,2.879012,99.87654,-3.456789e-11");

    // Vectors, but playing with maximum number
    const std::vector<int> int32Vector{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    CPPUNIT_ASSERT_EQUAL(std::string("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15"), toString(int32Vector)); // all is default
    // Ask for less - but at least first and last are shown and on each end up to 5 less than half maximum
    CPPUNIT_ASSERT_EQUAL(std::string("1,...(skip 13 values)...,15"), toString(int32Vector, 1));
    // From 14 on more than just first/last are shown
    CPPUNIT_ASSERT_EQUAL(std::string("1,2,...(skip 11 values)...,14,15"), toString(int32Vector, 14));
    // Check if vector size hits maximum, all elements are indeed treated
    CPPUNIT_ASSERT_EQUAL(std::string("2,3,4,5,6,7,8,9,10,11,12,13,14,15"),
                         toString(std::vector<int>(int32Vector.begin() + 1, int32Vector.end()), 14));

    // If some elements are skipped, ten (five on each end) less than the maximum are taken into account:
    const std::vector<long long> int64Vector(30, 12345);
    std::string only18_12345;
    for (size_t i = 0; i < int64Vector.size(); ++i) {
        if (i == 9) {
            only18_12345 += "...(skip 12 values)...,";
            i += 11;
        } else if (i == int64Vector.size() - 1) {
            only18_12345 += "12345";
        } else {
            only18_12345 += "12345,";
        }
    }
    CPPUNIT_ASSERT_EQUAL(only18_12345, toString(int64Vector, 28));

    // Sets - they are ordered
    CPPUNIT_ASSERT_EQUAL(std::string("2,3,4,5"), toString(std::set<unsigned int>({5, 4, 2, 3})));

    // Unordered_sets - they have any order, but all should be in
    const std::unordered_set<int> int32_unordered_set({5, 4, 2, 3});
    const std::string int32_unordered_set_asstring(toString(int32_unordered_set));
    auto vec = fromString<int, std::vector>(int32_unordered_set_asstring);
    // Check that all in created vector are all in original unordered_set
    CPPUNIT_ASSERT_EQUAL(vec.size(), int32_unordered_set.size());
    for (int i : int32_unordered_set) {
        CPPUNIT_ASSERT(std::find(vec.begin(), vec.end(), i) != vec.end());
    }

    // Wide strings
    const wchar_t wstr[] = L"abcd0123";
    string str = "abcd0123";
    CPPUNIT_ASSERT(toString(std::wstring(L"abcd0123")) == str);
    CPPUNIT_ASSERT(toString(L"abcd0123") == "abcd0123");
    CPPUNIT_ASSERT(toString(wstr) == "abcd0123");

    // ByteArray
    static char s[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11,
                       0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23};
    ByteArray ba = std::make_pair(std::shared_ptr<char>(s, [](char*) {}), 24);
    CPPUNIT_ASSERT(toString(ba, 16) == "0x0001020304050607...(skip 8 bytes)...1617181920212223");
    CPPUNIT_ASSERT(toString(ba, 24) == "0x000102030405060708091011121314151617181920212223");
    CPPUNIT_ASSERT(toString(ba, 4) == "0x0001...(skip 20 bytes)...2223");
}


void StringTools_Test::testWiden() {
    Widen<wchar_t> toWString;
    CPPUNIT_ASSERT(toWString("abcd0123") == L"abcd0123");
}


void StringTools_Test::testTokenize() {
    std::vector<string> out;

    // Test 1, 2, 3 single character tokens
    tokenize("a", out, '.');
    CPPUNIT_ASSERT_EQUAL(1ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string("a"), out[0]);

    tokenize("b.c", out, '.');
    CPPUNIT_ASSERT_EQUAL(2ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string("b"), out[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("c"), out[1]);

    tokenize("d.e.f", out, '.');
    CPPUNIT_ASSERT_EQUAL(3ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string("d"), out[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("e"), out[1]);
    CPPUNIT_ASSERT_EQUAL(std::string("f"), out[2]);

    // Test few more multi-character tokens
    tokenize("Aa.buu.c.undNochWa][", out, '.');
    CPPUNIT_ASSERT_EQUAL(4ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string("Aa"), out[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("buu"), out[1]);
    CPPUNIT_ASSERT_EQUAL(std::string("c"), out[2]);
    CPPUNIT_ASSERT_EQUAL(std::string("undNochWa]["), out[3]);

    // Test empty token in beginning, middle and end
    tokenize(".a", out, '.');
    CPPUNIT_ASSERT_EQUAL(2ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string(), out[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("a"), out[1]);

    tokenize("a..z", out, '.');
    CPPUNIT_ASSERT_EQUAL(3ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string("a"), out[0]);
    CPPUNIT_ASSERT_EQUAL(std::string(), out[1]);
    CPPUNIT_ASSERT_EQUAL(std::string("z"), out[2]);

    tokenize("a.", out, '.');
    CPPUNIT_ASSERT_EQUAL(2ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string("a"), out[0]);
    CPPUNIT_ASSERT_EQUAL(std::string(), out[1]);

    // Test up to three empty tokens in a row
    tokenize("", out, '.');
    CPPUNIT_ASSERT_EQUAL(1ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string(), out[0]);

    tokenize(".", out, '.');
    CPPUNIT_ASSERT_EQUAL(2ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string(), out[0]);
    CPPUNIT_ASSERT_EQUAL(std::string(), out[1]);

    tokenize("..", out, '.');
    CPPUNIT_ASSERT_EQUAL(3ul, out.size());
    CPPUNIT_ASSERT_EQUAL(std::string(), out[0]);
    CPPUNIT_ASSERT_EQUAL(std::string(), out[1]);
    CPPUNIT_ASSERT_EQUAL(std::string(), out[2]);

    std::string str1 = "\n\r\t AbRa - kaDaBRa\r\t\n";
    trim(str1);
    CPPUNIT_ASSERT_EQUAL(str1, std::string("AbRa - kaDaBRa"));

    toLower(str1);
    CPPUNIT_ASSERT_EQUAL(str1, std::string("abra - kadabra"));

    toUpper(str1);
    CPPUNIT_ASSERT_EQUAL(str1, std::string("ABRA - KADABRA"));

    // Split tests
    auto v1 = split(str1);
    CPPUNIT_ASSERT(v1.size() == 3);
    CPPUNIT_ASSERT_EQUAL(v1[0], std::string("ABRA"));
    CPPUNIT_ASSERT_EQUAL(v1[1], std::string("-"));
    CPPUNIT_ASSERT_EQUAL(v1[2], std::string("KADABRA"));

    std::string str2 = "HH:MM:SS";
    auto v2 = split(str2, ":");
    CPPUNIT_ASSERT(v2.size() == 3);
    CPPUNIT_ASSERT_EQUAL(v2[0], std::string("HH"));
    CPPUNIT_ASSERT_EQUAL(v2[1], std::string("MM"));
    CPPUNIT_ASSERT_EQUAL(v2[2], std::string("SS"));

    std::string str3 = "2024-10-25T12:32:44.035 [debug] abrakadabra : Body message that can be quite long... ";
    auto v3 = split(str3, " ", 5);
    CPPUNIT_ASSERT(v3.size() == 5);
    CPPUNIT_ASSERT_EQUAL(v3[0], std::string("2024-10-25T12:32:44.035"));
    CPPUNIT_ASSERT_EQUAL(v3[1], std::string("[debug]"));
    CPPUNIT_ASSERT_EQUAL(v3[2], std::string("abrakadabra"));
    CPPUNIT_ASSERT_EQUAL(v3[3], std::string(":"));
    CPPUNIT_ASSERT_EQUAL(v3[4], std::string("Body message that can be quite long... "));
}
