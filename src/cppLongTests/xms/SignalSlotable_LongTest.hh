/*
 * File:   SignalSlotable_LongTest.hh
 *
 * Created on Apr 4, 2013, 1:24:21 PM
 */

#ifndef SIGNALSLOTABLE_LONGTEST_HH
#define	SIGNALSLOTABLE_LONGTEST_HH

#include <cppunit/extensions/HelperMacros.h>

class SignalSlotable_LongTest : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(SignalSlotable_LongTest);

    CPPUNIT_TEST(testStressReplies);

    CPPUNIT_TEST_SUITE_END();

public:
    SignalSlotable_LongTest();
    virtual ~SignalSlotable_LongTest();
    void setUp();
    void tearDown();

private:
    //    void waitDemoOk(const boost::shared_ptr<SignalSlotDemo>& demo, int messageCalls, int trials = 10);

    void testStressReplies();
};

#endif	/* SIGNALSLOTABLE_LONGTEST_HH */
