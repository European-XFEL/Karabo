/*
 *
 * File:   Epochstamp_Test.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 19, 2013, 4:47:47 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef TIMESTAMP_TEST_HH
#define	TIMESTAMP_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Epochstamp_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(Epochstamp_Test);

    CPPUNIT_TEST(testConstructors);
    //
    CPPUNIT_TEST(testToIso8601String);
    CPPUNIT_TEST(testToIso8601StringExternal);
    CPPUNIT_TEST(testToFormattedString);
    //
    CPPUNIT_TEST(testStrToEpochstamp);

    CPPUNIT_TEST_SUITE_END();

public:
    Epochstamp_Test();
    virtual ~Epochstamp_Test();
    void setUp();
    void tearDown();

private:
    void testConstructors();
    //
    void testToIso8601String();
    void testToIso8601StringExternal();
    void testToFormattedString();
    //
    void testStrToEpochstamp();

};

#endif	/* TIMESTAMP_TEST_HH */

