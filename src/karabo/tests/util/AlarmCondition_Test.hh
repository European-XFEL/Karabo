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
 * File:   AlarmCondition_Test.hh
 * Author: haufs
 *
 * Created on Jun 9, 2016, 9:42:54 AM
 */

#ifndef ALARMCONDITION_TEST_HH
#define ALARMCONDITION_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>

#include "karabo/data/schema/Configurator.hh"

class AlarmCondition_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(AlarmCondition_Test);

    CPPUNIT_TEST(testOperators);
    CPPUNIT_TEST(testStringAssignmentRoundTrip);
    CPPUNIT_TEST(testSignificanceEvaluation);

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
