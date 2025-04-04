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
 * File:   ImageData.cc
 * Author:
 *
 * Created on April 20, 2015, 19:35 PM
 */

#include "ImageData.hh"

#include <climits>

#include "karabo/data/schema/NDArrayElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/data/types/ToSize.hh"
#include "karabo/data/types/Types.hh"
#include "karabo/data/types/Units.hh"

namespace karabo {
    namespace xms {

        using namespace karabo::data;


        bool Encoding::isIndexable(int encoding) {
            switch (encoding) {
                case Encoding::UNDEFINED:
                    return false;
                case Encoding::GRAY:
                case Encoding::RGB:
                case Encoding::RGBA:
                case Encoding::BGR:
                case Encoding::BGRA:
                case Encoding::CMYK:
                case Encoding::YUV:
                case Encoding::BAYER:
                case Encoding::YUV444:
                case Encoding::YUV422_YUYV:
                case Encoding::YUV422_UYVY:
                case Encoding::BAYER_RG:
                case Encoding::BAYER_BG:
                case Encoding::BAYER_GR:
                case Encoding::BAYER_GB:
                    return true;
                case Encoding::JPEG:
                case Encoding::PNG:
                case Encoding::BMP:
                case Encoding::TIFF:
                    return false;
                default:
                    throw KARABO_LOGIC_EXCEPTION("Encoding " + karabo::data::toString(encoding) + " invalid.");
                    return false; // pleasing the compiler
            }
        }


        void ImageData::expectedParameters(karabo::data::Schema& s) {
            NDARRAY_ELEMENT(s)
                  .key("pixels")
                  .displayedName("Pixel Data")
                  .description("The N-dimensional array containing the pixels")
                  .readOnly()
                  .commit();

            VECTOR_UINT64_ELEMENT(s)
                  .key("dims")
                  .displayedName("Dimensions")
                  .description(
                        "The length of the array reflects total dimensionality and each element the extension in this "
                        "dimension")
                  .readOnly()
                  .commit();

            VECTOR_INT32_ELEMENT(s)
                  .key("dimTypes")
                  .displayedName("Dimension Types")
                  .description("Any dimension should have an enumerated type")
                  .readOnly()
                  .commit();

            STRING_ELEMENT(s).key("dimScales").displayedName("Dimension Scales").description("").readOnly().commit();

            INT32_ELEMENT(s)
                  .key("encoding")
                  .displayedName("Encoding")
                  .description(
                        "Describes the color space of pixel encoding of"
                        " the data (e.g. GRAY, RGB, JPG, PNG etc.).")
                  .readOnly()
                  .commit();

            INT32_ELEMENT(s)
                  .key("bitsPerPixel")
                  .displayedName("Bits per pixel")
                  .description("The number of bits needed for each pixel")
                  .readOnly()
                  .commit();

            VECTOR_UINT64_ELEMENT(s)
                  .key("roiOffsets")
                  .displayedName("ROI Offsets")
                  .description(
                        "The offset of the Region-of-Interest (ROI); "
                        "it will contain zeros if the image has no ROI defined.")
                  .readOnly()
                  .commit();

            VECTOR_UINT64_ELEMENT(s)
                  .key("binning")
                  .displayedName("Binning")
                  .description(
                        "The number of binned adjacent pixels. They "
                        "are reported out of the camera as a single pixel.")
                  .readOnly()
                  .commit();

            INT32_ELEMENT(s)
                  .key("rotation")
                  .displayedName("Rotation")
                  .description("The image counterclockwise rotation.")
                  .options(std::vector<int>({0, 90, 180, 270}))
                  .unit(Unit::DEGREE)
                  .readOnly()
                  .commit();

            BOOL_ELEMENT(s)
                  .key("flipX")
                  .displayedName("Flip X")
                  .description("Image horizontal flip.")
                  .readOnly()
                  .commit();

            BOOL_ELEMENT(s)
                  .key("flipY")
                  .displayedName("Flip Y")
                  .description("Image vertical flip.")
                  .readOnly()
                  .commit();
        }


        ImageData::ImageData() : ImageData(karabo::data::NDArray(karabo::data::Dims())) {}


        ImageData::ImageData(const karabo::data::NDArray& data, const EncodingType encoding, const int bitsPerPixel)
            : ImageData(data, karabo::data::Dims(), encoding, bitsPerPixel) {}


        ImageData::ImageData(const karabo::data::NDArray& data, const karabo::data::Dims& dims,
                             const EncodingType encoding, const int bitsPerPixel) {
            setData(data);

            // Encoding might be deduced from data if not defined
            Dims dataDims(data.getShape());
            const int rank = dataDims.rank();
            EncodingType finalEncoding = encoding;
            if (encoding == Encoding::UNDEFINED) {
                // No encoding info -> try to guess it from ndarray shape
                if (rank == 2 || (rank == 3 && dataDims.x3() == 1)) {
                    finalEncoding = Encoding::GRAY;
                } else if (rank == 3 && dataDims.x3() == 3) {
                    finalEncoding = Encoding::RGB;
                } else if (rank == 3 && dataDims.x3() == 4) {
                    finalEncoding = Encoding::RGBA;
                } else if (rank == 3) {
                    // Assume it is a stack of GRAY images
                    finalEncoding = Encoding::GRAY;
                }
            }
            setEncoding(finalEncoding);

            // If Dims are not defined, they can be deduced from data as well in many cases
            if (dims.size() == 0) {
                if (!Encoding::isIndexable(finalEncoding)) {
                    throw KARABO_LOGIC_EXCEPTION("Dimensions must be supplied for encoded images");
                }
            } else {
                dataDims = dims;
            }

            // After setEncoding one can set dimensions
            setDimensions(dataDims);

            // bits per pixel - may calculate default, depending on type
            setBitsPerPixel((bitsPerPixel > 0 ? bitsPerPixel : defaultBitsPerPixel(finalEncoding, data)));

            const std::vector<unsigned long long> offsets(dataDims.rank(), 0ull);
            setROIOffsets(karabo::data::Dims(offsets));

            const std::vector<unsigned long long> binning(dataDims.rank(), 1ull);
            setBinning(karabo::data::Dims(binning));

            setRotation(Rotation::ROT_0);

            setFlipX(false);
            setFlipY(false);

            setDimensionScales(std::string());
        }


        karabo::data::Dims ImageData::getROIOffsets() const {
            return karabo::data::Dims(get<std::vector<unsigned long long> >("roiOffsets"));
        }


        void ImageData::setROIOffsets(const karabo::data::Dims& offsets) {
            // Make sure that ROI is within the image
            auto imgSize = get<std::vector<unsigned long long> >("dims");
            auto newOffset = offsets.toVector();

            if (newOffset.size() != imgSize.size()) {
                std::string msg =
                      "ImageData ROI must have the same length as the image shape: " + std::to_string(imgSize.size());
                throw KARABO_PARAMETER_EXCEPTION(msg);
            }

            set<std::vector<unsigned long long> >("roiOffsets", offsets.toVector());
        }


        karabo::data::Dims ImageData::getBinning() const {
            return karabo::data::Dims(get<std::vector<unsigned long long> >("binning"));
        }


        void ImageData::setBinning(const karabo::data::Dims& binning) {
            set<std::vector<unsigned long long> >("binning", binning.toVector());
        }


        int ImageData::getRotation() const {
            return get<int>("rotation");
        }


        void ImageData::setRotation(const RotationType rotation) {
            set<int>("rotation", rotation);
        }


        bool ImageData::getFlipX() const {
            return get<bool>("flipX");
        }


        bool ImageData::getFlipY() const {
            return get<bool>("flipY");
        }


        void ImageData::setFlipX(const bool flipX) {
            set<bool>("flipX", flipX);
        }


        void ImageData::setFlipY(const bool flipY) {
            set<bool>("flipY", flipY);
        }


        int ImageData::getBitsPerPixel() const {
            return get<int>("bitsPerPixel");
        }


        void ImageData::setBitsPerPixel(const int bitsPerPixel) {
            // Maximum depends on type in data and on encoding.
            // But if encoding cannot specify a maximum, just believe the input.
            const int maxBitsPerPixel = defaultBitsPerPixel(getEncoding(), getData());
            const int finalBitsPerPixel =
                  (maxBitsPerPixel == 0 ? bitsPerPixel : std::min<int>(bitsPerPixel, maxBitsPerPixel));
            set<int>("bitsPerPixel", finalBitsPerPixel);
        }


        int ImageData::getEncoding() const {
            return get<int>("encoding");
        }


        void ImageData::setEncoding(const int encoding) {
            set<int>("encoding", encoding);
        }


        karabo::data::Dims ImageData::getDimensions() const {
            return karabo::data::Dims(get<std::vector<unsigned long long> >("dims"));
        }


        void ImageData::setDimensions(const karabo::data::Dims& dims) {
            size_t rank = dims.rank();
            if (dims.size() == 0) {
                // Will use the shape information of underlying NDArray as best guess
                std::vector<unsigned long long> shape = get<NDArray>("pixels").getShape().toVector();
                set("dims", shape);
                rank = shape.size();
            } else {
                if (has("encoding")) {
                    if (Encoding::isIndexable(getEncoding())) {
                        // Make sure dimensions match the size of the data for indexable encodings
                        get<NDArray>("pixels").setShape(dims); // throws if size does not fit
                    }
                } else {
                    // Set the key, if it does not exist to avoid future exceptions
                    set<int>("encoding", Encoding::UNDEFINED);
                }
                set<std::vector<unsigned long long> >("dims", dims.toVector());
            }
            // In case the dimensionTypes were not yet set, inject a default here
            if (!has("dimTypes")) {
                setDimensionTypes(std::vector<int>(rank, Dimension::UNDEFINED));
            }
        }


        const std::vector<int> ImageData::getDimensionTypes() const {
            return get<std::vector<int> >("dimTypes");
        }


        void ImageData::setDimensionTypes(const std::vector<int>& dimTypes) {
            set<std::vector<int> >("dimTypes", dimTypes);
        }


        const std::string& ImageData::getDimensionScales() const {
            return get<std::string>("dimScales");
        }


        void ImageData::setDimensionScales(const std::string& scales) {
            set("dimScales", scales);
        }


        karabo::data::NDArray& ImageData::getData() {
            return get<karabo::data::NDArray>("pixels");
        }


        const karabo::data::NDArray& ImageData::getData() const {
            return get<karabo::data::NDArray>("pixels");
        }


        void ImageData::setData(const karabo::data::NDArray& array) {
            set<karabo::data::NDArray>("pixels", array);

            // We can't set dimensions without setting the encoding
            // First, make sure that the "encoding" key exists...
            // If you don't like the defaults set it yourself manually...
            std::size_t shapeRank = array.getShape().rank();

            if (!has("encoding")) {
                if (shapeRank == 3) {
                    if (array.getShape().x3() == 1) {
                        set<int>("encoding", Encoding::GRAY);
                    } else if (array.getShape().x3() == 3) {
                        set<int>("encoding", Encoding::RGB);
                    } else if (array.getShape().x3() == 4) {
                        set<int>("encoding", Encoding::RGBA);
                    } else {
                        set<int>("encoding", Encoding::UNDEFINED);
                    }
                } else if (shapeRank == 2) {
                    set<int>("encoding", Encoding::GRAY);
                } else {
                    set<int>("encoding", Encoding::UNDEFINED);
                }
            }

            // Once the encoding is definitely set, we can set the dimensions
            setDimensions(array.getShape());

            // Finally we set the bits per pixels
            defaultBitsPerPixel(get<int>("encoding"), array);
        }


        karabo::data::Types::ReferenceType ImageData::getDataType() const {
            return static_cast<karabo::data::Types::ReferenceType>(get<int>("pixels.type"));
        }


        void ImageData::setDataType(const karabo::data::Types::ReferenceType& type) {
            set("pixels.type", static_cast<int>(type));
        }


        int ImageData::defaultBitsPerPixel(int encoding, const karabo::data::NDArray& data) {
            const size_t numBytes = karabo::data::Types::to<karabo::data::ToSize>(data.getType());

            int factor = -1;
            switch (encoding) {
                case Encoding::GRAY:
                    factor = 1;
                    break;
                case Encoding::BAYER:
                    return numBytes * 8; // Plain 8 in http://www.ni.com/white-paper/3903/en/, independent of CHAR_BIT!
                case Encoding::RGB:
                case Encoding::BGR:
                case Encoding::YUV:
                    // NDArray's Dims.x3 should be 3
                    factor = 3;
                    break;
                case Encoding::RGBA:
                case Encoding::BGRA:
                case Encoding::CMYK:
                    // NDArray's Dims.x3 should be 4
                    factor = 4;
                    break;
                default:
                    // JPEG, PNG, BMP, TIFF, UNDEFINED: return 0 to indicate that it is not defined
                    factor = 0;
            }
            return factor * numBytes * CHAR_BIT; // CHAR_BIT from <climits> - usually 8
        }


        ImageData ImageData::copy() const {
            return ImageData(getData().copy(), EncodingType(getEncoding()), getBitsPerPixel());
        }
    } // namespace xms
} // namespace karabo
