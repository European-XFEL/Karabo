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


        RawImageData::RawImageData() : m_hash(0), m_isShared(false) {
        }


        RawImageData::RawImageData(const size_t byteSize,
                                   const karabo::util::Dims& dimensions,
                                   const EncodingType encoding,
                                   const ChannelSpaceType channelSpace,
                                   const karabo::util::Hash& header,
                                   const bool isBigEndian) : m_hash(0), m_isShared(false) {
            m_hash = new Hash();

            allocateData(byteSize);
            setDimensions(dimensions);
            setEncoding(encoding);
            setChannelSpace(channelSpace);
            setIsBigEndian(isBigEndian);
            setHeader(header);
        }


        RawImageData::RawImageData(karabo::util::Hash& imageHash, bool sharesData) : m_hash(0), m_isShared(sharesData) {
            if (m_isShared) {
                m_hash = &imageHash;
            } else {
                m_hash = new Hash();
                *m_hash = imageHash;
            }
        }


        RawImageData::RawImageData(const RawImageData& image) {
            m_hash = new Hash(*image.m_hash);
            m_isShared = false;
        }


        RawImageData::~RawImageData() {
            if (!m_isShared && m_hash) delete m_hash;
        }


        char* RawImageData::dataPointer() {
            return &(m_hash->get<std::vector< char> >("data")[0]);
        }


        char* RawImageData::dataPointer() const {
            return &(m_hash->get<std::vector< char> >("data")[0]);
        }


        const std::vector<char>& RawImageData::getData() const {
            return m_hash->get<std::vector<char> >("data");
        }


        void RawImageData::allocateData(const size_t byteSize) {
            std::vector<char>& buffer = m_hash->bindReference<std::vector<char> >("data");
            buffer.resize(byteSize);
        }


        size_t RawImageData::size() const {
            return getDimensions().size();
        }


        size_t RawImageData::getByteSize() const {
            return getData().size();
        }


        void RawImageData::setByteSize(const size_t& byteSize) {
            if (m_hash->has("data")) {
                m_hash->get<std::vector<char> >("data").resize(byteSize);
            } else {
                m_hash->bindReference<std::vector<char> >("data").resize(byteSize);
            }
        }


        karabo::util::Dims RawImageData::getDimensions() const {
            return karabo::util::Dims(m_hash->get<std::vector<unsigned long long> >("dims"));
        }


        void RawImageData::setDimensions(const karabo::util::Dims& dimensions) {
            m_hash->set<std::vector<unsigned long long> >("dims", dimensions.toVector());
        }


        int RawImageData::getEncoding() const {
            return m_hash->get<int>("encoding");
        }


        void RawImageData::setEncoding(const EncodingType encoding) {
            m_hash->set<int>("encoding", encoding);
        }


        int RawImageData::getChannelSpace() const {
            return m_hash->get<int>("channelSpace");
        }


        void RawImageData::setChannelSpace(const ChannelSpaceType channelSpace) {
            m_hash->set<int>("channelSpace", channelSpace);
        }


        void RawImageData::setIsBigEndian(const bool isBigEndian) {
            m_hash->set<bool>("isBigEndian", isBigEndian);
        }


        bool RawImageData::isBigEndian() const {
            return m_hash->get<bool>("isBigEndian");
        }


        Hash RawImageData::getHeader() const {
            if (m_hash->has("header")) return m_hash->get<Hash>("header");
            return Hash();
        }


        void RawImageData::setHeader(const karabo::util::Hash& header) const {
            m_hash->set("header", header);
        }


        const karabo::util::Hash& RawImageData::toHash() const {
            return *m_hash;
        }


        void RawImageData::swap(RawImageData& image) {
            std::swap(m_hash, image.m_hash);
            std::swap(m_isShared, m_isShared);
        }

        void RawImageData::toRGBAPremultiplied() {
            size_t size = this->size();
            int encoding = this->getEncoding();
            bool isBigEndian = this->isBigEndian();
            int channelSpace = this->getChannelSpace();

            if (encoding == Encoding::GRAY) {
	        vector<unsigned char> qtImage(size * 4); // Have to blow up for RGBA

                if (channelSpace == ChannelSpace::u_8_1) {
                    unsigned char* data = reinterpret_cast<unsigned char*>(this->dataPointer());
                    unsigned char pmax = 0, pmin = 0xFF;
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = data[i];
                        if (pmax < pix) pmax = pix;
                        if (pmin > pix) pmin = pix;
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char>(norm * (data[i] - pmin));
                        
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

		} else if (channelSpace == ChannelSpace::s_8_1) {
                    char* data = this->dataPointer();
                    unsigned char pmax = 0, pmin = 0xFF;
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = data[i] + 0x80; // back to 0-0xFF range
                        if (pmax < pix) pmax = pix;
                        if (pmin > pix) pmin = pix;
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char>(norm * (data[i] + 0x80 - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::u_16_2) {
                    unsigned short* data = reinterpret_cast<unsigned short*>(this->dataPointer());
                    unsigned short pmax = 0, pmin = 0xFFFF;

                    if (isBigEndian) {
                        for (size_t i = 0; i < size; i++) {
       	                    data[i] = bswap16(data[i]); // swap bytes
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                        }
                    } else {
                        for (size_t i = 0; i < size; i++) {
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                         }
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char>(norm * (data[i] - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::s_16_2) {
                    short* data = reinterpret_cast<short*>(this->dataPointer());
                    unsigned short pmax = 0, pmin = 0xFFFF;

                    if (isBigEndian) {
                        for (size_t i = 0; i < size; i++) {
                            data[i] = bswap16(data[i]); // swap bytes
                            unsigned short pix = data[i] + 0x8000; // go back to 0-0xFFFF range
                            if (pmax < pix) pmax = pix;
                            if (pmin > pix) pmin = pix;
                        }
                    } else {
                        for (size_t i = 0; i < size; i++) {
                            unsigned short pix = data[i] + 0x8000; // go back to 0-0xFFFF range
                            if (pmax < pix) pmax = pix;
                            if (pmin > pix) pmin = pix;
                        }
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char>(norm * (data[i] + 0x8000 - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }


                } else if (channelSpace == ChannelSpace::u_32_4) {
                    unsigned int* data = reinterpret_cast<unsigned int*>(this->dataPointer());
                    unsigned int pmax = 0, pmin = 0xFFFFFFFF;

                    if (isBigEndian) {
                        for (size_t i = 0; i < size; i++) {
                            data[i] = bswap32(data[i]); // swap bytes
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                        }
                    } else {
                        for (size_t i = 0; i < size; i++) {
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                        }
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char>(norm * (data[i] - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::s_32_4) {
                    int* data = reinterpret_cast<int*>(this->dataPointer());
                    unsigned int pmax = 0, pmin = 0xFFFFFFFF;

                    if (isBigEndian) {
                        for (size_t i = 0; i < size; i++) {
                            data[i] = bswap32(data[i]); // swap bytes
                            unsigned int pix = data[i] + 0x80000000; // go back to 0-0xFFFFFFFF range
                            if (pmax < pix) pmax = pix;
                            if (pmin > pix) pmin = pix;
                        }
                    } else {
                        for (size_t i = 0; i < size; i++) {
                            unsigned int pix = data[i] + 0x80000000; // go back to 0-0xFFFFFFFF range
                            if (pmax < pix) pmax = pix;
                            if (pmin > pix) pmin = pix;
                        }
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char>(norm * (data[i] + 0x80000000 - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::u_64_8) {
                    unsigned long* data = reinterpret_cast<unsigned long*>(this->dataPointer());
                    unsigned long pmax = 0, pmin = 0xFFFFFFFFFFFFFFFF;

                    if (isBigEndian) {
                        for (size_t i = 0; i < size; i++) {
                            data[i] = bswap64(data[i]); // swap bytes
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                        }
                    } else {
                        for (size_t i = 0; i < size; i++) {
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                        }
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char>(norm * (data[i] - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::s_64_8) {
                    long* data = reinterpret_cast<long*>(this->dataPointer());
                    unsigned long pmax = 0, pmin = 0xFFFFFFFFFFFFFFFF;

                    if (isBigEndian) {
                        for (size_t i = 0; i < size; i++) {
                            data[i] = bswap64(data[i]); // swap bytes
                            unsigned long pix = data[i] + 0x8000000000000000; // go back to 0-0xFFFFFFFFFFFFFFFF range
                            if (pmax < pix) pmax = pix;
                            if (pmin > pix) pmin = pix;
                        }
                    } else {
                        for (size_t i = 0; i < size; i++) {
                            unsigned long pix = data[i] + 0x8000000000000000; // go back to 0-0xFFFFFFFFFFFFFFFF range
                            if (pmax < pix) pmax = pix;
                            if (pmin > pix) pmin = pix;
                        }
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin) // normalization
                            pix = static_cast<unsigned char>(norm * (data[i] + 0x8000000000000000 - pmin));

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::f_32_4) {
                    float* data = reinterpret_cast<float*>(this->dataPointer());
                    float pmax = -std::numeric_limits<float>::max();
                    float pmin = std::numeric_limits<float>::max();

                    for (size_t i = 0; i < size; i++) {
                        if (pmax < data[i]) pmax = data[i];
                        if (pmin > data[i]) pmin = data[i];
                    }

                    if (isBigEndian) {
                        for (size_t i = 0; i < size; i++) {
                            unsigned int* data_i = (unsigned int*)(data+i);
                            *data_i = bswap32(*data_i); // swap bytes
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                        }
                    } else {
                        for (size_t i = 0; i < size; i++) {
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                        }
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin)
                            pix = static_cast<unsigned char>(norm * (data[i] - pmin)); // normalization

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                } else if (channelSpace == ChannelSpace::f_64_8) {
                    double* data = reinterpret_cast<double*>(this->dataPointer());
                    double pmax = -std::numeric_limits<double>::max();
                    double pmin = std::numeric_limits<double>::max();

                    for (size_t i = 0; i < size; i++) {
                        if (pmax < data[i]) pmax = data[i];
                        if (pmin > data[i]) pmin = data[i];
                    }

                    if (isBigEndian) {
                        for (size_t i = 0; i < size; i++) {
                            unsigned long* data_i = (unsigned long*)(data+i);
                            *data_i = bswap64(*data_i);  // swap bytes
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                        }
                    } else {
                        for (size_t i = 0; i < size; i++) {
                            if (pmax < data[i]) pmax = data[i];
                            if (pmin > data[i]) pmin = data[i];
                        }
                    }

                    size_t index = 0;
                    double norm = 1.0;
                    if (pmax > pmin)
                        norm = (double)0xFF / (pmax-pmin);
                    for (size_t i = 0; i < size; i++) {
                        unsigned char pix = 0x7F; // arbitrary
                        if (pmax > pmin)
                            pix = static_cast<unsigned char>(norm * (data[i] - pmin)); // normalization

                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = pix;
                        qtImage[index++] = 0xFF;
                    }

                }

                // update data in the hash
                this->setData(qtImage); // Update data
                this->setEncoding(Encoding::RGBA); // Update encoding
                this->setIsBigEndian(false); // Update endianness
            }

        }

    }
}
