/*
 * $Id: Scalar.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_IO_SCALAR_HH
#define	EXFEL_IO_SCALAR_HH

#include "RecordElement.hh"
#include "TypeTraits.hh"
#include <karabo/util/Hash.hh>
#include "ScalarFilter.hh"
#include <string>

#include <karabo/util/Time.hh>
#include "../ioProfiler.hh"
/**
 * The main European XFEL namespace
 */
namespace exfel {

    namespace io {

        namespace hdf5 {

            /**
             * ScalarWriter is needed to support bool type. HDF5 does not support bool and we need to specialize
             * this class. bool values are stored as unsigned chars (1byte)
             */
            template< typename T>
            class ScalarWriter {
            public:

                static void write(const T& value, H5::DataSet& dataSet, H5::DataSpace memoryDataSpace, H5::DataSpace fileDataSpace) {
                    dataSet.write(&value, ScalarTypes::getHdf5NativeType<T > (), memoryDataSpace, fileDataSpace);
                }
            };

            template<>
            class ScalarWriter<bool> {
            public:

                static void write(const bool& value, H5::DataSet& dataSet, H5::DataSpace memoryDataSpace, H5::DataSpace fileDataSpace) {
                    unsigned char converted = boost::numeric_cast<unsigned char>(value);
                    dataSet.write(&converted, ScalarTypes::getHdf5NativeType<unsigned char> (), memoryDataSpace, fileDataSpace);
                }
            };

            template< typename T>
            class ScalarReader {
            public:

                static void read(T& value, H5::DataSet& dataSet, H5::DataSpace memoryDataSpace, H5::DataSpace fileDataSpace) {
                    dataSet.read(&value, ScalarTypes::getHdf5NativeType<T > (), memoryDataSpace, fileDataSpace);
                }
            };

            template<>
            class ScalarReader<std::string> {
            public:

                static void read(std::string& value, H5::DataSet& dataSet, H5::DataSpace memoryDataSpace, H5::DataSpace fileDataSpace) {
                    dataSet.read(value, ScalarTypes::getHdf5NativeType<std::string > (), memoryDataSpace, fileDataSpace);
                }
            };

            template<class T>
            class Scalar : public exfel::io::RecordElement {
            public:

                EXFEL_CLASSINFO(Scalar, ScalarTypeTraits::classId<T>(), "1.0")

                Scalar() {
                    m_memoryDataSpace = scalarDataSpace();
                }

                virtual ~Scalar() {
                }

                void create(boost::shared_ptr<H5::Group> group, hsize_t chunkSize) {
                    m_group = group;
                    try {
                        createDataSetProperties(chunkSize);
                        m_fileDataSpace = scalarFileDataSpace(0);
                        m_dataSet = group->createDataSet(m_key.c_str(), ScalarTypes::getHdf5StandardType<T > (), m_fileDataSpace, *m_dataSetProperties);
                    } catch (...) {
                        RETHROW
                    }

                }

                void write(const exfel::util::Hash& data, hsize_t recordId) {

                    //here we do not use filters, for performance reason (?)
                    exfel::util::Hash::const_iterator it = data.find(m_key);
                    if (it == data.end()) { // TODO: do we need here to check if iterator is ok, is this performance issue
                        throw PARAMETER_EXCEPTION("Invalid key in the Hash");
                    }
                    try {
                        selectFileRecord(recordId);
                        ScalarWriter<T>::write(data.get<const T > (it), m_dataSet, m_memoryDataSpace, m_fileDataSpace);
                    } catch (...) {
                        RETHROW
                    }
                }

                void write(const exfel::util::Hash& data, hsize_t recordId, hsize_t len) {

                    EXFEL_PROFILER_SCALAR1

                    try {
                        EXFEL_PROFILER_START_SCALAR1("select")
                        selectFileRecord(recordId, len);
                        EXFEL_PROFILER_STOP_SCALAR1
                        EXFEL_PROFILER_START_SCALAR1("find")
                        exfel::util::Hash::const_iterator it = data.find(m_key);
                        EXFEL_PROFILER_STOP_SCALAR1
                        EXFEL_PROFILER_START_SCALAR1("any")
                        const boost::any& any = data.getAny(it);
                        EXFEL_PROFILER_STOP_SCALAR1

                        if (!m_filter) {
                            tracer << "creating a filter" << std::endl;
                            m_filter = ScalarFilter<T>::createDefault(any.type().name());
                        }
                        EXFEL_PROFILER_START_SCALAR1("filter")
                        m_filter->write(*this, any, len);
                        EXFEL_PROFILER_STOP_SCALAR1
                        EXFEL_PROFILER_REPORT_SCALAR1("select")
                        EXFEL_PROFILER_REPORT_SCALAR1("find")
                        EXFEL_PROFILER_REPORT_SCALAR1("any")
                        EXFEL_PROFILER_REPORT_SCALAR1("filter")
                    } catch (...) {
                        RETHROW
                    }

                }

                /*
                 * This function is not available via RecordElement interface
                 * To be used by filters only
                 */
                template<class U>
                void writeBuffer(const U* ptr, size_t len) const {
                    H5::DataSpace mds = this->getBufferDataSpace(len);
                    m_dataSet.write(ptr, ScalarTypes::getHdf5NativeType<U > (), mds, m_fileDataSpace);
                }

                /*
                 * This function is not available via RecordElement interface
                 * To be used by filters only
                 */

                template<class U>
                inline void readBuffer(U* ptr, size_t len) const {
                    try {
                        H5::DataSpace mds = getBufferDataSpace(len);
                        m_dataSet.read(ptr, ScalarTypes::getHdf5NativeType<U > (), mds, m_fileDataSpace);
                    } catch (...) {
                        RETHROW
                    }
                }

                /*
                 * This function is not available via RecordElement interface
                 * To be used by filters only.
                 * Variant with two parameters. Used for strings (and possibly to be used for converters)
                 */
                template<class U, class V >
                inline void readBuffer(U* ptr, const V& p, size_t len) const {
                    try {
                        H5::DataSpace mds = getBufferDataSpace(len);
                        m_dataSet.read(ptr, ScalarTypes::getHdf5NativeType<V > (), mds, m_fileDataSpace);
                    } catch (...) {
                        RETHROW
                    }
                }

                inline void allocate(exfel::util::Hash & data) {
                    data.set(m_key, T());
                }

                inline void allocate(exfel::util::Hash& buffer, size_t len) {
                    // check if one can use bindReference here
                    boost::shared_array<T> arr(new T[len]);
                    ArrayView<T> av(arr, len);
                    buffer.set(m_key, av);                                        
                }

                inline void read(exfel::util::Hash& data, hsize_t recordId) {
                    T& value = data.get<T > (m_key);
                    readValue(value, recordId);
                }

                inline void readValue(T& value, hsize_t recordId) {
                    try {
                        selectFileRecord(recordId);
                        ScalarReader<T>::read(value, m_dataSet, m_memoryDataSpace, m_fileDataSpace);
                    } catch (...) {
                        RETHROW
                    }
                }


                // buffered reading

                void read(exfel::util::Hash& data, hsize_t recordId, hsize_t len) {
                    try {
                        selectFileRecord(recordId, len);
                        exfel::util::Hash::iterator it = data.find(m_key);
                        boost::any& any = data.getAny(it);
                        if (!m_filter) {
                            tracer << "creating read filter" << std::endl;
                            m_filter = ScalarFilter<T>::createDefault(any.type().name());
                        }
                        m_filter->read(*this, any, len);
                    } catch (...) {
                        RETHROW
                    }

                }

                inline void readSpecificAttributes(exfel::util::Hash& attributes) {
                    attributes.setFromPath(m_key + ".rank", 0);
                    attributes.setFromPath(m_key + ".typeCategory", "Scalar");
                }


            protected:


            private:
                boost::shared_ptr<ScalarFilter<T> > m_filter;



            };

            // typedefs
            typedef Scalar<signed char> Int8Element;
            typedef Scalar<short> Int16Element;
            typedef Scalar<int> Int32Element;
            typedef Scalar<long long > Int64Element;
            typedef Scalar<unsigned char> UInt8Element;
            typedef Scalar<unsigned short> UInt16Element;
            typedef Scalar<unsigned int> UInt32Element;
            typedef Scalar<unsigned long long > UInt64Element;
            typedef Scalar<double> DoubleElement;
            typedef Scalar<float> FloatElement;
            typedef Scalar<std::string> StringElement;
            typedef Scalar<bool> BoolElement;


        }
    }
}

#endif	/* EXFEL_IO_SCALAR_HH */
