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
 * File:   InputOutputChannel_LongTest.hh
 * Author: gero.flucke@xfel.eu
 *
 * Created on May 2019
 */

#ifndef INPUTOUTPUTCHANNEL_LONGTEST_HH
#define INPUTOUTPUTCHANNEL_LONGTEST_HH

#include <cppunit/extensions/HelperMacros.h>

class InputOutputChannel_LongTest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(InputOutputChannel_LongTest);
    CPPUNIT_TEST(testDisconnectWhileSending1);
    CPPUNIT_TEST(testDisconnectWhileSending2);
    CPPUNIT_TEST(testDisconnectWhileSending3);
    CPPUNIT_TEST(testDisconnectWhileSending3a);
    CPPUNIT_TEST(testDisconnectWhileSending4);
    CPPUNIT_TEST(testDisconnectWhileSending5);
    CPPUNIT_TEST(testDisconnectWhileSending6);
    CPPUNIT_TEST(testDisconnectWhileSending6a);
    CPPUNIT_TEST(testDisconnectWhileSending7);
    CPPUNIT_TEST(testDisconnectWhileSending8);
    CPPUNIT_TEST(testDisconnectWhileSending9);
    CPPUNIT_TEST(testDisconnectWhileSending9a);
    CPPUNIT_TEST_SUITE_END();

   public:
    InputOutputChannel_LongTest();
    void setUp();
    void tearDown();

   private:
    void testDisconnectWhileSending1();
    void testDisconnectWhileSending2();
    void testDisconnectWhileSending3();
    void testDisconnectWhileSending3a();
    void testDisconnectWhileSending4();
    void testDisconnectWhileSending5();
    void testDisconnectWhileSending6();
    void testDisconnectWhileSending6a();
    void testDisconnectWhileSending7();
    void testDisconnectWhileSending8();
    void testDisconnectWhileSending9();
    void testDisconnectWhileSending9a();

    void testDisconnectWhileSending_impl(const std::string& sender_dataDistribution,
                                         const std::string& sender_onSlowness,
                                         const std::string& receiver_noInputShared,
                                         bool registerRoundRobinSelector = false);
};

#endif /* INPUTOUTPUTCHANNEL_TEST_HH */
