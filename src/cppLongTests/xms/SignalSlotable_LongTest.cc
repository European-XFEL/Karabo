/*
 * File:   SignalSlotable_LongTest.cc
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:22 PM
 */

#include <cppunit/TestAssert.h>

#include "SignalSlotable_LongTest.hh"

#include "karabo/xms/SignalSlotable.hh"
#include "karabo/util/StringTools.hh"

#include "boost/shared_ptr.hpp"

#include <string>
#include <chrono>
#include <iostream>
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


void SignalSlotable_LongTest::testStressSyncReplies() {

    auto instance = boost::make_shared<SignalSlotable>("instance");
    instance->start();

    std::atomic<size_t> firstCalledCounter(0);
    auto first = [&firstCalledCounter]() {
        firstCalledCounter++;
    };
    instance->registerSlot(first, "slot");


    // High number of iterations:
    const size_t numIterations(1000000); // saw failures after > 500,000 successes before a mutex bug was fixed...
    std::clog << "Long testStressSyncReplies starting (till " << numIterations << "): " << std::flush;
    const auto testStartTime = std::chrono::high_resolution_clock::now();

    size_t sentRequests(0); // Count the number of sent requests
    size_t receivedReplies(0);
    for (size_t counter(0); counter < numIterations; ++counter) {
        if (++sentRequests % 100000 == 0) {
            std::clog << sentRequests << " ";
        }
        // Our slot functions do not place any answers, so an empty one will be added.
        // We do self messaging...
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("Lost synchronous reply #" + toString(sentRequests),
                                        instance->request("", "slot").timeout(1000).receive());
        ++receivedReplies;
    }
    std::clog << std::endl;
    CPPUNIT_ASSERT_EQUAL(sentRequests, firstCalledCounter.load());
    CPPUNIT_ASSERT_EQUAL(sentRequests, receivedReplies);

    float sec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - testStartTime).count();
    sec /= 1000.f;
    std::clog << "Test duration: " << sec << " s, i.e. request-receive at " << numIterations / sec << " Hz"
            << std::endl;
}


void SignalSlotable_LongTest::testStressAsyncReplies() {

    auto instance = boost::make_shared<SignalSlotable>("instance");
    instance->start();

    auto slot = [&instance](unsigned int counter) {
        instance->reply(counter);
    };
    instance->registerSlot<unsigned int>(slot, "slot");

    std::promise <unsigned int> promise;
    unsigned int counter = 0;
    boost::function<void(bool, unsigned int) > handler = [&instance, &promise, &counter, &handler](bool failure, unsigned int countdown) {
        ++counter;
        if (failure || --countdown == 0) {
            promise.set_value(countdown);
        } else {
            if (counter % 100000 == 0) {
                std::clog << counter << " ";
            }
            auto successHandler = boost::bind(handler, false, _1);
            auto failureHandler = boost::bind(handler, true, countdown);
            instance->request("", "slot", countdown).timeout(1000)
                    .receiveAsync<unsigned int>(successHandler, failureHandler);
        }
    };

    // Now trigger messaging
    std::future<unsigned int> future = promise.get_future();
    const unsigned int numIterations = 1000000;
    std::clog << "Long testStressAsyncReplies starting (till " << numIterations << "): " << std::flush;
    const auto testStartTime = std::chrono::high_resolution_clock::now();
    handler(false, numIterations);

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, future.wait_for(std::chrono::seconds(600)));
    std::clog << std::endl;
    CPPUNIT_ASSERT_EQUAL(0u, future.get());
    CPPUNIT_ASSERT_EQUAL(numIterations, counter);

    float sec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - testStartTime).count();
    sec /= 1000.f;
    std::clog << "Test duration: " << sec << " s, i.e. request-receiveAsync at " << numIterations / sec << " Hz"
            << std::endl;
}
