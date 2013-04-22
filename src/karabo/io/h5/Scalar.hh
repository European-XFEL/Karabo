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

                void write(const karabo::util::Hash& data, hsize_t recordId) {

                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Scalar") << "Write dataset "
                            << m_h5PathName << " from Hash element " << m_key;
                    try {
                        if (recordId % m_chunkSize == 0) {
                            m_fileDataSpace = extend(m_dataSet, m_fileDataSpace, m_chunkSize);
                        }
                        m_fileDataSpace = Dataset::selectScalarRecord(m_fileDataSpace, recordId);
                        hid_t mds = Dataset::dataSpace();
                        m_datasetWriter->write(data.getNode(m_key, '/'), m_dataSet, mds, m_fileDataSpace);
                        m_numRecords++;
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key
                                                                      + " to dataset /" + m_h5PathName));
                    }
                }

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {

                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Scalar") << "Write " << len << " records to dataset "
                            << m_h5PathName << " from Hash element " << m_key;

//                    hsize_t extLen = 0;
//                    hsize_t m = std::min(recordId, m_numRecords);
//                    hsize_t a0 = (m + m_chunkSize - 1) / m_chunkSize;
//                    hsize_t a1 = (m + m_chunkSize + len - 1) / m_chunkSize;
//                    if (a1 > a0)
//                        extLen = (a1 - a0) * m_chunkSize;

                    // ch=5 
                    // la, r, l, n
                    //  0, 1, 6, 7    a=(0+5-1)/5=0 a0=(1+5-1)/5=1, a1=(1+5+6-1)/5=2     2 10 10
                    //  7, 8, 3, 11   a=(7+5-1)/5=2 a0=(8+5-1)/5=2  a1=(8+5+3-1)/5=3     1 5  15
                    // 11, 27, 9, 36  a=(11+5-1)/5=3 a0=(27+5-1)/5=6 a1=(27+5+9-1)/5=8   5 25 40
                    // 36, 0, 2, 36    a=(36+5-1)/5=8 a0=(0+5-1)/5=0 a1=(0+5+2-1)/5=1    
                    // 36, 34, 4, 38   a=(36+5-1)/5=8 a0=(34+5-1)/5=7 a1=(34+5+4-1)/5=8
                    //
                    //                    
                    //  '   [ ]  '  |  
                    //  '      [ ' ]|                    
                    //  '        '[]|
                    //  '        ' [|  ]                    
                    //  '        '  | [   ]
                    //  '      [ '  |   ]
                    //  "     "     "     "
                    //
                    // c = 5
                    //1  r + l <= na; al = 0
                    //2  r + l <= na; al = 0
                    //3  r + l <= na; al = 0
                    //4  r + l > na;  al = r + l - na
                    //5  r + l > na;  
                    //6  r + l > na;                    
                    //
                    //(4) na=10, c=5
                    // 8 + 4 - 10 + (5 - 1) = 6; 6/5 = 1
                    // 8 + 27 - 10 + 4 = 29; 29/5 = 5   5*5+10 = 35
                    // 10+ 30 - 10 + 4 = 34; 34/5 = 6   6*5+10 = 40
                    // 
                    //(5) na=10, c=5
                    // 12 + 3 - 10 + 4 = 9; 9/5 = 1     5*1+10 = 15
                    // 12 + 4 - 10 + 4 = 10; 10/5 = 2   5*2 + 10 = 20
                    //
                    //(6) na=10, c=5
                    // 
                    //
                    //
                    // last, rec+len
                    // (rec + len - last + chunk - 1)/chunk
                    // ch=5
                    // r  le la n
                    // 0, 4,  0   4  (0+4-0 + (5-1) )/5 = 1  5  5
                    // 4, 10, 4   14 (4+10-4 + 4)/5 = 2     10 15
                    // 23, 6, 14  29 (23+6-14 + 4)/5 = 3    15 30
                    // 23, 8, 29  31 (23+8-29+ 4)/5 =  1     5 35
                    // 12, 4, 31  31 (12+4-31+ 4)/5 = -2     0 35
                    // 50, 10, 31  60 (50+10-31+4)/5 = 6      30 65

                    //  0, 8, 0   8  (0+8+0 + 4)/5= 12/5= 2

//                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Scalar")
//                            << "rec=" << recordId << " len=" << len
//                            << " a0=" << a0 << " a1=" << a1 << " extLen=" << extLen;
                    try {
//                        if (extLen > 0) {
//                            KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Scalar") << "extending: rec=" << recordId
//                                    << " len=" << len;
//                            m_fileDataSpace = extend(m_dataSet, m_fileDataSpace, extLen);
//                        }
                        extend(recordId, len);
                        selectFileRecords(recordId, len);
                        
                        //m_fileDataSpace = selectRecord(m_fileDataSpace, recordId, len);
                        m_datasetWriter->write(data.getNode(m_key, '/'), len, m_dataSet, m_fileDataSpace);
                        

                    } catch (karabo::util::Exception& e) {
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
                //
                //            protected:
                //
                //
                //            private:
                //                boost::shared_ptr<ScalarFilter<T> > m_filter;



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
