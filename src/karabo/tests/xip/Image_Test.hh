/*
 * File:   ImageTest.hh
 * Author: heisenb
 *
 * Created on Nov 27, 2012, 8:56:02 AM
 */

#ifndef IMAGETEST_HH
#define	IMAGETEST_HH

#include <cppunit/extensions/HelperMacros.h>

class ImageTest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(ImageTest);

    CPPUNIT_TEST(testConstructors);

    CPPUNIT_TEST_SUITE_END();

public:
    ImageTest();
    virtual ~ImageTest();
    void setUp();
    void tearDown();

private:
    void testConstructors();
};

#endif	/* IMAGETEST_HH */

