/* 
 * File:   RawImageData.cc
 * Author: 
 * 
 * Created on July 10, 2013, 10:34 AM
 */

#include "RawImageData.hh"

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


        const Hash& RawImageData::getHeader() const {
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
    }
}
