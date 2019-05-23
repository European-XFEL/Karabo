/*
 * File:   SignalSlotable_LongTest.cc
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:22 PM
 */

#include <cppunit/TestAssert.h>

#include "SignalSlotable_LongTest.hh"

#include "karabo/xms/SignalSlotable.hh"
#include "karabo/util/Hash.hh"

#include "boost/thread/mutex.hpp"
#include "boost/shared_ptr.hpp"

#include <string>
#include <future>

using namespace karabo::util;
using namespace karabo::xms;


CPPUNIT_TEST_SUITE_REGISTRATION(SignalSlotable_LongTest);


SignalSlotable_LongTest::SignalSlotable_LongTest() {
}


SignalSlotable_LongTest::~SignalSlotable_LongTest() {
}


void SignalSlotable_LongTest::setUp() {
    //Logger::configure(Hash("priority", "ERROR"));
    //Logger::useOstream();
    // Event loop is started in xmsLongTestRunner.cc's main()
}


void SignalSlotable_LongTest::tearDown() {
}


void SignalSlotable_LongTest::testStressReplies() {

    auto instance = boost::make_shared<SignalSlotable>("instance");
    instance->start();

    std::atomic<size_t> firstCalledCounter(0);
    auto first = [&firstCalledCounter]() {
        firstCalledCounter++;
    };
    instance->registerSlot(first, "slot");


    // High number of iterations:
    // Saw it succeeding > 1000000 successful trials at times when a mutex was missing in SignalSlotable!
    const size_t numIterations(30000);
    std::clog << "Long test starting (till " << numIterations << "): " << std::flush;
    size_t sentRequests(0); // Count the number of sent requests
    size_t receivedReplies(0);
    for (size_t counter(0); counter < numIterations; ++counter) {
        if (++sentRequests % 100000 == 0) { // 100000 seem to take about 15 seconds
            std::clog << sentRequests << " ";
        }
        // Our slot functions do not place any answers, so an empty one will be added.
        try {
            // Self messaging...
            instance->request("", "slot").timeout(10000).receive();
            ++receivedReplies;
        } catch (const TimeoutException&) {
            CPPUNIT_ASSERT_MESSAGE("Lost synchronous reply #" + toString(sentRequests), false);
        }
    }
    std::clog << std::endl;
    CPPUNIT_ASSERT_EQUAL(sentRequests, firstCalledCounter.load());
    CPPUNIT_ASSERT_EQUAL(sentRequests, receivedReplies);
}
