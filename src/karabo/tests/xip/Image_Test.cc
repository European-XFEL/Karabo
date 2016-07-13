/*
 * File:   ImageTest.cc
 * Author: heisenb
 *
 * Created on Nov 27, 2012, 8:56:02 AM
 */

#include <karabo/xip/Image.hh>

#include "Image_Test.hh"

using namespace karabo::xip;
using namespace karabo::util;

CPPUNIT_TEST_SUITE_REGISTRATION(ImageTest);


ImageTest::ImageTest() {
}


ImageTest::~ImageTest() {
}


void ImageTest::setUp() {
}


void ImageTest::tearDown() {
}


void ImageTest::testConstructorsChar() {
    {
        Image<unsigned char> img(CPU);
        CPPUNIT_ASSERT(img.isEmpty() == true);
        CPPUNIT_ASSERT(img.byteSize() == 0);
    }

    {
        Image<unsigned char> img(CPU, resourcePath("in-3-3-3.asc"));
        CPPUNIT_ASSERT(img.dimensionality() == 3);
        CPPUNIT_ASSERT(img.dimX() == 3);
        CPPUNIT_ASSERT(img.dimY() == 3);
        CPPUNIT_ASSERT(img.dimZ() == 3);
        CPPUNIT_ASSERT(img(1, 2, 0) == 120);
        CPPUNIT_ASSERT(img(1, 0, 2) == 102);
    }

    {
        Image<unsigned char> img(CPU, 1024, 1024);
        CPPUNIT_ASSERT(img.dimensionality() == 2);
        CPPUNIT_ASSERT(img.dimX() == 1024);
        CPPUNIT_ASSERT(img.dimY() == 1024);
        CPPUNIT_ASSERT(img.dimZ() == 1);

    }

    {
        Image<unsigned char> img(CPU, 10, 1, 1, 5);
        CPPUNIT_ASSERT(img.dimensionality() == 1);
        CPPUNIT_ASSERT(img.dimX() == 10);
        CPPUNIT_ASSERT(img.dimY() == 1);
        CPPUNIT_ASSERT(img.dimZ() == 1);
        for (size_t i = 0; i < img.dimX(); ++i) {
            CPPUNIT_ASSERT(img[i] == 5);
        }
    }

    {
        Image<unsigned char> img(CPU, 4, 1, 1, "0,1,2,3", true);
        CPPUNIT_ASSERT(img.dimensionality() == 1);
        CPPUNIT_ASSERT(img.dimX() == 4);
        CPPUNIT_ASSERT(img.dimY() == 1);
        CPPUNIT_ASSERT(img.dimZ() == 1);
        for (size_t i = 0; i < img.dimX(); ++i) {
            CPPUNIT_ASSERT(img[i] == i);
        }
    }
}


void ImageTest::testImageHeader() {

    {
        Image<unsigned char> img(CPU, 128, 128, 2);

        Hash header = img.getHeader();
        assert(header.get<int>("__dimX") == 128);
        assert(header.get<int>("__dimY") == 128);
        assert(header.get<int>("__dimZ") == 2);
    }
    {
        Image<unsigned char> img(CPU, 4, 4);
        //img.setHeader(Hash("p1", "Just for fun", "p2", 9.87654321));
        img.setHeaderParam("p1", "Just for fun");
        img.setHeaderParam("p2", 9.87654321);

        Hash header = img.getHeader();
        assert(header.get<string>("p1") == "Just for fun");
        assert(header.get<double>("p2") == 9.87654321);
        assert(header.get<int>("__dimX") == 4);
        assert(header.get<int>("__dimY") == 4);
        assert(header.get<int>("__dimZ") == 1);
    }



}
