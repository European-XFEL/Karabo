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
                         const karabo::util::Dims& dimensions = karabo::util::Dims(),
                         const EncodingType encoding = Encoding::GRAY,
                         const ChannelSpaceType channelSpace = ChannelSpace::UNDEFINED) : NDArray(data, size, copy, dimensions) {
             
                if (dimensions.size() == 0) {
                    setROIOffsets(karabo::util::Dims(0));
                } else {                    
                    std::vector<unsigned long long> offsets(dimensions.rank(), 0);
                    setROIOffsets(karabo::util::Dims(offsets));
                }
                setEncoding(encoding);
                if (channelSpace == ChannelSpace::UNDEFINED) setChannelSpace(guessChannelSpace<T>());
                else setChannelSpace(channelSpace);
            }

            /**
             * Constructs from a hash that has to follow the correct format
             * @param imageHash
             */              
            ImageData(karabo::util::Hash& hash);
            
            ImageData(karabo::util::Hash::Pointer& hash);

            virtual ~ImageData();  
            
            karabo::util::Dims getROIOffsets() const;

            void setROIOffsets(const karabo::util::Dims& offsets);
            
             int getChannelSpace() const;

            void setChannelSpace(const int channelSpace);
            
            int getEncoding() const;
            
            void setEncoding(const int encoding);
            
            void toBigEndian();

            void toLittleEndian();
            
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
    }
}



#endif	/* RAWIMAGEDATA_HH */

