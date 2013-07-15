/* 
 * File:   RawImageData.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 10, 2013, 10:34 AM
 */

#ifndef KARABO_XIP_RAWIMAGEDATA_HH
#define	KARABO_XIP_RAWIMAGEDATA_HH

#include "AbstractImage.hh"
#include <karabo/io/TextSerializer.hh>
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
                u_8_1,
                s_8_1,
                u_10_2,
                s_10_2,
                u_12_2,
                s_12_2,
                u_12_1p5,
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
            
            typedef karabo::io::TextSerializer<karabo::util::Hash> Serializer;

            karabo::util::Hash* m_hash;
            Serializer::Pointer m_serializer;
            bool m_isShared;

        public:

            /**
             * Constructor from already existing memory (copies and owns data)
             * @param data
             * @param byteSize
             * @param dimensions
             * @param encoding
             * @param channelSpace
             * @param isBigEndian
             * @param header
             */
            template <class T>
            RawImageData(const T * const data,
                         const size_t byteSize,
                         const karabo::util::Dims& dimensions,
                         const EncodingType encoding,
                         const ChannelSpaceType channelSpace,
                         const bool isBigEndian = karabo::util::isBigEndian(),
                         const karabo::util::Hash& header = karabo::util::Hash()) : m_serializer(Serializer::create("Xml")), m_isShared(false) {
                             
                             

                m_hash = new karabo::util::Hash();

                std::vector<unsigned char>& buffer = m_hash->bindReference<std::vector<unsigned char> >("data");
                buffer.resize(byteSize);
                std::memcpy(&buffer[0], reinterpret_cast<const unsigned char*> (data), byteSize);
                // TODO split the following part into an function (e.g. assignMetaData)
                m_hash->set("dims", dimensions.toVector());
                m_hash->set<int>("encoding", encoding);
                m_hash->set<int>("channelSpace", channelSpace);
                m_hash->set<bool>("isBigEndian", isBigEndian);
                m_hash->set<std::string>("header", m_serializer->save(header));
            }

            /**
             * Constructor which only allocates memory. Data can be directly written using the dataPointer function
             */
            RawImageData(const size_t byteSize,
                         const karabo::util::Dims& dimensions,
                         const EncodingType encoding,
                         const ChannelSpaceType channelSpace,
                         const bool isBigEndian = karabo::util::isBigEndian(),
                         const karabo::util::Hash& header = karabo::util::Hash());

            /**
             * Constructs from a hash that has to follow the correct format
             * @param imageHash
             * @param shareData
             */
            RawImageData(karabo::util::Hash& imageHash, bool sharesData = true);

            virtual ~RawImageData();

            inline unsigned char* dataPointer() {
                return &(m_hash->get<std::vector< unsigned char> >("data")[0]);
            }

            inline unsigned char* dataPointer() const {
                return &(m_hash->get<std::vector< unsigned char> >("data")[0]);
            }

            inline const std::vector<unsigned char>& getData() const {
                return m_hash->get<std::vector<unsigned char> >("data");
            }

            template <class T>
            inline void setData(const std::vector<T>& data) {
                unsigned long long byteSize = data.size() * sizeof (T);
                boost::optional<karabo::util::Hash::Node&> node = m_hash->find("data");
                if (node) {
                    std::vector<unsigned char>& buffer = node->getValue<std::vector<unsigned char> >();
                    buffer.resize(byteSize);
                    std::memcpy(&buffer[0], reinterpret_cast<const unsigned char*> (&data[0]), byteSize);
                } else {
                    std::vector<unsigned char>& buffer = m_hash->bindReference<std::vector<unsigned char> >("data");
                    buffer.resize(byteSize);
                    std::memcpy(&buffer[0], reinterpret_cast<const unsigned char*> (&data[0]), byteSize);
                }
            }

            size_t size() const {
                return getDimensions().size();
            }

            size_t getByteSize() const {
                return getData().size();
            }
            
            void setByteSize(const size_t& byteSize) {
                if (m_hash->has("data")) {
                    m_hash->get<std::vector<unsigned char> >("data").resize(byteSize);
                } else {
                    m_hash->bindReference<std::vector<unsigned char> >("data").resize(byteSize);
                }
            }

            inline karabo::util::Dims getDimensions() const {
                return karabo::util::Dims(m_hash->get<std::vector<unsigned long long> >("dims"));
            }

            inline void setDimensions(const karabo::util::Dims& dimensions) {
                m_hash->set<std::vector<unsigned long long> >("dims", dimensions.toVector());
            }

            inline int getEncoding() const {
                return m_hash->get<int>("encoding");
            }

            inline void setEncoding(const EncodingType encoding) {
                m_hash->set<int>("encoding", encoding);
            }

            inline int getChannelSpace() const {
                return m_hash->get<int>("channelSpace");
            }

            inline void setChannelSpace(const ChannelSpaceType channelSpace) {
                m_hash->set<int>("channelSpace", channelSpace);
            }

            inline void setIsBigEndian(const bool isBigEndian) {
                m_hash->set<bool>("isBigEndian", isBigEndian);
            }

            inline bool isBigEndian() const {
                return m_hash->get<bool>("isBigEndian");
            }

            karabo::util::Hash getHeader() const;
            
            void setHeader(const karabo::util::Hash& header) const;
        };
    }
}



#endif	/* RAWIMAGEDATA_HH */

