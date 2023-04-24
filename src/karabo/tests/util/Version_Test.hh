/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Version_Test.hh
 * Author: cas
 *
 * Created on February 11, 2016, 2:23 PM
 */

#ifndef VERSION_TEST_HH
#define VERSION_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Version_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Version_Test);
    CPPUNIT_TEST(testVersion);
    CPPUNIT_TEST(testVersionFromString);
    CPPUNIT_TEST(testVersionComparison);
    CPPUNIT_TEST_SUITE_END();

   public:
    Version_Test();
    virtual ~Version_Test();

   private:
    void testVersion();
    void testVersionFromString();
    void testVersionComparison();
};

#endif /* VERSION_TEST_HH */
