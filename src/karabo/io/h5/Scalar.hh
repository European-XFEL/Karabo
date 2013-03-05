/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_SCALAR_HH
#define	KARABO_IO_H5_SCALAR_HH

#include <string>

#include "Dataset.hh"
#include "ErrorHandler.hh"
#include "TypeTraits.hh"
#include <karabo/util/util.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/FromTypeInfo.hh>

#include "DatasetReader.hh"
#include "DatasetWriter.hh"


#include "ioProfiler.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            template<class T>
            class Scalar : public karabo::io::h5::Dataset {
            public:

                KARABO_CLASSINFO(Scalar, karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0")


                Scalar(const karabo::util::Hash& input) : Dataset(input)               
                {

                }

            public:

                virtual ~Scalar() {
                }

                void create(hsize_t chunkSize) {
                    try {
                        m_chunkSize = chunkSize;
                        karabo::util::Dims dims(chunkSize);
                        createDataSetProperties(dims);
                        m_fileDataSpace = Dataset::dataSpace(0);
                        m_dataSet = H5Dcreate(m_group, m_h5name.c_str(), ScalarTypes::getHdf5StandardType<T > (),
                                m_fileDataSpace, H5P_DEFAULT, m_dataSetProperties, H5P_DEFAULT);
                        KARABO_CHECK_HDF5_STATUS(m_dataSet);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + m_h5PathName));
                    }

                }

                void write(const karabo::util::Hash& data, hsize_t recordId) {

                    try {
                        if (recordId % m_chunkSize == 0) {
                            m_fileDataSpace = extend(m_dataSet, m_fileDataSpace, m_chunkSize);
                        }
                        m_fileDataSpace = Dataset::selectScalarRecord(m_fileDataSpace, recordId);
                        hid_t mds = Dataset::dataSpace();
                        DatasetWriter<T>::write(data.get<const T > (m_key, '/'), m_dataSet, mds, m_fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                    }
                }

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {

                    try {
                        Dataset::extend(m_dataSet, m_fileDataSpace, len);
                        Dataset::selectRecord(m_fileDataSpace, recordId, len);

                        const T* ptr = data.get<T*>(m_key, '/');
                        hid_t mds = Dataset::dataSpace(len);
                        DatasetWriter<T>::write(ptr, len, m_dataSet, mds, m_fileDataSpace);
                    } catch (karabo::util::Exception& e) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                    }

                }


                //
                //                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                //1
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
                //                        DatasetReader<T>::read(value, m_dataSet, m_memoryDataSpace, m_fileDataSpace);
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
