/*
 * File:   SignalSlotable_Test.hh
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:21 PM
 */

#ifndef SIGNALSLOTABLE_TEST_HH
#define	SIGNALSLOTABLE_TEST_HH

#include <karabo/xms.hpp>
#include <cppunit/extensions/HelperMacros.h>

class SignalSlotDemo;

class SignalSlotable_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(SignalSlotable_Test);

    CPPUNIT_TEST(testMethod);
   
    CPPUNIT_TEST_SUITE_END();

public:
    SignalSlotable_Test();
    virtual ~SignalSlotable_Test();
    void setUp();
    void tearDown();

private:

    void testMethod();

    boost::shared_ptr<SignalSlotDemo> m_demo;
    boost::thread m_demoThread;
};

#endif	/* SIGNALSLOTABLE_TEST_HH */

