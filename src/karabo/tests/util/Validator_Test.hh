/* 
 * File:   Validator_Test.hh
 * Author: flucke
 *
 * Created on September 9, 2016, 12:18 PM
 */

#ifndef VALIDATOR_TEST_HH
#define	VALIDATOR_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Validator_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(Validator_Test);
    CPPUNIT_TEST(testTableDefault);
    CPPUNIT_TEST_SUITE_END();

public:
    Validator_Test();
    virtual ~Validator_Test();
    void setUp();
    void tearDown();

private:
    void testTableDefault();
};

#endif	/* VALIDATOR_TEST_HH */

