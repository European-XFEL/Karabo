/*
 * $Id: Scalar.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_SCALAR_HH
#define	KARABO_IO_H5_SCALAR_HH

#include <string>

#include "Dataset.hh"
#include "TypeTraits.hh"
#include <karabo/util/util.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/FromTypeInfo.hh>

//#include "ScalarFilter.hh"


//#include <karabo/util/Time.hh>
//#include "../ioProfiler.hh"

namespace karabo {

    namespace io {

        namespace h5 {


            //ScalarWriter
            //ScalarReader
            //Scalar

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

                static void write(const T* ptr, hsize_t len, H5::DataSet& dataSet, H5::DataSpace memoryDataSpace, H5::DataSpace fileDataSpace) {
                    dataSet.write(ptr, ScalarTypes::getHdf5NativeType<T > (), memoryDataSpace, fileDataSpace);
                }

                static void write(const std::vector<T>& vec, H5::DataSet& dataSet, H5::DataSpace memoryDataSpace, H5::DataSpace fileDataSpace) {
                    const T* ptr = &vec[0];
                    dataSet.write(ptr, ScalarTypes::getHdf5NativeType<T > (), memoryDataSpace, fileDataSpace);
                }



            };

            template<>
            class ScalarWriter<bool> {
            public:

                static void write(const bool& value, H5::DataSet& dataSet, H5::DataSpace memoryDataSpace, H5::DataSpace fileDataSpace) {
                    unsigned char converted = boost::numeric_cast<unsigned char>(value);
                    dataSet.write(&converted, ScalarTypes::getHdf5NativeType<unsigned char> (), memoryDataSpace, fileDataSpace);
                }

                static void write(const bool* ptr, hsize_t len, H5::DataSet& dataSet, H5::DataSpace memoryDataSpace, H5::DataSpace fileDataSpace) {

                    std::vector<unsigned char> converted(len, 0);
                    for (size_t i = 0; i < len; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                    }
                    dataSet.write(ptr, ScalarTypes::getHdf5NativeType<unsigned char > (), memoryDataSpace, fileDataSpace);
                }

                static void write(const std::vector<bool>& vec, H5::DataSet& dataSet, H5::DataSpace memoryDataSpace, H5::DataSpace fileDataSpace) {

                    hsize_t len = vec.size();
                    std::vector<unsigned char> converted(len, 0);
                    for (size_t i = 0; i < len; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(vec[i]);
                    }
                    const unsigned char* ptr = &converted[0];
                    dataSet.write(ptr, ScalarTypes::getHdf5NativeType<unsigned char> (), memoryDataSpace, fileDataSpace);
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
            class Scalar : public karabo::io::h5::Dataset {
            public:

                KARABO_CLASSINFO(Scalar, karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0")



                Scalar(const karabo::util::Hash& input) : Dataset(input) {

                }

                virtual ~Scalar() {
                }

                void create(hsize_t chunkSize) {
                    try {
                        m_chunkSize = chunkSize;
                        createDataSetProperties(chunkSize);
                        m_fileDataSpace = Dataset::dataSpace(0);
                        m_dataSet = m_group->createDataSet(m_key.c_str(), ScalarTypes::getHdf5StandardType<T > (), m_fileDataSpace, *m_dataSetProperties);
                    } catch (...) {
                        KARABO_RETHROW
                    }

                }

                void write(const karabo::util::Hash& data, hsize_t recordId) {

                    try {
                        if (recordId % m_chunkSize == 0) extend(m_chunkSize);
                        //selectFileRecord(recordId);
                        Dataset::selectScalarRecord(m_fileDataSpace, recordId);
                        ScalarWriter<T>::write(data.get<const T > (m_path_key, '/'), m_dataSet, H5::DataSpace(H5S_SCALAR), m_fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW
                    }
                }

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {

                    //std::clog << "buffer write" << std::endl;
                    try {
                        //extend(len);
                        Dataset::extend(m_dataSet, m_fileDataSpace, len);
                        //selectFileRecord(recordId, len);
                        Dataset::selectRecord(m_fileDataSpace, recordId, len);

                        const T* ptr = data.get<T*>(m_path_key, '/');
                        H5::DataSpace mds = Dataset::dataSpace(len); //  this->getBufferDataSpace(len);
                        ScalarWriter<T>::write(ptr, len, m_dataSet, mds, m_fileDataSpace);
                    } catch (karabo::util::Exception& e) {
                        std::clog << "exception" << e << std::endl;
                        KARABO_RETHROW
                    }

                }


                //
                //                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                //
                //                    KARABO_PROFILER_SCALAR1
                //
                //                    try {
                //                        KARABO_PROFILER_START_SCALAR1("select")
                //                        selectFileRecord(recordId, len);
                //                        KARABO_PROFILER_STOP_SCALAR1
                //                        KARABO_PROFILER_START_SCALAR1("find")
                //                        karabo::util::Hash::const_iterator it = data.find(m_key);
                //                        KARABO_PROFILER_STOP_SCALAR1
                //                        KARABO_PROFILER_START_SCALAR1("any")
                //                        const boost::any& any = data.getAny(it);
                //                        KARABO_PROFILER_STOP_SCALAR1
                //
                //                        if (!m_filter) {
                //                            tracer << "creating a filter" << std::endl;
                //                            m_filter = ScalarFilter<T>::createDefault(any.type().name());
                //                        }
                //                        KARABO_PROFILER_START_SCALAR1("filter")
                //                        m_filter->write(*this, any, len);
                //                        KARABO_PROFILER_STOP_SCALAR1
                //                        KARABO_PROFILER_REPORT_SCALAR1("select")
                //                        KARABO_PROFILER_REPORT_SCALAR1("find")
                //                        KARABO_PROFILER_REPORT_SCALAR1("any")
                //                        KARABO_PROFILER_REPORT_SCALAR1("filter")
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //
                //                }
                //
                //                /*
                //                 * This function is not available via Element interface
                //                 * To be used by filters only
                //                 */
                //                template<class U>
                //                void writeBuffer(const U* ptr, size_t len) const {
                //                    H5::DataSpace mds = this->getBufferDataSpace(len);
                //                    m_dataSet.write(ptr, ScalarTypes::getHdf5NativeType<U > (), mds, m_fileDataSpace);
                //                }
                //
                //                /*
                //                 * This function is not available via Element interface
                //                 * To be used by filters only
                //                 */
                //
                //                template<class U>
                //                inline void readBuffer(U* ptr, size_t len) const {
                //                    try {
                //                        H5::DataSpace mds = getBufferDataSpace(len);
                //                        m_dataSet.read(ptr, ScalarTypes::getHdf5NativeType<U > (), mds, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //
                //                /*
                //                 * This function is not available via Element interface
                //                 * To be used by filters only.
                //                 * Variant with two parameters. Used for strings (and possibly to be used for converters)
                //                 */
                //                template<class U, class V >
                //                inline void readBuffer(U* ptr, const V& p, size_t len) const {
                //                    try {
                //                        H5::DataSpace mds = getBufferDataSpace(len);
                //                        m_dataSet.read(ptr, ScalarTypes::getHdf5NativeType<V > (), mds, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //
                //                inline void allocate(karabo::util::Hash & data) {
                //                    data.set(m_key, T());
                //                }
                //
                //                inline void allocate(karabo::util::Hash& buffer, size_t len) {
                //                    // check if one can use bindReference here
                //                    boost::shared_array<T> arr(new T[len]);
                //                    ArrayView<T> av(arr, len);
                //                    buffer.set(m_key, av);                                        
                //                }
                //
                //                inline void read(karabo::util::Hash& data, hsize_t recordId) {
                //                    T& value = data.get<T > (m_key);
                //                    readValue(value, recordId);
                //                }
                //
                //                inline void readValue(T& value, hsize_t recordId) {
                //                    try {
                //                        selectFileRecord(recordId);
                //                        ScalarReader<T>::read(value, m_dataSet, m_memoryDataSpace, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //
                //
                //                // buffered reading
                //
                //                void read(karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                //                    try {
                //                        selectFileRecord(recordId, len);
                //                        karabo::util::Hash::iterator it = data.find(m_key);
                //                        boost::any& any = data.getAny(it);
                //                        if (!m_filter) {
                //                            tracer << "creating read filter" << std::endl;
                //                            m_filter = ScalarFilter<T>::createDefault(any.type().name());
                //                        }
                //                        m_filter->read(*this, any, len);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //
                //                }
                //
                //                inline void readSpecificAttributes(karabo::util::Hash& attributes) {
                //                    attributes.setFromPath(m_key + ".rank", 0);
                //                    attributes.setFromPath(m_key + ".typeCategory", "Scalar");
                //                }
                //
                //
                //            protected:
                //
                //
                //            private:
                //                boost::shared_ptr<ScalarFilter<T> > m_filter;









            };

            // typedefs
            typedef Scalar<char> CharElement;
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

#endif	/* KARABO_IO_SCALAR_HH */
