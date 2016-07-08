/* 
 * File:   States_Test.hh
 * Author: haufs
 *
 * Created on July 8, 2016, 10:08 AM
 */

#ifndef KARABO_STATES_TEST_HH
#define	KARABO_STATES_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class States_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(States_Test);
    CPPUNIT_TEST(testStringRoundTrip);
    CPPUNIT_TEST(testSignifier);
    CPPUNIT_TEST(testSignifierNonDefaultList);
    CPPUNIT_TEST_SUITE_END();
    
    
public:
    States_Test();
    virtual ~States_Test();
    void setUp();
    void tearDown();
    
    

private:
    void testStringRoundTrip();
    void testSignifier();
    void testSignifierNonDefaultList();
    
};



#endif	/* SKARABO_TATES_TEST_HH */

