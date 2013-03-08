/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_FIXEDLENGTHARRAY_HH
#define	KARABO_IO_FIXEDLENGTHARRAY_HH


#include <vector>
#include <string>

#include "Dataset.hh"
#include "Scalar.hh"
#include "TypeTraits.hh"
#include "DatasetReader.hh"
#include "DatasetWriter.hh"
#include "ErrorHandler.hh"

#include <karabo/util/util.hh>
#include "ioProfiler.hh"



namespace karabo {

    namespace io {

        namespace h5 {

            template<typename T, typename U = T*>
                    class FixedLengthArray : public Dataset {
            public:

                KARABO_CLASSINFO(FixedLengthArray, "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "2.0")

                FixedLengthArray(const karabo::util::Hash& input) : Dataset(input)
#ifdef KARABO_USE_PROFILER_SCALAR1
                , scalar1("scalar1")
#endif
                {
                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));
                    m_memoryDataSpace1 = Dataset::dataSpace(m_dims);
                }

                virtual ~FixedLengthArray() {
                    //                    KARABO_PROFILER_REPORT_SCALAR1("write");
                    //                    KARABO_PROFILER_REPORT_SCALAR1("dataspace");
                    //                    KARABO_PROFILER_REPORT_SCALAR1("writeBuffer");
                    //                    KARABO_PROFILER_REPORT_SCALAR1("dataspaceBuffer");

                }

                static void expectedParameters(karabo::util::Schema& expected) {

                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                            .key("dims")
                            .displayedName("Dimensions")
                            .description("Array dimensions.")
                            .assignmentOptional().noDefaultValue()
                            .init()
                            .commit();

                    karabo::util::STRING_ELEMENT(expected)
                            .key("type")
                            .displayedName("Type")
                            .description("Data Type in Hash")
                            .assignmentOptional().noDefaultValue()
                            .reconfigurable()
                            .commit();

                }

                void close() {
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(m_memoryDataSpace1));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(m_fileDataSpace)); 
                    Dataset::close();
                }

                void create(hsize_t chunkSize) {

                    try {

                        m_chunkSize = chunkSize;

                        std::vector<unsigned long long> chunkVector(m_dims.rank() + 1, 0);
                        chunkVector[0] = chunkSize;
                        for (size_t i = 0; i < m_dims.rank(); ++i) {
                            chunkVector[i + 1] = m_dims.extentIn(i);
                        }
                        karabo::util::Dims chunk(chunkVector);
                        m_dimsPlus1 = chunk;


                        createDataSetProperties(chunk);

                        chunkVector[0] = 0;
                        karabo::util::Dims zeroDataSpace(chunkVector);

                        m_fileDataSpace = dataSpace(zeroDataSpace);
                        m_dataSet = H5Dcreate(m_parentGroup, m_h5name.c_str(), ScalarTypes::getHdf5StandardType<T > (),
                                m_fileDataSpace, H5P_DEFAULT, m_dataSetProperties, H5P_DEFAULT);
                        KARABO_CHECK_HDF5_STATUS(m_dataSet);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + m_h5PathName));
                    }
                }

                void write(const karabo::util::Hash& data, hsize_t recordId) {

                    try {
                        KARABO_PROFILER_START_SCALAR1("dataspace")

                        if (recordId % m_chunkSize == 0) {
                            m_fileDataSpace = Dataset::extend(m_dataSet, m_fileDataSpace, m_chunkSize);
                        }

                        m_fileDataSpace = selectRecord(m_fileDataSpace, recordId, 1);
                        const std::vector<T>& vec = data.get<std::vector<T> >(m_key, '/');

                        KARABO_PROFILER_STOP_SCALAR1
                        KARABO_PROFILER_START_SCALAR1("write")
                        DatasetWriter<T>::write(vec, m_dataSet, m_memoryDataSpace1, m_fileDataSpace);
                        KARABO_PROFILER_STOP_SCALAR1
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));

                    }
                }
                //
                //                /*
                //                 * This function is not available via RecordElement interface
                //                 * To be used by filters only
                //                 */
                //                template<class U>
                //                inline void write(const U* ptr) const {
                //                    m_dataSet.write(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), m_memoryDataSpace, m_fileDataSpace);
                //                }
                //
                //                /*
                //                 * This function is not available via RecordElement interface
                //                 * To be used by filters only
                //                 */
                //
                //                template<class U>
                //                void writeBuffer(const U* ptr, size_t len) const {
                //                    H5::DataSpace mds = this->getBufferDataSpace(len);
                //                    m_dataSet.write(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), mds, m_fileDataSpace);
                //                }
                //

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {

                    try {

                        //                        std::clog << "300: m_chunkSize = " << m_chunkSize << std::endl;
                        //                        std::clog << "301: recordId = " << recordId << std::endl;
                        //                        std::clog << "302: len = " << len << std::endl;
                        KARABO_PROFILER_START_SCALAR1("dataspaceBuffer")
                        if (recordId % m_chunkSize == 0) {
                            m_fileDataSpace = Dataset::extend(m_dataSet, m_fileDataSpace, len);
                        }
                        m_fileDataSpace = selectRecord(m_fileDataSpace, recordId, len);
                        const std::vector<T>& vec = data.get<std::vector<T> >(m_key, '/');
                        //                        std::clog << "303: vec.size = " << vec.size() << std::endl;
                        std::vector<hsize_t> vdims = m_dimsPlus1.toVector();
                        vdims[0] = len;
                        karabo::util::Dims memoryDims(vdims);
                        hid_t mds = Dataset::dataSpace(memoryDims);

                        KARABO_PROFILER_STOP_SCALAR1
                        KARABO_PROFILER_START_SCALAR1("writeBuffer")

                        DatasetWriter<T>::write(vec, m_dataSet, mds, m_fileDataSpace);
                        KARABO_PROFILER_STOP_SCALAR1
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                    }


                    //                    try {
                    //                        selectFileRecord(recordId, len);
                    //                        karabo::util::Hash::const_iterator it = data.find(m_key);
                    //                        const boost::any& any = data.getAny(it);
                    //                        if (!m_bufferFilter) {
                    //                            //std::cout << "creating a filter" << std::endl;
                    //                            m_bufferFilter = FLArrayFilterBuffer<T>::createDefault(any.type().name());
                    //                        }
                    //                        m_bufferFilter->write(*this, any, m_dims, len);
                    //                    } catch (...) {
                    //                        KARABO_RETHROW
                    //                    }
                    //
                }

                void bind(karabo::util::Hash & data) {
                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        std::vector<T>& vec = data.bindReference<std::vector<T> >(m_key, '/');
                        vec.resize(m_dims.size());
                        data.setAttribute(m_key, "dims", m_dims.toVector(), '/');
                        m_readData = ScalarReader<T>::getPointerFromVector(vec);
                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            std::vector<T>& vec = node->getValue< std::vector<T> >();
                            m_readData = ScalarReader<T>::getPointerFromVector(vec);
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            T* ptr = node->getValue<T* >();
                            m_readData = ScalarReader<T>::getPointerFromRaw(ptr, m_dims.size());
                            data.setAttribute(m_key, "dims", m_dims.toVector(), '/');
                        }
                    }

                }

                //                void allocate(karabo::util::Hash& buffer, size_t len) {
                //
                //
                //                    boost::shared_array<T> arr(new T[m_size * len]);
                //                    std::vector<hsize_t> bufDims(m_dims.size() + 1, 0);
                //                    bufDims[0] = len;
                //                    for (size_t i = 0; i < m_dims.size(); ++i) {
                //                        bufDims[i + 1] = m_dims[i];
                //                    }
                //                    ArrayView<T> av(arr, bufDims);
                //                    
                //                    ArrayView<ArrayView<T> > bufferArrayView = av.indexable();
                //                    buffer.set(m_key, bufferArrayView);
                //                    //std::vector<ArrayView<T> >& vec = buffer.bindReference<std::vector<ArrayView<T> > >(m_key);
                //                    //av.getVectorOfArrayViews(vec);
                //                }
                //

                void read(hsize_t recordId) {
                    try {
                        m_fileDataSpace = Dataset::selectRecord(m_fileDataSpace, recordId);
                        ScalarReader<T>::read(m_readData, m_dataSet, m_memoryDataSpace1, m_fileDataSpace);
                    } catch (karabo::util::Exception &e) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Could not read " + m_h5PathName + " dataset"));
                    }
                }



                //
                //                // buffered reading
                //
                //                void read(karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                //
                //                    // "any" must contain a vector of ArrayView's of type T
                //                    // and it must be a continues block of memory to fit all elements of the vector (single buffer)
                //                    try {
                //                        selectFileRecord(recordId, len);
                //                        karabo::util::Hash::iterator it = data.find(m_key);
                //                        boost::any& any = data.getAny(it);
                //                        if (!m_bufferFilter) {
                //                            //std::cout << "creating read filter" << std::endl;
                //                            m_bufferFilter = FLArrayFilterBuffer<T>::createDefault(any.type().name());
                //                        }
                //                        m_bufferFilter->read(*this, any, m_dims, len);
                //
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //
                //
                //                }
                //
                //                /*
                //                 * This function is not available via RecordElement interface
                //                 * To be used by filters only
                //                 */
                //
                //                template<class U>
                //                inline void read(U* ptr) const {
                //                    try {
                //                        m_dataSet.read(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), m_memoryDataSpace, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //
                //                /*
                //                 * This function is not available via RecordElement interface
                //                 * To be used by filters only.
                //                 * Variant with two parameters. Used for strings (and possibly to be used for converters)
                //                 */
                //                template<class U, class V >
                //                inline void read(U* ptr, const V& p) const {
                //                    try {
                //                        m_dataSet.read(ptr, ArrayTypes::getHdf5NativeType<V > (m_dims), m_memoryDataSpace, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //
                //                /*
                //                 * This function is not available via RecordElement interface
                //                 * To be used by filters only
                //                 */
                //
                //                template<class U>
                //                inline void readBuffer(U* ptr, size_t len) const {
                //                    try {
                //                        H5::DataSpace mds = this->getBufferDataSpace(len);
                //                        m_dataSet.read(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), mds, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //
                //                void readSpecificAttributes(karabo::util::Hash& attributes) {
                //                    attributes.setFromPath(m_key + ".rank", static_cast<int> (m_dims.size()));
                //                    attributes.setFromPath(m_key + ".dims", m_dims);
                //                    attributes.setFromPath(m_key + ".typeCategory", "FixedLengthArray");
                //                }
                //


            protected:


#ifdef KARABO_USE_PROFILER_SCALAR1
                karabo::util::Profiler scalar1;
#endif                
                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsPlus1;

                hid_t m_dataAccessPropListId;


                U m_readData; // this is a pointer to the data Hash (apart from case of bool type )
                hid_t m_memoryDataSpace1; // memory data space for one record element, defined here for performance optimization

            };

            // typedefs
            typedef FixedLengthArray<signed char> Int8ArrayElement;
            typedef FixedLengthArray<short> Int16ArrayElement;
            typedef FixedLengthArray<int> Int32ArrayElement;
            typedef FixedLengthArray<long long > Int64ArrayElement;
            typedef FixedLengthArray<unsigned char> UInt8ArrayElement;
            typedef FixedLengthArray<unsigned short> UInt16ArrayElement;
            typedef FixedLengthArray<unsigned int> UInt32ArrayElement;
            typedef FixedLengthArray<unsigned long long > UInt64ArrayElement;
            typedef FixedLengthArray<double> DoubleArrayElement;
            typedef FixedLengthArray<float> FloatArrayElement;
            typedef FixedLengthArray<std::string> StringArrayElement;
            typedef FixedLengthArray<bool, boost::shared_ptr<ScalarReader<bool>::Mapping > > BoolArrayElement;

        }
    }
}

#endif	/* KARABO_IO_FIXEDLENGTHARRAY_HH */
