/*
 *
 * File:   Timestamp_Test.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 19, 2013, 4:47:47 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef TIMESTAMP_TEST_HH
#define	TIMESTAMP_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Timestamp_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(Timestamp_Test);

    CPPUNIT_TEST(testConstructors);
    CPPUNIT_TEST(testToString);
    CPPUNIT_TEST(testToIsoString);
    CPPUNIT_TEST(testToFormattedString);
    CPPUNIT_TEST(testCalculateAndReturnMsSinceEpoch);
    CPPUNIT_TEST(testStrToTimestamp);

    CPPUNIT_TEST_SUITE_END();

public:
    Timestamp_Test();
    virtual ~Timestamp_Test();
    void setUp();
    void tearDown();

private:
    void testConstructors();
    void testToString();
    void testToIsoString();
    void testToFormattedString();
    //
    void testCalculateAndReturnMsSinceEpoch();
    void testStrToTimestamp();

};

#endif	/* TIMESTAMP_TEST_HH */

