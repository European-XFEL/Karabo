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

#include <gtest/gtest.h>

#include <karabo/xms.hpp>

#include "karabo/data/types/NDArray.hh"

using namespace std;
using namespace karabo::data;
using namespace karabo::data;
using namespace karabo::net;
using namespace karabo::xms;


TEST(TestImageData, testConstructor) {
    {
        NDArray arr(Dims(200, 100), 2); // filled with 2s (which are int and thus ReferenceType INT32)
        ImageData image(arr);

        Dims imageDims = image.getDimensions();
        Dims imageOffsets(image.getROIOffsets());
        Dims imageBinning(image.getBinning());

        EXPECT_TRUE(image.getData().size() == 100 * 200);
        ASSERT_EQ(2, image.getData().getData<int>()[0]);
        EXPECT_TRUE(image.getEncoding() == Encoding::GRAY);
        EXPECT_TRUE(image.isIndexable());

        EXPECT_TRUE(imageDims.size() == 100 * 200);
        EXPECT_TRUE(imageDims.rank() == 2);
        EXPECT_TRUE(imageDims.x1() == 200);
        EXPECT_TRUE(imageDims.x2() == 100);

        EXPECT_TRUE(imageOffsets.rank() == 2);
        EXPECT_TRUE(imageOffsets.x1() == 0);
        EXPECT_TRUE(imageOffsets.x2() == 0);

        EXPECT_TRUE(imageBinning.rank() == 2);
        EXPECT_TRUE(imageBinning.x1() == 1);
        EXPECT_TRUE(imageBinning.x2() == 1);

        EXPECT_TRUE(image.getRotation() == Rotation::ROT_0);

        ASSERT_EQ(image.getFlipX(), false);
        ASSERT_EQ(image.getFlipY(), false);

        ASSERT_EQ(32, image.getBitsPerPixel());
    }
    {
        Dims dims(200, 100, 3);
        NDArray arr(dims, 2); // Will be interpreted by default as RGB

        ImageData image1(arr, Encoding::UNDEFINED);
        EXPECT_TRUE(image1.getEncoding() == Encoding::RGB);

        ImageData image2(arr, dims, Encoding::UNDEFINED);
        EXPECT_TRUE(image2.getEncoding() == Encoding::RGB);
    }
    {
        Dims dims(200, 100, 4);
        NDArray arr(dims, 2); // Will be interpreted by default as RGBA

        ImageData image1(arr, Encoding::UNDEFINED);
        EXPECT_TRUE(image1.getEncoding() == Encoding::RGBA);

        ImageData image2(arr, dims, Encoding::UNDEFINED);
        EXPECT_TRUE(image2.getEncoding() == Encoding::RGBA);
    }
    {
        Dims dims(200, 100, 11);
        NDArray arr(dims, 2); // Will be interpreted by default as a stack of GRAY images

        ImageData image1(arr, Encoding::UNDEFINED);
        EXPECT_TRUE(image1.getEncoding() == Encoding::GRAY);

        ImageData image2(arr, dims, Encoding::UNDEFINED);
        EXPECT_TRUE(image2.getEncoding() == Encoding::GRAY);
    }
    {
        // Default constructor - we do not mind values, but all getters must not throw!
        ImageData image;
        EXPECT_NO_THROW(image.getBitsPerPixel());
        EXPECT_NO_THROW(image.getData());
        EXPECT_NO_THROW(image.getDimensions());
        EXPECT_NO_THROW(image.getEncoding());
        EXPECT_NO_THROW(image.getROIOffsets());
        EXPECT_NO_THROW(image.getBinning());
        EXPECT_NO_THROW(image.getRotation());
        EXPECT_NO_THROW(image.getFlipX());
        EXPECT_NO_THROW(image.getFlipY());
    }
}


TEST(TestImageData, testSetAndGetMethods) {
    Dims dims(200, 100); // height, width
    Dims offsets(10, 50);
    Dims binning(3, 8);
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

        // Get
        Dims imageDims(image.getDimensions());
        Dims imageOffsets(image.getROIOffsets());
        Dims imageBinning(image.getBinning());

        EXPECT_TRUE(imageDims.rank() == 2);
        EXPECT_TRUE(imageDims.x1() == 200);
        EXPECT_TRUE(imageDims.x2() == 100);

        EXPECT_TRUE(imageOffsets.rank() == 2);
        EXPECT_TRUE(imageOffsets.x1() == 10);
        EXPECT_TRUE(imageOffsets.x2() == 50);

        EXPECT_TRUE(imageBinning.rank() == 2);
        EXPECT_TRUE(imageBinning.x1() == 3);
        EXPECT_TRUE(imageBinning.x2() == 8);

        EXPECT_TRUE(image.getRotation() == Rotation::ROT_90);

        ASSERT_EQ(image.getFlipX(), true);
        ASSERT_EQ(image.getFlipY(), false);
        ASSERT_EQ(8, image.getBitsPerPixel()); // as determined for UINT8
        image.setBitsPerPixel(5);
        ASSERT_EQ(5, image.getBitsPerPixel());
        // Setting to larger than what the type can carry will be manipulated to maximum of type
        image.setBitsPerPixel(10);
        ASSERT_EQ(8, image.getBitsPerPixel());
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
        EXPECT_NO_THROW(image2.setROIOffsets(offsets_valid));
        EXPECT_THROW(image2.setROIOffsets(offsets_wrong_length), karabo::data::ParameterException);

        ASSERT_EQ(image2.getEncoding(), Encoding::BGRA);
        ASSERT_EQ(image2.getDataType(), Types::INT16);
        ASSERT_EQ(image2.getBitsPerPixel(), 64);

        image2.setDataType(Types::UINT16);
        ASSERT_EQ(image2.getDataType(), Types::UINT16);
    }
}


TEST(TestImageData, testImageDataElement) {
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
        ASSERT_EQ(3u, sch.getMaxSize("ide.pixels.shape"));
        ASSERT_EQ(3u, sch.getMaxSize("ide.dims"));
        ASSERT_EQ(3u, sch.getMaxSize("ide.roiOffsets"));
        ASSERT_EQ(3u, sch.getMaxSize("ide.binning"));

        // Testing shapes
        EXPECT_TRUE(sch.getDefaultValueAs<std::string>("ide.pixels.shape") == "480,640,3");
        EXPECT_TRUE(sch.getDefaultValueAs<std::string>("ide.dims") == "480,640,3");
        EXPECT_TRUE(sch.getDefaultValue<std::vector<unsigned long long>>("ide.dims") ==
                    std::vector<unsigned long long>({480, 640, 3}));

        // Testing datatypes
        ASSERT_EQ(static_cast<int>(Encoding::RGB), sch.getDefaultValueAs<int>("ide.encoding"));
        ASSERT_EQ(static_cast<int>(Types::INT16), sch.getDefaultValueAs<int>("ide.pixels.type"));
    }

    // Test default data type
    IMAGEDATA_ELEMENT(sch).key("ide2").commit();
    ASSERT_EQ(static_cast<int>(Types::UNKNOWN), sch.getDefaultValue<int>("ide2.pixels.type"));

    IMAGEDATA_ELEMENT(sch).key("ide3").setDimensions(std::vector<unsigned long long>({480ull, 640ull})).commit();
    EXPECT_TRUE(sch.getDefaultValue<std::vector<unsigned long long>>("ide3.dims") ==
                std::vector<unsigned long long>({480ull, 640ull}));
}
