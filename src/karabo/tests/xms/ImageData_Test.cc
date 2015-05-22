/*
 * File:   ImageData_Test.cc
 * Author: parenti
 *
 * Created on May 21, 2015, 3:05:22 PM
 */

#include <boost/thread/pthread/thread_data.hpp>

#include "ImageData_Test.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::net;
using namespace karabo::xms;

CPPUNIT_TEST_SUITE_REGISTRATION(ImageData_Test);


ImageData_Test::ImageData_Test() {
}


ImageData_Test::~ImageData_Test() {
}


void ImageData_Test::setUp() {
}


void ImageData_Test::tearDown() {
}

void ImageData_Test::testConstructor() {
    
    Dims dims(200, 100);
    std::vector<int> someData(dims.size(), 2);
    
    {
        ImageData image(&someData[0], someData.size(), true, dims);
        
        Dims imageDims(image.getDimensions());
        Dims imageOffsets(image.getROIOffsets());
        
        CPPUNIT_ASSERT(image.getByteSize() == 20000 * sizeof (int));
        CPPUNIT_ASSERT(image.getChannelSpace() == ChannelSpace::s_32_4);
        CPPUNIT_ASSERT(image.getEncoding() == Encoding::GRAY);
        
        CPPUNIT_ASSERT(imageDims.size() == 20000);
        CPPUNIT_ASSERT(imageDims.rank() == 2);
        CPPUNIT_ASSERT(imageDims.x1() == 200);
        CPPUNIT_ASSERT(imageDims.x2() == 100);
        
        CPPUNIT_ASSERT(imageOffsets.rank() == 2);
        CPPUNIT_ASSERT(imageOffsets.x1() == 0);
        CPPUNIT_ASSERT(imageOffsets.x2() == 0);
        
        CPPUNIT_ASSERT(image.getDataType() == "INT32");
    }
    
    {
        karabo::xip::CpuImage<unsigned char> cpuImage(200, 100, 5);
        ImageData image(cpuImage);
        
        Dims imageDims(image.getDimensions());
        Dims imageOffsets(image.getROIOffsets());
        
        CPPUNIT_ASSERT(image.getByteSize() == 200 * 100 * 5 * sizeof (unsigned char));
        CPPUNIT_ASSERT(image.getChannelSpace() == ChannelSpace::u_8_1);
        CPPUNIT_ASSERT(image.getEncoding() == Encoding::GRAY);
        
        CPPUNIT_ASSERT(imageDims.size() == 100000);
        CPPUNIT_ASSERT(imageDims.rank() == 3);
        CPPUNIT_ASSERT(imageDims.x1() == 200);
        CPPUNIT_ASSERT(imageDims.x2() == 100);
        CPPUNIT_ASSERT(imageDims.x3() == 5);
        
        CPPUNIT_ASSERT(imageOffsets.rank() == 3);
        CPPUNIT_ASSERT(imageOffsets.x1() == 0);
        CPPUNIT_ASSERT(imageOffsets.x2() == 0);
        CPPUNIT_ASSERT(imageOffsets.x3() == 0);
        
        CPPUNIT_ASSERT(image.getDataType() == "UINT8");
    }
    
}

void ImageData_Test::testSetAndGetMethods() {
    
    Dims dims(200, 100);
    Dims offsets(10, 50);
    std::vector<int> dimTypes(1, 2);
    std::vector<int> someData(dims.size(), 2);
    
    {
        ImageData image;
        
        // Set
        image.setData(&someData[0], someData.size(), true);
        image.setDimensions(dims);
        image.setROIOffsets(offsets);
        image.setDimensionTypes(dimTypes);
        
        // Get
        Dims imageDims(image.getDimensions());
        Dims imageOffsets(image.getROIOffsets());
        std::vector<int> imageDimTypes = image.getDimensionTypes();
        
        CPPUNIT_ASSERT(imageDims.rank() == 2);
        CPPUNIT_ASSERT(imageDims.x1() == 200);
        CPPUNIT_ASSERT(imageDims.x2() == 100);
        
        CPPUNIT_ASSERT(imageOffsets.rank() == 2);
        CPPUNIT_ASSERT(imageOffsets.x1() == 10);
        CPPUNIT_ASSERT(imageOffsets.x2() == 50);
        
        CPPUNIT_ASSERT(imageDimTypes.size() == 2);
        CPPUNIT_ASSERT(imageDimTypes[0] == 1);
        CPPUNIT_ASSERT(imageDimTypes[1] == 2);
         
    }
    
}