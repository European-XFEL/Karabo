/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "BufferSet.hh"

#include "BinarySerializer.hh"
#include "boost/core/null_deleter.hpp"
#include "karabo/data/types/StringTools.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace io {

        typedef std::vector<char> BufferType;


        BufferSet::BufferSet(bool copyAllData) : m_currentBuffer(0), m_copyAllData(copyAllData) {
            m_buffers.push_back(Buffer());
        };


        BufferSet::~BufferSet() {
            clear();
        }


        void BufferSet::add() {
            updateSize();
            if (m_buffers.empty() || m_buffers.back().size ||
                m_buffers.back().contentType == BufferSet::NO_COPY_BYTEARRAY_CONTENTS) { // empty ByteArray
                m_buffers.push_back(Buffer());
                m_currentBuffer++;
            }
        }


        void BufferSet::add(std::size_t size, int type) {
            updateSize();

            if (type == BufferContents::COPY) { // allocate space as std::vector<char>
                auto vec = std::shared_ptr<BufferType>(new BufferType(size));
                auto ptr = std::shared_ptr<BufferType::value_type>();
                if (!m_buffers.size() || m_buffers.back().size) {
                    m_buffers.push_back(Buffer(vec, ptr, size, BufferContents::COPY));
                    m_currentBuffer++;
                } else {
                    m_buffers[m_buffers.size() - 1] = Buffer(vec, ptr, size, BufferContents::COPY);
                }
            } else if (type == BufferContents::NO_COPY_BYTEARRAY_CONTENTS) { // allocate space as char array
                if (m_buffers.empty() || m_buffers.back().size) {
                    // See https://www.boost.org/doc/libs/1_61_0/libs/smart_ptr/sp_techniques.html#array
                    m_buffers.push_back(Buffer(std::shared_ptr<BufferType>(new BufferType()),
                                               std::shared_ptr<char>(new char[size], std::default_delete<char[]>()),
                                               size, BufferContents::NO_COPY_BYTEARRAY_CONTENTS));
                    m_currentBuffer++;
                } else {
                    // Last buffer in BufferSet has size 0 - assign it a newly allocated buffer of size.
                    m_buffers[m_buffers.size() - 1] =
                          Buffer(std::shared_ptr<BufferType>(new BufferType()),
                                 std::shared_ptr<char>(new char[size], std::default_delete<char[]>()), size,
                                 BufferContents::NO_COPY_BYTEARRAY_CONTENTS);
                }
            } else {
                throw KARABO_LOGIC_EXCEPTION("Unknown buffer type!");
            }
        }


        bool BufferSet::next() const {
            if (m_currentBuffer + 1 < m_buffers.size()) {
                m_currentBuffer++;
                return true;
            }
            return false;
        }


        void BufferSet::emplaceBack(const karabo::data::ByteArray& array, bool writeSize) {
            if (writeSize) {
                BufferType& buffer = current();
                buffer.reserve(buffer.size() + sizeof(unsigned int) + array.second);

                const unsigned int size = static_cast<unsigned int>(array.second);
                const char* src = reinterpret_cast<const char*>(&size);
                const size_t n = sizeof(unsigned int);
                const size_t pos = buffer.size();
                buffer.resize(pos + n);
                std::memcpy(buffer.data() + pos, src, n);
            }

            updateSize();
            const size_t arraySize = array.second;
            if (m_copyAllData) {
                // Copy, but keep an extra buffer: That's beneficial when further processed, e.g. de-serialised.
                m_buffers.push_back(Buffer(std::shared_ptr<BufferType>(new BufferType()),
                                           std::shared_ptr<char>(new char[arraySize], std::default_delete<char[]>()),
                                           arraySize, BufferContents::NO_COPY_BYTEARRAY_CONTENTS));

                auto* rawPtrDest = m_buffers.back().ptr.get();
                const auto* rawPtrSrc = array.first.get();
                std::memcpy(rawPtrDest, rawPtrSrc, arraySize);
            } else {
                m_buffers.push_back(Buffer(std::shared_ptr<BufferType>(new BufferType()),
                                           std::const_pointer_cast<BufferType::value_type>(array.first), arraySize,
                                           BufferContents::NO_COPY_BYTEARRAY_CONTENTS));
            }
            m_currentBuffer++;
            add();
        }


        void BufferSet::emplaceBack(const std::shared_ptr<BufferType>& ptr) {
            if (m_copyAllData) {
                const char* src = reinterpret_cast<const char*>(ptr->data());
                const size_t n = ptr->size();
                if (m_buffers.back().size != 0) {
                    add();
                }
                BufferType& buffer = current();
                const size_t pos = buffer.size();
                buffer.resize(pos + n);
                std::memcpy(buffer.data() + pos, src, n);
                updateSize();
            } else {
                if (m_buffers.back().size == 0) {
                    Buffer& buffer = m_buffers.back();
                    buffer.vec = std::const_pointer_cast<BufferType>(ptr);
                    buffer.ptr = std::shared_ptr<BufferType::value_type>();
                    buffer.size = ptr->size();
                    buffer.contentType = BufferContents::COPY;
                } else {
                    m_buffers.push_back(Buffer(std::const_pointer_cast<BufferType>(ptr),
                                               std::shared_ptr<BufferType::value_type>(), ptr->size(),
                                               BufferContents::COPY));

                    m_currentBuffer++;
                }
            }
        }


        void BufferSet::appendTo(BufferSet& other, bool copy) const {
            for (auto it = m_buffers.begin(); it != m_buffers.end(); ++it) {
                if (it->size == 0) {
                    if (it->vec && it->vec->size()) { // Buffer is inconsistent - how/where to catch earlier?
                        throw KARABO_LOGIC_EXCEPTION("Buffer size zero, but vector not empty.");
                    }
                    continue; // Empty buffer can be skipped.
                }
                if (it->contentType == BufferContents::NO_COPY_BYTEARRAY_CONTENTS) {
                    // do not write the size as it is in previous buffer
                    other.emplaceBack(std::make_pair(it->ptr, it->size), false);
                } else {
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
        }


        karabo::data::ByteArray BufferSet::currentAsByteArray() const {
            return std::make_pair(std::const_pointer_cast<char>(m_buffers[m_currentBuffer].ptr),
                                  m_buffers[m_currentBuffer].size);
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
            using namespace karabo::data;
            os << "BufferSet content:\n";
            os << "\t\"copyAllData\" flag is\t" << std::boolalpha << bs.m_copyAllData << '\n';
            os << "\tCurrent buffer index is\t" << bs.m_currentBuffer << '\n';

            std::vector<decltype(bs.m_buffers.front().size)> size_vec;
            std::vector<decltype(bs.m_buffers.front().contentType)> contentType_vec;
            for (auto it = bs.m_buffers.begin(); it != bs.m_buffers.end(); ++it) {
                size_vec.push_back(it->size);
                contentType_vec.push_back(it->contentType);
            }
            os << "\tBuffer sizes ...\t" << toString(size_vec) << '\n';

            os << "\tNon-copied buffers...\t" << toString(contentType_vec) << '\n';
            os << "\tSize of buffer group is\t" << bs.m_buffers.size() << ", total size is " << bs.totalSize() << '\n';
            os << "\tBuffer content ...\n";
            std::vector<size_t> badBuffers;
            size_t i = 0;
            for (auto it = bs.m_buffers.begin(); it != bs.m_buffers.end(); ++it) {
                bool localOk = true;
                if (it->vec && !it->vec->empty() && it->vec->size() != it->size) {
                    localOk = false;
                    badBuffers.push_back(i);
                }
                os << "\t\t" << std::dec << i << "\t"
                   << (it->contentType == BufferSet::NO_COPY_BYTEARRAY_CONTENTS ? "nocopy" : "copy");
                if (localOk) {
                    os << "\t size=" << std::setw(12) << std::setfill(' ') << it->size;
                } else { // Print both of the inconsistent sizes...
                    os << "\t size=" << std::setw(5) << std::setfill(' ') << it->size << "/" << std::setw(5)
                       << std::setfill(' ') << it->vec->size();
                }
                os << " :  0x" << std::hex;
                for (size_t j = 0; j < std::min(size_t(30), it->size); ++j) {
                    os << std::setw(2) << std::setfill('0')
                       << int(it->contentType == BufferSet::BufferContents::NO_COPY_BYTEARRAY_CONTENTS
                                    ? it->ptr.get()[j]
                                    : it->vec->data()[j]);
                }
                os << std::dec << (it->size > 30 ? "..." : "") << '\n';
                i++;
            }
            if (!badBuffers.empty()) {
                os << "\t Bad BufferSet: buffers " << toString(badBuffers) << " have inconsistent sizes!\n";
            }
            return os;
        }
    } // namespace io
} // namespace karabo
