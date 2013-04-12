/*
 * File:   Authenticate_Test.hh
 * Author: heisenb
 *
 * Created on Apr 12, 2013, 4:24:42 PM
 */

#ifndef AUTHENTICATE_TEST_HH
#define	AUTHENTICATE_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Authenticate_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(Authenticate_Test);

    CPPUNIT_TEST(testLogin);

    CPPUNIT_TEST_SUITE_END();

public:
    Authenticate_Test();
    virtual ~Authenticate_Test();
    void setUp();
    void tearDown();

private:
    void testLogin();
};

#endif	/* AUTHENTICATE_TEST_HH */

