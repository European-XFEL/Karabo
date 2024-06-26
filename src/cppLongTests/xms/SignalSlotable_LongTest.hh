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
 * File:   SignalSlotable_LongTest.hh
 *
 * Created on Apr 4, 2013, 1:24:21 PM
 */

#ifndef SIGNALSLOTABLE_LONGTEST_HH
#define SIGNALSLOTABLE_LONGTEST_HH

#include <cppunit/extensions/HelperMacros.h>

class SignalSlotable_LongTest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(SignalSlotable_LongTest);

    CPPUNIT_TEST(testStressSyncReplies);
    CPPUNIT_TEST(testStressAsyncReplies);

    CPPUNIT_TEST_SUITE_END();

   public:
    SignalSlotable_LongTest();
    virtual ~SignalSlotable_LongTest();
    void setUp();
    void tearDown();

   private:
    void testStressSyncReplies();
    void testStressAsyncReplies();
};

#endif /* SIGNALSLOTABLE_LONGTEST_HH */
