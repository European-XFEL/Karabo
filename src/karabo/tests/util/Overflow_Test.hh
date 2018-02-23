/* 
 * File:   Overflow_Test.hh
 * Author: gero.flucke@xfel.eu
 *
 * Created on Feb 23, 2018, 2:35 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef OVERFLOW_TEST_HH
#define	OVERFLOW_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Overflow_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(Overflow_Test);
    CPPUNIT_TEST(testOverflow);
    CPPUNIT_TEST_SUITE_END();

public:

    Overflow_Test();
    virtual ~Overflow_Test();
    void setUp();
    void tearDown();

private:
    void testOverflow();
};

#endif	/* OVERFLOW_TEST_HH */

