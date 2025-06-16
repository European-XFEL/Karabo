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
 * File:   Types_Test.cc
 * Author: heisenb
 *
 * Created on February 27, 2013, 3:09 PM
 */

#include "Types_Test.hh"

#include <cstdlib>
#include <iostream>

#include "karabo/data/types/FromInt.hh"
#include "karabo/data/types/FromLiteral.hh"
#include "karabo/data/types/FromTypeInfo.hh"
#include "karabo/data/types/ToCppString.hh"
#include "karabo/data/types/ToLiteral.hh"
#include "karabo/data/types/Types.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Types_Test);

using namespace std;
using namespace karabo::data;


Types_Test::Types_Test() {}


Types_Test::~Types_Test() {}


void Types_Test::setUp() {}


void Types_Test::tearDown() {}


void Types_Test::testCategory() {
    CPPUNIT_ASSERT(Types::category(Types::UINT32) == Types::SIMPLE);
    CPPUNIT_ASSERT(Types::category(Types::INT64) == Types::SIMPLE);
    CPPUNIT_ASSERT(Types::category(Types::BOOL) == Types::SIMPLE);
    CPPUNIT_ASSERT(Types::category(Types::VECTOR_FLOAT) == Types::SEQUENCE);
    CPPUNIT_ASSERT(Types::category(Types::VECTOR_INT32) == Types::SEQUENCE);
    CPPUNIT_ASSERT(Types::category(Types::SCHEMA) == Types::SCHEMA);
    CPPUNIT_ASSERT(Types::category(Types::VECTOR_HASH) == Types::VECTOR_HASH);
}


void Types_Test::testIsVector() {
    CPPUNIT_ASSERT(Types::isVector(Types::UINT16) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::BOOL) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::CHAR) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::INT8) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::UINT8) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::INT16) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::UINT16) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::INT32) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::UINT32) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::INT64) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::UINT64) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::FLOAT) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::DOUBLE) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::COMPLEX_FLOAT) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::COMPLEX_DOUBLE) == false);
    CPPUNIT_ASSERT(Types::isVector(Types::STRING) == false);

    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_UINT16) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_BOOL) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_CHAR) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_INT8) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_UINT8) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_INT16) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_UINT16) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_INT32) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_UINT32) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_INT64) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_UINT64) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_FLOAT) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_DOUBLE) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_COMPLEX_FLOAT) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_COMPLEX_DOUBLE) == true);
    CPPUNIT_ASSERT(Types::isVector(Types::VECTOR_STRING) == true);
}


void Types_Test::testIsSimple() {
    CPPUNIT_ASSERT(Types::isSimple(Types::UINT16) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::BOOL) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::CHAR) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::INT8) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::UINT8) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::INT16) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::UINT16) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::INT32) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::UINT32) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::INT64) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::UINT64) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::FLOAT) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::DOUBLE) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::COMPLEX_FLOAT) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::COMPLEX_DOUBLE) == true);
    CPPUNIT_ASSERT(Types::isSimple(Types::STRING) == true);

    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_UINT16) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_BOOL) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_CHAR) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_INT8) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_UINT8) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_INT16) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_UINT16) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_INT32) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_UINT32) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_INT64) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_UINT64) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_FLOAT) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_DOUBLE) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_COMPLEX_FLOAT) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_COMPLEX_DOUBLE) == false);
    CPPUNIT_ASSERT(Types::isSimple(Types::VECTOR_STRING) == false);
}


void Types_Test::testFrom() {
    CPPUNIT_ASSERT(Types::from<FromTypeInfo>(typeid(int)) == Types::INT32);
    CPPUNIT_ASSERT(Types::from<double>(1.23) == Types::DOUBLE);
    CPPUNIT_ASSERT(Types::from(1.23) == Types::DOUBLE);
    CPPUNIT_ASSERT(Types::from(12345678987654ULL) == Types::UINT64);
    CPPUNIT_ASSERT(Types::from(123456.789F) == Types::FLOAT);
}


void Types_Test::testTo() {
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::DOUBLE) == "double");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::HASH) == "Hash");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::SCHEMA) == "Schema");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::VECTOR_STRING) == "vector<string>");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::VECTOR_UINT8) == "vector<unsigned char>");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::VECTOR_INT8) == "vector<signed char>");
    CPPUNIT_ASSERT(Types::to<ToCppString>(Types::VECTOR_HASH) == "vector<Hash>");
}


void Types_Test::testConvert() {
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToCppString>(typeid(bool)) == "bool"));
    CPPUNIT_ASSERT((Types::convert<FromLiteral, ToCppString>("UINT32") == "unsigned int"));
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToLiteral>(typeid(vector<bool>)) == "VECTOR_BOOL"));
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToLiteral>(typeid(1LL)) == "INT64"));
    CPPUNIT_ASSERT((Types::convert<FromTypeInfo, ToLiteral>(typeid(2ULL)) == "UINT64"));
    CPPUNIT_ASSERT((Types::convert<FromInt, ToCppString>(12) == "int"));
    CPPUNIT_ASSERT((Types::convert<FromInt, ToCppString>(17) == "vector<long long>"));
}
