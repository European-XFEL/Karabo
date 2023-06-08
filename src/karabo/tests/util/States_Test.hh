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
 * File:   States_Test.hh
 * Author: haufs
 *
 * Created on July 8, 2016, 10:08 AM
 */

#ifndef KARABO_STATES_TEST_HH
#define KARABO_STATES_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class States_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(States_Test);
    CPPUNIT_TEST(testStringRoundTrip);
    CPPUNIT_TEST(testSignifier);
    CPPUNIT_TEST(testSignifierInitTrump);
    CPPUNIT_TEST(testSignifierNonDefaultList);
    CPPUNIT_TEST(testRunningTrumpActivePassive);
    CPPUNIT_TEST(testChainStatesPassive);
    CPPUNIT_TEST(testChainStatesActive);
    CPPUNIT_TEST(testComparisons);

    // Tests that were translated from their Python counterparts
    // in '...pythonKarabo/karabo/common/tests/test_states.py'.
    CPPUNIT_TEST(testStatesSignifierDefault);
    CPPUNIT_TEST(testStatesSignifierActiveDecreasing);
    CPPUNIT_TEST(testStatesSignifierPassiveIncreasing);
    CPPUNIT_TEST(testStatesSignifierActiveIncreasing);
    CPPUNIT_TEST(testAcquiringChangingOnPassive);
    CPPUNIT_TEST(testAcquiringChangingOnActive);
    CPPUNIT_TEST(testStatesSignifierNonDefList);

    CPPUNIT_TEST_SUITE_END();


   public:
    States_Test();
    virtual ~States_Test();
    void setUp();
    void tearDown();

   private:
    void testStringRoundTrip();
    void testSignifier();
    void testSignifierInitTrump();
    void testInterlockTrump();
    void testSignifierNonDefaultList();
    void testRunningTrumpActivePassive();
    void testComparisons();
    void testChainStatesPassive();
    void testChainStatesActive();

    // Test cases to match the ones for Bound-Python.
    // The cases are named after their Python counterparts.
    void testStatesSignifierDefault();
    void testStatesSignifierActiveDecreasing();
    void testStatesSignifierPassiveIncreasing();
    void testStatesSignifierActiveIncreasing();
    void testAcquiringChangingOnPassive();
    void testAcquiringChangingOnActive();
    void testStatesSignifierNonDefList();
};

#endif /* SKARABO_TATES_TEST_HH */
