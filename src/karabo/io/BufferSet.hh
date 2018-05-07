/* 
 * File:   BufferSet.hh
 * Author: haufs
 *
 * Created on April 17, 2018, 9:22 AM
 */

#ifndef KARABO_IO_BUFFERSET_HH
#define	KARABO_IO_BUFFERSET_HH

#include <ostream>                                          //std::ostream
#include <vector>                                           //std::vector
#include <boost/shared_ptr.hpp>                             //boost::shared_ptr<T>
#include <boost/asio/buffer.hpp>                            //boost::asio::const_buffer
#include <boost/core/null_deleter.hpp>                      //boost::null_delter
#include <karabo/util/Types.hh>                             //karabo::util::ByteArray

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

            typedef std::vector<char> BufferType;
        private:
            
                        
            /*
            * @enum BufferContents
            * @brief An enumerator qualifying the contants of a given buffer in a BufferSet
            */
            enum BufferContents {
                COPY = 0,
                NO_COPY_BYTEARRAY_CONTENTS
            };
            
            /*
            * @class Buffer
            * @brief The Buffer groups vectors, pointers, sizes and contents of a BufferSet
            */
            struct Buffer {
                boost::shared_ptr<BufferType::value_type> ptr;
                boost::shared_ptr<BufferType> vec;
                std::size_t size;
                int contentType;
                
                Buffer() {                    
                    vec = boost::shared_ptr<BufferType>(new BufferType());
                    ptr = boost::shared_ptr<BufferType::value_type>(vec->data(), boost::null_deleter());
                    size = 0;
                    contentType = BufferContents::COPY;
                }
                
                Buffer(boost::shared_ptr<BufferType> v, boost::shared_ptr<BufferType::value_type> p, std::size_t s, BufferContents cType) {
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
             * Add a buffer to the BufferSet
             */
            void add();

            /**
             * Update the size of the current buffer to reflect the size of the vector is refers to
             */
            void updateSize() {
                if (m_buffers.size()) {
                    if(m_buffers.back().contentType == BufferContents::COPY) {
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
            void emplaceBack(const karabo::util::ByteArray& array, bool writeSize=true);

            /**
             * Emplace a shared pointer to a vector at the end of the BufferSet
             */
            void emplaceBack(const boost::shared_ptr<BufferType>& ptr);

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
            * Append the buffers of this BufferSet to, for instance, a vector of boost asio buffers, const or mutable
            * @param boost_buffers to a append to
            */
            template<typename BufferSequenceType>
            void appendTo(BufferSequenceType& boost_buffers) const {
                for (auto it = m_buffers.begin(); it != m_buffers.end(); ++it) {
                    if (it->size) {
                        if (it->contentType == BufferContents::NO_COPY_BYTEARRAY_CONTENTS) {                                                  
                            boost_buffers.push_back(boost::asio::buffer(it->ptr.get(), it->size));
                        } else {
                            boost_buffers.push_back(boost::asio::buffer(&(it->vec->front()), it->size));
                        }
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

        private:

            std::vector<Buffer> m_buffers;
            mutable size_t m_currentBuffer;
            bool m_copyAllData;

        };
    }
}

#endif	/* KARABO_IO_BUFFERSET_HH */

