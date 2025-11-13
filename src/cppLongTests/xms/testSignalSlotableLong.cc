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
 * File:   testSignalSlotableLong.cc
 *
 * Created on Apr 4, 2013, 1:24:21 PM
 */

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <iostream>
#include <string>

#include "boost/shared_ptr.hpp"
#include "karabo/data/types/StringTools.hh"
#include "karabo/xms/SignalSlotable.hh"


class TestSignalSlotableLong : public testing::Test {
   protected:
    TestSignalSlotableLong();
    virtual ~TestSignalSlotableLong() override;
    void SetUp() override;
    void TearDown() override;
};


using namespace karabo::data;
using namespace karabo::xms;

using std::placeholders::_1;


TestSignalSlotableLong::TestSignalSlotableLong() {}


TestSignalSlotableLong::~TestSignalSlotableLong() {}


void TestSignalSlotableLong::SetUp() {
    // Logger::configure(Hash("priority", "ERROR"));
    // Logger::useConsole();
    //  Event loop is started in xmsLongTestRunner.cc's main()
}


void TestSignalSlotableLong::TearDown() {}


TEST_F(TestSignalSlotableLong, testStressSyncReplies) {
    auto instance = std::make_shared<SignalSlotable>("instance");
    instance->start();

    std::atomic<size_t> firstCalledCounter(0);
    auto first = [&firstCalledCounter]() { firstCalledCounter++; };
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
        ASSERT_NO_THROW(instance->request("", "slot").timeout(1000).receive())
              << "Lost synchronous reply #" << sentRequests;
        ++receivedReplies;
    }
    std::clog << std::endl;
    ASSERT_EQ(sentRequests, firstCalledCounter.load());
    ASSERT_EQ(sentRequests, receivedReplies);

    float sec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -
                                                                      testStartTime)
                      .count();
    sec /= 1000.f;
    std::clog << "Test duration: " << sec << " s, i.e. request-receive at " << numIterations / sec << " Hz"
              << std::endl;
}


TEST_F(TestSignalSlotableLong, testStressAsyncReplies) {
    auto instance = std::make_shared<SignalSlotable>("instance");
    instance->start();

    auto slot = [&instance](unsigned int counter) { instance->reply(counter); };
    instance->registerSlot<unsigned int>(slot, "slot");

    std::promise<unsigned int> promise;
    unsigned int counter = 0;
    std::function<void(bool, unsigned int)> handler = [&instance, &promise, &counter, &handler](
                                                            bool failure, unsigned int countdown) {
        ++counter;
        if (failure || --countdown == 0) {
            promise.set_value(countdown);
        } else {
            if (counter % 100000 == 0) {
                std::clog << counter << " ";
            }
            auto successHandler = std::bind(handler, false, _1);
            auto failureHandler = std::bind(handler, true, countdown);
            instance->request("", "slot", countdown)
                  .timeout(1000)
                  .receiveAsync<unsigned int>(successHandler, failureHandler);
        }
    };

    // Now trigger messaging
    std::future<unsigned int> future = promise.get_future();
    const unsigned int numIterations = 1000000;
    std::clog << "Long testStressAsyncReplies starting (till " << numIterations << "): " << std::flush;
    const auto testStartTime = std::chrono::high_resolution_clock::now();
    handler(false, numIterations);

    ASSERT_EQ(std::future_status::ready, future.wait_for(std::chrono::seconds(600)));
    std::clog << std::endl;
    ASSERT_EQ(0u, future.get());
    ASSERT_EQ(numIterations, counter);

    float sec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -
                                                                      testStartTime)
                      .count();
    sec /= 1000.f;
    std::clog << "Test duration: " << sec << " s, i.e. request-receiveAsync at " << numIterations / sec << " Hz"
              << std::endl;
}
