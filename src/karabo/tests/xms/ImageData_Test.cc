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

    NDArray arr(Dims(200, 100), 2); // filled with 2s (which are int and thus ReferenceType INT32)
    {
        ImageData image(arr);

        Dims imageDims = image.getDimensions();
        Dims imageOffsets(image.getROIOffsets());
        Dims imageBinning(image.getBinning());

        CPPUNIT_ASSERT(image.getData().size() == 100 * 200);
        CPPUNIT_ASSERT_EQUAL(2, image.getData().getData<int>()[0]);
        CPPUNIT_ASSERT(image.getEncoding() == Encoding::GRAY);
        CPPUNIT_ASSERT(image.isIndexable());

        CPPUNIT_ASSERT(imageDims.size() == 100 * 200);
        CPPUNIT_ASSERT(imageDims.rank() == 2);
        CPPUNIT_ASSERT(imageDims.x1() == 200);
        CPPUNIT_ASSERT(imageDims.x2() == 100);

        CPPUNIT_ASSERT(imageOffsets.rank() == 2);
        CPPUNIT_ASSERT(imageOffsets.x1() == 0);
        CPPUNIT_ASSERT(imageOffsets.x2() == 0);

        CPPUNIT_ASSERT(imageBinning.rank() == 2);
        CPPUNIT_ASSERT(imageBinning.x1() == 1);
        CPPUNIT_ASSERT(imageBinning.x2() == 1);

        CPPUNIT_ASSERT_EQUAL(32, image.getBitsPerPixel());

        CPPUNIT_ASSERT_EQUAL(std::string(), image.getDimensionScales());
        CPPUNIT_ASSERT_EQUAL((size_t) 2u, image.getDimensionTypes().size());

        // No header was specified: get an empty one
        karabo::util::Hash header;
        CPPUNIT_ASSERT_NO_THROW(header = image.getHeader());
        CPPUNIT_ASSERT(header.empty());

        // Don't care about what kind of geometry - but it must not throw...
        CPPUNIT_ASSERT_NO_THROW(image.getGeometry());
    }
    {
        // Default constructor - we do not mind values, but all getters must not throw!
        ImageData image;
        CPPUNIT_ASSERT_NO_THROW(image.getBitsPerPixel());
        CPPUNIT_ASSERT_NO_THROW(image.getData());
        CPPUNIT_ASSERT_NO_THROW(image.getDimensionScales());
        CPPUNIT_ASSERT_NO_THROW(image.getDimensionTypes());
        CPPUNIT_ASSERT_NO_THROW(image.getDimensions());
        CPPUNIT_ASSERT_NO_THROW(image.getEncoding());
        CPPUNIT_ASSERT_NO_THROW(image.getGeometry());
        CPPUNIT_ASSERT_NO_THROW(image.getHeader());
        CPPUNIT_ASSERT_NO_THROW(image.getROIOffsets());
        CPPUNIT_ASSERT_NO_THROW(image.getBinning());
    }
}


void ImageData_Test::testSetAndGetMethods() {

    Dims dims(200, 100); // height, width
    Dims offsets(10, 50);
    Dims binning(3, 8);
    int tmp[] = {Dimension::DATA, Dimension::STACK};
    std::vector<int> dimTypes(tmp, tmp + 2);
    std::vector<unsigned char> someData(dims.size(), 2); // i.e. type UINT8

    {
        NDArray arr(&someData[0], someData.size());

        // Set
        ImageData image(arr);
        image.setDimensions(dims);
        image.setROIOffsets(offsets);
        image.setBinning(binning);
        image.setDimensionTypes(dimTypes);
        image.setHeader(Hash("one", 1));

        // Get
        Dims imageDims(image.getDimensions());
        Dims imageOffsets(image.getROIOffsets());
        Dims imageBinning(image.getBinning());
        std::vector<int> imageDimTypes = image.getDimensionTypes();

        CPPUNIT_ASSERT(imageDims.rank() == 2);
        CPPUNIT_ASSERT(imageDims.x1() == 200);
        CPPUNIT_ASSERT(imageDims.x2() == 100);

        CPPUNIT_ASSERT(imageOffsets.rank() == 2);
        CPPUNIT_ASSERT(imageOffsets.x1() == 10);
        CPPUNIT_ASSERT(imageOffsets.x2() == 50);

        CPPUNIT_ASSERT(imageBinning.rank() == 2);
        CPPUNIT_ASSERT(imageBinning.x1() == 3);
        CPPUNIT_ASSERT(imageBinning.x2() == 8);

        CPPUNIT_ASSERT(imageDimTypes.size() == 2);
        CPPUNIT_ASSERT(imageDimTypes[0] == Dimension::DATA);
        CPPUNIT_ASSERT(imageDimTypes[1] == Dimension::STACK);

        CPPUNIT_ASSERT_EQUAL(1, image.getHeader().get<int>("one"));

        CPPUNIT_ASSERT_EQUAL(8, image.getBitsPerPixel()); // as determined for UINT8
        image.setBitsPerPixel(5);
        CPPUNIT_ASSERT_EQUAL(5, image.getBitsPerPixel());
        // Setting to larger than what the type can carry will be manipulated to maximum of type
        image.setBitsPerPixel(10);
        CPPUNIT_ASSERT_EQUAL(8, image.getBitsPerPixel());
    }

}