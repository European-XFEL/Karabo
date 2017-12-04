/* 
 * File:   StringTools_Test.cc
 * Author: parenti
 * 
 * Created on December 15, 2014, 12:16 PM
 */

#include "StringTools_Test.hh"
#include <karabo/util/StringTools.hh>

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
    CPPUNIT_ASSERT(toString((float) 1.2345678) == "1.234568");
    CPPUNIT_ASSERT(toString(float(0.1)) == "0.1");
    CPPUNIT_ASSERT(toString(float(-0.1)) == "-0.1");
    CPPUNIT_ASSERT(toString((float) 0.000000099999987) == "9.999999e-08");
    CPPUNIT_ASSERT(toString((float) -99999987198.0) == "-9.999999e+10");
    CPPUNIT_ASSERT(toString((float) 3333332187236.0) == "3.333332e+12");
    
    CPPUNIT_ASSERT(toString((double) 1.234567890123456) == "1.23456789012346");
    CPPUNIT_ASSERT(toString((double) 0.123456789012345) == "0.123456789012345");
    CPPUNIT_ASSERT(toString((double) 0.01234567890123456) == "0.0123456789012346");
    

    // Complex types
    CPPUNIT_ASSERT(toString(std::complex<float>(1.2345678, 1.3456789)) == "(1.234568,1.345679)");
    CPPUNIT_ASSERT(toString(std::complex<float>(123456700000.0, 0.00000000013456780)) == "(1.234567e+11,1.345678e-10)");
    CPPUNIT_ASSERT(toString(std::complex<double>(1.234567890123456, 6.543210987654321)) == "(1.23456789012346,6.54321098765432)");

    // Vectors
    std::vector<unsigned int> uint32Vector(4, 12345);
    CPPUNIT_ASSERT(toString(uint32Vector) == "12345,12345,12345,12345");
    std::vector<float> floatVector{1,0.5,3.1415956,0.1, 0.09999878, 2.8790123,99.8765411, -0.00000000003456789};
    CPPUNIT_ASSERT(toString(floatVector) == "1,0.5,3.141596,0.1,0.09999878,2.879012,99.87654,-3.456789e-11");

    // Wide strings
    const wchar_t wstr[] = L"abcd0123";
    string str = "abcd0123";
    CPPUNIT_ASSERT(toString(std::wstring(L"abcd0123")) == str);
    CPPUNIT_ASSERT(toString(L"abcd0123") == "abcd0123");
    CPPUNIT_ASSERT(toString(wstr) == "abcd0123");
}


void StringTools_Test::testWiden() {
    Widen<wchar_t> toWString;
    CPPUNIT_ASSERT(toWString("abcd0123") == L"abcd0123");
}
