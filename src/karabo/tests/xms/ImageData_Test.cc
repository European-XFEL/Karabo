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

    NDArray arr(Dims(200, 100), 2);
    {
        ImageData image(arr);

        Dims imageDims = image.getDimensions();
        Dims imageOffsets(image.getROIOffsets());

        CPPUNIT_ASSERT(image.getData().size() == 100 * 200);
        CPPUNIT_ASSERT(image.getEncoding() == Encoding::GRAY);
        CPPUNIT_ASSERT(image.isIndexable());

        CPPUNIT_ASSERT(imageDims.size() == 100 * 200);
        CPPUNIT_ASSERT(imageDims.rank() == 2);
        CPPUNIT_ASSERT(imageDims.x1() == 200);
        CPPUNIT_ASSERT(imageDims.x2() == 100);

        CPPUNIT_ASSERT(imageOffsets.rank() == 2);
        CPPUNIT_ASSERT(imageOffsets.x1() == 0);
        CPPUNIT_ASSERT(imageOffsets.x2() == 0);
    }
}


void ImageData_Test::testSetAndGetMethods() {

    Dims dims(200, 100); // width, height
    Dims offsets(10, 50);
    int tmp[] = {Dimension::DATA, Dimension::STACK};
    std::vector<int> dimTypes(tmp, tmp + 2);
    std::vector<unsigned char> someData(dims.size(), 2);

    {
        NDArray arr(&someData[0], someData.size());

        // Set
        ImageData image(arr);
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
        CPPUNIT_ASSERT(imageDimTypes[0] == Dimension::DATA);
        CPPUNIT_ASSERT(imageDimTypes[1] == Dimension::STACK);

    }

}