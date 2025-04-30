/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   ImageData_Test.cc
 * Author: parenti
 *
 * Created on May 21, 2015, 3:05:22 PM
 */

#include "ImageData_Test.hh"

#include "karabo/data/types/NDArray.hh"

using namespace std;
using namespace karabo::data;
using namespace karabo::data;
using namespace karabo::net;
using namespace karabo::xms;

CPPUNIT_TEST_SUITE_REGISTRATION(ImageData_Test);


ImageData_Test::ImageData_Test() {}


ImageData_Test::~ImageData_Test() {}


void ImageData_Test::setUp() {}


void ImageData_Test::tearDown() {}


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
        CPPUNIT_ASSERT_EQUAL((size_t)2u, image.getDimensionTypes().size());
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
    DimensionType tmp[] = {DimensionType::DATA, DimensionType::STACK};
    std::vector<DimensionType> dimTypes(tmp, tmp + 2);
    std::vector<unsigned char> someData(dims.size(), 2); // i.e. type UINT8

    {
        NDArray arr(&someData[0], someData.size());

        // Set
        ImageData image(arr);
        image.setDimensions(dims);
        image.setROIOffsets(offsets);
        image.setBinning(binning);
        image.setRotation(Rotation::ROT_90);
        // false/true flip is tested in bound
        image.setFlipX(true);
        image.setFlipY(false);
        image.setDimensionTypes(dimTypes);

        // Get
        Dims imageDims(image.getDimensions());
        Dims imageOffsets(image.getROIOffsets());
        Dims imageBinning(image.getBinning());
        std::vector<DimensionType> imageDimTypes = image.getDimensionTypes();

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
        CPPUNIT_ASSERT(imageDimTypes[0] == DimensionType::DATA);
        CPPUNIT_ASSERT(imageDimTypes[1] == DimensionType::STACK);

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
        Dims dims(640, 480, 4);             // Dont care about dimension order
        NDArray arr_v2(dims, Types::INT16); // Will be interpreted by default as RGBA


        Dims offsets_valid(10, 50, 0);
        Dims offsets_wrong_length(10, 50); // Wrong Length

        // If the encoding was manually set, setData() should not change it
        image2.setEncoding(Encoding::BGRA);
        // Fill the image (this automatically sets a few parameters)
        image2.setData(arr_v2);

        // Test valid and invalid values for offset
        CPPUNIT_ASSERT_NO_THROW(image2.setROIOffsets(offsets_valid));
        CPPUNIT_ASSERT_THROW(image2.setROIOffsets(offsets_wrong_length), karabo::data::ParameterException);

        CPPUNIT_ASSERT_EQUAL(image2.getEncoding(), Encoding::BGRA);
        CPPUNIT_ASSERT_EQUAL(image2.getDataType(), Types::INT16);
        CPPUNIT_ASSERT_EQUAL(image2.getBitsPerPixel(), 64);

        image2.setDataType(Types::UINT16);
        CPPUNIT_ASSERT_EQUAL(image2.getDataType(), Types::UINT16);
    }
}


void ImageData_Test::testImageDataElement() {
    // Testing if the shape, maximum size and data type was set correctly in the schema (required for DAQ)
    Schema sch;
    IMAGEDATA_ELEMENT(sch)
          .key("ide")
          .setDimensions("480,640,3")
          .setType(Types::INT16)
          .setEncoding(Encoding::RGB)
          .commit();

    {
        // Testing max size
        CPPUNIT_ASSERT_EQUAL(3u, sch.getMaxSize("ide.pixels.shape"));
        CPPUNIT_ASSERT_EQUAL(3u, sch.getMaxSize("ide.dims"));
        CPPUNIT_ASSERT_EQUAL(3u, sch.getMaxSize("ide.dimTypes"));
        CPPUNIT_ASSERT_EQUAL(3u, sch.getMaxSize("ide.roiOffsets"));
        CPPUNIT_ASSERT_EQUAL(3u, sch.getMaxSize("ide.binning"));

        // Testing shapes
        CPPUNIT_ASSERT(sch.getDefaultValueAs<std::string>("ide.pixels.shape") == "480,640,3");
        CPPUNIT_ASSERT(sch.getDefaultValueAs<std::string>("ide.dims") == "480,640,3");
        CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long>>("ide.dims") ==
                       std::vector<unsigned long long>({480, 640, 3}));

        // Testing datatypes
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(Encoding::RGB), sch.getDefaultValueAs<int>("ide.encoding"));
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(Types::INT16), sch.getDefaultValueAs<int>("ide.pixels.type"));
    }

    // Test default data type
    IMAGEDATA_ELEMENT(sch).key("ide2").commit();
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(Types::UNKNOWN), sch.getDefaultValue<int>("ide2.pixels.type"));

    IMAGEDATA_ELEMENT(sch).key("ide3").setDimensions(std::vector<unsigned long long>({480ull, 640ull})).commit();
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long>>("ide3.dims") ==
                   std::vector<unsigned long long>({480ull, 640ull}));
}
