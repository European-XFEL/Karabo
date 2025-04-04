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
 * File:   Serializable_Test.cc
 * Author: heisenb
 *
 * Created on August 5, 2016, 12:13 PM
 */

#include "Serializable_Test.h"

#include "karabo/data/types/Hash.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Serializable_Test);

using namespace karabo::data;
using namespace std;


void FancyData::setScalar(const int value) {
    set("scalar", value);
}


const int& FancyData::getScalar() const {
    return get<int>("scalar");
}


Serializable_Test::Serializable_Test() {}


Serializable_Test::~Serializable_Test() {}


void Serializable_Test::testMethod() {
    FancyData fd1;
    fd1.setScalar(2);

    Hash h;
    h.set("fd1", fd1); // Here the original object on the stack is copied
    h.set("h1", Hash("someRegular", "hash"));

    CPPUNIT_ASSERT(h.get<FancyData>("fd1").getScalar() == 2);

    // The classId is automatically added as attribute
    CPPUNIT_ASSERT(h.getAttribute<string>("fd1", KARABO_HASH_CLASS_ID) == "FancyData");

    // This doesn't not happen for plain nested hashes
    CPPUNIT_ASSERT(h.hasAttribute("h1", KARABO_HASH_CLASS_ID) == false);

    h.get<FancyData>("fd1").setScalar(-2);
    CPPUNIT_ASSERT(fd1.getScalar() == 2);

    // Here a copy is done
    FancyData fd2 = h.get<FancyData>("fd1");
    fd2.setScalar(1);

    CPPUNIT_ASSERT(fd2.getScalar() == 1);
    CPPUNIT_ASSERT(h.get<FancyData>("fd1").getScalar() == -2);

    // But the one in the hash
    const FancyData& fd3 = h.get<FancyData>("fd1");
    CPPUNIT_ASSERT(fd3.getScalar() == -2);
}
