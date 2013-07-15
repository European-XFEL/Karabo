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

        
        RawImageData::RawImageData(const size_t byteSize,
                                   const karabo::util::Dims& dimensions,
                                   const EncodingType encoding,
                                   const ChannelSpaceType channelSpace,
                                   const bool isBigEndian,
                                   const karabo::util::Hash& header) : m_serializer(Serializer::create("Xml")), m_isShared(false) {
            m_hash = new Hash();

            std::vector<unsigned char>& buffer = m_hash->bindReference<std::vector<unsigned char> >("data");
            buffer.resize(byteSize);
            m_hash->set("dims", dimensions.toVector());
            m_hash->set<int>("encoding", encoding);
            m_hash->set<int>("channelSpace", channelSpace);
            m_hash->set<bool>("isBigEndian", isBigEndian);
            m_hash->set<std::string>("header", m_serializer->save(header));
        }


        RawImageData::RawImageData(karabo::util::Hash& imageHash, bool sharesData) : m_serializer(Serializer::create("Xml")), m_isShared(sharesData) {
            if (m_isShared) {
                m_hash = &imageHash;
            } else {
                m_hash = new Hash();
                *m_hash = imageHash;
            }
        }


        RawImageData::~RawImageData() {
            if (!m_isShared) delete m_hash;
        }


        Hash RawImageData::getHeader() const {
            if (m_hash->has("header")) {
                return m_serializer->load(m_hash->get<string>("header"));
            }
            return Hash();
        }
        
        void RawImageData::setHeader(const karabo::util::Hash& header) const {
            string& archive = m_hash->bindReference<string>("header");
            m_serializer->save(header, archive);
        }
    }
}
