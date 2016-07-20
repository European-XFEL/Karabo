/*
 * File:   ImageTest.hh
 * Author: heisenb
 *
 * Created on Nov 27, 2012, 8:56:02 AM
 */

#ifndef IMAGETEST_HH
#define	IMAGETEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include "TestPathSetup.hh"

class ImageTest : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(ImageTest);

    CPPUNIT_TEST(testConstructorsFloat);
    CPPUNIT_TEST(testConstructorsDouble);
    CPPUNIT_TEST(testConstructorsChar);
    CPPUNIT_TEST(testImageHeader);
    CPPUNIT_TEST_SUITE_END();

public:
    ImageTest();
    virtual ~ImageTest();
    void setUp();
    void tearDown();

private:

    void testConstructorsDouble() {
        this->testConstructors<double>();
    }

    void testConstructorsFloat() {
        this->testConstructors<float>();
    }

    void testConstructorsChar();

    void testImageHeader();

    template <class T>
    void testConstructors() {

        using namespace karabo::xip;

        {
            Image<T> img(CPU);
            CPPUNIT_ASSERT(img.isEmpty() == true);
            CPPUNIT_ASSERT(img.byteSize() == 0);
        }

        {
            Image<T> img(CPU, resourcePath("in-3-3-3.asc"));
            CPPUNIT_ASSERT(img.dimensionality() == 3);
            CPPUNIT_ASSERT(img.dimX() == 3);
            CPPUNIT_ASSERT(img.dimY() == 3);
            CPPUNIT_ASSERT(img.dimZ() == 3);
            CPPUNIT_ASSERT(img(2, 2, 2) == 222);
            CPPUNIT_ASSERT(img(1, 0, 2) == 102);
        }

        {
            Image<T> img(CPU, 1024, 1024);
            CPPUNIT_ASSERT(img.dimensionality() == 2);
            CPPUNIT_ASSERT(img.dimX() == 1024);
            CPPUNIT_ASSERT(img.dimY() == 1024);
            CPPUNIT_ASSERT(img.dimZ() == 1);

        }

        {
            Image<T> img(CPU, 10, 1, 1, 5.5);
            CPPUNIT_ASSERT(img.dimensionality() == 1);
            CPPUNIT_ASSERT(img.dimX() == 10);
            CPPUNIT_ASSERT(img.dimY() == 1);
            CPPUNIT_ASSERT(img.dimZ() == 1);
            for (size_t i = 0; i < img.dimX(); ++i) {
                CPPUNIT_ASSERT(img[i] == 5.5);
            }
        }

        {
            Image<T> img(CPU, 4, 1, 1, "0,1,2,3", true);
            CPPUNIT_ASSERT(img.dimensionality() == 1);
            CPPUNIT_ASSERT(img.dimX() == 4);
            CPPUNIT_ASSERT(img.dimY() == 1);
            CPPUNIT_ASSERT(img.dimZ() == 1);
            for (size_t i = 0; i < img.dimX(); ++i) {
                CPPUNIT_ASSERT(img[i] == i);
            }
        }
    }
};

#endif	/* IMAGETEST_HH */

