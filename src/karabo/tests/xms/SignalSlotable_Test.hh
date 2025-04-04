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
 * File:   SignalSlotable_Test.hh
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:21 PM
 */

#ifndef SIGNALSLOTABLE_TEST_HH
#define SIGNALSLOTABLE_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/shared_ptr.hpp>

#include "karabo/data/types/Hash.hh"

class SignalSlotDemo;

class SignalSlotable_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(SignalSlotable_Test);

    CPPUNIT_TEST(testUniqueInstanceId);
    CPPUNIT_TEST(testValidInstanceId);
    CPPUNIT_TEST(testReceiveAsync);
    CPPUNIT_TEST(testReceiveAsyncError);
    CPPUNIT_TEST(testReceiveAsyncNoReply);
    CPPUNIT_TEST(testReceiveExceptions);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testConnectAsync);
    CPPUNIT_TEST(testConnectAsyncMulti);
    CPPUNIT_TEST(testDisconnectAsync);
    CPPUNIT_TEST(testDisconnectConnectAsyncStress);
    CPPUNIT_TEST(testAsyncReply);
    CPPUNIT_TEST(testAutoConnectSignal);
    CPPUNIT_TEST(testAutoConnectSlot);
    CPPUNIT_TEST(testRegisterSlotTwice);
    CPPUNIT_TEST(testAsyncConnectInputChannel);
    CPPUNIT_TEST(testUuid);

    CPPUNIT_TEST_SUITE_END();

   public:
    SignalSlotable_Test();
    virtual ~SignalSlotable_Test();
    void setUp();
    void tearDown();

   private:
    void waitDemoOk(const std::shared_ptr<SignalSlotDemo>& demo, int messageCalls, int trials = 10);
    void _loopFunction(const std::string& functionName, const std::function<void()>& testFunction);
    void testUniqueInstanceId();
    void testValidInstanceId();
    void testReceiveAsync();
    void testReceiveAsyncError();
    void testReceiveAsyncNoReply();
    void testReceiveExceptions();
    void testMethod();
    void testConnectAsync();
    void testConnectAsyncMulti();
    void testDisconnectAsync();
    void testDisconnectConnectAsyncStress();
    void testAsyncReply();
    void testAutoConnectSignal();
    void testAutoConnectSlot();
    void testRegisterSlotTwice();
    void testAsyncConnectInputChannel();
    void testUuid();
    void _testUniqueInstanceId();
    void _testValidInstanceId();
    void _testReceiveAsync();
    void _testReceiveAsyncError();
    void _testReceiveAsyncNoReply();
    void _testReceiveExceptions();
    void _testMethod();
    void _testConnectAsync();
    void _testConnectAsyncMulti();
    void _testDisconnectAsync();
    void _testDisconnectConnectAsyncStress();
    void _testAsyncReply();
    void _testAutoConnectSignal();
    void _testAutoConnectSlot();
    void _testRegisterSlotTwice();
    void _testAsyncConnectInputChannel();


    std::string m_karaboBrokerBackup;
    std::string m_amqpTimeoutBackup;
    // using a Karabo Hash to match the insertion order.
    karabo::data::Hash m_brokersUnderTest;
};

#endif /* SIGNALSLOTABLE_TEST_HH */
