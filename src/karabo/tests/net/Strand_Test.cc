/* 
 * File:   Strand_Test.cc
 * Author: flucke
 * 
 * Created on November 15, 2017, 12:26 PM
 */
#include "karabo/net/EventLoop.hh"
#include "karabo/net/Strand.hh"
#include "karabo/util/Epochstamp.hh"
#include "karabo/util/TimeDuration.hh"

#include "boost/asio/deadline_timer.hpp"

#include "Strand_Test.hh"

using karabo::net::EventLoop;
using karabo::util::Epochstamp;
using karabo::util::TimeDuration;

CPPUNIT_TEST_SUITE_REGISTRATION(Strand_Test);


Strand_Test::Strand_Test() : m_nThreadsInPool(4) {
}


Strand_Test::~Strand_Test() {
}


void Strand_Test::setUp() {

    m_thread = boost::make_shared<boost::thread>(EventLoop::work);
    // really switch on parallelism:
    EventLoop::addThread(m_nThreadsInPool);
}


void Strand_Test::tearDown() {

    EventLoop::removeThread(m_nThreadsInPool);
    EventLoop::stop();

    m_thread->join();
    m_thread.reset();
}


void Strand_Test::testSequential() {

    boost::mutex aMutex;
    unsigned int counter = 0;
    const unsigned int sleepTimeMs = 40; // must be above 10, see below

    auto sleepAndCount = [&aMutex, &counter, &sleepTimeMs] () {
        boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTimeMs));

        boost::mutex::scoped_lock lock(aMutex);
        ++counter;

    };
    // All helpers before timing starts via creating 'now'
    const unsigned int numPosts = m_nThreadsInPool;
    TimeDuration duration;
    unsigned int numTest = 50;

    // Strands need to be pointed to by shared_ptr
    auto strand = boost::make_shared<karabo::net::Strand>(EventLoop::getIOService());
    Epochstamp now;
    // A timer to concurrently run Strand::post (and to start the duration),
    // not sure whether several handlers of the timer will really be executed at the same time or not...
    boost::asio::deadline_timer timer(EventLoop::getIOService());
    timer.expires_from_now(boost::posix_time::milliseconds(10));
    timer.async_wait([&now] (const boost::system::error_code & e) {
        now.now();
    });
    for (unsigned int i = 0; i < numPosts; ++i) {
        timer.async_wait([&strand, &sleepAndCount](const boost::system::error_code & e) {
            strand->post(sleepAndCount);
        });
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
    boost::function<void(int) > handler = [&vec, &done](int i) {
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

    // We stop the test before all posts have been processed - in principle the Strand could have posted
    // to the event loop before it died and then the handler is called when the test function is done and
    // its scope is cleaned.
    // By using (copies of) shared pointers inside the handler we avoid any crash potential of that.
    auto aMutexPtr = boost::make_shared<boost::mutex>();
    auto counterPtr = boost::make_shared<unsigned int>(0);
    const unsigned int sleepTimeMs = 40; // must be above 10, see below

    auto sleepAndCount = [aMutexPtr, counterPtr, sleepTimeMs] () {
        boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTimeMs));

        boost::mutex::scoped_lock lock(*aMutexPtr);
        ++(*counterPtr);

    };
    const unsigned int numPosts = m_nThreadsInPool;

    // Strands need to be pointed to by shared_ptr
    auto strand = boost::make_shared<karabo::net::Strand>(EventLoop::getIOService());
    for (unsigned int i = 0; i < numPosts; ++i) {
        strand->post(sleepAndCount);
    }

    unsigned int numTest = 50;
    while (--numTest > 0) {
        {
            boost::mutex::scoped_lock lock(*aMutexPtr);
            if (*counterPtr >= numPosts / 2) {
                // After half of the sequential posts, let the strand die - thus counter will not increase anymore!
                strand.reset();
            } else if (*counterPtr >= numPosts) {
                break;
            }
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTimeMs / 10));
    }

    // The above loop came to an end since the last handlers where not called.
    CPPUNIT_ASSERT(*counterPtr < numPosts);
    CPPUNIT_ASSERT_EQUAL(numTest, 0u);
 }

