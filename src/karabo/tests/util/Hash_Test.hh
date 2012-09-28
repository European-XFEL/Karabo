/*
 * File:   Hash_Test.hh
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:47:59 PM
 */

#ifndef HASH_TEST_HH
#define	HASH_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Hash_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Hash_Test);

    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testFailedMethod);

    CPPUNIT_TEST_SUITE_END();

public:
    Hash_Test();
    virtual ~Hash_Test();
    void setUp();
    void tearDown();

private:
    void testMethod();
    void testFailedMethod();
};

#endif	/* HASH_TEST_HH */

