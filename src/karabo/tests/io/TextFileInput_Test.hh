/* 
 * File:   TextFileInput_Test.hh
 * Author: heisenb
 *
 * Created on March 1, 2013, 6:22 PM
 */

#ifndef TEXTFILEINPUT_TEST_HH
#define	TEXTFILEINPUT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class TextFileInput_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TextFileInput_Test);
    CPPUNIT_TEST(testLoad);
    CPPUNIT_TEST_SUITE_END();

public:
    TextFileInput_Test();
    virtual ~TextFileInput_Test();
    void setUp();
    void tearDown();

private:
    void testLoad();
};

#endif	/* TEXTFILEINPUT_TEST_HH */

