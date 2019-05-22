/*
 * File:   SignalSlotable_Test.hh
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:21 PM
 */

#ifndef SIGNALSLOTABLE_TEST_HH
#define	SIGNALSLOTABLE_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/shared_ptr.hpp>

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
    CPPUNIT_TEST(testAsyncReply);
    CPPUNIT_TEST(testAutoConnectSignal);
    CPPUNIT_TEST(testAutoConnectSlot);
    CPPUNIT_TEST(testRegisterSlotTwice);
    CPPUNIT_TEST(testAsyncConnectInputChannel);
    CPPUNIT_TEST(testUuid);
    CPPUNIT_TEST(testStressReplies);

    CPPUNIT_TEST_SUITE_END();

public:
    SignalSlotable_Test();
    virtual ~SignalSlotable_Test();
    void setUp();
    void tearDown();

private:
    void waitDemoOk(const boost::shared_ptr<SignalSlotDemo>& demo, int messageCalls, int trials = 10);

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
    void testAsyncReply();
    void testAutoConnectSignal();
    void testAutoConnectSlot();
    void testRegisterSlotTwice();
    void testAsyncConnectInputChannel();
    void testUuid();
    void testStressReplies();
};

#endif	/* SIGNALSLOTABLE_TEST_HH */
