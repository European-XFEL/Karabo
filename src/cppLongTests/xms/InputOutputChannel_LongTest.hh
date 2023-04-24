/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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
    CPPUNIT_TEST(testDisconnectWhileSending4);
    CPPUNIT_TEST(testDisconnectWhileSending5);
    CPPUNIT_TEST(testDisconnectWhileSending6);
    CPPUNIT_TEST(testDisconnectWhileSending7);
    CPPUNIT_TEST(testDisconnectWhileSending8);
    CPPUNIT_TEST(testDisconnectWhileSending9);
    CPPUNIT_TEST_SUITE_END();

   public:
    InputOutputChannel_LongTest();
    void setUp();
    void tearDown();

   private:
    void testDisconnectWhileSending1();
    void testDisconnectWhileSending2();
    void testDisconnectWhileSending3();
    void testDisconnectWhileSending4();
    void testDisconnectWhileSending5();
    void testDisconnectWhileSending6();
    void testDisconnectWhileSending7();
    void testDisconnectWhileSending8();
    void testDisconnectWhileSending9();

    void testDisconnectWhileSending_impl(const std::string& sender_dataDistribution,
                                         const std::string& sender_onSlowness,
                                         const std::string& receiver_distributionMode,
                                         const std::string& receiver_noInputShared);
};

#endif /* INPUTOUTPUTCHANNEL_TEST_HH */
