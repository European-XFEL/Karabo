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
 * File:   Serializable_Test.h
 * Author: heisenb
 *
 * Created on August 5, 2016, 12:13 PM
 */

#ifndef SERIALIZABLE_TEST_H
#define SERIALIZABLE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/data/types/Hash.hh"

class FancyData : protected karabo::data::Hash {
   public:
    typedef karabo::data::Hash type;

    KARABO_CLASSINFO(FancyData, "FancyData", "1.0");

    void setScalar(const int value);

    const int& getScalar() const;
};

class Serializable_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Serializable_Test);
    CPPUNIT_TEST(testMethod);

    CPPUNIT_TEST_SUITE_END();

   public:
    Serializable_Test();
    virtual ~Serializable_Test();

   private:
    void testMethod();
};

#endif /* SERIALIZABLE_TEST_H */
