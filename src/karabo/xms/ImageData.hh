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
#include <karabo/util/NDArrayElement.hh>
#include "Data.hh"
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
        }

        typedef Encoding::EncodingType EncodingType;

        class ImageData : protected karabo::util::Hash {

        public:

            KARABO_CLASSINFO(ImageData, "ImageData", "1.5")

            static void expectedParameters(karabo::util::Schema& s);

            ImageData();

            /**
             * Constructor from NDArray
             * NOTE: If no dimensions are specified the shape of NDArray is used, but in reversed order!
             *
             * @param data NDArray
             * @param dims The dimensions of the image data
             * @param encoding The encoding of the bytes
             * @param bitsPerPixel
             */
            ImageData(const karabo::util::NDArray& data,
                      const karabo::util::Dims& dims = karabo::util::Dims(),
                      const EncodingType encoding = Encoding::GRAY,
                      const int bitsPerPixel = 8);

            virtual ~ImageData() {
            };

            const karabo::util::NDArray& getData() const;

            void setData(const karabo::util::NDArray& array);

            karabo::util::Dims getROIOffsets() const;

            void setROIOffsets(const karabo::util::Dims& offsets);

            int getBitsPerPixel() const;

            void setBitsPerPixel(const int bitsPerPixel);

            int getEncoding() const;

            void setEncoding(const int encoding);

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

            karabo::util::DetectorGeometry getGeometry();

            const karabo::util::Hash& getHeader() const;

            void setHeader(const karabo::util::Hash & header);

        };

        /**********************************************************************
         * Declaration ImageDataElement
         **********************************************************************/

        class ImageDataElement : public karabo::util::CustomNodeElement<ImageData > {

        public:

            ImageDataElement(karabo::util::Schema& s) : CustomNodeElement<ImageData>(s) {
            }

            CustomNodeElement<ImageData>& setDimensionScales(const std::string& scales) {
                return CustomNodeElement<ImageData >::setDefaultValue("dimScales", scales);
            }

            CustomNodeElement<ImageData>& setDimensions(const std::string& dimensions) {
                std::vector<unsigned long long> tmp = karabo::util::fromString<unsigned long long, std::vector>(dimensions);
                return CustomNodeElement<ImageData >::setDefaultValue("dims", tmp);
            }

            CustomNodeElement<ImageData >& setEncoding(const EncodingType& encoding) {
                return CustomNodeElement<ImageData >::setDefaultValue("encoding", (int) encoding);
            }

            // TODO Make Geometry a serializable object, too

            CustomNodeElement<ImageData >& setGeometry(karabo::util::DetectorGeometry& geometry) {
                geometry.toSchema("data.geometry", this->m_schema);
                return CustomNodeElement<ImageData >::setDefaultValue("detectorGeometry", geometry.toHash());
            }

        };

        typedef ImageDataElement IMAGEDATA_ELEMENT; // TODO Discuss whether to support or not the _ELEMENT noise
        typedef ImageDataElement IMAGEDATA;
    }
}

#endif	/* KARABO_XMS_IMAGEDATA_HH */

