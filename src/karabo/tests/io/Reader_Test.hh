/*
 * File:   Reader_Test.hh
 * Author: irinak
 *
 * Created on Oct 2, 2012, 4:47:15 PM
 */

#ifndef READER_TEST_HH
#define	READER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Reader_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Reader_Test);

    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testFailedMethod);

    CPPUNIT_TEST_SUITE_END();

public:
    Reader_Test();
    virtual ~Reader_Test();
    void setUp();
    void tearDown();

private:
    void testMethod();
    void testFailedMethod();
};

#endif	/* READER_TEST_HH */

