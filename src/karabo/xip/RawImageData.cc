/* 
 * File:   RawImageData.cc
 * Author: 
 * 
 * Created on July 10, 2013, 10:34 AM
 */

#include "RawImageData.hh"
#include "karabo/io/HashXmlSerializer.hh"
#include "karabo/io/FileTools.hh"
#include <karabo/util/ByteSwap.hh>
#include <boost/filesystem/operations.hpp>

using namespace karabo::util;

namespace karabo {
    namespace xip {


        RawImageData::RawImageData()  {
            setStandardHeader(this);
        }


        RawImageData::RawImageData(karabo::util::Hash& hash, bool copiesHash) : m_hash(hash) {
            setStandardHeader(this);
            if(hash.has("header")){
                Hash fromHeader = hash.get<Hash>("header");
                Hash toHeader = this->getHeader();
                toHeader.merge(fromHeader);
                this->setHeader(toHeader);
            }
        }


        //RawImageData::RawImageData(const RawImageData& other)  {
        //    m_hash = other.m_hash;
        //}


        RawImageData::~RawImageData() {
            //m_hash.clear();
        }


        const char* RawImageData::getDataPointer() const {
            boost::optional<const karabo::util::Hash::Node&> node = m_hash.find("data");
            if (node) {
                if (node->getType() == Types::VECTOR_CHAR) {
                    return &(m_hash.get<std::vector<char> >("data"))[0];
                }
                return m_hash.get<std::pair<const char*, size_t> >("data").first;
            }
            return 0;
        }


        const std::vector<char>& RawImageData::getData() {
            ensureDataOwnership();
            return m_hash.get<std::vector<char> >("data");
        }


        size_t RawImageData::getByteSize() const {
            boost::optional<const karabo::util::Hash::Node&> node = m_hash.find("data");
            if (node) {
                if (node->getType() == Types::VECTOR_CHAR) {
                    return m_hash.get<std::vector<char> >("data").size();
                }
                return m_hash.get<std::pair<const char*, size_t> >("data").second;
            }
            return 0;
        }


        karabo::util::Dims RawImageData::getDimensions() const {
            return karabo::util::Dims(m_hash.get<std::vector<unsigned long long> >("dims"));
        }


        void RawImageData::setDimensions(const karabo::util::Dims& dimensions) {
            m_hash.set<std::vector<unsigned long long> >("dims", dimensions.toVector());
        }


        karabo::util::Dims RawImageData::getROIOffsets() const {
            return karabo::util::Dims(m_hash.get<std::vector<unsigned long long> >("roiOffsets"));
        }


        void RawImageData::setROIOffsets(const karabo::util::Dims& offsets) {
            m_hash.set<std::vector<unsigned long long> >("roiOffsets", offsets.toVector());
        }


        size_t RawImageData::getSize() const {
            return getDimensions().size();
        }


        int RawImageData::getEncoding() const {
            return m_hash.get<int>("encoding");
        }


        void RawImageData::setEncoding(const int encoding) {
            m_hash.set<int>("encoding", encoding);
        }


        const std::string& RawImageData::getType() const {
            return m_hash.get<string>("type");
        }


        int RawImageData::getChannelSpace() const {
            return m_hash.get<int>("channelSpace");
        }


        void RawImageData::setChannelSpace(const int channelSpace) {
            m_hash.set<int>("channelSpace", channelSpace);
        }


        void RawImageData::setIsBigEndian(const bool isBigEndian) {
            m_hash.set<bool>("isBigEndian", isBigEndian);
        }


        bool RawImageData::isBigEndian() const {
            return m_hash.get<bool>("isBigEndian");
        }


        Hash RawImageData::getHeader() const {
            if (m_hash.has("header")) return m_hash.get<Hash>("header");
            return Hash();
        }


        void RawImageData::setHeader(const karabo::util::Hash& header) {
            Hash existingHeader;
            if(m_hash.has("header")){
                existingHeader = m_hash.get<Hash>("header");
            }
            existingHeader.merge(header);
            m_hash.set<Hash>("header", existingHeader);
           
        }
        
        DetectorGeometry RawImageData::getGeometry() const{
            if (m_hash.has("detectorGeometry")) return DetectorGeometry(m_hash.get<Hash>("detectorGeometry"));
            return DetectorGeometry();
        }
        
       
        void RawImageData::setGeometry(DetectorGeometry geometry){
            m_hash.set<Hash>("detectorGeometry", geometry.toHash());
            
        }
        
       
        std::vector<long> RawImageData::getTileId() const {
            if (m_hash.has("tileId")) return m_hash.get<std::vector<long> >("tileId");
            return std::vector<long>(1, 0);
        }

        void RawImageData::setTileId(long id) {
            m_hash.set<std::vector<long> >("tileId", std::vector<long>(1,id));

        
        }
        
        void RawImageData::setTileId(std::vector<long> id) {
            m_hash.set<std::vector<long> >("tileId", id);

        }


        const karabo::util::Hash& RawImageData::hash() const {
            return m_hash;
        }


        karabo::util::Hash& RawImageData::hash() {
            return m_hash;
        }


        void RawImageData::swap(RawImageData& other) {
            std::swap(m_hash, other.m_hash);
            std::swap(m_padX, other.m_padX);
            std::swap(m_padY, other.m_padY);
        }


        void RawImageData::toBigEndian() {
            if (!isBigEndian()) {
                swapEndianess();
                setIsBigEndian(true);
            }
        }


        bool RawImageData::dataIsCopy() const {
            boost::optional<const karabo::util::Hash::Node&> node = m_hash.find("data");
            if (node) return node->getType() == Types::VECTOR_CHAR;
            else return true;
        }


        void RawImageData::swapEndianess() {
            ensureDataOwnership();
            int cs = getChannelSpace();
            switch (cs) {
                case ChannelSpace::u_16_2:
                case ChannelSpace::s_16_2:
                {
                    unsigned short* data = reinterpret_cast<unsigned short*> (const_cast<char*> (getDataPointer()));
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = bswap16(data[i]);
                    }
                    break;
                }
                case ChannelSpace::u_32_4:
                case ChannelSpace::s_32_4:
                case ChannelSpace::f_32_4:
                {
                    unsigned int* data = reinterpret_cast<unsigned int*> (const_cast<char*> (getDataPointer()));
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = bswap32(data[i]);
                    }
                    break;
                }
                case ChannelSpace::u_64_8:
                case ChannelSpace::s_64_8:
                case ChannelSpace::f_64_8:
                {
                    unsigned long long* data = reinterpret_cast<unsigned long long*> (const_cast<char*> (getDataPointer()));
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = bswap64(data[i]);
                    }
                    break;
                }
                default:
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Endianess conversion not implemented for this channel type");
            }
        }


        void RawImageData::toLittleEndian() {
            if (isBigEndian()) {
                swapEndianess();
                setIsBigEndian(false);
            }
        }


        void RawImageData::ensureDataOwnership() {
            if (!dataIsCopy()) {
                setData(getDataPointer(), getByteSize());
            }
        }
        
        karabo::util::Hash RawImageData::m_standardHeader = karabo::util::Hash();
        
        void RawImageData::setStandardHeader(RawImageData* caller){
            if(m_standardHeader.size() == 0){
                boost::filesystem::path headerFile =  std::string(getenv("HOME"))+"/.karabo/RawImageHeader.xml";
                if(boost::filesystem::exists(headerFile)){
                    
                    karabo::io::loadFromFile(m_standardHeader, headerFile.string());
                } else {
                    
                    Hash header;
                    DetectorGeometry geo;
                    
                    Hash identifiers;
                    header.set("geometry", geo.toHash());
                    identifiers.set("tileIds", std::vector<long long>(1,-1));
                    identifiers.set("trainIds", std::vector<long long>(1,-1));
                    identifiers.set("frameIds", std::vector<long long>(1,-1));
                    identifiers.set("uIds", std::vector<long long>(1,-1));
                    header.set<unsigned long long>("tileDimensionIs", 2);

                    Hash passport;
                    passport.set("detector", "NOT_SPECIFIED");
                    passport.set<long long>("detectorId", -1);
                    passport.set("operator", "NOT_SPECIFIED");
                    passport.set("facility", "XFEL.EU");
                    passport.set("instrument", "NOT_SPECIFIED");
                    passport.set("dataType", "UNPROCESSED");
                    header.set("passport", passport);

                    Hash conditions("detector", Hash(), "instrument", Hash(), "beam", Hash());
                    header.set("initialConditions", conditions);

                    

                    header.set("identifiers", identifiers);
                    header.set("conditions", karabo::util::Hash());
                    m_standardHeader = header;
                }
            }
            
            Hash history;
            history.set("history", std::vector<std::string>(1, "Dataset created"));
            karabo::util::Timestamp t;
            history.set("timestamps", std::vector<std::string>(1, t.toFormattedString()));
            m_standardHeader.set("history", history);
            
            caller->setHeader(m_standardHeader);
            
            
        }


        void RawImageData::toRGBAPremultiplied() {

            this->toLittleEndian();

            size_t size = this->getSize();
            int encoding = this->getEncoding();
            int channelSpace = this->getChannelSpace();

            if (encoding == Encoding::GRAY) {
                vector<unsigned char> qtImage(size * 4); // Have to blow up for RGBA

                if (channelSpace == ChannelSpace::u_8_1) {
                    const unsigned char* data = reinterpret_cast<const unsigned char*> (this->getDataPointer());
                    unsigned char pmax = 0, pmin = 0xFF;
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = data[i];
                        if (pmax < pix) pmax = pix;
                        if (pmin > pix) pmin = pix;
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char> (norm * (data[i] - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::s_8_1) {
                    const char* data = this->getDataPointer();
                    unsigned char pmax = 0, pmin = 0xFF;
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = data[i] + 0x80; // back to 0-0xFF range
                        if (pmax < pix) pmax = pix;
                        if (pmin > pix) pmin = pix;
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char> (norm * (data[i] + 0x80 - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::u_16_2) {
                    const unsigned short* data = reinterpret_cast<const unsigned short*> (this->getDataPointer());
                    unsigned short pmax = 0, pmin = 0xFFFF;

                    for (size_t i = 0; i < size; i++) {
                        if (pmax < data[i]) pmax = data[i];
                        if (pmin > data[i]) pmin = data[i];
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char> (norm * (data[i] - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::s_16_2) {
                    const short* data = reinterpret_cast<const short*> (this->getDataPointer());
                    unsigned short pmax = 0, pmin = 0xFFFF;

                    for (size_t i = 0; i < size; i++) {
                        unsigned short pix = data[i] + 0x8000; // go back to 0-0xFFFF range
                        if (pmax < pix) pmax = pix;
                        if (pmin > pix) pmin = pix;
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char> (norm * (data[i] + 0x8000 - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }


                } else if (channelSpace == ChannelSpace::u_32_4) {
                    const unsigned int* data = reinterpret_cast<const unsigned int*> (this->getDataPointer());
                    unsigned int pmax = 0, pmin = 0xFFFFFFFF;

                    for (size_t i = 0; i < size; i++) {
                        if (pmax < data[i]) pmax = data[i];
                        if (pmin > data[i]) pmin = data[i];
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char> (norm * (data[i] - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::s_32_4) {
                    const int* data = reinterpret_cast<const int*> (this->getDataPointer());
                    unsigned int pmax = 0, pmin = 0xFFFFFFFF;

                    for (size_t i = 0; i < size; i++) {
                        unsigned int pix = data[i] + 0x80000000; // go back to 0-0xFFFFFFFF range
                        if (pmax < pix) pmax = pix;
                        if (pmin > pix) pmin = pix;
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char> (norm * (data[i] + 0x80000000 - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::u_64_8) {
                    const unsigned long* data = reinterpret_cast<const unsigned long*> (this->getDataPointer());
                    unsigned long pmax = 0, pmin = 0xFFFFFFFFFFFFFFFF;

                    for (size_t i = 0; i < size; i++) {
                        if (pmax < data[i]) pmax = data[i];
                        if (pmin > data[i]) pmin = data[i];
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char> (norm * (data[i] - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::s_64_8) {
                    const long* data = reinterpret_cast<const long*> (this->getDataPointer());
                    unsigned long pmax = 0, pmin = 0xFFFFFFFFFFFFFFFF;


                    for (size_t i = 0; i < size; i++) {
                        unsigned long pix = data[i] + 0x8000000000000000; // go back to 0-0xFFFFFFFFFFFFFFFF range
                        if (pmax < pix) pmax = pix;
                        if (pmin > pix) pmin = pix;
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char> (norm * (data[i] + 0x8000000000000000 - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::f_32_4) {
                    const float* data = reinterpret_cast<const float*> (this->getDataPointer());
                    float pmax = -std::numeric_limits<float>::max();
                    float pmin = std::numeric_limits<float>::max();

                    for (size_t i = 0; i < size; i++) {
                        if (pmax < data[i]) pmax = data[i];
                        if (pmin > data[i]) pmin = data[i];
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin)
                            pix = static_cast<unsigned char> (norm * (data[i] - pmin)); // normalization

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::f_64_8) {
                    const double* data = reinterpret_cast<const double*> (this->getDataPointer());
                    double pmax = -std::numeric_limits<double>::max();
                    double pmin = std::numeric_limits<double>::max();

                    for (size_t i = 0; i < size; i++) {
                        if (pmax < data[i]) pmax = data[i];
                        if (pmin > data[i]) pmin = data[i];
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double) 0xFF / (pmax - pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin)
                            pix = static_cast<unsigned char> (norm * (data[i] - pmin)); // normalization

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                }

                // update data in the hash
                this->setData(qtImage); // Update data
                this->setChannelSpace(ChannelSpace::u_32_4); // Update channel space
                this->setEncoding(Encoding::RGBA); // Update encoding
            }

        }
        
        const RawImageData& RawImageData::write(const std::string& filename, const bool enableAppendMode) const {
            karabo::util::Hash h("RawImageFile.filename", filename, "RawImageFile.enableAppendMode", enableAppendMode);
            karabo::io::Output<RawImageData >::Pointer out = karabo::io::Output<RawImageData >::create(h);
            out->write(*this);
            return *this;
        }

    }
}
