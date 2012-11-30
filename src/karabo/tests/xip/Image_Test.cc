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
        Image<float> img(CPU);
        CPPUNIT_ASSERT(img.isEmpty() == true);
        CPPUNIT_ASSERT(img.byteSize() == 0);
    }
    
    {
        boost::filesystem::path resourceDir(std::string(TESTPATH) + std::string("xip/resources/in-3-3-3.asc"));
        std::cout << "FILE: " << resourceDir.normalize().string() << std::endl;
        Image<float> img(CPU, resourceDir.normalize().string());
        img.print();
    }
    
    
}


