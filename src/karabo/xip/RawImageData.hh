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
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/DetectorGeometry.hh>
#include "ImageEnums.hh"

namespace karabo {
    namespace xip {
        
        class RawImageFileWriter;

        class RawImageData {
            
            friend class RawImageFileWriter;

        protected:

            karabo::util::Hash m_hash;
            size_t m_padX;
            size_t m_padY;
            
            static karabo::util::Hash m_standardHeader;
           

        public:

            KARABO_CLASSINFO(RawImageData, "RawImageData", "1.0")

            RawImageData();

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
            RawImageData(const T * const data,
                         const size_t size,
                         const bool copy = true,
                         const karabo::util::Dims& dimensions = karabo::util::Dims(),
                         const karabo::xip::EncodingType encoding = Encoding::GRAY,
                         const karabo::xip::ChannelSpaceType channelSpace = ChannelSpace::UNDEFINED,
                         const bool isBigEndian = karabo::util::isBigEndian()) : m_hash() {
                setData(data, size, copy);
                if (dimensions.size() == 0) {
                    setDimensions(karabo::util::Dims(size));
                    setROIOffsets(karabo::util::Dims(0));
                } else {
                    setDimensions(dimensions);
                    std::vector<unsigned long long> offsets(dimensions.rank(), 0);
                    setROIOffsets(karabo::util::Dims(offsets));
                }
                setEncoding(encoding);
                if (channelSpace == ChannelSpace::UNDEFINED) setChannelSpace(guessChannelSpace<T>());
                else setChannelSpace(channelSpace);
                setIsBigEndian(isBigEndian);
                m_hash.set("type", karabo::util::Types::to<karabo::util::ToLiteral>(karabo::util::Types::from<T>()));
            }

            /**
             * Constructs from a hash that has to follow the correct format
             * @param imageHash
             * @param sharesHash
             */
            RawImageData(karabo::util::Hash& imageHash, bool copiesHash = true);

            //RawImageData(const RawImageData& other);

            virtual ~RawImageData();

            const char* getDataPointer() const;

            size_t getByteSize() const;

            const std::vector<char>& getData();

            template <class T>
            inline void setData(const std::vector<T>& data, const bool copy = true) {
                this->setData(&data[0], data.size(), copy);
            }

            template <class T>
            inline void setData(const T* data, const size_t size, const bool copy = true, const karabo::xip::ChannelSpaceType channelSpace = ChannelSpace::UNDEFINED) {

                size_t byteSize = size * sizeof (T);

                if (copy) {
                    boost::optional<karabo::util::Hash::Node&> node = m_hash.find("data");
                    if (node) {
                        std::vector<char>& buffer = node->getValue<std::vector<char> >();
                        buffer.resize(byteSize);
                        std::memcpy(&buffer[0], reinterpret_cast<const char*> (data), byteSize);
                    } else {
                        std::vector<char>& buffer = m_hash.bindReference<std::vector<char> >("data");
                        buffer.resize(byteSize);
                        std::memcpy(&buffer[0], reinterpret_cast<const char*> (data), byteSize);
                    }
                } else {
                    // We have to be very careful with the exact type here. For the RTTI const T* and T* are different!
                    // The Type system currently knows only about pair<const T*, size_t> NOT pair<T*, size_t>
                    m_hash.set("data", std::make_pair(reinterpret_cast<const char*> (data), byteSize));
                }

                m_hash.set("type", karabo::util::Types::to<karabo::util::ToLiteral>(karabo::util::Types::from<T>()));
                if (channelSpace == ChannelSpace::UNDEFINED) setChannelSpace(guessChannelSpace<T>());
                else setChannelSpace(channelSpace);
            }

            karabo::util::Dims getDimensions() const;

            void setDimensions(const karabo::util::Dims& dimensions);

            karabo::util::Dims getROIOffsets() const;

            void setROIOffsets(const karabo::util::Dims& offsets);

            size_t getSize() const;

            int getEncoding() const;

            void setEncoding(const int encoding);

            const std::string& getType() const;

            int getChannelSpace() const;

            void setChannelSpace(const int channelSpace);

            void setIsBigEndian(const bool isBigEndian);

            bool isBigEndian() const;

            karabo::util::Hash getHeader() const;

            void setHeader(const karabo::util::Hash& header);
            
            karabo::util::DetectorGeometry getGeometry() const;
            
            
                        
            void setGeometry(karabo::util::DetectorGeometry geometry);
            
           
            
            std::vector<long> getTileId() const;

            void setTileId(long id);
            
            void setTileId(std::vector<long> id);
            
            const karabo::util::Hash& hash() const;

            karabo::util::Hash& hash();

            void swap(RawImageData& image);

            void toBigEndian();

            void toLittleEndian();

            void toRGBAPremultiplied();
            
            const RawImageData& write(const std::string& filename, const bool enableAppendMode = false) const;
            
            friend std::ostream& operator<<(std::ostream& os, const RawImageData& image) {
                os<<image.hash();
                return os;
            }

        private:

            bool dataIsCopy() const;

            void swapEndianess();

            void ensureDataOwnership();
            
            static void setStandardHeader(RawImageData* caller);

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

