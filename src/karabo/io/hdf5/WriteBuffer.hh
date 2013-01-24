/*
 * $Id: WriteBuffer.hh 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_WRITEBUFFER_HH
#define	KARABO_IO_WRITEBUFFER_HH

#include <string>
#include <vector>

#include <karabo/util/Hash.hh>
#include <karabo/util/Types.hh>

#include <boost/function.hpp>
#include "DataFormat.hh"
#include <karabo/util/Profiler.hh>

#ifdef DEBUG_IO_HDF5_WRITEBUFFER
#define tracer if(0); else std::cerr
#else 
#define tracer if(1); else std::cerr
#endif

namespace karabo {
    namespace io {

        class WriteBuffer {
        public:
            typedef boost::function< void (const karabo::util::Hash&, size_t&) > WriteHandler;


        public:

            void registerWriteHandler(WriteHandler handler) {
                m_handler = handler;
            }

            WriteBuffer(size_t size) : m_capacity(size), m_size(size), m_index(0), m_keyNumber(std::vector<boost::any*>()) {
            }

            virtual ~WriteBuffer() {
            }

            template<class T>
            size_t defineArrayColumn(const std::string& key, karabo::io::ArrayDimensions dims) {


                // only for format discovery
                karabo::io::ArrayView<T > recordView(0, dims);
                m_record.setFromPath(key, recordView, "/");


                // allocates own memory for buffer in a continues block                                
                //dims.insert(dims.begin(), static_cast<unsigned long long> (m_capacity));
                karabo::io::ArrayDimensions bufDims(m_capacity);
                karabo::io::ArrayView< karabo::io::ArrayView<T> > arrayView(bufDims);
                m_buffer.setFromPath(key, arrayView, "/");
                

                karabo::util::Hash::iterator it = m_buffer.find(key);
                if (it != m_buffer.end()) {
                    m_keyNumber.push_back(&(m_buffer.getAny(it)));
                    m_keys.push_back(key);
                }

                return m_keyNumber.size() - 1;
            }

            template<class T>
            size_t defineColumn(const std::string& key) {



                karabo::io::ArrayDimensions dims(m_size);
                karabo::io::ArrayView<T > arrayView(dims);
                m_buffer.setFromPath(key, arrayView, "/");

                m_record.setFromPath(key, T(), "/");

                karabo::util::Hash::iterator it = m_buffer.find(key);
                if (it != m_buffer.end()) {
                    m_keyNumber.push_back(&(m_buffer.getAny(it)));
                    m_keys.push_back(key);
                }

                return m_keyNumber.size() - 1;
            }

            virtual void commitDefinition() {
            }

            template<class T>
            void set(size_t keyNumber, const T& value) {
                boost::any* anyElement = m_keyNumber[keyNumber];
                if (anyElement->type() != typeid (karabo::io::ArrayView<T>)) {
                    const karabo::util::Types& types = karabo::util::Types::getInstance();
                    karabo::util::Hash::const_iterator it = m_record.find(m_keys[keyNumber]);
                    std::ostringstream os;
                    os << "Expected " << types.getTypeAsString(m_record.getAny(it).type()) << " type";
                    throw KARABO_CAST_EXCEPTION(os.str());
                }
                karabo::io::ArrayView<T>& av = *(boost::any_cast<karabo::io::ArrayView<T> >(anyElement));
                av[m_index] = value;                
            }

            template<class T>
            void setArray(size_t keyNumber, karabo::io::ArrayView<T>& value) {
                
                boost::any* anyElement = m_keyNumber[keyNumber];
                if (anyElement->type() != typeid (karabo::io::ArrayView<karabo::io::ArrayView<T> >)) {
                    const karabo::util::Types& types = karabo::util::Types::getInstance();
                    karabo::util::Hash::const_iterator it = m_record.find(m_keys[keyNumber]);
                    std::ostringstream os;
                    os << "Expected " << types.getTypeAsString(m_record.getAny(it).type()) << " type";
                    throw KARABO_CAST_EXCEPTION(os.str());
                }
                karabo::io::ArrayView<karabo::io::ArrayView<T> >& av = *(boost::any_cast<karabo::io::ArrayView<karabo::io::ArrayView<T> > >(anyElement));
                av[m_index] = value;
            }


            inline void next() {
                m_index++;
                tracer << "m_index: " << m_index << " m_size: " << m_size << std::endl;
                if (m_index == m_size) {
                    m_handler(m_buffer, m_size);
                    m_index = 0;
                    m_size = m_capacity;
                }
            }

            inline void flush() {
                if (m_index == 0) return;
                size_t currentSize = m_index;
                m_handler(m_buffer, currentSize);
                m_size = m_capacity - m_index;
                m_index = 0;
            }

            virtual void close() {}

        protected:
            WriteHandler m_handler;

            size_t m_capacity;
            size_t m_size;
            size_t m_index;
            std::vector<boost::any* > m_keyNumber;
            std::vector<std::string > m_keys;
            karabo::util::Hash m_buffer;
            karabo::util::Hash m_record;




        };

        namespace hdf5 {

            class WriteBuffer : public karabo::io::WriteBuffer {
            public:

                WriteBuffer(File& file, const std::string& tableName, size_t size) : karabo::io::WriteBuffer(size), m_file(file), m_tableName(tableName) {
                }

                virtual ~WriteBuffer() {
                }

                void commitDefinition() {
                    DataFormat::Pointer df = DataFormat::discoverFromData(m_record);
                    m_table = m_file.createTable(m_tableName, df, m_size);
                    m_handler = boost::bind(&WriteBuffer::write, this, _1, _2);
                }

                void close() {
                    this->flush();
                    m_table->close();
                }
 
            private:

                void write(const karabo::util::Hash& data, size_t size) {
                    static size_t recordNumber = 0;
                    tracer << "writing Hash of vectors of sizes: " << size << " at position " << recordNumber << std::endl;
                    m_table->writeBuffer(data, recordNumber, size);
                    recordNumber += size;
                }



                karabo::io::hdf5::File& m_file;
                karabo::io::hdf5::Table::Pointer m_table;
                std::string m_tableName;


            };
        }
    }
}


#endif	/* KARABO_IO_WRITEBUFFER_HH */

