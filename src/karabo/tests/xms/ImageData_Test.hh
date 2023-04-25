/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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
