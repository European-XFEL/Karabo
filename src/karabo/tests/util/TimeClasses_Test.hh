/* 
 * File:   TimeClasses_Test.hh
 * Author: boukhele
 *
 * Created on July 10, 2013, 2:35 PM
 */

#ifndef TIMECLASSES_TEST_HH
#define	TIMECLASSES_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class TimeClasses_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(TimeClasses_Test);
    CPPUNIT_TEST(testEpochstamp);
    CPPUNIT_TEST(testTimePeriod);
    CPPUNIT_TEST(testTimeDuration);
    CPPUNIT_TEST(testTimeProfiler);
    CPPUNIT_TEST_SUITE_END();

public:
    TimeClasses_Test();
    virtual ~TimeClasses_Test();
    void setUp();
    void tearDown();

private:
    void testEpochstamp();
    void testTimePeriod();
    void testTimeDuration();
    void testTimeProfiler();
};

#endif	/* TIMECLASSES_TEST_HH */

