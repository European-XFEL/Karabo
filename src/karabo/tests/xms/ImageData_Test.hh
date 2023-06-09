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
 * File:   ImageData_Test.hh
 * Author: parenti
 *
 * Created on May 21, 2015, 3:06:21 PM
 */

#ifndef IMAGEDATA_TEST_HH
#define IMAGEDATA_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/xms.hpp>

class ImageData_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(ImageData_Test);
    CPPUNIT_TEST(testConstructor);
    CPPUNIT_TEST(testSetAndGetMethods);
    CPPUNIT_TEST(testImageDataElement);
    CPPUNIT_TEST_SUITE_END();

   public:
    ImageData_Test();
    virtual ~ImageData_Test();
    void setUp();
    void tearDown();

   private:
    void testConstructor();

    void testSetAndGetMethods();

    void testImageDataElement();
};

#endif /* IMAGEDATA_TEST_HH */
