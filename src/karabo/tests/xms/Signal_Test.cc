/*
 * File:   Signal_Test.cc
 *
 * Created on Feb 9, 2023
 */

#include "Signal_Test.hh"

#include <cppunit/TestAssert.h>

#include "karabo/xms/Signal.hh"
#include "karabo/xms/SignalSlotable.hh"


CPPUNIT_TEST_SUITE_REGISTRATION(Signal_Test);


Signal_Test::Signal_Test() {}


Signal_Test::~Signal_Test() {}

void Signal_Test::setUp() {
    // Logger::configure(Hash("priority", "ERROR"));
    // Logger::useOstream();
    //  Event loop is started in xmsTestRunner.cc's main()
    //  Store broker environment variable
}


void Signal_Test::tearDown() {}


void Signal_Test::testRegisterSlots() {
    auto sigSlot = boost::make_shared<karabo::xms::SignalSlotable>("one");
    // sigSlot->start(); not needed here to start communication

    karabo::xms::Signal s(sigSlot.get(), sigSlot->getConnection(), sigSlot->getInstanceId(), "mySignal", //
                          10, 10000); // irrelevant priority and time to live

    //  test register
    CPPUNIT_ASSERT(s.registerSlot("otherId", "slotA"));
    CPPUNIT_ASSERT(!s.registerSlot("otherId", "slotA")); // cannot register twice
    CPPUNIT_ASSERT(s.registerSlot("otherId", "slotB"));

    // test unregister
    CPPUNIT_ASSERT(!s.unregisterSlot("otherId", "slotC"));  // unknown slot
    CPPUNIT_ASSERT(!s.unregisterSlot("otherId2", "slotA")); // unknown instance
    CPPUNIT_ASSERT(s.unregisterSlot("otherId", "slotA"));
    CPPUNIT_ASSERT(!s.unregisterSlot("otherId", "slotA")); // already unregistered
    CPPUNIT_ASSERT(s.unregisterSlot("otherId", ""));       // all remaining unregistered
    CPPUNIT_ASSERT(!s.unregisterSlot("otherId", "slotB")); // already unregistered as remaining
    CPPUNIT_ASSERT(!s.unregisterSlot("otherId", ""));      // already unregistered
    CPPUNIT_ASSERT(!s.unregisterSlot("otherId2 ", ""));    // was never registered
}
