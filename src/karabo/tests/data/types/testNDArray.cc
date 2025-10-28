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
 * File:   NDArray_Test.hh
 * Author: parenti
 *
 * Created on May 22, 2015, 11:41:21 AM
 */

#include <gtest/gtest.h>

#include <karabo/data/types/NDArray.hh>

#include "karabo/data/types/StringTools.hh"

using namespace std;
using namespace karabo::data;

TEST(TestNDArray, testConstructor) {
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
        EXPECT_THROW(NDArray(someData.begin() + 1, someData.begin()), std::bad_alloc);
        EXPECT_THROW(NDArray(&(someData[1]), &(someData[0])), std::bad_alloc);

        const Dims& flyShape = fly.getShape();
        const Dims& cpyShape = cpy.getShape();
        const Dims& iCpShape = iCp.getShape();
        const Dims& refShape = ref.getShape();

        EXPECT_TRUE(flyShape.x1() == 100);
        EXPECT_TRUE(flyShape.x2() == 200);
        for (int i = 0; i < maxSeries; ++i) {
            EXPECT_EQ(2, fly.getData<int>()[i]);
        }
        EXPECT_TRUE(fly.getData<int>()[124] == 2);
        EXPECT_TRUE(fly.size() == 100 * 200);
        EXPECT_EQ(sizeof(int), fly.itemSize());
        EXPECT_EQ(sizeof(int) * static_cast<size_t>(100 * 200), fly.byteSize());

        EXPECT_TRUE(cpyShape.x1() == 100);
        EXPECT_TRUE(cpyShape.x2() == 200);
        for (int i = 0; i < maxSeries; ++i) {
            EXPECT_EQ(i, cpy.getData<int>()[i]);
        }
        EXPECT_TRUE(cpy.getData<int>()[124] == 2);
        EXPECT_TRUE(cpy.size() == 100 * 200);
        EXPECT_EQ(sizeof(int), cpy.itemSize());
        EXPECT_EQ(sizeof(int) * static_cast<size_t>(100 * 200), cpy.byteSize());

        EXPECT_TRUE(iCpShape.x1() == 100);
        EXPECT_TRUE(iCpShape.x2() == 200);
        for (int i = 0; i < maxSeries; ++i) {
            EXPECT_EQ(i, iCp.getData<int>()[i]);
        }
        EXPECT_TRUE(iCp.getData<int>()[124] == 2);
        EXPECT_TRUE(iCp.size() == 100 * 200);
        EXPECT_EQ(sizeof(int), iCp.itemSize());
        EXPECT_EQ(sizeof(int) * static_cast<size_t>(100 * 200), iCp.byteSize());

        EXPECT_TRUE(refShape.x1() == 100);
        EXPECT_TRUE(refShape.x2() == 200);
        for (int i = 0; i < maxSeries; ++i) {
            EXPECT_EQ(i, ref.getData<int>()[i]);
        }
        EXPECT_TRUE(ref.getData<int>()[124] == 2);
        EXPECT_TRUE(ref.size() == 100 * 200);
        EXPECT_EQ(sizeof(int), ref.itemSize());
        EXPECT_EQ(sizeof(int) * static_cast<size_t>(100 * 200), ref.byteSize());

        // Setting content affects underlying data for ref, but not for cpy and iCp which have copied data:
        cpy.getData<int>()[124] = 0;
        EXPECT_EQ(0, cpy.getData<int>()[124]);
        EXPECT_TRUE(someData[124] == 2);

        iCp.getData<int>()[124] = 0;
        EXPECT_EQ(0, iCp.getData<int>()[124]);
        EXPECT_TRUE(someData[124] == 2);

        ref.getData<int>()[124] = 0;
        EXPECT_EQ(0, ref.getData<int>()[124]);
        EXPECT_TRUE(someData[124] == 0);

        h.set("cpy", cpy);
        h.set("iCp", iCp);
        h.set("ref", ref);
    }

    // What we get from Hash is still a reference to someData:
    NDArray& ref = h.get<NDArray>("ref");
    EXPECT_TRUE(ref.getData<int>()[124] == 0);
    EXPECT_EQ(0, someData[124]);
    ref.getData<int>()[124] = 124;
    EXPECT_EQ(124, someData[124]);
    EXPECT_TRUE(ref.getShape().x1() == 100);
    EXPECT_TRUE(ref.size() == 100 * 200);
}


TEST(TestNDArray, testShapeException) {
    vector<int> data(10, -42);
    const Dims badShape(2, 500);

    { EXPECT_THROW(NDArray(&data[0], data.size(), badShape), karabo::data::ParameterException); }
}


TEST(TestNDArray, testDataTypeException) {
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
    EXPECT_TRUE(exceptionMsg.find("from INT32") != std::string::npos) << exceptionMsg;
    EXPECT_TRUE(exceptionMsg.find("to DOUBLE") != std::string::npos) << exceptionMsg;

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
    EXPECT_TRUE(exceptionMsg.find("from _invalid_") != std::string::npos) << "'from _invalid_'" << msg << exceptionMsg;
    EXPECT_TRUE(exceptionMsg.find(toString(12345678)) != std::string::npos) << "'12345678'" << msg << exceptionMsg;
    EXPECT_TRUE(exceptionMsg.find("to INT16") != std::string::npos) << "'to INT16'" << msg << exceptionMsg;
}
