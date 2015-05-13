/* 
 * File:   ImageData.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 20, 2015, 19:35 PM
 */

#ifndef KARABO_XIP_IMAGEDATA_HH
#define	KARABO_XIP_IMAGEDATA_HH

#include <karabo/util/Hash.hh>
#include <karabo/util/Dims.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/xip/CpuImage.hh>
#include "NDArray.hh"

namespace karabo {
    namespace xms {

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

        namespace ChannelSpace {

            enum ChannelSpaceType {

                UNDEFINED = -1,
                u_8_1, // unsigned, 8 bits per color-channel, 1 byte per pixel
                s_8_1,
                u_10_2,
                s_10_2, // signed, 10 bits per color-channel, 2 bytes per pixel
                u_12_2,
                s_12_2,
                u_12_1p5, // unsigned, 12 bits per color-channel, 1.5 bytes per pixel (i.e. 3 bytes encode 2 pixels, 2 x 12 bits must be read)
                s_12_1p5,
                u_16_2,
                s_16_2,
                f_16_2,
                u_32_4,
                s_32_4,
                f_32_4,
                u_64_8,
                s_64_8,
                f_64_8,
            };
        }

        typedef ChannelSpace::ChannelSpaceType ChannelSpaceType;

        //class RawImageFileWriter;

        class ImageData : public NDArray {
            
            friend class RawImageFileWriter;

        public:

            KARABO_CLASSINFO(ImageData, "ImageData", "1.3")

            static void expectedParameters(karabo::util::Schema& expected);

            ImageData();

            /**
             * Constructor from already existing memory
             * @param data
             * @param size
             * @param copy
             * @param dimensions
             * @param encoding
             * @param channelSpace             
             * @param isBigEndian
             */
            template <class T>
            ImageData( const T * const data,
                      const size_t size,
                      const bool copy = true,
                      const karabo::util::Dims& dims = karabo::util::Dims(),
                      const EncodingType encoding = Encoding::GRAY,
                      const ChannelSpaceType channelSpace = ChannelSpace::UNDEFINED) : NDArray(data, size, copy, dims) {

                if (dims.size() == 0) {
                    setROIOffsets(karabo::util::Dims(0));
                } else {
                    std::vector<unsigned long long> offsets(dims.rank(), 0);
                    setROIOffsets(karabo::util::Dims(offsets));
                }
                setEncoding(encoding);
                if (channelSpace == ChannelSpace::UNDEFINED) setChannelSpace(guessChannelSpace<T>());
                else setChannelSpace(channelSpace);
            }
            
            template <class T>
            ImageData(const karabo::xip::CpuImage<T>& image) {
                using namespace karabo::util;
                int nDims = image.dimensionality();
                Dims dims;
                if (nDims == 1) {
                    dims = Dims(image.size());
                } else if (nDims == 2) {
                    dims = Dims(image.height(), image.width());
                } else {
                    dims = Dims(image.depth(), image.height(), image.width());
                }
                this->setData(image.pixelPointer(), image.size(), true);
                this->setDimensions(dims);
                if (dims.size() == 0) {
                    setROIOffsets(karabo::util::Dims(0));
                } else {
                    std::vector<unsigned long long> offsets(dims.rank(), 0);
                    setROIOffsets(karabo::util::Dims(offsets));
                }
                setEncoding(Encoding::GRAY);
                setChannelSpace(guessChannelSpace<T>());
            }

            /**
             * Constructs from a hash that has to follow the correct format
             * @param imageHash
             */
            ImageData(const karabo::util::Hash& hash);

            ImageData(const karabo::util::Hash::Pointer& data);

            virtual ~ImageData();

            karabo::util::Dims getROIOffsets() const;

            void setROIOffsets(const karabo::util::Dims& offsets);

            int getChannelSpace() const;

            void setChannelSpace(const int channelSpace);

            int getEncoding() const;

            void setEncoding(const int encoding);

            void toBigEndian();

            void toLittleEndian();
            
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

            //const ImageData& write(const std::string& filename, const bool enableAppendMode = false) const;

            friend std::ostream& operator<<(std::ostream& os, const ImageData& image) {
                os << *image.hash();
                return os;
            }

        private:

            void swapEndianess();

            template <class T>
            ChannelSpaceType guessChannelSpace() const {
                using namespace karabo::util;
                Types::ReferenceType type = Types::from<T>();
                switch (type) {
                    case Types::UINT8:
                    case Types::CHAR:
                        return ChannelSpace::u_8_1;
                    case Types::INT8:
                        return ChannelSpace::s_8_1;
                    case Types::UINT16:
                        return ChannelSpace::u_16_2;
                    case Types::INT16:
                        return ChannelSpace::s_16_2;
                    case Types::UINT32:
                        return ChannelSpace::u_32_4;
                    case Types::INT32:
                        return ChannelSpace::s_32_4;
                    case Types::UINT64:
                        return ChannelSpace::u_64_8;
                    case Types::INT64:
                        return ChannelSpace::s_64_8;
                    case Types::FLOAT:
                        return ChannelSpace::f_32_4;
                    case Types::DOUBLE:
                        return ChannelSpace::f_64_8;
                    default:
                        return ChannelSpace::UNDEFINED;
                }
            }
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
                return setDefaultValue("encoding", (int)encoding);
            }

            ImageDataElement& setChannelSpace(const ChannelSpaceType& channelSpace) {
                return setDefaultValue("channelSpace", (int)channelSpace);
            }
        };

        typedef ImageDataElement IMAGEDATA_ELEMENT;
        typedef ImageDataElement IMAGEDATA;

        class ToChannelSpace {
        public:

            typedef ChannelSpaceType ReturnType;

            template <int RefType>
            static ReturnType to() {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
            }
        };

        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, BOOL, ChannelSpace::u_8_1)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, CHAR, ChannelSpace::s_8_1)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, INT8, ChannelSpace::s_8_1)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, UINT8, ChannelSpace::u_8_1)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, INT16, ChannelSpace::s_16_2)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, UINT16, ChannelSpace::u_16_2)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, INT32, ChannelSpace::s_32_4)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, UINT32, ChannelSpace::u_32_4)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, INT64, ChannelSpace::s_64_8)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, UINT64, ChannelSpace::u_64_8)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, FLOAT, ChannelSpace::f_32_4)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, DOUBLE, ChannelSpace::f_64_8)

    }
}



#endif	/* RAWIMAGEDATA_HH */

