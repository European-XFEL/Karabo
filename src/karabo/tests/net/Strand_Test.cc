/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Strand_Test.cc
 * Author: flucke
 *
 * Created on November 15, 2017, 12:26 PM
 */
#include "karabo/net/Strand.hh"

#include "Strand_Test.hh"
#include "boost/asio/deadline_timer.hpp"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/TimeDuration.hh"

using karabo::net::EventLoop;
using karabo::net::Strand;
using karabo::util::Configurator;
using karabo::util::Epochstamp;
using karabo::util::Hash;
using karabo::util::TimeDuration;

CPPUNIT_TEST_SUITE_REGISTRATION(Strand_Test);


Strand_Test::Strand_Test() : m_nThreadsInPool(4) {}


Strand_Test::~Strand_Test() {}


void Strand_Test::setUp() {
    m_thread = boost::make_shared<boost::thread>(EventLoop::work);
    // really switch on parallelism:
    EventLoop::addThread(m_nThreadsInPool);
}


void Strand_Test::tearDown() {
    // No need to do EventLoop::removeThread(m_nThreadsInPool) since joining the main eventloop thread implicitly
    // removes all threads, i.e. a new start of the event loop starts "from scratch". In best case, this cures hanging
    // tests as in https://git.xfel.eu/Karabo/Framework/-/jobs/357339 and
    // https://git.xfel.eu/Karabo/Framework/-/jobs/357451.
    EventLoop::stop();

    m_thread->join();
    m_thread.reset();
}


void Strand_Test::testSequential() {
    boost::mutex aMutex;
    unsigned int counter = 0;
    const unsigned int sleepTimeMs = 40; // must be above 10, see below

    auto sleepAndCount = [&aMutex, &counter, &sleepTimeMs]() {
        boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTimeMs));

        boost::mutex::scoped_lock lock(aMutex);
        ++counter;
    };
    // All helpers before timing starts via creating 'now'
    const unsigned int numPosts = m_nThreadsInPool;
    TimeDuration duration;
    unsigned int numTest = 50;

    Strand::Pointer strand = Configurator<Strand>::create("Strand", Hash());
    Epochstamp now;
    // A timer to concurrently run Strand::post (and to start the duration),
    // not sure whether several handlers of the timer will really be executed at the same time or not...
    boost::asio::deadline_timer timer(EventLoop::getIOService());
    timer.expires_from_now(boost::posix_time::milliseconds(10));
    timer.async_wait([&now](const boost::system::error_code& e) { now.now(); });
    for (unsigned int i = 0; i < numPosts; ++i) {
        timer.async_wait(
              [&strand, &sleepAndCount](const boost::system::error_code& e) { strand->post(sleepAndCount); });
    }

    while (--numTest > 0) {
        {
            boost::mutex::scoped_lock lock(aMutex);
            if (counter >= numPosts) {
                duration = Epochstamp().elapsed(now);
                break;
            }
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTimeMs / 10));
    }

    CPPUNIT_ASSERT(numTest > 0);
    CPPUNIT_ASSERT(duration.getTotalSeconds() * 1000ull + duration.getFractions(karabo::util::MILLISEC) // total ms
                   >= numPosts * sleepTimeMs);
}


void Strand_Test::testThrowing() {
    // Test that a std::exception thrown in posted handler does not stop the Strand to work
    // but goes on with next handler

    auto strand = boost::make_shared<karabo::net::Strand>(EventLoop::getIOService());
    const int size = 10;
    std::vector<int> vec(size, -1); // initialize all entries with -1
    bool done = false;
    boost::function<void(int)> handler = [&vec, &done](int i) {
        if (i == 2) {
            throw std::runtime_error("trouble");
        } else {
            vec[i] = i;
            if (i == size - 1) done = true;
        }
    };
    for (int i = 0; i < size; ++i) {
        strand->post(boost::bind(handler, i));
    }
    while (!done) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }

    for (int i = 0; i < size; ++i) {
        if (i == 2) {
            // vector element untouched by handler
            CPPUNIT_ASSERT_EQUAL(-1, vec[i]);
        } else {
            CPPUNIT_ASSERT_EQUAL(i, vec[i]);
        }
    }
}

void Strand_Test::testStrandDies() {
    // Test various configs and whether all handlers are called for them:
    const std::vector<std::pair<Hash, bool>> testCases{{Hash("guaranteeToRun", true), true},
                                                       {Hash(), false}, // default
                                                       {Hash("guaranteeToRun", false), false},
                                                       // Caveat: If maxInARow is too close to 'numPosts' below,
                                                       //         may not loose posts so test fails!
                                                       {Hash("guaranteeToRun", false, "maxInARow", 3u), false},
                                                       {Hash("guaranteeToRun", true, "maxInARow", 3u), true}};
    // Some initial sleep needed to get the event loop ready as just started in setUp().
    // Otherwise the first case ("guaranteeToRun" is true) has not enough time.
    boost::this_thread::sleep(boost::posix_time::milliseconds(300));
    for (const std::pair<Hash, bool>& testCase : testCases) {
        const Hash& cfg = testCase.first;
        const bool allHandlersRun = testCase.second;
        // We stop the test before all posts have been processed - in principle the Strand could have posted
        // to the event loop before it died and then the handler is called when the test function is done and
        // its scope is cleaned.
        // By using a copy of shared pointer inside the handler we avoid any crash potential of that.
        auto counterPtr = boost::make_shared<std::atomic<unsigned int>>(0);
        const unsigned int sleepTimeMs = 10;

        auto sleepAndCount = [counterPtr, sleepTimeMs]() {
            boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTimeMs));

            ++(*counterPtr);
        };
        const unsigned int numPosts = 10;

        Strand::Pointer strand = Configurator<Strand>::create("Strand", cfg);
        for (unsigned int i = 0; i < numPosts; ++i) {
            strand->post(sleepAndCount);
        }
        unsigned int numTest = 30;
        const unsigned int waitLoopSleep = sleepTimeMs;
        // Assert that the following loop is long enough to give the handlers time to be called one after another
        CPPUNIT_ASSERT(numTest * waitLoopSleep > numPosts * sleepTimeMs);
        while (--numTest > 0) {
            {
                if (*counterPtr >= numPosts) {
                    break; // no need to wait longer
                } else if (*counterPtr >= numPosts / 2) {
                    // After half of the sequential posts, let the strand die:
                    // Thus counter will only increase if 'guaranteeToRun' is true
                    strand.reset();
                }
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(waitLoopSleep));
        }

        CPPUNIT_ASSERT_GREATER(0u, counterPtr->load());
        if (allHandlersRun) {
            // Despite killing the strand half way through, all handlers are run
            CPPUNIT_ASSERT_EQUAL(numPosts, counterPtr->load());
        } else {
            // Strand was not configured to run all handlers when dying, about half them are likely lost.
            // No need to control exact number (the strand posts with bind_weak), just test that not all are run
            CPPUNIT_ASSERT_LESS(numPosts, counterPtr->load());
        }
    }
}

void Strand_Test::testMaxInARow() {
    // This tests that one can gain a little execution speed for a busy strand if "maxInARow" is specified since that
    // means potentially less jumps from one thread to another.
    constexpr unsigned int maxInARow = 10;
    constexpr unsigned int numPosts = 2000 * maxInARow;

    auto strand1 = Configurator<Strand>::create("Strand", Hash());
    auto strandMany = Configurator<Strand>::create("Strand", Hash("maxInARow", maxInARow));

    std::promise<Epochstamp> promise1;
    auto future1 = promise1.get_future();
    boost::function<void(int)> handler1 = [numPosts, &promise1](unsigned int i) {
        if (i == numPosts) promise1.set_value(Epochstamp());
    };

    std::promise<Epochstamp> promiseMany;
    auto futureMany = promiseMany.get_future();
    boost::function<void(int)> handlerMany = [numPosts, &promiseMany](unsigned int i) {
        if (i == numPosts) promiseMany.set_value(Epochstamp());
    };

    const Epochstamp beforePost;
    for (unsigned int i = 0; i < numPosts; ++i) {
        strand1->post(boost::bind(handler1, i + 1));
        strandMany->post(boost::bind(handlerMany, i + 1));
    }

    const Epochstamp doneManyStamp = futureMany.get();
    const Epochstamp done1Stamp = future1.get();

    CPPUNIT_ASSERT_MESSAGE("1: " + done1Stamp.toIso8601Ext() += ", many: " + doneManyStamp.toIso8601Ext(),
                           done1Stamp > doneManyStamp);
    // The speed gain of strandMany compared to strand1 scales roughly linearly with both 'maxInARow' and
    // 'numPosts', though the absolute time varies a bit. For maxInARow = 10 and numPosts = 2000 * maxInARow
    // and m_nThreadsInPool = 4 we get e.g. (Ubuntu20 with 12 cores):
    // Before to end many: 0.325196 s
    // Before to end    1: 0.429868 s
    // (The difference gets smaller with less threads, e.g. about 60 ms instead of 100 ms for m_nThreadsInPool = 0.)
    // std::clog << "\nBefore to end many: " << static_cast<double>(doneManyStamp - beforePost) << " s" << std::endl;
    // std::clog << "Before to end    1: " << static_cast<double>(done1Stamp - beforePost) << " s" << std::endl;
}
