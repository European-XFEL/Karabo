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

        class RawImageData {

            karabo::util::Hash* m_hash;
            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_serializer;
            bool m_isShared;

        public:

            /**
             * Constructor from already existing memory (copies and owns data)
             * @param data
             * @param byteSize
             * @param dimensions
             * @param encoding
             * @param channelSpace
             * @param endianness
             * @param header
             */
            template <class T>
            RawImageData(const T * const data,
                         const size_t byteSize,
                         const karabo::util::Dims& dimensions,
                         const EncodingType encoding,
                         const ChannelSpaceType channelSpace,
                         const EndiannessType endianness,
                         const karabo::util::Hash& header) : m_isShared(false) {
                             
                m_hash = new karabo::util::Hash();

                std::vector<unsigned char>& buffer = m_hash->bindReference<std::vector<unsigned char> >("data");
                buffer.resize(byteSize);
                std::memcpy(&buffer[0], reinterpret_cast<const unsigned char*> (data), byteSize);
                // TODO split the following part into an function (e.g. assignMetaData)
                m_hash->set("dims", dimensions.toVector());
                m_hash->set<int>("encoding", encoding);
                m_hash->set<int>("channelSpace", channelSpace);
                if (endianness == Endianness::UNDEFINED) {
                    if (karabo::util::isBigEndian()) {
                        m_hash->set<int>("endianness", Endianness::MSB);
                    } else {
                        m_hash->set<int>("endianness", Endianness::LSB);
                    }
                } else {
                    m_hash->set<int>("endianness", endianness);
                }
                m_hash->set<std::string>("header", m_serializer->save(header));
            }

            /**
             * Constructor which only allocates memory. Data can be directly written using the getDataPointer function
             * @param encoding
             * @param channelSpace
             * @param type
             * @param header
             */
            RawImageData(const size_t byteSize,
                         const karabo::util::Dims& dimensions,
                         const EncodingType encoding,
                         const ChannelSpaceType channelSpace,
                         const EndiannessType endianness = Endianness::UNDEFINED,
                         const karabo::util::Hash& header = karabo::util::Hash());

            /**
             * Constructs from a hash that has to follow the correct format
             * @param imageHash
             * @param shareData
             */
            RawImageData(karabo::util::Hash& imageHash, bool sharesData = true);

            virtual ~RawImageData();

            template <class T>
            inline T* dataPointer() {
                return reinterpret_cast<T*> (&(m_hash->get<std::vector< unsigned char> >("data")[0]));
            }

            template <class T>
            inline const T* dataPointer() const {
                return reinterpret_cast<const T*> (&(m_hash->get<std::vector< unsigned char> >("data")[0]));
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

            inline karabo::util::Dims getDimensions() const {
                return karabo::util::Dims(m_hash->get<std::vector<unsigned long long> >("dims"));
            }

            inline void reshape(const karabo::util::Dims& dimensions) {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Lazyness won");
            }

            inline const int& encoding() const {
                return m_hash->get<int>("encoding");
            }

            inline int& encoding() {
                return m_hash->get<int>("encoding");
            }

            inline const int& channelSpace() const {
                return m_hash->get<int>("channelSpace");
            }

            inline int& channelSpace() {
                return m_hash->get<int>("channelSpace");
            }

            inline const int& endianness() const {
                return m_hash->get<int>("endianness");
            }

            inline int& endianness() {
                return m_hash->get<int>("endianness");
            }

            karabo::util::Hash getHeader() const;

        };
    }
}



#endif	/* RAWIMAGEDATA_HH */

