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
 * File:   ImageData.hh

 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 20, 2015, 19:35 PM
 */

#ifndef KARABO_XMS_IMAGEDATA_HH
#define KARABO_XMS_IMAGEDATA_HH

#include <karabo/util/ByteSwap.hh>
#include <karabo/util/CustomNodeElement.hh>
#include <karabo/util/Dims.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/NDArray.hh>
#include <karabo/util/Validator.hh>

namespace karabo {
    namespace xms {

        namespace Dimension {

            enum DimensionType {

                UNDEFINED = 0,
                STACK = -1,
                DATA = 1,
            };
        }

        typedef Dimension::DimensionType DimensionType;

        namespace Encoding {

            enum EncodingType {

                UNDEFINED = -1,
                GRAY,
                RGB,
                RGBA,
                BGR,
                BGRA,
                CMYK,
                YUV,
                BAYER,
                JPEG,
                PNG,
                BMP,
                TIFF,
                BAYER_RG,
                BAYER_BG,
                BAYER_GR,
                BAYER_GB,
                YUV444,
                YUV422_YUYV,
                YUV422_UYVY,
            };
            /// True if encoding is such that one can index the underlying array
            bool isIndexable(int encoding);
        } // namespace Encoding

        typedef Encoding::EncodingType EncodingType;

        namespace Rotation {

            enum RotationType {

                UNDEFINED = -1,
                ROT_0 = 0,
                ROT_90 = 90,
                ROT_180 = 180,
                ROT_270 = 270,
            };
        }

        typedef Rotation::RotationType RotationType;


        /**
         * Helper class to store typical image data from color and monochrome cameras.
         * Along the raw pixel values it also stores useful metadata (like encoding, bit depth, binning)
         * and basic transformations (like flip, rotation, ROI).
         * To make ImageData DAQ compliant, one needs to specify the image size and the datatype. The image size can
         * be either 2 or 3D if it is a monochrome/color image.
         *
         * @code
         * IMAGEDATA_ELEMENT(data).key("data.image")
         *     .setDimensions(std::string("480,640,3"))
         *     .setType(util::Types::UINT16)
         *     .setEncoding(xms::Encoding::RGB)
         *     .commit();
         * @endcode
         *
         */
        class ImageData : protected karabo::util::Hash {
           public:
            KARABO_CLASSINFO(ImageData, "ImageData", "1.5")

            static void expectedParameters(karabo::util::Schema& s);

            /**
             * Constructor of an empty ImageData
             *
             * Take care to keep the object consistent if it is later filled with the set-methods.
             */
            ImageData();

            /**
             * Constructor from NDArray where the dimensions of the image data will be deduced from NDArray
             * if encoding is indexable.
             *
             * @param data NDArray - note that the copy of the NDArray kept inside ImageData will refer to the same raw
             *                       memory as this input
             * @param encoding The encoding of the bytes - defaults to GRAY.
             *                 If UNDEFINED, anything matching GRAY, RGB or RGBA will be identified as such.
             * @param bitsPerPixel The number of bits used in the original data. Can be smaller than 8 times
             *                     the size in bytes of the type used in the NDArray 'data'. If zero (default) or
             *                     negative, a value matching the NDArray type will be calculated (8, 16, ...).
             */
            ImageData(const karabo::util::NDArray& data, const EncodingType encoding = Encoding::GRAY,
                      const int bitsPerPixel = 0);

            /**
             * Constructor from NDArray with the possibility to specify other dimensions than NDArray,
             * as needed for non-indexable formats like JPEG, TIFF, ...
             *
             * @param data NDArray - note that the copy of the NDArray kept inside ImageData will refer to the same raw
             *                       memory as this input
             * @param dims The dimensions of the image data - if 'empty' and encoding is indexable, will be deduced from
             * data
             * @param encoding The encoding of the bytes - defaults to GRAY.
             *                 If UNDEFINED, anything matching GRAY, RGB or RGBA will be identified as such.
             * @param bitsPerPixel The number of bits used in the original data. Can be smaller than 8 times
             *                     the size in bytes of the type used in the NDArray 'data'. If zero (default) or
             *                     negative, a value matching the NDArray type will be calculated (8, 16, ...).
             */
            ImageData(const karabo::util::NDArray& data, const karabo::util::Dims& dims,
                      const EncodingType encoding = Encoding::GRAY, const int bitsPerPixel = 0);

            ImageData(const ImageData& other) = default;

            virtual ~ImageData() {}

            /**
             * Get a reference to the underlying image data structure.
             * Interpretation depends on ImageData::getEncoding().
             */
            karabo::util::NDArray& getData();

            /**
             * Get a reference to the underlying image data structure.
             * Interpretation depends on ImageData::getEncoding().
             */
            const karabo::util::NDArray& getData() const;

            /**
             * Set image data.
             *             * Note that the copy stored inside ImageData will refer to the same memory as the input.
             */
            void setData(const karabo::util::NDArray& array);

            karabo::util::Types::ReferenceType getDataType() const;

            void setDataType(const karabo::util::Types::ReferenceType&);

            karabo::util::Dims getROIOffsets() const;

            void setROIOffsets(const karabo::util::Dims& offsets);

            karabo::util::Dims getBinning() const;

            void setBinning(const karabo::util::Dims& binning);

            int getRotation() const;

            void setRotation(const RotationType rotation);

            bool getFlipX() const;
            bool getFlipY() const;

            void setFlipX(const bool flipX);
            void setFlipY(const bool flipY);

            /**
             * Get number of bits per pixel used to achieve the image data.
             * Can be less than number of bits used per pixel in getData().
             */
            int getBitsPerPixel() const;

            /**
             * Set number of bits per pixel used to achieve the image data.
             * Can be less than number of bits used per pixel in getData(), but not more (if a too large value is
             * specified, it will e truncated).
             */
            void setBitsPerPixel(const int bitsPerPixel);

            int getEncoding() const;

            void setEncoding(const int encoding);

            /**
             * Returns true if the image data can be directly indexed.
             */
            bool isIndexable() const {
                return Encoding::isIndexable(getEncoding());
            }

            /**
             * See 'setDimensions' for explanation.
             *
             * @return Dimension object
             */
            karabo::util::Dims getDimensions() const;

            /**
             * Say x = fasted changing, y = medium fast and z = slowest changing index
             * then set the dimension like
             * @code setDimensions(Dims(x,y,z))
             * Or in other words, if you think about width, height and depth use:
             * @code setDimensions(Dims(width, height, depth);
             * For 2D single images, leave away the depth
             * @param dims Dimensionality
             */
            void setDimensions(const karabo::util::Dims& dims);

            const std::vector<int> getDimensionTypes() const;

            void setDimensionTypes(const std::vector<int>& dimTypes);

            const std::string& getDimensionScales() const;

            void setDimensionScales(const std::string& scales);

            ImageData copy() const;

           private:
            static int defaultBitsPerPixel(int encoding, const karabo::util::NDArray& data);
        };

        /**********************************************************************
         * Declaration ImageDataElement
         **********************************************************************/

        class ImageDataElement : public karabo::util::CustomNodeElement<ImageDataElement, ImageData> {
            typedef karabo::util::CustomNodeElement<ImageDataElement, ImageData> ParentType;

           public:
            ImageDataElement(karabo::util::Schema& s) : ParentType(s) {}

            ImageDataElement& setDimensionScales(const std::string& scales) {
                return ParentType::setDefaultValue("dimScales", scales);
            }

            ImageDataElement& setDimensions(const std::string& dimensions) {
                return setDimensions(karabo::util::fromString<unsigned long long, std::vector>(dimensions));
            }

            ImageDataElement& setDimensions(const std::vector<unsigned long long>& dimensions) {
                // It is up to the user to explicitly specify the number of channels for RGB cameras.
                // i.e. for monochrome image it's "480,640" and for color image it must be "480,640,3"
                // Encoding should be set accordingly (but not necessary)

                // Setting shapes
                ImageDataElement& ret = ParentType::setDefaultValue("dims", dimensions);
                ret.setDefaultValue("pixels.shape", dimensions);
                // Setting maximum number of dimensions for all vectors for the DAQ
                ret.setMaxSize("dims", dimensions.size());
                ret.setMaxSize("pixels.shape", dimensions.size());
                ret.setMaxSize("dimTypes", dimensions.size());
                ret.setMaxSize("roiOffsets", dimensions.size());
                ret.setMaxSize("binning", dimensions.size());
                return ret;
            }

            ImageDataElement& setType(const karabo::util::Types::ReferenceType type) {
                return ParentType::setDefaultValue("pixels.type", static_cast<int>(type));
            }

            ImageDataElement& setEncoding(const EncodingType& encoding) {
                return ParentType::setDefaultValue("encoding", (int)encoding);
            }

            void commit() {
                // As ImageDataElement is only used for channel descriptions, it should always be read only.
                readOnly();
                ParentType::commit();
            }
        };

        typedef ImageDataElement IMAGEDATA_ELEMENT; // TODO Discuss whether to support or not the _ELEMENT noise
        typedef ImageDataElement IMAGEDATA;
    } // namespace xms
} // namespace karabo

#endif /* KARABO_XMS_IMAGEDATA_HH */
