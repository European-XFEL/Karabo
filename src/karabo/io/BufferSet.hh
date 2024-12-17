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
/*
 * File:   BufferSet.hh
 * Author: haufs
 *
 * Created on April 17, 2018, 9:22 AM
 */

#ifndef KARABO_IO_BUFFERSET_HH
#define KARABO_IO_BUFFERSET_HH

#include <boost/asio/buffer.hpp> //boost::asio::const_buffer
#include <memory>                //std::shared_ptr<T>
#include <ostream>               //std::ostream
#include <vector>                //std::vector

#include "karabo/util/ClassInfo.hh" //BufferSet::Pointer
#include "karabo/util/Exception.hh" //KARABO_LOGIC_EXCEPTION
#include "karabo/util/Types.hh"     //karabo::util::ByteArray

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace io {

        /*
         * @class BufferSet
         * @brief The BufferSet implements a set of buffers to be used for binary Hash serialization. It can be
         * configured to always copy all data, or when possible only hold shared pointers to data.
         */
        class BufferSet {
           public:
            KARABO_CLASSINFO(karabo::io::BufferSet, "BufferSet", "1.0");

            typedef std::vector<char> BufferType;

            /*
             * @enum BufferContents
             * @brief An enumerator qualifying the contants of a given buffer in a BufferSet
             */
            enum BufferContents { COPY = 0, NO_COPY_BYTEARRAY_CONTENTS };

           private:
            /*
             * @class Buffer
             * @brief The Buffer can contain data in either of two ways
             *
             * - either data in a shared_ptr to a BufferType (member 'vec')
             * - or shared_ptr to BufferType::value_type ('ptr') where the length of the data is
             *   given by member 'size' (useful to keep ByteArray data).
             *
             * If the data is kept in the first way, it is the responsibility of the user of the
             * Buffer to synchronize 'size' and 'vec.size()'. To synchronize its current Buffer,
             * the BufferSet provides the method updateSize().
             *
             * The member 'contentType' shall take values of the enum BufferSetContents only
             * to indicate which kind of buffer we have.
             */
            struct Buffer {
                std::shared_ptr<BufferType::value_type> ptr;
                std::shared_ptr<BufferType> vec;
                std::size_t size;
                int contentType;

                Buffer() : size(0), contentType(BufferContents::COPY) {
                    vec = std::shared_ptr<BufferType>(new BufferType());
                    ptr = std::shared_ptr<BufferType::value_type>(vec->data(), [](void*) {});
                }

                Buffer(std::shared_ptr<BufferType> v, std::shared_ptr<BufferType::value_type> p, std::size_t s,
                       BufferContents cType) {
                    ptr = p;
                    vec = v;
                    size = s;
                    contentType = cType;
                }
            };

           public:
            /**
             * Construct a BufferSet
             * @param copyAllData, set to true if data should always be copied
             */
            explicit BufferSet(bool copyAllData = false);

            virtual ~BufferSet();

            /**
             * Add empty buffer (COPY type) to the BufferSet.
             *
             * No new buffer is added if the last buffer is still empty and of COPY type.
             * Also makes internal information about size of last buffer (before adding a new one) consistent.
             */
            void add();
            /**
             * Add a buffer to the BufferSet
             * @param size - size of buffer
             * @param type - BufferContents::COPY means buffer is COPY type
             *               BufferContents::NO_COPY_BYTEARRAY_CONTENTS means buffer where space allocated in char array
             * (NO_COPY_BYTEARRAY_CONTENTS type)
             */
            void add(std::size_t size, int type);

            /**
             * Update the size of the current buffer to reflect the size of the vector is refers to
             */
            void updateSize() {
                if (m_buffers.size()) {
                    if (m_buffers.back().contentType == BufferContents::COPY) {
                        m_buffers.back().size = m_buffers.back().vec->size();
                    }
                }
            }

            /**
             * Return the last buffer in the BufferSet
             */
            BufferType& back() const {
                return *m_buffers.back().vec;
            }

            /**
             * Return the current buffer in the BufferSet
             * @return
             */
            BufferType& current() const {
                return *(m_buffers[m_currentBuffer].vec);
            }

            /**
             * Rewind the BufferSet to the first position
             */
            void rewind() const {
                m_currentBuffer = 0;
            }

            /**
             * Advance to the next buffer in the BufferSet
             * @return true if a next buffer exists, false otherwise
             */
            bool next() const;

            /**
             * Emplace a ByteArray at the back of the BufferSet
             */
            void emplaceBack(const karabo::util::ByteArray& array, bool writeSize = true);

            /**
             * Emplace a shared pointer to a vector at the end of the BufferSet
             */
            void emplaceBack(const std::shared_ptr<BufferType>& ptr);

            /**
             * Append the contents of this BufferSet to another BufferSet
             * @param other: the BufferSet to append to
             * @param copy
             */
            void appendTo(BufferSet& other, bool copy = true) const;

            /**
             * Return the current buffer as a ByteArray
             * @return
             */
            karabo::util::ByteArray currentAsByteArray() const;

            /**
             * Clear the BufferSet
             */
            void clear();

            /**
             * Return the combined byte size of all buffers in the BufferSet.
             * @return
             */
            size_t totalSize() const;


            /**
             * Will return true if any data in the BufferSet is a reference or a pointer to data not managed by
             * the BufferSet.
             */
            inline bool containsNonCopies() const {
                for (auto it = m_buffers.begin(); it != m_buffers.end(); ++it) {
                    if (it->contentType != BufferContents::COPY) {
                        return true;
                    }
                }
                return false;
            }

            /**
             * Append the buffers of this BufferSet to, for instance, a vector of boost asio buffers, const or mutable
             * @param boost_buffers to a append to
             */
            template <typename BufferSequenceType>
            void appendTo(BufferSequenceType& boost_buffers) const {
                for (auto it = m_buffers.begin(); it != m_buffers.end(); ++it) {
                    if (it->size) {
                        if (it->contentType == BufferContents::NO_COPY_BYTEARRAY_CONTENTS) {
                            boost_buffers.push_back(boost::asio::buffer(it->ptr.get(), it->size));
                        } else {
                            boost_buffers.push_back(boost::asio::buffer(it->vec->data(), it->size));
                        }
                    } else if (it->vec && !it->vec->empty()) {
                        throw KARABO_LOGIC_EXCEPTION("Buffer size zero, but vector not empty.");
                    }
                }
            }


            /**
             * Returns true if the current buffer is a copy of a ByteArray
             * @return
             */
            bool currentIsByteArrayCopy() const {
                return m_buffers[m_currentBuffer].contentType == BufferContents::COPY;
            }

            friend std::ostream& operator<<(std::ostream& os, const BufferSet& bs);

            /**
             * @return Returns vector of sizes of this BufferSet
             */
            std::vector<unsigned int> sizes() const {
                std::vector<unsigned int> v;
                for (const auto& b : m_buffers) {
                    v.push_back(b.size);
                }
                return v;
            }

            /**
             * @return Returns vector of buffer types for this BufferSet
             */
            std::vector<int> types() const {
                std::vector<int> v;
                for (const auto& b : m_buffers) {
                    v.push_back(b.contentType);
                }
                return v;
            }

            template <typename BufferSequenceType>
            static void appendTo(BufferSequenceType& boostBuffers, const std::vector<BufferSet::Pointer>& bufferSets) {
                for (const auto& b : bufferSets) {
                    b->appendTo<BufferSequenceType>(boostBuffers);
                }
            }

           private:
            std::vector<Buffer> m_buffers;
            mutable size_t m_currentBuffer;
            bool m_copyAllData;
        };
    } // namespace io
} // namespace karabo

#endif /* KARABO_IO_BUFFERSET_HH */
