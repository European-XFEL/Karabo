/*
 * File:   Exception_Test.hh
 * Author: heisenb
 *
 * Created on September 29, 2016, 5:28 PM
 */

#ifndef EXCEPTION_TEST_HH
#define EXCEPTION_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Exception_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Exception_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST_SUITE_END();

   public:
    Exception_Test();
    virtual ~Exception_Test();

   private:
    void testMethod();
};

#endif /* EXCEPTION_TEST_HH */
