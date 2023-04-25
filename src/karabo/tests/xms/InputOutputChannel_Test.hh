/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   InputOutputChannel_Test.hh
 * Author: flucke
 *
 * Created on November 8, 2016, 3:54 PM
 */

#ifndef INPUTOUTPUTCHANNEL_TEST_HH
#define INPUTOUTPUTCHANNEL_TEST_HH

#include <cppunit/extensions/HelperMacros.h>


class InputOutputChannel_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(InputOutputChannel_Test);
    CPPUNIT_TEST(testOutputChannelElement);
    CPPUNIT_TEST(testConnectDisconnect);
    CPPUNIT_TEST(testManyToOne);
    CPPUNIT_TEST(testConcurrentConnect);
    CPPUNIT_TEST(testOutputPreparation);
    CPPUNIT_TEST(testConnectHandler);
    CPPUNIT_TEST(testWriteUpdateFlags);
    CPPUNIT_TEST_SUITE_END();

   public:
    InputOutputChannel_Test();
    void setUp();
    void tearDown();

   private:
    void testOutputChannelElement();
    void testConnectDisconnect();
    void testManyToOne();
    void testConcurrentConnect();
    void testOutputPreparation();
    void testConnectHandler();
    void testWriteUpdateFlags();
};

#endif /* INPUTOUTPUTCHANNEL_TEST_HH */
