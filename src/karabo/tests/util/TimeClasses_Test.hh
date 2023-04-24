/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   TimeClasses_Test.hh
 * Author: boukhele
 *
 * Created on July 10, 2013, 2:35 PM
 */

#ifndef TIMECLASSES_TEST_HH
#define TIMECLASSES_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>

class TimeClasses_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TimeClasses_Test);
    CPPUNIT_TEST(testEpochstamp);
    CPPUNIT_TEST(testEpochstampConversion);
    CPPUNIT_TEST(testTimePeriod);
    CPPUNIT_TEST(testTimeDuration);
    CPPUNIT_TEST(testTimeProfiler);
    CPPUNIT_TEST(testTrainstamp);
    CPPUNIT_TEST(testTimestamp);
    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(TimeClasses_Test, "TimeClasses_Test", "1.0");

    TimeClasses_Test();
    virtual ~TimeClasses_Test();
    void setUp();
    void tearDown();

   private:
    void testEpochstamp();
    void testEpochstampConversion();
    void testTimePeriod();
    void testTimeDuration();
    void testTimeProfiler();
    void testTrainstamp();
    void testTimestamp();
};

#endif /* TIMECLASSES_TEST_HH */
