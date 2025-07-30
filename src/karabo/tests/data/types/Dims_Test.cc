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
 * File:   Dims_Test.cc
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on February 19, 2013, 1:33 PM
 */

#include "Dims_Test.hh"

#include <boost/shared_array.hpp>
#include <karabo/util/ArrayTools.hh>

#include "karabo/data/types/Dims.hh"


using namespace karabo::data;
using namespace karabo::util;
using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION(Dims_Test);


Dims_Test::Dims_Test() {}


Dims_Test::~Dims_Test() {}


void Dims_Test::setUp() {}


void Dims_Test::tearDown() {}


void Dims_Test::testDims() {
    Dims a0;
    CPPUNIT_ASSERT(a0.rank() == 0);
    CPPUNIT_ASSERT(a0.size() == 0);

    Dims a1(1);
    CPPUNIT_ASSERT(a1.rank() == 1);
    CPPUNIT_ASSERT(a1.size() == 1);

    Dims a2(1, 1);
    CPPUNIT_ASSERT(a2.rank() == 2);
    CPPUNIT_ASSERT(a2.size() == 1);


    Dims a(2, 12);
    CPPUNIT_ASSERT(a.rank() == 2);
    CPPUNIT_ASSERT(a.size() == 24);
    CPPUNIT_ASSERT(a.extentIn(0) == 2);
    CPPUNIT_ASSERT(a.extentIn(1) == 12);

    Dims b = a;
    CPPUNIT_ASSERT(b.rank() == 2);
    CPPUNIT_ASSERT(b.size() == 24);
    CPPUNIT_ASSERT(b.extentIn(0) == 2);
    CPPUNIT_ASSERT(b.extentIn(1) == 12);

    Dims c(a);
    CPPUNIT_ASSERT(c.rank() == 2);
    CPPUNIT_ASSERT(c.size() == 24);
    CPPUNIT_ASSERT(c.extentIn(0) == 2);
    CPPUNIT_ASSERT(c.extentIn(1) == 12);

    vector<unsigned long long> vec(5, 2);
    vec[1] = 4;
    vec[3] = 10;
    vec[4] = 3;
    Dims d(vec);
    CPPUNIT_ASSERT(d.rank() == 5);
    CPPUNIT_ASSERT(d.size() == 480);
    CPPUNIT_ASSERT(d.extentIn(0) == 2);
    CPPUNIT_ASSERT(d.extentIn(1) == 4);
    CPPUNIT_ASSERT(d.extentIn(2) == 2);
    CPPUNIT_ASSERT(d.extentIn(3) == 10);
    CPPUNIT_ASSERT(d.extentIn(4) == 3);

    // Test operators
    CPPUNIT_ASSERT(a == c);
    CPPUNIT_ASSERT(!(a != c));
    CPPUNIT_ASSERT(c != d);
    CPPUNIT_ASSERT(!(c == d));

    std::ostringstream oss;
    oss << d;
    CPPUNIT_ASSERT_EQUAL(std::string("(2,4,2,10,3)"), oss.str());

    oss.str(std::string());
    oss << Dims();
    CPPUNIT_ASSERT_EQUAL(std::string("()"), oss.str());
}


void Dims_Test::testArrayTools() {
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
    CPPUNIT_ASSERT(dimsDD.rank() == 2);
    CPPUNIT_ASSERT(dimsDD.size() == 60);
    CPPUNIT_ASSERT(dimsDD.extentIn(0) == 10);
    CPPUNIT_ASSERT(dimsDD.extentIn(1) == 6);


    for (size_t i = 0; i < dimsDD.size(); ++i) {
        CPPUNIT_ASSERT(dd[i] == 100 + i);
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
        CPPUNIT_ASSERT(*aaPtr == 287);
        CPPUNIT_ASSERT(dd.rank() == 0);
        CPPUNIT_ASSERT(dd.size() == 0);
    }
}
