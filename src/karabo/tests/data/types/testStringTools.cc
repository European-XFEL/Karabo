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

#include <gtest/gtest.h>

#include <boost/core/null_deleter.hpp>
#include <set>
#include <unordered_set>

#include "karabo/data/types/StringTools.hh"

using namespace karabo::data;
using namespace std;
using namespace std::string_view_literals;


TEST(TestStringTools, testFromString) {
    // Unsigned integer
    EXPECT_TRUE((unsigned char)234 == fromString<unsigned char>("234"));
    EXPECT_TRUE((unsigned short)34567 == fromString<unsigned short>("34567"));
    EXPECT_TRUE((unsigned int)2345678901 == fromString<unsigned int>("2345678901"));
    EXPECT_TRUE((unsigned long long)123456789012 == fromString<unsigned long long>("123456789012"));
    EXPECT_TRUE((unsigned int)1 == fromString<unsigned int>("1.23"));
    EXPECT_TRUE((unsigned int)0 == fromString<unsigned int>("1.23e-10"));
    EXPECT_TRUE((unsigned int)123 == fromString<unsigned int>("1.23e2"));

    // Signed integer
    EXPECT_TRUE((signed char)123 == fromString<signed char>("123"));
    EXPECT_TRUE((short)23456 == fromString<short>("23456"));
    EXPECT_TRUE((int)1234567890 == fromString<int>("1234567890"));
    EXPECT_TRUE((int)1 == fromString<int>("1.23"));
    EXPECT_TRUE((int)0 == fromString<int>("1.23e-10"));
    EXPECT_TRUE((long long)123456789012 == fromString<long long>("123456789012"));
    EXPECT_TRUE((unsigned long long)1 == fromString<unsigned long long>("1.23"));
    EXPECT_TRUE((unsigned long long)0 == fromString<unsigned long long>("1.23e-10"));
    EXPECT_TRUE((unsigned long long)123 == fromString<unsigned long long>("1.23e2"));

    // Boolean
    EXPECT_TRUE(false == fromString<bool>("false"));
    EXPECT_TRUE(false == fromString<bool>("no"));
    EXPECT_TRUE(false == fromString<bool>("n"));
    EXPECT_TRUE(false == fromString<bool>("0"));

    EXPECT_TRUE(true == fromString<bool>("true"));
    EXPECT_TRUE(true == fromString<bool>("yes"));
    EXPECT_TRUE(true == fromString<bool>("y"));
    EXPECT_TRUE(true == fromString<bool>("1"));

    // Floating point
    EXPECT_TRUE((float)1.234567 == fromString<float>("1.234567"));
    EXPECT_TRUE((double)1.23456789012345 == fromString<double>("1.23456789012345"));

    // Complex
    EXPECT_TRUE(std::complex<float>(1.234, 5.678) == fromString<std::complex<float> >("(1.234,5.678)"));
    EXPECT_TRUE(std::complex<double>(1.234, 5.678) == fromString<std::complex<double> >("(1.234,5.678)"));

    // Test from vector
    // Vector bool is always special
    {
        std::vector<bool> aVector = fromString<bool, std::vector>("true", ",");
        EXPECT_EQ(1ul, aVector.size());
        EXPECT_TRUE(true == aVector[0]);

        aVector = fromString<bool, std::vector>("", ",");
        EXPECT_EQ(0ul, aVector.size());

        aVector = fromString<bool, std::vector>("y,0,false,1", ",");
        EXPECT_EQ(4ul, aVector.size());
        EXPECT_TRUE(true == aVector[0]);
        EXPECT_TRUE(false == aVector[1]);
        EXPECT_TRUE(false == aVector[2]);
        EXPECT_TRUE(true == aVector[3]);
    }
    // Vector short is using the templated version
    {
        std::vector<short> aVector = fromString<short, std::vector>("77", ",");
        EXPECT_EQ(1ul, aVector.size());
        EXPECT_EQ(static_cast<short>(77), aVector[0]);

        aVector = fromString<short, std::vector>("", ",");
        EXPECT_EQ(0ul, aVector.size());

        aVector = fromString<short, std::vector>("-32768, -77, 32767", ",");
        EXPECT_EQ(3ul, aVector.size());
        EXPECT_EQ(static_cast<short>(-32768), aVector[0]);
        EXPECT_EQ(static_cast<short>(-77), aVector[1]);
        EXPECT_EQ(static_cast<short>(32767), aVector[2]);
    }

    // Vector unsigned char has a special implementation for backward compatibility
    // (base64 encoding applied before Karabo 3)
    {
        std::vector<unsigned char> aVector = fromString<unsigned char, std::vector>("13", ",");
        EXPECT_EQ(1ul, aVector.size());
        EXPECT_TRUE(13u == aVector[0]);

        // Corner case: empty string and thus vector
        aVector = fromString<unsigned char, std::vector>("", ",");
        EXPECT_EQ(0ul, aVector.size());

        // Corner case: longest string without separator
        aVector = fromString<unsigned char, std::vector>("222", ",");
        EXPECT_EQ(1ul, aVector.size());
        EXPECT_TRUE(222u == aVector[0]);

        // Corner case: longest string with a separator
        aVector = fromString<unsigned char, std::vector>("1,8", ",");
        EXPECT_EQ(2ul, aVector.size());
        EXPECT_TRUE(1u == aVector[0]);
        EXPECT_TRUE(8u == aVector[1]);

        // All values smaller than max signed (!) char
        EXPECT_NO_THROW((aVector = fromString<unsigned char, std::vector>("1, 127", ",")));
        EXPECT_EQ(2ul, aVector.size());
        EXPECT_TRUE(1u == aVector[0]);
        EXPECT_TRUE(127u == aVector[1]);

        // Now also bigger ones
        EXPECT_NO_THROW((aVector = fromString<unsigned char, std::vector>("0, 200, 255", ",")));
        EXPECT_EQ(3ul, aVector.size());
        EXPECT_TRUE(0u == aVector[0]);
        EXPECT_TRUE(200u == aVector[1]);
        EXPECT_TRUE(255u == aVector[2]);

        // An old, base64 encoded string (from before Karabo 3)
        aVector = fromString<unsigned char, std::vector>("KSorLC0u", ",");
        EXPECT_EQ(6ul, aVector.size());
        EXPECT_EQ(static_cast<unsigned char>(41), aVector[0]);
        EXPECT_EQ(static_cast<unsigned char>(42), aVector[1]);
        EXPECT_EQ(static_cast<unsigned char>(43), aVector[2]);
        EXPECT_EQ(static_cast<unsigned char>(44), aVector[3]);
        EXPECT_EQ(static_cast<unsigned char>(45), aVector[4]);
        EXPECT_EQ(static_cast<unsigned char>(46), aVector[5]);
    }
    // Vector int has a template specialisation
    {
        std::vector<int> aVector = fromString<int, std::vector>("77", ",");
        EXPECT_EQ(1ul, aVector.size());
        EXPECT_EQ(77, aVector[0]);

        aVector = fromString<int, std::vector>("", ",");
        EXPECT_EQ(0ul, aVector.size());

        aVector = fromString<int, std::vector>("-32768, -77, 32767", ",");
        EXPECT_EQ(3ul, aVector.size());
        EXPECT_EQ(-32768, aVector[0]);
        EXPECT_EQ(-77, aVector[1]);
        EXPECT_EQ(32767, aVector[2]);
    }

    // Vector unsigned int has another template specialisation
    {
        std::vector<unsigned int> aVector = fromString<unsigned int, std::vector>("77", ",");
        EXPECT_EQ(1ul, aVector.size());
        EXPECT_EQ(77u, aVector[0]);

        aVector = fromString<unsigned int, std::vector>("", ",");
        EXPECT_EQ(0ul, aVector.size());

        aVector = fromString<unsigned int, std::vector>(" [0, 77, 65535] ", ",");
        EXPECT_EQ(3ul, aVector.size());
        EXPECT_EQ(0u, aVector[0]);
        EXPECT_EQ(77u, aVector[1]);
        EXPECT_EQ(65535u, aVector[2]);
    }

    // Vector long long has yet another template specialisation
    {
        std::vector<long long> aVector = fromString<long long, std::vector>(" 77 ", ",");
        EXPECT_EQ(1ul, aVector.size());
        EXPECT_EQ(77ll, aVector[0]);

        aVector = fromString<long long, std::vector>("", ",");
        EXPECT_EQ(0ul, aVector.size());

        aVector = fromString<long long, std::vector>(" -2147483648, -77 , 2147483647", ",");
        EXPECT_EQ(3ul, aVector.size());
        EXPECT_EQ(-2147483648ll, aVector[0]);
        EXPECT_EQ(-77ll, aVector[1]);
        EXPECT_EQ(2147483647ll, aVector[2]);
    }

    // Vector unsigned long long has yet another template specialisation
    {
        std::vector<unsigned long long> aVector = fromString<unsigned long long, std::vector>("77", ",");
        EXPECT_EQ(1ul, aVector.size());
        EXPECT_EQ(77ull, aVector[0]);

        aVector = fromString<unsigned long long, std::vector>("", ",");
        EXPECT_EQ(0ul, aVector.size());

        aVector = fromString<unsigned long long, std::vector>(" 0, 77, 4294967295      ", ",");
        EXPECT_EQ(3ul, aVector.size());
        EXPECT_EQ(0ull, aVector[0]);
        EXPECT_EQ(77ull, aVector[1]);
        EXPECT_EQ(4294967295ull, aVector[2]);
    }

    // tests for set
    {
        auto aSet = fromStringToSortedCont<int, std::set>("77", ",");
        EXPECT_EQ(1ul, aSet.size());
        EXPECT_EQ(77, *(aSet.begin()));

        aSet = fromStringToSortedCont<int, std::set>("", ",");
        EXPECT_EQ(0ul, aSet.size());

        aSet = fromStringToSortedCont<int, std::set>("-32768, -77, 32767, -77", ",");
        EXPECT_EQ(3ul, aSet.size());
        auto it = aSet.begin();
        EXPECT_EQ(-32768, *it);
        ++it;
        EXPECT_EQ(-77, *it);
        ++it;
        EXPECT_EQ(32767, *it);
    }
}


TEST(TestStringTools, testToString) {
    // Unsigned int types
    EXPECT_TRUE(toString((unsigned char)234) == "234");
    EXPECT_TRUE(toString((unsigned short)34567) == "34567");
    EXPECT_TRUE(toString((unsigned int)2345678901) == "2345678901");
    EXPECT_TRUE(toString((unsigned long long)123456789012) == "123456789012");

    // Signed int types
    EXPECT_TRUE(toString((signed char)123) == "123");
    EXPECT_TRUE(toString((short)23456) == "23456");
    EXPECT_TRUE(toString((int)1234567890) == "1234567890");
    EXPECT_TRUE(toString((long long)123456789012) == "123456789012");

    // Boolean
    EXPECT_TRUE(toString(false) == "0");
    EXPECT_TRUE(toString(true) == "1");

    // Floating point types
    EXPECT_TRUE(toString(1.2345678f) == "1.234568");
    EXPECT_TRUE(toString(0.1f) == "0.1");
    EXPECT_TRUE(toString(-0.1f) == "-0.1");
    EXPECT_TRUE(toString(0.000000099999987f) == "9.999999e-08");
    EXPECT_TRUE(toString(-99999987198.0f) == "-9.999999e+10");
    EXPECT_TRUE(toString(3333332187236.0f) == "3.333332e+12");

    EXPECT_TRUE(toString(1.234567890123456) == "1.23456789012346");
    EXPECT_TRUE(toString(0.123456789012345) == "0.123456789012345");
    EXPECT_TRUE(toString(0.01234567890123456) == "0.0123456789012346");

    // Border(?) cases ...
    EXPECT_TRUE(toString(0.0001f) == "0.0001");
    EXPECT_TRUE(toString(0.00001f) == "1e-05");
    EXPECT_TRUE(toString(1000000.f) == "1000000");
    EXPECT_TRUE(toString(10000000.f) == "1e+07");

    // Complex types
    EXPECT_TRUE(toString(std::complex<float>(1.2345678f, 1.3456789f)) == "(1.234568,1.345679)");
    EXPECT_TRUE(toString(std::complex<float>(123456700000.0f, 0.00000000013456780f)) == "(1.234567e+11,1.345678e-10)");
    EXPECT_TRUE(toString(std::complex<double>(1.234567890123456, 6.543210987654321)) ==
                "(1.23456789012346,6.54321098765432)");

    // Vectors
    std::vector<unsigned int> uint32Vector(4, 12345);
    EXPECT_TRUE(toString(uint32Vector) == "12345,12345,12345,12345");
    std::vector<float> floatVector{1.f,         0.5f,       3.1415956f,  0.1f,
                                   0.09999878f, 2.8790123f, 99.8765411f, -0.00000000003456789f};
    EXPECT_TRUE(toString(floatVector) == "1,0.5,3.141596,0.1,0.09999878,2.879012,99.87654,-3.456789e-11");

    // Vectors, but playing with maximum number
    const std::vector<int> int32Vector{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    EXPECT_STREQ("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15", toString(int32Vector).c_str()); // all is default
    // Ask for less - but at least first and last are shown and on each end up to 5 less than half maximum
    EXPECT_STREQ("1,...(skip 13 values)...,15", toString(int32Vector, 1).c_str());
    // From 14 on more than just first/last are shown
    EXPECT_STREQ("1,2,...(skip 11 values)...,14,15", toString(int32Vector, 14).c_str());
    // Check if vector size hits maximum, all elements are indeed treated
    EXPECT_STREQ("2,3,4,5,6,7,8,9,10,11,12,13,14,15",
                 toString(std::vector<int>(int32Vector.begin() + 1, int32Vector.end()), 14).c_str());

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
    EXPECT_STREQ(only18_12345.c_str(), toString(int64Vector, 28).c_str());

    // Sets - they are ordered
    EXPECT_STREQ("2,3,4,5", toString(std::set<unsigned int>({5, 4, 2, 3})).c_str());

    // Unordered_sets - they have any order, but all should be in
    const std::unordered_set<int> int32_unordered_set({5, 4, 2, 3});
    const std::string int32_unordered_set_asstring(toString(int32_unordered_set));
    auto vec = fromString<int, std::vector>(int32_unordered_set_asstring);
    // Check that all in created vector are all in original unordered_set
    EXPECT_EQ(vec.size(), int32_unordered_set.size());
    for (int i : int32_unordered_set) {
        EXPECT_TRUE(std::find(vec.begin(), vec.end(), i) != vec.end());
    }

    // Wide strings
    const wchar_t wstr[] = L"abcd0123";
    string str = "abcd0123";
    EXPECT_TRUE(toString(std::wstring(L"abcd0123")) == str);
    EXPECT_TRUE(toString(L"abcd0123") == "abcd0123");
    EXPECT_TRUE(toString(wstr) == "abcd0123");

    // ByteArray
    static char s[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11,
                       0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23};
    ByteArray ba = std::make_pair(std::shared_ptr<char>(s, [](char*) {}), 24);
    EXPECT_TRUE(toString(ba, 16) == "0x0001020304050607...(skip 8 bytes)...1617181920212223");
    EXPECT_TRUE(toString(ba, 24) == "0x000102030405060708091011121314151617181920212223");
    EXPECT_TRUE(toString(ba, 4) == "0x0001...(skip 20 bytes)...2223");
}


TEST(TestStringTools, testWiden) {
    Widen<wchar_t> toWString;
    EXPECT_TRUE(toWString("abcd0123") == L"abcd0123");
}


TEST(TestStringTools, testTokenize) {
    std::vector<string> out;

    // Test 1, 2, 3 single character tokens
    tokenize("a", out, '.');
    EXPECT_EQ(1ul, out.size());
    EXPECT_STREQ("a", out[0].c_str());

    tokenize("b.c", out, '.');
    EXPECT_EQ(2ul, out.size());
    EXPECT_STREQ("b", out[0].c_str());
    EXPECT_STREQ("c", out[1].c_str());

    tokenize("d.e.f", out, '.');
    EXPECT_EQ(3ul, out.size());
    EXPECT_STREQ("d", out[0].c_str());
    EXPECT_STREQ("e", out[1].c_str());
    EXPECT_STREQ("f", out[2].c_str());

    // Test few more multi-character tokens
    tokenize("Aa.buu.c.undNochWa][", out, '.');
    EXPECT_EQ(4ul, out.size());
    EXPECT_STREQ("Aa", out[0].c_str());
    EXPECT_STREQ("buu", out[1].c_str());
    EXPECT_STREQ("c", out[2].c_str());
    EXPECT_STREQ("undNochWa][", out[3].c_str());

    // Test empty token in beginning, middle and end
    tokenize(".a", out, '.');
    EXPECT_EQ(2ul, out.size());
    EXPECT_STREQ("", out[0].c_str());
    EXPECT_STREQ("a", out[1].c_str());

    tokenize("a..z", out, '.');
    EXPECT_EQ(3ul, out.size());
    EXPECT_STREQ("a", out[0].c_str());
    EXPECT_STREQ("", out[1].c_str());
    EXPECT_STREQ("z", out[2].c_str());

    tokenize("a.", out, '.');
    EXPECT_EQ(2ul, out.size());
    EXPECT_STREQ("a", out[0].c_str());
    EXPECT_STREQ("", out[1].c_str());

    // Test up to three empty tokens in a row
    tokenize("", out, '.');
    EXPECT_EQ(1ul, out.size());
    EXPECT_STREQ("", out[0].c_str());

    tokenize(".", out, '.');
    EXPECT_EQ(2ul, out.size());
    EXPECT_STREQ("", out[0].c_str());
    EXPECT_STREQ("", out[1].c_str());

    tokenize("..", out, '.');
    EXPECT_EQ(3ul, out.size());
    EXPECT_STREQ("", out[0].c_str());
    EXPECT_STREQ("", out[1].c_str());
    EXPECT_STREQ("", out[2].c_str());

    // Another tokenize, based on std::string_view
    std::string input = "";
    auto vsv1 = tokenize(input, '.');
    EXPECT_EQ(1ul, vsv1.size());
    EXPECT_EQ(""sv, vsv1[0]);

    input.assign(".");
    auto vsv2 = tokenize(input, '.');
    EXPECT_EQ(2ul, vsv2.size());
    EXPECT_EQ(""sv, vsv2[0]);
    EXPECT_EQ(""sv, vsv2[1]);

    input.assign("a");
    auto vsv3 = tokenize(input, '.');
    EXPECT_EQ(1ul, vsv3.size());
    EXPECT_EQ("a"sv, vsv3[0]);

    input.assign(".a");
    auto vsv4 = tokenize(input, '.');
    EXPECT_EQ(2ul, vsv4.size());
    EXPECT_EQ(""sv, vsv4[0]);
    EXPECT_EQ("a"sv, vsv4[1]);

    input.assign("a.");
    auto vsv5 = tokenize(input, '.');
    EXPECT_EQ(2ul, vsv5.size());
    EXPECT_EQ("a"sv, vsv5[0]);
    EXPECT_EQ(""sv, vsv5[1]);

    input.assign("a.b");
    auto vsv6 = tokenize(input, '.');
    EXPECT_EQ(2ul, vsv6.size());
    EXPECT_EQ("a"sv, vsv6[0]);
    EXPECT_EQ("b"sv, vsv6[1]);

    input.assign("a.b[12].c[32].d");
    auto vsv7 = tokenize(input, '.');
    EXPECT_EQ(4ul, vsv7.size());
    EXPECT_EQ("a"sv, vsv7[0]);
    EXPECT_EQ("b[12]"sv, vsv7[1]);
    EXPECT_EQ("c[32]"sv, vsv7[2]);
    EXPECT_EQ("d"sv, vsv7[3]);

    // getAndCropIndex...
    input.assign("abcde[1234]");
    auto [index1, sview1] = getAndCropIndex(input);
    EXPECT_EQ(index1, 1234);
    EXPECT_EQ(sview1, "abcde"sv);

    input.assign("abcde[1234");
    auto [index2, sview2] = getAndCropIndex(input);
    EXPECT_EQ(index2, -1);
    EXPECT_EQ(sview2, "abcde[1234"sv);

    input.assign("abcde1234]");
    auto [index3, sview3] = getAndCropIndex(input);
    EXPECT_EQ(index3, -1);
    EXPECT_EQ(sview3, "abcde1234]"sv);

    input.assign("abcde[]");
    auto [index4, sview4] = getAndCropIndex(input);
    EXPECT_EQ(index4, 0);
    EXPECT_EQ(sview4, "abcde"sv);

    std::string str1 = "\n\r\t AbRa - kaDaBRa\r\t\n";
    trim(str1);
    EXPECT_STREQ(str1.c_str(), "AbRa - kaDaBRa");

    toLower(str1);
    EXPECT_STREQ(str1.c_str(), "abra - kadabra");

    toUpper(str1);
    EXPECT_STREQ(str1.c_str(), "ABRA - KADABRA");

    // Split tests
    auto v1 = split(str1);
    EXPECT_TRUE(v1.size() == 3);
    EXPECT_STREQ(v1[0].c_str(), "ABRA");
    EXPECT_STREQ(v1[1].c_str(), "-");
    EXPECT_STREQ(v1[2].c_str(), "KADABRA");

    std::string str2 = "HH:MM:SS";
    auto v2 = split(str2, ":");
    EXPECT_TRUE(v2.size() == 3);
    EXPECT_STREQ(v2[0].c_str(), "HH");
    EXPECT_STREQ(v2[1].c_str(), "MM");
    EXPECT_STREQ(v2[2].c_str(), "SS");

    std::string str3 = "2024-10-25T12:32:44.035 [debug] abrakadabra : Body message that can be quite long... ";
    auto v3 = split(str3, " ", 5);
    EXPECT_TRUE(v3.size() == 5);
    EXPECT_STREQ(v3[0].c_str(), "2024-10-25T12:32:44.035");
    EXPECT_STREQ(v3[1].c_str(), "[debug]");
    EXPECT_STREQ(v3[2].c_str(), "abrakadabra");
    EXPECT_STREQ(v3[3].c_str(), ":");
    EXPECT_STREQ(v3[4].c_str(), "Body message that can be quite long... ");
}
