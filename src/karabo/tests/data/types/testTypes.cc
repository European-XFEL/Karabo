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

#include <gtest/gtest.h>

#include <cstdlib>
#include <iostream>

#include "karabo/data/types/FromInt.hh"
#include "karabo/data/types/FromLiteral.hh"
#include "karabo/data/types/FromTypeInfo.hh"
#include "karabo/data/types/ToCppString.hh"
#include "karabo/data/types/ToLiteral.hh"
#include "karabo/data/types/Types.hh"

using namespace std;
using namespace karabo::data;


TEST(TestTypes, testCategory) {
    EXPECT_TRUE(Types::category(Types::UINT32) == Types::SIMPLE);
    EXPECT_TRUE(Types::category(Types::INT64) == Types::SIMPLE);
    EXPECT_TRUE(Types::category(Types::BOOL) == Types::SIMPLE);
    EXPECT_TRUE(Types::category(Types::VECTOR_FLOAT) == Types::SEQUENCE);
    EXPECT_TRUE(Types::category(Types::VECTOR_INT32) == Types::SEQUENCE);
    EXPECT_TRUE(Types::category(Types::SCHEMA) == Types::SCHEMA);
    EXPECT_TRUE(Types::category(Types::VECTOR_HASH) == Types::VECTOR_HASH);
}


TEST(TestTypes, testIsVector) {
    EXPECT_TRUE(Types::isVector(Types::UINT16) == false);
    EXPECT_TRUE(Types::isVector(Types::BOOL) == false);
    EXPECT_TRUE(Types::isVector(Types::CHAR) == false);
    EXPECT_TRUE(Types::isVector(Types::INT8) == false);
    EXPECT_TRUE(Types::isVector(Types::UINT8) == false);
    EXPECT_TRUE(Types::isVector(Types::INT16) == false);
    EXPECT_TRUE(Types::isVector(Types::UINT16) == false);
    EXPECT_TRUE(Types::isVector(Types::INT32) == false);
    EXPECT_TRUE(Types::isVector(Types::UINT32) == false);
    EXPECT_TRUE(Types::isVector(Types::INT64) == false);
    EXPECT_TRUE(Types::isVector(Types::UINT64) == false);
    EXPECT_TRUE(Types::isVector(Types::FLOAT) == false);
    EXPECT_TRUE(Types::isVector(Types::DOUBLE) == false);
    EXPECT_TRUE(Types::isVector(Types::COMPLEX_FLOAT) == false);
    EXPECT_TRUE(Types::isVector(Types::COMPLEX_DOUBLE) == false);
    EXPECT_TRUE(Types::isVector(Types::STRING) == false);

    EXPECT_TRUE(Types::isVector(Types::VECTOR_UINT16) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_BOOL) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_CHAR) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_INT8) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_UINT8) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_INT16) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_UINT16) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_INT32) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_UINT32) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_INT64) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_UINT64) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_FLOAT) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_DOUBLE) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_COMPLEX_FLOAT) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_COMPLEX_DOUBLE) == true);
    EXPECT_TRUE(Types::isVector(Types::VECTOR_STRING) == true);
}


TEST(TestTypes, testIsSimple) {
    EXPECT_TRUE(Types::isSimple(Types::UINT16) == true);
    EXPECT_TRUE(Types::isSimple(Types::BOOL) == true);
    EXPECT_TRUE(Types::isSimple(Types::CHAR) == true);
    EXPECT_TRUE(Types::isSimple(Types::INT8) == true);
    EXPECT_TRUE(Types::isSimple(Types::UINT8) == true);
    EXPECT_TRUE(Types::isSimple(Types::INT16) == true);
    EXPECT_TRUE(Types::isSimple(Types::UINT16) == true);
    EXPECT_TRUE(Types::isSimple(Types::INT32) == true);
    EXPECT_TRUE(Types::isSimple(Types::UINT32) == true);
    EXPECT_TRUE(Types::isSimple(Types::INT64) == true);
    EXPECT_TRUE(Types::isSimple(Types::UINT64) == true);
    EXPECT_TRUE(Types::isSimple(Types::FLOAT) == true);
    EXPECT_TRUE(Types::isSimple(Types::DOUBLE) == true);
    EXPECT_TRUE(Types::isSimple(Types::COMPLEX_FLOAT) == true);
    EXPECT_TRUE(Types::isSimple(Types::COMPLEX_DOUBLE) == true);
    EXPECT_TRUE(Types::isSimple(Types::STRING) == true);

    EXPECT_TRUE(Types::isSimple(Types::VECTOR_UINT16) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_BOOL) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_CHAR) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_INT8) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_UINT8) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_INT16) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_UINT16) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_INT32) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_UINT32) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_INT64) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_UINT64) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_FLOAT) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_DOUBLE) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_COMPLEX_FLOAT) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_COMPLEX_DOUBLE) == false);
    EXPECT_TRUE(Types::isSimple(Types::VECTOR_STRING) == false);
}


TEST(TestTypes, testFrom) {
    EXPECT_TRUE(Types::from<FromTypeInfo>(typeid(int)) == Types::INT32);
    EXPECT_TRUE(Types::from<double>(1.23) == Types::DOUBLE);
    EXPECT_TRUE(Types::from(1.23) == Types::DOUBLE);
    EXPECT_TRUE(Types::from(12345678987654ULL) == Types::UINT64);
    EXPECT_TRUE(Types::from(123456.789F) == Types::FLOAT);
}


TEST(TestTypes, testTo) {
    EXPECT_TRUE(Types::to<ToCppString>(Types::DOUBLE) == "double");
    EXPECT_TRUE(Types::to<ToCppString>(Types::HASH) == "Hash");
    EXPECT_TRUE(Types::to<ToCppString>(Types::SCHEMA) == "Schema");
    EXPECT_TRUE(Types::to<ToCppString>(Types::VECTOR_STRING) == "vector<string>");
    EXPECT_TRUE(Types::to<ToCppString>(Types::VECTOR_UINT8) == "vector<unsigned char>");
    EXPECT_TRUE(Types::to<ToCppString>(Types::VECTOR_INT8) == "vector<signed char>");
    EXPECT_TRUE(Types::to<ToCppString>(Types::VECTOR_HASH) == "vector<Hash>");
}


TEST(TestTypes, testConvert) {
    EXPECT_TRUE((Types::convert<FromTypeInfo, ToCppString>(typeid(bool)) == "bool"));
    EXPECT_TRUE((Types::convert<FromLiteral, ToCppString>("UINT32") == "unsigned int"));
    EXPECT_TRUE((Types::convert<FromTypeInfo, ToLiteral>(typeid(vector<bool>)) == "VECTOR_BOOL"));
    EXPECT_TRUE((Types::convert<FromTypeInfo, ToLiteral>(typeid(1LL)) == "INT64"));
    EXPECT_TRUE((Types::convert<FromTypeInfo, ToLiteral>(typeid(2ULL)) == "UINT64"));
    EXPECT_TRUE((Types::convert<FromInt, ToCppString>(12) == "int"));
    EXPECT_TRUE((Types::convert<FromInt, ToCppString>(17) == "vector<long long>"));
}
