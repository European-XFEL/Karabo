/*
 * File:   ImageTest.cc
 * Author: heisenb
 *
 * Created on Nov 27, 2012, 8:56:02 AM
 */

#include <karabo/xip/Image.hh>

#include "Image_Test.hh"

using namespace karabo::xip;

CPPUNIT_TEST_SUITE_REGISTRATION(ImageTest);

ImageTest::ImageTest() {
}

ImageTest::~ImageTest() {
}

void ImageTest::setUp() {
}

void ImageTest::tearDown() {
}

void ImageTest::testConstructors() {
   
    {
        Image<float> img(GPU);
        //CPPUNIT_ASSERT(img.isEmpty() == true);
    }
    {
        Image<float> img(CPU, "someFile");
    }
    
    
}


