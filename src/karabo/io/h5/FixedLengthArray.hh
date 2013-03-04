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

#include <karabo/util/util.hh>
#include "ioProfiler.hh"



namespace karabo {

    namespace io {

        namespace h5 {

            template<typename T>
            class FixedLengthArray : public Dataset {
            public:

                KARABO_CLASSINFO(FixedLengthArray, "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "2.0")

                FixedLengthArray(const karabo::util::Hash& input) : Dataset(input), scalar1("scalar1") {

                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));
                }

                virtual ~FixedLengthArray() {
                    KARABO_PROFILER_REPORT_SCALAR1("write");
                    KARABO_PROFILER_REPORT_SCALAR1("dataspace");
                    KARABO_PROFILER_REPORT_SCALAR1("writeBuffer");
                    KARABO_PROFILER_REPORT_SCALAR1("dataspaceBuffer");

                }

                static void expectedParameters(karabo::util::Schema& expected) {

                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                            .key("dims")
                            .displayedName("Dimensions")
                            .description("Array dimensions.")
                            .assignmentOptional().noDefaultValue()
                            .init()
                            .commit();

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
                        m_dataSet = H5Dcreate2(m_group, m_h5name.c_str(), ScalarTypes::getHdf5StandardType<T > (), m_fileDataSpace, H5P_DEFAULT, m_dataSetProperties, H5P_DEFAULT);
                        //m_dataSet = m_group->createDataSet(m_key.c_str(), ScalarTypes::getHdf5StandardType<T > (), m_fileDataSpace, *m_dataSetProperties);
                        // need to use C interface because C++ does not allow specifying dataset access property list
                        //                        hid_t gid = m_group->getId();
                        //                        hid_t linkCreatePropListId = H5Pcreate(H5P_LINK_CREATE);
                        //                        hid_t dataSetId = H5Dcreate2(gid, m_key.c_str(), ScalarTypes::getHdf5StandardType<T > ().getId(), m_fileDataSpace.getId(), linkCreatePropListId, m_dataSetProperties->getId(), m_dataAccessPropListId);
                        //                        std::clog << "104" << std::endl;
                        //                        m_dataSet = H5::DataSet(dataSetId);
                    } catch (...) {
                        KARABO_RETHROW
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


                        hid_t mds = Dataset::dataSpace(m_dims);
                        KARABO_PROFILER_STOP_SCALAR1
                        KARABO_PROFILER_START_SCALAR1("write")

                        DatasetWriter<T>::write(vec, m_dataSet, mds, m_fileDataSpace);
                        KARABO_PROFILER_STOP_SCALAR1
                    } catch (...) {
                        //tracer << "exception caught" << std::endl;
                        KARABO_RETHROW;
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
                        std::clog << "303: vec.size = " << vec.size() << std::endl;
                        std::vector<hsize_t> vdims = m_dimsPlus1.toVector();
                        vdims[0] = len;
                        karabo::util::Dims memoryDims(vdims);
                        hid_t mds = Dataset::dataSpace(memoryDims);

                        KARABO_PROFILER_STOP_SCALAR1
                        KARABO_PROFILER_START_SCALAR1("writeBuffer")

                        DatasetWriter<T>::write(vec, m_dataSet, mds, m_fileDataSpace);
                        KARABO_PROFILER_STOP_SCALAR1
                    } catch (...) {
                        KARABO_RETHROW;
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
                //
                //                void allocate(karabo::util::Hash & data) {
                //
                //                    if (!data.has(m_key)) {
                //                        // if element is not set allocate memory
                //                        boost::shared_array<T> arr(new T[m_size]);
                //                        ArrayView<T> av(arr, m_dims);
                //                        data.set(m_key, av);
                //                    }
                //                    karabo::util::Hash::iterator it = data.find(m_key);
                //                    boost::any& any = data.getAny(it);
                //                    m_filter = FLArrayFilter<T>::createDefault(any.type().name());
                //                }
                //
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
                //                void read(karabo::util::Hash& data, hsize_t recordId) {
                //                    selectFileRecord(recordId);
                //                    karabo::util::Hash::iterator it = data.find(m_key);
                //                    boost::any& any = data.getAny(it);
                //                    //tracer << "READING type=" << any.type().name() << std::endl;
                //                    m_filter->read(*this, any, m_dims);
                //
                //                }
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


                karabo::util::Profiler scalar1;
                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsPlus1;

                hid_t m_dataAccessPropListId;

                //                boost::shared_ptr<FLArrayFilter<T> > m_filter;
                //                boost::shared_ptr<FLArrayFilterBuffer<T> > m_bufferFilter;


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
            typedef FixedLengthArray<bool> BoolArrayElement;

        }
    }
}

#endif	/* KARABO_IO_FIXEDLENGTHARRAY_HH */
