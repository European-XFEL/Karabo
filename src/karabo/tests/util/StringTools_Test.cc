/* 
 * File:   StringTools_Test.cc
 * Author: parenti
 * 
 * Created on December 15, 2014, 12:16 PM
 */

#include "StringTools_Test.hh"
#include <karabo/util/StringTools.hh>
#include <boost/core/null_deleter.hpp>

CPPUNIT_TEST_SUITE_REGISTRATION(StringTools_Test);

using namespace karabo::util;
using namespace std;


StringTools_Test::StringTools_Test() {
}


StringTools_Test::~StringTools_Test() {
}


void StringTools_Test::setUp() {
}


void StringTools_Test::tearDown() {
}


void StringTools_Test::testFromString() {

    // Unsigned integer
    CPPUNIT_ASSERT((unsigned char) 234 == fromString<unsigned char>("234"));
    CPPUNIT_ASSERT((unsigned short) 34567 == fromString<unsigned short>("34567"));
    CPPUNIT_ASSERT((unsigned int) 2345678901 == fromString<unsigned int>("2345678901"));
    CPPUNIT_ASSERT((unsigned long long) 123456789012 == fromString<unsigned long long>("123456789012"));

    // Signed integer
    CPPUNIT_ASSERT((signed char) 123 == fromString<signed char>("123"));
    CPPUNIT_ASSERT((short) 23456 == fromString<short>("23456"));
    CPPUNIT_ASSERT((int) 1234567890 == fromString<int>("1234567890"));
    CPPUNIT_ASSERT((long long) 123456789012 == fromString<long long>("123456789012"));

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
    CPPUNIT_ASSERT((float) 1.234567 == fromString<float>("1.234567"));
    CPPUNIT_ASSERT((double) 1.23456789012345 == fromString<double>("1.23456789012345"));

    // Complex
    CPPUNIT_ASSERT(std::complex<float>(1.234, 5.678) == fromString<std::complex<float> >("(1.234,5.678)"));
    CPPUNIT_ASSERT(std::complex<double>(1.234, 5.678) == fromString<std::complex<double> >("(1.234,5.678)"));

    // Vector
    std::vector<unsigned int> uint32Vector = fromString<unsigned int, std::vector >("12345,23456,34567,45678", ",");
    CPPUNIT_ASSERT(4 == uint32Vector.size());
    CPPUNIT_ASSERT(12345 == uint32Vector[0]);
    CPPUNIT_ASSERT(45678 == uint32Vector[3]);

}


void StringTools_Test::testToString() {
    // Unsigned int types
    CPPUNIT_ASSERT(toString((unsigned char) 234) == "234");
    CPPUNIT_ASSERT(toString((unsigned short) 34567) == "34567");
    CPPUNIT_ASSERT(toString((unsigned int) 2345678901) == "2345678901");
    CPPUNIT_ASSERT(toString((unsigned long long) 123456789012) == "123456789012");

    // Signed int types
    CPPUNIT_ASSERT(toString((signed char) 123) == "123");
    CPPUNIT_ASSERT(toString((short) 23456) == "23456");
    CPPUNIT_ASSERT(toString((int) 1234567890) == "1234567890");
    CPPUNIT_ASSERT(toString((long long) 123456789012) == "123456789012");

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
    CPPUNIT_ASSERT(toString(std::complex<float>(123456700000.0f, 0.00000000013456780f)) == "(1.234567e+11,1.345678e-10)");
    CPPUNIT_ASSERT(toString(std::complex<double>(1.234567890123456, 6.543210987654321)) == "(1.23456789012346,6.54321098765432)");

    // Vectors
    std::vector<unsigned int> uint32Vector(4, 12345);
    CPPUNIT_ASSERT(toString(uint32Vector) == "12345,12345,12345,12345");
    std::vector<float> floatVector{1.f,0.5f,3.1415956f,0.1f, 0.09999878f, 2.8790123f,99.8765411f, -0.00000000003456789f};
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

    // Wide strings
    const wchar_t wstr[] = L"abcd0123";
    string str = "abcd0123";
    CPPUNIT_ASSERT(toString(std::wstring(L"abcd0123")) == str);
    CPPUNIT_ASSERT(toString(L"abcd0123") == "abcd0123");
    CPPUNIT_ASSERT(toString(wstr) == "abcd0123");

    // ByteArray
    static char s[]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                     0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                     0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23};
    ByteArray ba = std::make_pair(boost::shared_ptr<char>(s, boost::null_deleter()), 24);
    CPPUNIT_ASSERT(toString(ba,16) == "0x0001020304050607...(skip 8 bytes)...1617181920212223");
    CPPUNIT_ASSERT(toString(ba,24) == "0x000102030405060708091011121314151617181920212223");
    CPPUNIT_ASSERT(toString(ba,4) == "0x0001...(skip 20 bytes)...2223");
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
}