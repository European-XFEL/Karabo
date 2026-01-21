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
 * File:   NDArray_Test.cc
 * Author: parenti
 *
 * Created on May 22, 2015, 11:43:22 AM
 */

#include "NDArray_Test.hh"

#include <cppunit/TestAssert.h>

#include "karabo/data/types/NDArray.hh"
#include "karabo/data/types/StringTools.hh"

using namespace std;
using namespace karabo::data;

CPPUNIT_TEST_SUITE_REGISTRATION(NDArray_Test);


NDArray_Test::NDArray_Test() {}


NDArray_Test::~NDArray_Test() {}


void NDArray_Test::setUp() {}


void NDArray_Test::tearDown() {}


void NDArray_Test::testConstructor() {
    const Dims shape(100, 200);
    // Underlying data: all 2 but the first 100 entries which are 0 to 99
    vector<int> someData(100 * 200, 2);
    const int maxSeries = 100; // must be smaller than 124, see below!
    for (int i = 0; i < maxSeries; ++i) {
        someData[i] = i;
    }

    Hash h;

    {
        NDArray fly(shape, 2);                                // This NDArray has everything as 2
        NDArray cpy(&someData[0], someData.size(), shape);    // copy of someData using raw pointer
        NDArray iCp(someData.begin(), someData.end(), shape); // copy of someData using iterator range
        NDArray ref(&someData[0], someData.size(), NDArray::NullDeleter(), shape); // reference to someData

        // Invalid iterator range throws like it does for std::vector - for both 'real' iterators and bare pointers:
        CPPUNIT_ASSERT_THROW(NDArray(someData.begin() + 1, someData.begin()), std::bad_alloc);
        CPPUNIT_ASSERT_THROW(NDArray(&(someData[1]), &(someData[0])), std::bad_alloc);

        const Dims& flyShape = fly.getShape();
        const Dims& cpyShape = cpy.getShape();
        const Dims& iCpShape = iCp.getShape();
        const Dims& refShape = ref.getShape();

        CPPUNIT_ASSERT(flyShape.x1() == 100);
        CPPUNIT_ASSERT(flyShape.x2() == 200);
        for (int i = 0; i < maxSeries; ++i) {
            CPPUNIT_ASSERT_EQUAL(2, fly.getData<int>()[i]);
        }
        CPPUNIT_ASSERT(fly.getData<int>()[124] == 2);
        CPPUNIT_ASSERT(fly.size() == 100 * 200);
        CPPUNIT_ASSERT_EQUAL(sizeof(int), fly.itemSize());
        CPPUNIT_ASSERT_EQUAL(sizeof(int) * static_cast<size_t>(100 * 200), fly.byteSize());

        CPPUNIT_ASSERT(cpyShape.x1() == 100);
        CPPUNIT_ASSERT(cpyShape.x2() == 200);
        for (int i = 0; i < maxSeries; ++i) {
            CPPUNIT_ASSERT_EQUAL(i, cpy.getData<int>()[i]);
        }
        CPPUNIT_ASSERT(cpy.getData<int>()[124] == 2);
        CPPUNIT_ASSERT(cpy.size() == 100 * 200);
        CPPUNIT_ASSERT_EQUAL(sizeof(int), cpy.itemSize());
        CPPUNIT_ASSERT_EQUAL(sizeof(int) * static_cast<size_t>(100 * 200), cpy.byteSize());

        CPPUNIT_ASSERT(iCpShape.x1() == 100);
        CPPUNIT_ASSERT(iCpShape.x2() == 200);
        for (int i = 0; i < maxSeries; ++i) {
            CPPUNIT_ASSERT_EQUAL(i, iCp.getData<int>()[i]);
        }
        CPPUNIT_ASSERT(iCp.getData<int>()[124] == 2);
        CPPUNIT_ASSERT(iCp.size() == 100 * 200);
        CPPUNIT_ASSERT_EQUAL(sizeof(int), iCp.itemSize());
        CPPUNIT_ASSERT_EQUAL(sizeof(int) * static_cast<size_t>(100 * 200), iCp.byteSize());

        CPPUNIT_ASSERT(refShape.x1() == 100);
        CPPUNIT_ASSERT(refShape.x2() == 200);
        for (int i = 0; i < maxSeries; ++i) {
            CPPUNIT_ASSERT_EQUAL(i, ref.getData<int>()[i]);
        }
        CPPUNIT_ASSERT(ref.getData<int>()[124] == 2);
        CPPUNIT_ASSERT(ref.size() == 100 * 200);
        CPPUNIT_ASSERT_EQUAL(sizeof(int), ref.itemSize());
        CPPUNIT_ASSERT_EQUAL(sizeof(int) * static_cast<size_t>(100 * 200), ref.byteSize());

        // Setting content affects underlying data for ref, but not for cpy and iCp which have copied data:
        cpy.getData<int>()[124] = 0;
        CPPUNIT_ASSERT_EQUAL(0, cpy.getData<int>()[124]);
        CPPUNIT_ASSERT(someData[124] == 2);

        iCp.getData<int>()[124] = 0;
        CPPUNIT_ASSERT_EQUAL(0, iCp.getData<int>()[124]);
        CPPUNIT_ASSERT(someData[124] == 2);

        ref.getData<int>()[124] = 0;
        CPPUNIT_ASSERT_EQUAL(0, ref.getData<int>()[124]);
        CPPUNIT_ASSERT(someData[124] == 0);

        h.set("cpy", cpy);
        h.set("iCp", iCp);
        h.set("ref", ref);
    }

    // What we get from Hash is still a reference to someData:
    NDArray& ref = h.get<NDArray>("ref");
    CPPUNIT_ASSERT(ref.getData<int>()[124] == 0);
    CPPUNIT_ASSERT_EQUAL(0, someData[124]);
    ref.getData<int>()[124] = 124;
    CPPUNIT_ASSERT_EQUAL(124, someData[124]);
    CPPUNIT_ASSERT(ref.getShape().x1() == 100);
    CPPUNIT_ASSERT(ref.size() == 100 * 200);
}


void NDArray_Test::testShapeException() {
    vector<int> data(10, -42);
    const Dims badShape(2, 500);

    { CPPUNIT_ASSERT_THROW(NDArray(&data[0], data.size(), badShape), karabo::data::ParameterException); }
}


void NDArray_Test::testDataTypeException() {
    const int data[] = {1, 2, 3, 4};
    NDArray arr(data, sizeof(data) / sizeof(data[0]));

    std::string exceptionMsg;
    try {
        // cannot cast int to double
        arr.getData<double>();
    } catch (const karabo::data::CastException& e) {
        exceptionMsg = e.what();
    } catch (...) {
        exceptionMsg = "not a cast exception";
    }
    CPPUNIT_ASSERT_MESSAGE(exceptionMsg, exceptionMsg.find("from INT32") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE(exceptionMsg, exceptionMsg.find("to DOUBLE") != std::string::npos);

    // Non-supported types do not compile since Types::from<T> works only for template specialisations:
    // class AnUnknown {};
    // arr.getData<AnUnknown>();

    // Manipulate internals as if NDArray was corrupted:
    reinterpret_cast<Hash*>(&arr)->set("type", 12345678);
    exceptionMsg.clear();
    try {
        arr.getData<short>();
    } catch (const karabo::data::CastException& e) {
        exceptionMsg = e.what();
    } catch (...) {
        exceptionMsg = "not a cast exception";
    }
    const std::string msg(" missing from exception message: ");
    CPPUNIT_ASSERT_MESSAGE("'from _invalid_'" + msg + exceptionMsg,
                           exceptionMsg.find("from _invalid_") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("'12345678'" + msg + exceptionMsg,
                           exceptionMsg.find(toString(12345678)) != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE("'to INT16'" + msg + exceptionMsg, exceptionMsg.find("to INT16") != std::string::npos);
}

void NDArray_Test::testReplaceNDArraysByCopy() {
    // Some NDArrays (all with size() == 12)
    NDArray arr0(Dims(12), static_cast<short>(42));
    NDArray arr1(Dims(6, 2), -42);
    NDArray arr2(Dims(3, 4), 3.14f);
    Hash h("arr0", arr0, "b.arr1", arr1, "c.d.arr2", arr2,           //
           "other", "string", "b.nested", 0, "c.emptyHash", Hash()); // just other junk

    // NDArrays above and those inside 'h' share data pointers
    CPPUNIT_ASSERT_EQUAL(arr0.getData<short>(), h.get<NDArray>("arr0").getData<short>());
    CPPUNIT_ASSERT_EQUAL(arr1.getData<int>(), h.get<NDArray>("b.arr1").getData<int>());
    CPPUNIT_ASSERT_EQUAL(arr2.getData<float>(), h.get<NDArray>("c.d.arr2").getData<float>());

    const std::pair<size_t, size_t> nums = deepCopyNDArrays(h);
    // Copied 3 NDArrays and all their bytes
    CPPUNIT_ASSERT_EQUAL(nums.first, 3ul);
    CPPUNIT_ASSERT_EQUAL(nums.second, arr0.byteSize() + arr1.byteSize() + arr2.byteSize());

    // After copy of the data, pointers differ
    CPPUNIT_ASSERT(arr0.getData<short>() != h.get<NDArray>("arr0").getData<short>());
    CPPUNIT_ASSERT(arr1.getData<int>() != h.get<NDArray>("b.arr1").getData<int>());
    CPPUNIT_ASSERT(arr2.getData<float>() != h.get<NDArray>("c.d.arr2").getData<float>());
    // But data stays equal
    for (size_t i = 0; i < 12; ++i) {
        CPPUNIT_ASSERT_EQUAL(h.get<NDArray>("arr0").getData<short>()[i], static_cast<short>(42));
        CPPUNIT_ASSERT_EQUAL(h.get<NDArray>("b.arr1").getData<int>()[i], -42);
        CPPUNIT_ASSERT_EQUAL(h.get<NDArray>("c.d.arr2").getData<float>()[i], 3.14f);
    }
}
