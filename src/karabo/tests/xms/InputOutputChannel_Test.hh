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
    CPPUNIT_TEST(testAsyncUpdate1a);
    CPPUNIT_TEST(testAsyncUpdate1b);
    CPPUNIT_TEST(testAsyncUpdate2a);
    CPPUNIT_TEST(testAsyncUpdate2b);
    CPPUNIT_TEST(testAsyncUpdate3a);
    CPPUNIT_TEST(testAsyncUpdate3b);
    CPPUNIT_TEST(testAsyncUpdate4a);
    CPPUNIT_TEST(testAsyncUpdate4b);
    CPPUNIT_TEST(testAsyncUpdate5a);
    CPPUNIT_TEST(testAsyncUpdate5b);
    CPPUNIT_TEST(testAsyncUpdate6a);
    CPPUNIT_TEST(testAsyncUpdate6b);

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
    void testInputHandler();
    void testOutputPreparation();
    void testSchemaValidation();
    void testConnectHandler();
    void testWriteUpdateFlags();
    void testAsyncUpdate1a();
    void testAsyncUpdate1b();
    void testAsyncUpdate2a();
    void testAsyncUpdate2b();
    void testAsyncUpdate3a();
    void testAsyncUpdate3b();
    void testAsyncUpdate4a();
    void testAsyncUpdate4b();
    void testAsyncUpdate5a();
    void testAsyncUpdate5b();
    void testAsyncUpdate6a();
    void testAsyncUpdate6b();

    void testAsyncUpdate(const std::string& onSlowness, const std::string& dataDistribution,
                         const std::string& memoryLocation);
};

#endif /* INPUTOUTPUTCHANNEL_TEST_HH */
