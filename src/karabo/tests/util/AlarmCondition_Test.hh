/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   AlarmCondition_Test.hh
 * Author: haufs
 *
 * Created on Jun 9, 2016, 9:42:54 AM
 */

#ifndef ALARMCONDITION_TEST_HH
#define ALARMCONDITION_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>

class AlarmCondition_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(AlarmCondition_Test);

    CPPUNIT_TEST(testOperators);
    CPPUNIT_TEST(testStringAssignmentRoundTrip);
    CPPUNIT_TEST(testSignificanceEvaluation);
    CPPUNIT_TEST(testValidationConditionalRoundTrip);
    CPPUNIT_TEST(testValidation);


    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(AlarmCondition_Test, "AlarmCondition_Test", "1.0");

    AlarmCondition_Test();
    virtual ~AlarmCondition_Test();
    void setUp();
    void tearDown();

   private:
    void testOperators();
    void testStringAssignmentRoundTrip();
    void testSignificanceEvaluation();
    void testValidationConditionalRoundTrip();
    void testValidation();
};

#endif /* ALARMCONDITION_TEST_HH */
