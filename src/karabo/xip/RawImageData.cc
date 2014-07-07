/* 
 * File:   RawImageData.cc
 * Author: 
 * 
 * Created on July 10, 2013, 10:34 AM
 */

#include "RawImageData.hh"
#include <karabo/util/ByteSwap.hh>

using namespace karabo::util;

namespace karabo {
    namespace xip {


        RawImageData::RawImageData() : m_hash() {
        }


        RawImageData::RawImageData(karabo::util::Hash& hash, bool copiesHash) : m_hash(hash) {
        }


        RawImageData::RawImageData(const RawImageData& other) : m_hash(other.m_hash) {
        }


        RawImageData::~RawImageData() {
            m_hash.clear();
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
            m_hash.set<Hash>("header", header);
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

    }
}
