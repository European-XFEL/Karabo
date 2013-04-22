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
#include <karabo/util/Configurator.hh>
#include <karabo/log/Tracer.hh>

#include "DatasetReader.hh"
#include "DatasetWriter.hh"


#include "ioProfiler.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            template<class T>
            class Scalar : public karabo::io::h5::Dataset {

            public:

                KARABO_CLASSINFO(Scalar,
                                 karabo::util::ToType<karabo::util::ToLiteral>::
                                 to(
                                    karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0"
                                 )


                Scalar(const karabo::util::Hash& input) : Dataset(input), m_numRecords(0) {
                    m_fileDataSpace = Dataset::dataSpace1dim(0);
                    m_memoryDataSpace1 = Dataset::dataSpace1dim(1);
                    m_datasetWriter = DatasetWriter<T>::create
                            ("DatasetWriter_" + Scalar<T>::classInfo().getClassId(),
                             karabo::util::Hash("dims", karabo::util::Dims().toVector())
                             );
                }

            public:

                virtual ~Scalar() {
                }

                void close() {
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(m_memoryDataSpace1));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(m_fileDataSpace));
                    Dataset::close();
                }

                void create(hsize_t chunkSize) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Scalar") << "Create dataset " << m_h5PathName
                            << " with chunk size = " << chunkSize;
                    try {
                        m_chunkSize = chunkSize;
                        karabo::util::Dims dims(chunkSize);
                        createDataSetProperties(dims);
                        m_dataSet = H5Dcreate(m_parentGroup, m_h5name.c_str(), ScalarTypes::getHdf5StandardType<T > (),
                                              m_fileDataSpace, H5P_DEFAULT, m_dataSetProperties, H5P_DEFAULT);
                        KARABO_CHECK_HDF5_STATUS(m_dataSet);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + m_h5PathName));
                    }

                }

                void writeNode(const karabo::util::Hash::Node& node, hsize_t len) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("FixedLengthArray") << "writing " << len << " records of " << m_key;
                    try {
                        m_datasetWriter->write(node, len, m_dataSet, m_fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                    }
                }
                
                
                
                inline void bind(karabo::util::Hash & data) {
                    if (!data.has(m_key, '/')) {
                        T & value = data.bindReference<T > (m_key, '/');
                        m_readData = &value;
                    } else {
                        T & value = data.get<T > (m_key, '/');
                        m_readData = &value;
                        ;
                    }

                }

                inline void read(hsize_t recordId) {
                    try {
                        m_fileDataSpace = Dataset::selectScalarRecord(m_fileDataSpace, recordId);
                        //std::clog << "m_key: " << m_key << std::endl;                                                
                        ScalarReader<T>::read(m_readData, m_dataSet, m_memoryDataSpace1, m_fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW;
                    }

                }

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


                T* m_readData;
                hid_t m_memoryDataSpace1;
                typename karabo::io::h5::DatasetWriter<T>::Pointer m_datasetWriter;
                hsize_t m_numRecords;



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
