/*
 * File:   ImageData.hh

 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 20, 2015, 19:35 PM
 */

#ifndef KARABO_XMS_IMAGEDATA_HH
#define	KARABO_XMS_IMAGEDATA_HH

#include <karabo/util/Hash.hh>
#include <karabo/util/Dims.hh>
#include <karabo/util/Validator.hh>
#include <karabo/util/NDArray.hh>
#include <karabo/util/DetectorGeometry.hh>
#include <karabo/util/ByteSwap.hh>
#include <karabo/util/CustomNodeElement.hh>

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
            };
            /// True if encoding is such that one can index the underlying array
            bool isIndexable(int encoding);
        }

        typedef Encoding::EncodingType EncodingType;

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
             * Constructor from NDArray
             *
             * @param data NDArray - note that the copy of the NDArray kept inside ImageData will refer to the same raw
             *                       memory as this input
             * @param encoding The encoding of the bytes - defaults to GRAY.
             *                 If UNDEFINED, anything matching GRAY, RGB or RGBA will be identified as such.
             * @param bitsPerPixel The number of bits used in the original data. Can be smaller than 8 times
             *                     the size in bytes of the type used in the NDArray 'data'. If zero (default) or
             *                     negative, a value matching the NDArray type will be calculated (8, 16, ...).
             */
            ImageData(const karabo::util::NDArray& data,
                      const EncodingType encoding = Encoding::GRAY,
                      const int bitsPerPixel = 0);

            /**
             * Constructor from NDArray with the possibility to specify other dimensions than NDArray,
             * as needed for non-indexable formats like JPEG, TIFF, ...
             *
             * @param data NDArray - note that the copy of the NDArray kept inside ImageData will refer to the same raw
             *                       memory as this input
             * @param dims The dimensions of the image data - if 'empty' and encoding is indexable, will be deduced from data
             * @param encoding The encoding of the bytes - defaults to GRAY.
             *                 If UNDEFINED, anything matching GRAY, RGB or RGBA will be identified as such.
             * @param bitsPerPixel The number of bits used in the original data. Can be smaller than 8 times
             *                     the size in bytes of the type used in the NDArray 'data'. If zero (default) or
             *                     negative, a value matching the NDArray type will be calculated (8, 16, ...).
             */
            ImageData(const karabo::util::NDArray& data,
                      const karabo::util::Dims& dims,
                      const EncodingType encoding = Encoding::GRAY,
                      const int bitsPerPixel = 0);

            ImageData(const ImageData& other) = default;

            virtual ~ImageData() {
            }

            /**
             * Get a reference to the underlying image data structure.
             * Interpretation depends on ImageData::getEncoding().
             *
             * @param array
             */
            const karabo::util::NDArray& getData() const;

            /**
             * Set image data.
             *             * Note that the copy stored inside ImageData will refer to the same memory as the input.
             */
            void setData(const karabo::util::NDArray& array);

            karabo::util::Dims getROIOffsets() const;

            void setROIOffsets(const karabo::util::Dims& offsets);

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

            void setGeometry(const karabo::util::DetectorGeometry & geometry);

            karabo::util::DetectorGeometry getGeometry() const;

            const karabo::util::Hash& getHeader() const;

            void setHeader(const karabo::util::Hash & header);

        private:
            static int defaultBitsPerPixel(int encoding, const karabo::util::NDArray& data);
        };

        /**********************************************************************
         * Declaration ImageDataElement
         **********************************************************************/

        class ImageDataElement : public karabo::util::CustomNodeElement<ImageDataElement, ImageData > {

            typedef karabo::util::CustomNodeElement<ImageDataElement, ImageData > ParentType;

        public:

            ImageDataElement(karabo::util::Schema& s) : ParentType(s) {
            }

            ImageDataElement& setDimensionScales(const std::string& scales) {
                return ParentType::setDefaultValue("dimScales", scales);
            }

            ImageDataElement& setDimensions(const std::string& dimensions) {
                std::vector<unsigned long long> tmp = karabo::util::fromString<unsigned long long, std::vector>(dimensions);
                return ParentType::setDefaultValue("dims", tmp);
            }

            ImageDataElement& setEncoding(const EncodingType& encoding) {
                return ParentType::setDefaultValue("encoding", (int) encoding);
            }

            // TODO Make Geometry a serializable object, too

            ImageDataElement& setGeometry(karabo::util::DetectorGeometry& geometry) {
                geometry.toSchema("data.geometry", this->m_schema);
                return ParentType::setDefaultValue("detectorGeometry", geometry.toHash());
            }

            void commit() {
                // As ImageDataElement is only used for channel descriptions, it should always be read only.
                readOnly();
                ParentType::commit();
            }

        };

        typedef ImageDataElement IMAGEDATA_ELEMENT; // TODO Discuss whether to support or not the _ELEMENT noise
        typedef ImageDataElement IMAGEDATA;
    }
}

#endif	/* KARABO_XMS_IMAGEDATA_HH */

