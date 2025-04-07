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
 * File:   HashFilter_Test.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on April 12, 2013, 11:56 AM
 */

#ifndef HASHFILTER_TEST_HH
#define HASHFILTER_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

// #include "karabo/data/types/ClassInfo.hh"

// #define HASHFILTER_HDF5TEST

class HashFilter_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(HashFilter_Test);
    CPPUNIT_TEST(testFilterByTag);
    CPPUNIT_TEST(testFilterByAccessMode);
#ifdef HASHFILTER_HDF5TEST
    CPPUNIT_TEST(testHdf5Filter);
#endif
    CPPUNIT_TEST_SUITE_END();

   public:
    // KARABO_CLASSINFO(HashFilter_Test, "HashFilter_Test", "1.0");

    HashFilter_Test();
    virtual ~HashFilter_Test();
    void setUp();
    void tearDown();

   private:
    void testFilterByTag();
    void testFilterByAccessMode();
#ifdef HASHFILTER_HDF5TEST
    void testHdf5Filter();
#endif
};

#endif /* FILTER_TEST_HH */
