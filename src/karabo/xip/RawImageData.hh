/* 
 * File:   RawImageData.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 10, 2013, 10:34 AM
 */

#ifndef KARABO_XIP_RAWIMAGEDATA_HH
#define	KARABO_XIP_RAWIMAGEDATA_HH

#include <karabo/util/Hash.hh>
#include <karabo/util/Dims.hh>

namespace karabo {
    namespace xip {

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

        namespace Endianness {

            enum EndiannessType {

                UNDEFINED = -1,
                LSB,
                MSB
            };
        }

        typedef Endianness::EndiannessType EndiannessType;

        class RawImageData {

            karabo::util::Hash* m_hash;
            bool m_isShared;

        public:
            
            KARABO_CLASSINFO(RawImageData, "RawImageData", "1.0")
            
            RawImageData();

            /**
             * Constructor from already existing memory (copies and owns data)
             * @param data
             * @param byteSize
             * @param dimensions
             * @param encoding
             * @param channelSpace             
             * @param header
             * @param isBigEndian
             */
            template <class T>
            RawImageData(const T * const data,
                         const size_t byteSize,
                         const karabo::util::Dims& dimensions,
                         const EncodingType encoding,
                         const ChannelSpaceType channelSpace,
                         const karabo::util::Hash& header = karabo::util::Hash(), 
                         const bool isBigEndian = karabo::util::isBigEndian()) : m_hash(0), m_isShared(false) {
                
                m_hash = new karabo::util::Hash();
                
                setData(data, byteSize);
                setDimensions(dimensions);
                setEncoding(encoding);
                setChannelSpace(channelSpace);
                setIsBigEndian(isBigEndian);
                setHeader(header);
            }

            /**
             * Constructor which only allocates memory. Data can be directly written using the dataPointer function
             * @param byteSize
             * @param dimensions
             * @param encoding
             * @param channelSpace             
             * @param header
             * @param isBigEndian
             */
            RawImageData(const size_t byteSize,
                         const karabo::util::Dims& dimensions,
                         const EncodingType encoding,
                         const ChannelSpaceType channelSpace,
                         const karabo::util::Hash& header = karabo::util::Hash(),
                         const bool isBigEndian = karabo::util::isBigEndian());

            /**
             * Constructs from a hash that has to follow the correct format
             * @param imageHash
             * @param shareData
             */
            RawImageData(karabo::util::Hash& imageHash, bool sharesData = false);
            
            RawImageData(const RawImageData& image);

            virtual ~RawImageData();

            char* dataPointer();
            
            char* dataPointer() const;
                
            const std::vector<char>& getData() const;
            
            template <class T>
            inline void setData(const std::vector<T>& data) {
                unsigned long long byteSize = data.size() * sizeof (T);
                boost::optional<karabo::util::Hash::Node&> node = m_hash->find("data");
                if (node) {
                    std::vector<char>& buffer = node->getValue<std::vector<char> >();
                    buffer.resize(byteSize);
                    std::memcpy(&buffer[0], reinterpret_cast<const char*> (&data[0]), byteSize);
                } else {
                    std::vector<char>& buffer = m_hash->bindReference<std::vector<char> >("data");
                    buffer.resize(byteSize);
                    std::memcpy(&buffer[0], reinterpret_cast<const char*> (&data[0]), byteSize);
                }
            }
            
            template <class T>
            inline void setData(const T* data, const size_t byteSize) {
                std::vector<char>& buffer = m_hash->bindReference<std::vector<char> >("data");
                buffer.resize(byteSize);
                std::memcpy(&buffer[0], reinterpret_cast<const char*> (data), byteSize);
            }
            
            void allocateData(const size_t byteSize);
            
            size_t size() const;
                
            size_t getByteSize() const;
            
            void setByteSize(const size_t& byteSize);
            
            karabo::util::Dims getDimensions() const;
            
            void setDimensions(const karabo::util::Dims& dimensions);
            
            int getEncoding() const;
            
            void setEncoding(const EncodingType encoding);
            
            int getChannelSpace() const;
            
            void setChannelSpace(const ChannelSpaceType channelSpace);
                
            void setIsBigEndian(const bool isBigEndian);
            
            bool isBigEndian() const;
                
            karabo::util::Hash getHeader() const;

            void setHeader(const karabo::util::Hash& header) const;

            const karabo::util::Hash& toHash() const;
            
            void swap(RawImageData& image);
            
            void toRGBAPremultiplied();
        };
    }
}



#endif	/* RAWIMAGEDATA_HH */

