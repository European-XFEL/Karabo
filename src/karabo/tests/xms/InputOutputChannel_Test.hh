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
    CPPUNIT_TEST(testInputHandler);
    CPPUNIT_TEST(testOutputPreparation);
    CPPUNIT_TEST(testSchemaValidation);
    CPPUNIT_TEST(testConnectHandler);
    CPPUNIT_TEST(testWriteUpdateFlags);
    CPPUNIT_TEST(testAsyncUpdate1a1);
    CPPUNIT_TEST(testAsyncUpdate1a2);
    CPPUNIT_TEST(testAsyncUpdate1b0);
    CPPUNIT_TEST(testAsyncUpdate2a1);
    CPPUNIT_TEST(testAsyncUpdate2a2);
    CPPUNIT_TEST(testAsyncUpdate2b1);
    CPPUNIT_TEST(testAsyncUpdate2b2);
    CPPUNIT_TEST(testAsyncUpdate3a1);
    CPPUNIT_TEST(testAsyncUpdate3a2);
    CPPUNIT_TEST(testAsyncUpdate3b0);
    CPPUNIT_TEST(testAsyncUpdate4a1);
    CPPUNIT_TEST(testAsyncUpdate4a2);
    CPPUNIT_TEST(testAsyncUpdate4b0);
    CPPUNIT_TEST(testAsyncUpdate5a1);
    CPPUNIT_TEST(testAsyncUpdate5a2);
    CPPUNIT_TEST(testAsyncUpdate5b1);
    CPPUNIT_TEST(testAsyncUpdate5b2);
    CPPUNIT_TEST(testAsyncUpdate6a1);
    CPPUNIT_TEST(testAsyncUpdate6a2);
    CPPUNIT_TEST(testAsyncUpdate6b0);

    CPPUNIT_TEST_SUITE_END();

   public:
    InputOutputChannel_Test();
    void setUp();
    void tearDown();

   private:
    static bool m_calledTestAsyncUpdate;

    void testOutputChannelElement();
    void testConnectDisconnect();
    void testManyToOne();
    void testConcurrentConnect();
    void testInputHandler();
    void testOutputPreparation();
    void testSchemaValidation();
    void testConnectHandler();
    void testWriteUpdateFlags();
    void testAsyncUpdate1a1();
    void testAsyncUpdate1a2();
    void testAsyncUpdate1b0();
    void testAsyncUpdate2a1();
    void testAsyncUpdate2a2();
    void testAsyncUpdate2b1();
    void testAsyncUpdate2b2();
    void testAsyncUpdate3a1();
    void testAsyncUpdate3a2();
    void testAsyncUpdate3b0();
    void testAsyncUpdate4a1();
    void testAsyncUpdate4a2();
    void testAsyncUpdate4b0();
    void testAsyncUpdate5a1();
    void testAsyncUpdate5a2();
    void testAsyncUpdate5b1();
    void testAsyncUpdate5b2();
    void testAsyncUpdate6a1();
    void testAsyncUpdate6a2();
    void testAsyncUpdate6b0();

    void testAsyncUpdate(const std::string& onSlowness, const std::string& dataDistribution,
                         const std::string& memoryLocation, bool safeNDArray);
};

#endif /* INPUTOUTPUTCHANNEL_TEST_HH */
