/* 
 * File:   Factory_Test.hh
 * Author: heisenb
 *
 * Created on September 19, 2012, 10:34 AM
 */

#ifndef FACTORY_TEST_HH
#define	FACTORY_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Factory_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Factory_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testFailedMethod);
    CPPUNIT_TEST_SUITE_END();

public:
    Factory_Test();
    virtual ~Factory_Test();
    void setUp();
    void tearDown();

private:
    int *example;
    void testMethod();
    void testFailedMethod();
};

#endif	/* FACTORY_TEST_HH */

