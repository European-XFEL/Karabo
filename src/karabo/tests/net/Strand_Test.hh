/* 
 * File:   Strand_Test.hh
 * Author: flucke
 *
 * Created on November 15, 2017, 12:26 PM
 */

#ifndef STRAND_TEST_HH
#define	STRAND_TEST_HH

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include <cppunit/extensions/HelperMacros.h>

class Strand_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(Strand_Test);
    CPPUNIT_TEST(testSequential);
    CPPUNIT_TEST(testThrowing);
    CPPUNIT_TEST(testStrandDies);
    CPPUNIT_TEST_SUITE_END();

public:
    Strand_Test();
    virtual ~Strand_Test();
    void setUp();
    void tearDown();

private:
    void testSequential();

    void testThrowing();

    void testStrandDies();

    boost::shared_ptr<boost::thread> m_thread;
    const unsigned int m_nThreadsInPool;
};

#endif	/* STRAND_TEST_HH */

