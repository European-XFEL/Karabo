/* 
 * File:   StringTools_Test.hh
 * Author: parenti
 *
 * Created on December 15, 2014, 12:16 PM
 */

#ifndef STRINGTOOLS_TEST_HH
#define	STRINGTOOLS_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class StringTools_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(StringTools_Test);
    CPPUNIT_TEST(testFromString);
    CPPUNIT_TEST(testToString);
    CPPUNIT_TEST(testWiden);
    CPPUNIT_TEST_SUITE_END();

public:
    StringTools_Test();
    virtual ~StringTools_Test();
    void setUp();
    void tearDown();

private:
    void testFromString();
    void testToString();
    void testWiden();
};

#endif	/* STRINGTOOLS_TEST_HH */

