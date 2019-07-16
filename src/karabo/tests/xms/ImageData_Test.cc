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
    {
        NDArray arr(Dims(200, 100), 2); // filled with 2s (which are int and thus ReferenceType INT32)
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

        CPPUNIT_ASSERT(image.getRotation() == Rotation::ROT_0);

        CPPUNIT_ASSERT_EQUAL(image.getFlipX(), false);
        CPPUNIT_ASSERT_EQUAL(image.getFlipY(), false);

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
        Dims dims(200, 100, 3);
        NDArray arr(dims, 2); // Will be interpreted by default as RGB

        ImageData image1(arr, Encoding::UNDEFINED);
        CPPUNIT_ASSERT(image1.getEncoding() == Encoding::RGB);

        ImageData image2(arr, dims, Encoding::UNDEFINED);
        CPPUNIT_ASSERT(image2.getEncoding() == Encoding::RGB);
    }
    {
        Dims dims(200, 100, 4);
        NDArray arr(dims, 2); // Will be interpreted by default as RGBA

        ImageData image1(arr, Encoding::UNDEFINED);
        CPPUNIT_ASSERT(image1.getEncoding() == Encoding::RGBA);

        ImageData image2(arr, dims, Encoding::UNDEFINED);
        CPPUNIT_ASSERT(image2.getEncoding() == Encoding::RGBA);
    }
    {
        Dims dims(200, 100, 11);
        NDArray arr(dims, 2); // Will be interpreted by default as a stack of GRAY images

        ImageData image1(arr, Encoding::UNDEFINED);
        CPPUNIT_ASSERT(image1.getEncoding() == Encoding::GRAY);

        ImageData image2(arr, dims, Encoding::UNDEFINED);
        CPPUNIT_ASSERT(image2.getEncoding() == Encoding::GRAY);
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
        CPPUNIT_ASSERT_NO_THROW(image.getRotation());
        CPPUNIT_ASSERT_NO_THROW(image.getFlipX());
        CPPUNIT_ASSERT_NO_THROW(image.getFlipY());
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
        image.setRotation(Rotation::ROT_90);
        // false/true flip is tested in bound_api
        image.setFlipX(true);
        image.setFlipY(false);
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

        CPPUNIT_ASSERT(image.getRotation() == Rotation::ROT_90);

        CPPUNIT_ASSERT_EQUAL(image.getFlipX(), true);
        CPPUNIT_ASSERT_EQUAL(image.getFlipY(), false);

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

    // Unit tests for automatic assignments and range checks
    {
        ImageData image2;
        Dims dims(640, 480, 4); // Dont care about dimension order
        NDArray arr_v2(dims, Types::INT16); // Will be interpreted by default as RGBA


        Dims offsets_valid(10, 50, 0);
        Dims offsets_wrong_length(10, 50); // Wrong Length

        // If the encoding was manually set, setData() should not change it
        image2.setEncoding(EncodingType::BGRA);
        // Fill the image (this automatically sets a few parameters)
        image2.setData(arr_v2);

        // Test valid and invalid values for offset
        CPPUNIT_ASSERT_NO_THROW(image2.setROIOffsets(offsets_valid));
        CPPUNIT_ASSERT_THROW(image2.setROIOffsets(offsets_wrong_length), std::exception);

        CPPUNIT_ASSERT_EQUAL(image2.getEncoding(), static_cast<int> (EncodingType::BGRA));
        CPPUNIT_ASSERT_EQUAL(image2.getDataType(), Types::INT16);
        CPPUNIT_ASSERT_EQUAL(image2.getBitsPerPixel(), 64);

        image2.setDataType(Types::UINT16);
        CPPUNIT_ASSERT_EQUAL(image2.getDataType(), Types::UINT16);
    }
}


void ImageData_Test::testImageDataElementMaxSize() {
    // Testing if the shape, maximum size and data type was set correctly in the schema (required for DAQ)
    Schema sch;
    IMAGEDATA_ELEMENT(sch).key("ide").setDimensions("480,640,3")
            .setType(Types::INT16).setEncoding(EncodingType::RGB).commit();

    {
        // Testing max size
        CPPUNIT_ASSERT_EQUAL((int) sch.getMaxSize("ide.pixels.shape"), 3);
        CPPUNIT_ASSERT_EQUAL((int) sch.getMaxSize("ide.dims"), 3);
        CPPUNIT_ASSERT_EQUAL((int) sch.getMaxSize("ide.dimTypes"), 3);
        CPPUNIT_ASSERT_EQUAL((int) sch.getMaxSize("ide.roiOffsets"), 3);
        CPPUNIT_ASSERT_EQUAL((int) sch.getMaxSize("ide.binning"), 3);

        // Testing shapes
        CPPUNIT_ASSERT(sch.getDefaultValueAs<std::string>("ide.pixels.shape") == "480,640,3");
        CPPUNIT_ASSERT(sch.getDefaultValueAs<std::string>("ide.dims") == "480,640,3");

        // Testing datatypes
        CPPUNIT_ASSERT_EQUAL(static_cast<int> (EncodingType::RGB), sch.getDefaultValueAs<int>("ide.encoding"));
        CPPUNIT_ASSERT_EQUAL(static_cast<int> (Types::INT16), sch.getDefaultValueAs<int>("ide.pixels.type"));

    }
}