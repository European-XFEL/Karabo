/* 
 * File:   Types_Test.cc
 * Author: heisenb
 * 
 * Created on February 27, 2013, 3:09 PM
 */

#include <cstdlib>
#include <iostream>
#include "Types_Test.hh"
#include "karabo/util/ToCppString.hh"
#include <karabo/util/Types.hh>
#include <karabo/util/FromTypeInfo.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/ToXsd.hh>
#include <karabo/util/FromInt.hh>

CPPUNIT_TEST_SUITE_REGISTRATION(Types_Test);

using namespace std;
using namespace karabo::util;


Types_Test::Types_Test() {
}


Types_Test::~Types_Test() {
}


void Types_Test::setUp() {
}


void Types_Test::tearDown() {
}


void Types_Test::testCategory() {
    CPPUNIT_ASSERT(Types::category(Types::UINT32) == Types::SIMPLE);
    CPPUNIT_ASSERT(Types::category(Types::INT64) == Types::SIMPLE);
    CPPUNIT_ASSERT(Types::category(Types::BOOL) == Types::SIMPLE);
    CPPUNIT_ASSERT(Types::category(Types::VECTOR_FLOAT) == Types::SEQUENCE);
    CPPUNIT_ASSERT(Types::category(Types::VECTOR_INT32) == Types::SEQUENCE);
    CPPUNIT_ASSERT(Types::category(Types::PTR_FLOAT) == Types::SEQUENCE);
    CPPUNIT_ASSERT(Types::category(Types::SCHEMA) == Types::SCHEMA);
    CPPUNIT_ASSERT(Types::category(Types::VECTOR_HASH) == Types::VECTOR_HASH);
    CPPUNIT_ASSERT(Types::category(Types::ANY) == Types::ANY);
}


void Types_Test::testFrom() {
    CPPUNIT_ASSERT(Types::from<FromTypeInfo>(typeid (int)) == Types::INT32);
    CPPUNIT_ASSERT(Types::from<double>(1.23) == Types::DOUBLE);
    CPPUNIT_ASSERT(Types::from(1.23) == Types::DOUBLE);
    CPPUNIT_ASSERT(Types::from(12345678987654ULL) == Types::UINT64);
    CPPUNIT_ASSERT(Types::from(123456.789F) == Types::FLOAT);
}


void Types_Test::testTo() {
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::DOUBLE) == "double");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::HASH) == "Hash");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::SCHEMA) == "Schema");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::PTR_CHAR) == "char*");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::PTR_INT8) == "signed char*");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::PTR_UINT8) == "unsigned char*");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::PTR_UINT64) == "unsigned long long*");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::PTR_COMPLEX_DOUBLE) == "complex<double>*");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::PTR_STRING) == "string*");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::VECTOR_STRING) == "vector<string>");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::VECTOR_UINT8) == "vector<unsigned char>");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::VECTOR_INT8) == "vector<signed char>");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::VECTOR_HASH) == "vector<Hash>");
    
    CPPUNIT_ASSERT(Types::to<ToXsd>(Types::CHAR) == "xs:byte");
    CPPUNIT_ASSERT(Types::to<ToXsd>(Types::VECTOR_CHAR) == "xs:byte");
    CPPUNIT_ASSERT(Types::to<ToXsd>(Types::FLOAT) == "xs:float");
    CPPUNIT_ASSERT(Types::to<ToXsd>(Types::DOUBLE) == "xs:double");
    CPPUNIT_ASSERT(Types::to<ToXsd>(Types::VECTOR_STRING) == "xs:string");
    CPPUNIT_ASSERT(Types::to<ToXsd>(Types::VECTOR_UINT8) == "xs:unsignedByte");
    CPPUNIT_ASSERT(Types::to<ToXsd>(Types::VECTOR_INT8) == "xs:byte");
    CPPUNIT_ASSERT(Types::to<ToXsd>(Types::VECTOR_INT32) == "xs:int");
}


void Types_Test::testConvert() {
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToCppString>(typeid (bool)) == "bool"));
    CPPUNIT_ASSERT((Types::convert<FromLiteral, ToCppString>("UINT32") == "unsigned int"));
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToLiteral>(typeid(vector<bool>)) == "VECTOR_BOOL"));
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToLiteral>(typeid(signed char*)) == "PTR_INT8"));
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToLiteral>(typeid(long long*)) == "PTR_INT64"));
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToLiteral>(typeid(1LL)) == "INT64"));
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToLiteral>(typeid(2ULL)) == "UINT64"));
    CPPUNIT_ASSERT((Types::convert<FromInt, ToCppString>(12) == "int"));
    CPPUNIT_ASSERT((Types::convert<FromInt, ToCppString>(17) == "vector<long long>"));
}
