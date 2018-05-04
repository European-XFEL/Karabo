#include "boost/core/null_deleter.hpp"
#include "BufferSet.hh"
#include <karabo/util/StringTools.hh>
#include "BinarySerializer.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace io {

        typedef std::vector<char> BufferType;


        BufferSet::BufferSet(bool copy_all_data) : m_currentBuffer(0), m_copyAllData(copy_all_data) {
              m_buffers.push_back(Buffer());          
        };


        BufferSet::~BufferSet() {
            clear();
        }


        void BufferSet::add() {
            updateSize();
            m_buffers.push_back(Buffer());
            m_currentBuffer++;
        }


        bool BufferSet::next() const {
            if (m_currentBuffer + 1 < m_buffers.size()) {
                m_currentBuffer++;
                return true;
            }
            return false;
        }


        void BufferSet::emplaceBack(const karabo::util::ByteArray& array, bool writeSize) {
            BufferType& buffer = current();
            if(writeSize) {
                buffer.reserve(buffer.size() + sizeof(unsigned int) + array.second);
                {
                    unsigned int size = static_cast<unsigned int> (array.second);
                    const char* src = reinterpret_cast<const char*> (&size);
                    const size_t n = sizeof (unsigned int);
                    const size_t pos = buffer.size();
                    buffer.resize(pos + n);
                    std::memcpy(buffer.data() + pos, src, n);
                }
            }
            if (m_copyAllData) {
                {
                    const char* src = array.first.get();
                    const size_t n = array.second;
                    const size_t pos = buffer.size();
                    buffer.resize(pos + n);
                    std::memcpy(buffer.data() + pos, src, n);
                }
            } else {
                updateSize();
                m_buffers.push_back(Buffer(boost::shared_ptr<BufferType>(new BufferType()),
                                                    boost::const_pointer_cast<BufferType::value_type>(array.first),
                                                    array.second,
                                                    BufferContents::NO_COPY_BYTEARRAY_CONTENTS));
                m_currentBuffer++;
                add();
            }
        }


        void BufferSet::emplaceBack(const boost::shared_ptr<BufferType>& ptr) {
            if (m_copyAllData) {

                const char* src = reinterpret_cast<const char*> (ptr->data());
                const size_t n = ptr->size();
                if (m_buffers.back().size != 0) {
                    add();
                }
                BufferType& buffer = current();
                const size_t pos = buffer.size();
                buffer.resize(pos + n);
                std::memcpy(buffer.data() + pos, src, n);
                add();
            } else {
                if (m_buffers.back().size == 0) {
                    Buffer & buffer = m_buffers.back();
                    buffer.vec = boost::const_pointer_cast<BufferType>(ptr);
                    buffer.ptr = boost::shared_ptr<BufferType::value_type>(0);
                    buffer.size = ptr->size();
                    buffer.contentType = BufferContents::COPY;
                    add();
                } else {
                    m_buffers.push_back(Buffer(boost::const_pointer_cast<BufferType>(ptr),
                                                        boost::shared_ptr<BufferType::value_type>(0),
                                                        ptr->size(),
                                                        BufferContents::COPY));
                    
                    m_currentBuffer++;
                    add();
                }
            }
        }


        void BufferSet::appendTo(BufferSet& other, bool copy) const {
            for (auto it = m_buffers.begin(); it != m_buffers.end(); ++it) {
                if (it->contentType == BufferContents::NO_COPY_BYTEARRAY_CONTENTS) {
                    //do not write the size as it is in previous buffer
                    other.emplaceBack(std::make_pair(it->ptr, it->size), false);
                } else if (it->size) {
                    if (copy) {
                        const char* src = it->vec->data();
                        const size_t n = it->size;
                        if (other.current().size()) {
                            other.add();
                        }
                        BufferType& buffer = other.current();
                        const size_t pos = buffer.size();
                        buffer.resize(pos + n);
                        std::memcpy(buffer.data() + pos, src, n);
                    } else {
                        other.emplaceBack(it->vec);
                    }
                }

            }
            other.add();
        }


        karabo::util::ByteArray BufferSet::currentAsByteArray() const {
            return std::make_pair(boost::const_pointer_cast<char>(m_buffers[m_currentBuffer].ptr), m_buffers[m_currentBuffer].size);
        }


        void BufferSet::clear() {
            m_buffers.clear();
            m_buffers.push_back(Buffer());
            m_currentBuffer = 0;
        }


        size_t BufferSet::totalSize() const {
            size_t total = 0;
            for (auto it = m_buffers.begin(); it != m_buffers.end(); ++it) {
                total += it->size;
            }
            return total;
        }

        std::ostream& operator<<(std::ostream& os, const BufferSet& bs) {
            using namespace karabo::util;
            os << "BufferSet content:\n";
            os << "\t\"copy_all_data\" flag is\t" << std::boolalpha << bs.m_copyAllData << '\n';
            os << "\tCurrent buffer index is\t" << bs.m_currentBuffer << '\n';
            
            std::vector<decltype(bs.m_buffers.front().size)> size_vec;
            std::vector<decltype(bs.m_buffers.front().contentType)> contentType_vec;
            for(auto it = bs.m_buffers.begin(); it != bs.m_buffers.end(); ++it) {
                size_vec.push_back(it->size);
                contentType_vec.push_back(it->contentType);
            }
            os << "\tBuffer sizes ...\t" << toString(size_vec) << '\n';
            
            os << "\tNon-copied buffers...\t" << toString(contentType_vec) << '\n';
            os << "\tSize of buffer group is\t" << bs.m_buffers.size() << '\n';
            os << "\tBuffer content ...\n";
            size_t i = 0;
            for (auto it = bs.m_buffers.begin(); it != bs.m_buffers.end(); ++it) {
                os << "\t\t" << std::dec << i << "\t" << (it->contentType == BufferSet::NO_COPY_BYTEARRAY_CONTENTS ? "nocopy" : "copy")
                        << "\t size=" << std::setw(12) << std::setfill(' ') << it->size
                        << " :  0x" << std::hex;
                for (size_t j = 0; j < std::min(size_t(30), it->size); ++j) {
                    os << std::setw(2) << std::setfill('0') << int(it->contentType == BufferSet::BufferContents::NO_COPY_BYTEARRAY_CONTENTS ?
                        it->ptr.get()[j] : it->vec->data()[j]);
                }
                os << std::dec << (it->size > 30 ? "...":"") << '\n';
                i++;
            }
            return os;
        }
    }
}
