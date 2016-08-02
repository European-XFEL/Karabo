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
#include <karabo/util/ToLiteral.hh>
#include "Data.hh"
#include <karabo/util/DetectorGeometry.hh>

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

        class ImageDataFileWriter;

        class ImageData : public Data {


            friend class ImageDataFileWriter;

        public:

            KARABO_CLASSINFO(ImageData, "ImageData", "1.5")

            static void expectedParameters(karabo::util::Schema& expected);

            ImageData();

            /**
             * Constructor from already existing memory
             * @param data
             * @param size
             * @param copy
             * @param dimensions
             * @param encoding
             * @param bitsPerPixel
             */
            ImageData(const unsigned char* const data,
                      const size_t size,
                      const bool copy = true,
                      const karabo::util::Dims& dims = karabo::util::Dims(),
                      const EncodingType encoding = Encoding::GRAY,
                      const int bitsPerPixel = 8);

            /**
             * Constructs from a hash that has to follow the correct format
             * @param imageHash
             */
            ImageData(const karabo::util::Hash& hash);

            ImageData(const karabo::util::Hash::Pointer& data);

            virtual ~ImageData();

            const karabo::util::NDArray<unsigned char>& getData() const;

            void setData(const unsigned char* data, const size_t size, const bool copy);

            karabo::util::Dims getROIOffsets() const;

            void setROIOffsets(const karabo::util::Dims& offsets);

            int getBitsPerPixel() const;

            void setBitsPerPixel(const int bitsPerPixel);

            int getEncoding() const;

            void setEncoding(const int encoding);

            void toBigEndian();

            void toLittleEndian();

            void setIsBigEndian(const bool isBigEndian);

            bool isBigEndian() const;

            size_t getByteSize() const;

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

            const ImageData& write(const std::string& filename, const bool enableAppendMode = false) const;

            friend std::ostream& operator<<(std::ostream& os, const ImageData& image) {
                os << *image.hash();
                return os;
            }

            void setGeometry(const karabo::util::DetectorGeometry & geometry);

            karabo::util::DetectorGeometry getGeometry();

            const karabo::util::Hash& getHeader() const;

            void setHeader(const karabo::util::Hash & header);

        private:

            void swapEndianess();

        };

        struct ImageDataElement : public DataElement<ImageDataElement, ImageData> {

            ImageDataElement(karabo::util::Schema& s) : DataElement<ImageDataElement, ImageData>(s) {
            }

            ImageDataElement& setDimensionScales(const std::string& scales) {
                return setDefaultValue("dimScales", scales);
            }

            ImageDataElement& setDimensions(const std::string& dimensions) {
                std::vector<unsigned long long> tmp = karabo::util::fromString<unsigned long long, std::vector>(dimensions);
                std::reverse(tmp.begin(), tmp.end());
                return setDefaultValue("dims", tmp);
            }

            ImageDataElement& setEncoding(const EncodingType& encoding) {
                return setDefaultValue("encoding", (int) encoding);
            }

            ImageDataElement& setGeometry(karabo::util::DetectorGeometry & geometry) {
                geometry.toSchema("data.geometry", m_schema);
                return setDefaultValue("detectorGeometry", geometry.toHash());


            }
        };

        typedef ImageDataElement IMAGEDATA_ELEMENT;
        typedef ImageDataElement IMAGEDATA;

    }
}



#endif	/* KARABO_XMS_IMAGEDATA_HH */

