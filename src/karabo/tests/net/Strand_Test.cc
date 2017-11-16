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
    for (unsigned int i = 0; i < numPosts; ++i) {
        strand->post(sleepAndCount);
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

    /////////////////////////////////////////////////////////////////////////////////
    // The same again, but posting to EventLoop: Now we are done much more quickly.
    // (This ensures that previous success is not 'pure luck' or due to a single
    //  threaded EventLoop.)
    /////////////////////////////////////////////////////////////////////////////////

    // First resetting
    {
        boost::mutex::scoped_lock lock(aMutex);
        counter = 0;
    }
    numTest = 50;
    now.now();

    for (unsigned int i = 0; i < numPosts; ++i) {
        EventLoop::getIOService().post(sleepAndCount);
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
    // Done much faster than sleeping numPosts times:
    CPPUNIT_ASSERT(duration.getTotalSeconds() * 1000ull + duration.getFractions(karabo::util::MILLISEC) // total ms
                   < numPosts / 2 * sleepTimeMs);
}


void Strand_Test::testStrandDies() {

    boost::mutex aMutex;
    unsigned int counter = 0;
    const unsigned int sleepTimeMs = 40; // must be above 10, see below

    auto sleepAndCount = [&aMutex, &counter, &sleepTimeMs] () {
        boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTimeMs));

        boost::mutex::scoped_lock lock(aMutex);
        ++counter;

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
            boost::mutex::scoped_lock lock(aMutex);
            if (counter >= numPosts / 2) {
                // After half of the sequential posts, let the strand die - thus counter will not increase anymore!
                strand.reset();
            } else if (counter >= numPosts) {
                break;
            }
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTimeMs / 10));
    }

    // The above loop came to an end since the last handlers where not called.
    CPPUNIT_ASSERT(counter < numPosts);
    CPPUNIT_ASSERT_EQUAL(numTest, 0u);
 }

