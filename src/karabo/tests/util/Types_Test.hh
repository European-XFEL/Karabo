/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Types_Test.hh
 * Author: heisenb
 *
 * Created on February 27, 2013, 3:09 PM
 */

#ifndef TYPES_TEST_HH
#define TYPES_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Types_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Types_Test);
    CPPUNIT_TEST(testIsPointer);
    CPPUNIT_TEST(testIsVector);
    CPPUNIT_TEST(testIsSimple);
    CPPUNIT_TEST(testCategory);
    CPPUNIT_TEST(testFrom);
    CPPUNIT_TEST(testTo);
    CPPUNIT_TEST(testConvert);
    CPPUNIT_TEST_SUITE_END();

   public:
    Types_Test();
    virtual ~Types_Test();
    void setUp();
    void tearDown();

   private:
    void testCategory();
    void testFrom();
    void testTo();
    void testConvert();
    void testIsPointer();
    void testIsVector();
    void testIsSimple();
};

#endif /* TYPES_TEST_HH */
