/*
 * File:   AlarmCondition_Test.hh
 * Author: haufs
 *
 * Created on Jun 9, 2016, 9:42:54 AM
 */

#ifndef ALARMCONDITION_TEST_HH
#define	ALARMCONDITION_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class AlarmCondition_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(AlarmCondition_Test);

    CPPUNIT_TEST(testStringAssignmentRoundTrip);
    CPPUNIT_TEST(testSignificanceEvaluation);
    CPPUNIT_TEST(testValidationConditionalRoundTrip);
    CPPUNIT_TEST(testValidation);


    CPPUNIT_TEST_SUITE_END();

public:
    AlarmCondition_Test();
    virtual ~AlarmCondition_Test();
    void setUp();
    void tearDown();

private:
    void testStringAssignmentRoundTrip();
    void testSignificanceEvaluation();
    void testValidationConditionalRoundTrip();
    void testValidation();

};

#endif	/* ALARMCONDITION_TEST_HH */

