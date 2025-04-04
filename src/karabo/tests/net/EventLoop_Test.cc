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
 * File:   EventLoop_Test.cc
 * Author: heisenb
 *
 * Created on July 29, 2016, 2:06 PM
 */

#include "karabo/net/EventLoop.hh"

#include <chrono>
#include <csignal>

#include "EventLoop_Test.hh"
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeDuration.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/log/Logger.hh"


using namespace karabo::data;
using namespace karabo::net;
using namespace std::chrono;
using namespace std::literals::chrono_literals;

CPPUNIT_TEST_SUITE_REGISTRATION(EventLoop_Test);


EventLoop_Test::EventLoop_Test() {}


EventLoop_Test::~EventLoop_Test() {}


void EventLoop_Test::handler1(boost::asio::steady_timer& timer, int count) {
    if (count == -1) {
        CPPUNIT_ASSERT(EventLoop::getNumberOfThreads() == 0);
        return;
    }

    CPPUNIT_ASSERT(EventLoop::getNumberOfThreads() == size_t(count));

    if (count == 5) {
        EventLoop::removeThread(5);
        count = -1;
        timer.expires_at(timer.expires_at() + 500ms);
        timer.async_wait(std::bind(&EventLoop_Test::handler1, this, std::ref(timer), count));
        return;
    }

    EventLoop::addThread();
    count++;

    timer.expires_at(timer.expires_at() + 500ms);
    timer.async_wait(std::bind(&EventLoop_Test::handler1, this, std::ref(timer), count));
}


void EventLoop_Test::testMethod() {
    boost::asio::steady_timer timer(EventLoop::getIOService(), 500ms);
    timer.async_wait(std::bind(&EventLoop_Test::handler1, this, std::ref(timer), 0));

    EventLoop::run();
}


void EventLoop_Test::handler2() {
    if (m_finished) return;

    boost::asio::steady_timer timer(EventLoop::getIOService(), 5ms);
    EventLoop::getIOService().post(std::bind(&EventLoop_Test::handler2, this));
}


void EventLoop_Test::handler3() {
    EventLoop::stop();
}


void EventLoop_Test::testMethod2() {
    boost::asio::io_context::work work(EventLoop::getIOService());
    std::jthread t(std::bind(&EventLoop::run));

    m_finished = false;
    boost::asio::steady_timer timer(EventLoop::getIOService(), 500ms);
    EventLoop::addThread(10);
    EventLoop::getIOService().post(std::bind(&EventLoop_Test::handler2, this));
    timer.async_wait(std::bind(&EventLoop_Test::handler3, this));

    t.join();

    m_finished = true;

    CPPUNIT_ASSERT(true);
}


void EventLoop_Test::testSignalCapture() {
    std::jthread t(std::bind(&EventLoop::work));

    bool terminateCaught = false;
    EventLoop::setSignalHandler([&terminateCaught](int signal) {
        if (signal == SIGTERM) {
            terminateCaught = true;
        }
    });

    // Allow signal handling to be activated (1 ms sleep seems OK, but the test fails without sleep).
    std::this_thread::sleep_for(milliseconds(10));

    std::raise(SIGTERM);

    t.join();

    CPPUNIT_ASSERT(terminateCaught);
}

void EventLoop_Test::testPost() {
    // Post a method and verify it is executed
    {
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        auto func = [promise]() { promise->set_value(true); };
        EventLoop::post(func);
        EventLoop::run();

        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, future.wait_for(milliseconds(2000)));
        CPPUNIT_ASSERT(future.get());
    }

    // Post a method that throws and verify we stay alive if catch exception flag is true
    {
        const bool oldFlag = EventLoop::setCatchExceptions(true);
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        auto func = [promise]() {
            promise->set_value(true);
            throw std::runtime_error("induced");
        };
        EventLoop::post(func);
        EventLoop::run();

        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, future.wait_for(std::chrono::milliseconds(2000)));
        CPPUNIT_ASSERT(future.get());

        EventLoop::setCatchExceptions(oldFlag);
    }

    // Post a method that throws and see the exception leaking out of the event loop
    {
        const bool oldFlag = EventLoop::setCatchExceptions(false);
        auto func = []() { throw std::runtime_error("induced"); };
        EventLoop::post(func);

        CPPUNIT_ASSERT_THROW(EventLoop::run(), std::runtime_error);

        EventLoop::setCatchExceptions(oldFlag);
    }

    // Post method with a delay and verify delay
    {
        auto promise = std::make_shared<std::promise<bool>>();
        auto future = promise->get_future();
        auto func = [promise]() { promise->set_value(true); };

        const Epochstamp before;
        EventLoop::post(std::move(func), 100); // 100 ms delay, move semantics function
        EventLoop::run();

        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, future.wait_for(milliseconds(2000)));
        const TimeDuration period = before.elapsed();
        CPPUNIT_ASSERT(future.get());
        CPPUNIT_ASSERT_EQUAL(0ull, period.getTotalSeconds());               // less than a full second
        CPPUNIT_ASSERT_GREATEREQUAL(100ull, period.getFractions(MILLISEC)); // at least 100 milliseconds
    }
}

void EventLoop_Test::testAddThreadDirectly() {
    // Tests that, even in a so far single threaded event loop, adding a thread before blocking on something that
    // requires another task on the event loop to unblock, actually helps.

    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();

    std::future_status status = std::future_status::deferred;
    auto question = [&future, &status]() {
        EventLoop::addThread();                       // thread is added and allows 'answer' to run
        status = future.wait_for(milliseconds(1000)); // wait for 'answer' to unblock here
        EventLoop::removeThread();
    };

    auto answer = [promise]() { promise->set_value(true); };

    // Post question and answer and run them.
    EventLoop::getIOService().post(question);
    EventLoop::getIOService().post(answer);
    EventLoop::run();

    // Check that indeed 'answer' unblocked 'question'
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, status);
    CPPUNIT_ASSERT(future.get());
}

void EventLoop_Test::testExceptionTrace() {
    // Test thread safety of karabo::data::Exception's trace here and not in Exception_Test since requires event loop
    using karabo::data::Exception;

    std::jthread t(std::bind(&EventLoop::work));

    const int nParallel = 10;
    EventLoop::addThread(nParallel);
    std::this_thread::sleep_for(milliseconds(100)); // now all threads should be available


    std::vector<std::promise<std::string>> promises(nParallel);
    std::vector<std::future<std::string>> futures;
    for (int i = 0; i < nParallel; ++i) {
        futures.push_back(promises[i].get_future());
    }

    std::function<void(int)> func = [&promises](int n) {
        const std::string strN(karabo::data::toString(n));
        try {
            try {
                try {
                    try {
                        try {
                            try {
                                throw KARABO_CAST_EXCEPTION(strN + ": problem");
                            } catch (const Exception&) {
                                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION(strN + ": propagated 1"));
                            }
                        } catch (const Exception&) {
                            KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION(strN + ": propagated 2"));
                        }
                    } catch (const Exception&) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION(strN + ": propagated 3"));
                    }
                } catch (const Exception&) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION(strN + ": propagated 4"));
                }
            } catch (const Exception&) {
                KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION(strN + ": propagated 5"));
            }
        } catch (const Exception& e) {
            promises[n].set_value(e.userFriendlyMsg());
        }
    };

    // Now start executing all exception handling in parallel
    for (int i = 0; i < nParallel; ++i) {
        EventLoop::getIOService().post(std::bind(func, i));
    }

    // Collect all traces
    std::vector<std::string> exceptionTxts(nParallel);
    for (int i = 0; i < nParallel; ++i) {
        exceptionTxts[i] = futures[i].get();
    }

    // Stop loop and join thread before tests to avoid that failures leave dangling event loop thread behind
    EventLoop::stop();
    t.join(); // collects all threads added

    // Finally collect that exception traces contain al 'their' messages
    for (int i = 0; i < nParallel; ++i) {
        std::string numStr(karabo::data::toString(i));
        std::vector<std::string> split = splitByPattern(exceptionTxts[i], "because: ");
        // obsolete:
        // boost::algorithm::split_regex(split, exceptionTxts[i], boost::regex("because: "));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Trace for " + numStr + " is " + exceptionTxts[i], //
                                     6ul, split.size());                                // 6 exceptions
        numStr += ": ";
        for (const std::string& singleMsg : split) {
            // All exception messages were prepended with 'their' number
            // (exception order within trace is tested in Exception_Test::testTraceOrder)
            CPPUNIT_ASSERT_MESSAGE(singleMsg + " does not start with " + numStr, boost::starts_with(singleMsg, numStr));
        }
    }
}


std::vector<std::string> EventLoop_Test::splitByPattern(std::string_view source, std::string_view pattern) {
    std::vector<std::string> output;
    std::string::size_type prev = 0, pos = 0;
    while ((pos = source.find(pattern, pos)) != std::string::npos) {
        std::string sub(source.substr(prev, pos - prev));
        output.push_back(sub);
        pos += pattern.size();
        prev = pos;
    }
    std::string tail{source.substr(prev)};
    if (!tail.empty()) output.push_back(tail);
    return output;
}
