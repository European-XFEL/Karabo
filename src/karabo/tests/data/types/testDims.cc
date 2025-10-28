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
 * File:   Dims_Test.hh
 * Author: wrona
 *
 * Created on February 19, 2013, 1:33 PM
 */

#include <gtest/gtest.h>

#include <boost/shared_array.hpp>
#include <karabo/util/ArrayTools.hh>

#include "karabo/data/types/Dims.hh"


using namespace karabo::data;
using namespace karabo::util;
using namespace std;


TEST(TestDims, testDims) {
    Dims a0;
    EXPECT_TRUE(a0.rank() == 0);
    EXPECT_TRUE(a0.size() == 0);

    Dims a1(1);
    EXPECT_TRUE(a1.rank() == 1);
    EXPECT_TRUE(a1.size() == 1);

    Dims a2(1, 1);
    EXPECT_TRUE(a2.rank() == 2);
    EXPECT_TRUE(a2.size() == 1);


    Dims a(2, 12);
    EXPECT_TRUE(a.rank() == 2);
    EXPECT_TRUE(a.size() == 24);
    EXPECT_TRUE(a.extentIn(0) == 2);
    EXPECT_TRUE(a.extentIn(1) == 12);

    Dims b = a;
    EXPECT_TRUE(b.rank() == 2);
    EXPECT_TRUE(b.size() == 24);
    EXPECT_TRUE(b.extentIn(0) == 2);
    EXPECT_TRUE(b.extentIn(1) == 12);

    Dims c(a);
    EXPECT_TRUE(c.rank() == 2);
    EXPECT_TRUE(c.size() == 24);
    EXPECT_TRUE(c.extentIn(0) == 2);
    EXPECT_TRUE(c.extentIn(1) == 12);

    vector<unsigned long long> vec(5, 2);
    vec[1] = 4;
    vec[3] = 10;
    vec[4] = 3;
    Dims d(vec);
    EXPECT_TRUE(d.rank() == 5);
    EXPECT_TRUE(d.size() == 480);
    EXPECT_TRUE(d.extentIn(0) == 2);
    EXPECT_TRUE(d.extentIn(1) == 4);
    EXPECT_TRUE(d.extentIn(2) == 2);
    EXPECT_TRUE(d.extentIn(3) == 10);
    EXPECT_TRUE(d.extentIn(4) == 3);

    // Test operators
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(!(a != c));
    EXPECT_TRUE(c != d);
    EXPECT_TRUE(!(c == d));

    std::ostringstream oss;
    oss << d;
    EXPECT_EQ(std::string("(2,4,2,10,3)"), oss.str());

    oss.str(std::string());
    oss << Dims();
    EXPECT_EQ(std::string("()"), oss.str());
}


TEST(TestDims, testArrayTools) {
    Dims dimsD(10, 6);
    // boost::shared_array<unsigned short> d = boost::shared_array<unsigned short>(new unsigned short[dimsD.size()]);
    std::shared_ptr<unsigned short[]> d = std::shared_ptr<unsigned short[]>(new unsigned short[dimsD.size()]);
    for (size_t i = 0; i < dimsD.size(); ++i) {
        d[i] = 100 + i;
    }


    Dims dimsDD;
    unsigned short* dd;

    {
        Hash data;
        addPointerToHash(data, "d", d.get(), dimsD);
        getPointerFromHash(data, "d", dd, dimsDD);
    }

    // clog << "rank: " << dimsDD.rank() << endl;
    EXPECT_TRUE(dimsDD.rank() == 2);
    EXPECT_TRUE(dimsDD.size() == 60);
    EXPECT_TRUE(dimsDD.extentIn(0) == 10);
    EXPECT_TRUE(dimsDD.extentIn(1) == 6);


    for (size_t i = 0; i < dimsDD.size(); ++i) {
        EXPECT_TRUE(dd[i] == 100 + i);
    }


    {
        Dims d, dd;
        int a = 287;
        int* aPtr = &a;
        int* aaPtr = 0;
        {
            Hash data;
            addPointerToHash(data, "a", aPtr, d);
            getPointerFromHash(data, "a", aaPtr, dd);
        }
        // clog << "aaPtr:  " << aaPtr << " value: " << *aaPtr << endl;
        EXPECT_TRUE(*aaPtr == 287);
        EXPECT_TRUE(dd.rank() == 0);
        EXPECT_TRUE(dd.size() == 0);
    }
}
