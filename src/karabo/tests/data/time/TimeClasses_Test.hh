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
 * File:   TimeClasses_Test.hh
 * Author: boukhele
 *
 * Created on July 10, 2013, 2:35 PM
 */

#ifndef TIMECLASSES_TEST_HH
#define TIMECLASSES_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>

#include "karabo/data/schema/Configurator.hh"

class TimeClasses_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TimeClasses_Test);
    CPPUNIT_TEST(testEpochstamp);
    CPPUNIT_TEST(testEpochstampConversion);
    CPPUNIT_TEST(testTimePeriod);
    CPPUNIT_TEST(testTimeDuration);
    CPPUNIT_TEST(testTimeProfiler);
    CPPUNIT_TEST(testTimeId);
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
    void testTimeId();
    void testTimestamp();
};

#endif /* TIMECLASSES_TEST_HH */
