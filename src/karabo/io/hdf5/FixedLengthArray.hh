/*
 * $Id: FixedLengthArray.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_FIXEDLENGTHARRAY_HH
#define	KARABO_IO_FIXEDLENGTHARRAY_HH

#include "RecordElement.hh"
#include "TypeTraits.hh"
#include "../ArrayView.hh"
#include "FLArrayFilter.hh"
#include "FLArrayFilterBuffer.hh"
#include <vector>
#include <string>
#include "../ioProfiler.hh"


/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package io
     */
    namespace io {

        namespace hdf5 {

            template<typename T>
            class FixedLengthArray : public RecordElement {
            public:

                KARABO_CLASSINFO(FixedLengthArray, ArrayTypeTraits::classId<T>(), "1.1")

                FixedLengthArray() {
                    // H5::ArrayType needs scalar space
                    m_memoryDataSpace = scalarDataSpace();
                }

                virtual ~FixedLengthArray() {
                }

                static void expectedParameters(karabo::util::Schema& expected) {

                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                            .key("dims")
                            .displayedName("Dimensions")
                            .description("Array dimensions.")
                            .assignmentOptional().noDefaultValue()
                            .commit();

                    // deprecated - use dims instead
                    karabo::util::INT32_ELEMENT(expected)
                            .key("size")
                            .displayedName("Array size")
                            .description("Size of the array")
                            .minExc(0)
                            .assignmentOptional().noDefaultValue()
                            .reconfigurable()
                            .commit();

                    karabo::util::INT32_ELEMENT(expected)
                            .key("chunkCacheSize")
                            .displayedName("chunk cache size")
                            .description("Size of the chunk cache in MB. 0 effectively means disabling the cache")
                            .minInc(0)
                            .assignmentOptional().noDefaultValue()
                            .init()
                            .advanced()
                            .commit();

                }

                void configure(const karabo::util::Hash& input) {

                    // TODO
                    // size is deprecated - will be removed soon
                    if (!input.has("size") && !input.has("dims")) {
                        throw KARABO_PARAMETER_EXCEPTION("Size of the array not specified");
                    }

                    if (input.has("size")) {
                        m_size = input.get<int> ("size");
                        m_dims.push_back(m_size);

                    } else if (input.has("dims")) {

                        m_dims = ArrayDimensions(input.get<std::vector<unsigned long long> >("dims"));
                        m_size = m_dims.getNumberOfElements();
                    }


                    // chunkCacheSize configuration
                    m_dataAccessPropListId = H5Pcreate(H5P_DATASET_ACCESS);
                    if (input.has("chunkCacheSize")) {
                        size_t cacheSize = 1024 * 1024; // in MB
                        cacheSize *= input.get<int>("chunkCacheSize");
                        tracer << "chunk cache Size: " << cacheSize << std::endl;
                        H5Pset_chunk_cache(m_dataAccessPropListId, (size_t) 521, cacheSize, 0.75);
                    }
                }

                void create(boost::shared_ptr<H5::Group> group, hsize_t chunkSize) {

                    try {
                        createDataSetProperties(chunkSize);

                        // ArrayType needs scalar DataSpace
                        m_fileDataSpace = scalarFileDataSpace(0);
                        // need to use C interface because C++ does not allow specifying dataset access property list
                        hid_t gid = group->getId();
                        hid_t linkCreatePropListId = H5Pcreate(H5P_LINK_CREATE);
                        hid_t dataSetId = H5Dcreate2(gid, m_key.c_str(), ArrayTypes::getHdf5StandardType<T > (m_dims).getId(), m_fileDataSpace.getId(), linkCreatePropListId, m_dataSetProperties->getId(), m_dataAccessPropListId);
                        m_dataSet = H5::DataSet(dataSetId);
                    } catch (...) {
                        KARABO_RETHROW
                    }
                }

                void write(const karabo::util::Hash& data, hsize_t recordId) {

                    try {
                        selectFileRecord(recordId);
                        karabo::util::Hash::const_iterator it = data.find(m_key);
                        if (it == data.end()) { // TODO: do we need here to check if iterator is ok, is this performance issue
                            throw KARABO_PARAMETER_EXCEPTION("Invalid key in the Hash");
                        }
                        const boost::any& any = data.getAny(it);
                        if (!m_filter) {
                            tracer << "creating a filter for FixedLengthArray " << any.type().name() << std::endl;
                            // this uses factory mechanism combined with rtti.
                            m_filter = FLArrayFilter<T>::createDefault(any.type().name());
                        }
                        //tracer << "about to write " << any.type().name() << std::endl;                        
                        m_filter->write(*this, any, m_dims);
                        //tracer << m_filter << std::endl;
                        //tracer << "after write " << any.type().name() << std::endl;                        
                    } catch (...) {
                        //tracer << "exception caught" << std::endl;
                        KARABO_RETHROW;
                    }
                }

                /*
                 * This function is not available via RecordElement interface
                 * To be used by filters only
                 */
                template<class U>
                inline void write(const U* ptr) const {
                    m_dataSet.write(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), m_memoryDataSpace, m_fileDataSpace);
                }

                /*
                 * This function is not available via RecordElement interface
                 * To be used by filters only
                 */

                template<class U>
                void writeBuffer(const U* ptr, size_t len) const {
                    H5::DataSpace mds = this->getBufferDataSpace(len);
                    m_dataSet.write(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), mds, m_fileDataSpace);
                }

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                    try {
                        selectFileRecord(recordId, len);
                        karabo::util::Hash::const_iterator it = data.find(m_key);
                        const boost::any& any = data.getAny(it);
                        if (!m_bufferFilter) {
                            //std::cout << "creating a filter" << std::endl;
                            m_bufferFilter = FLArrayFilterBuffer<T>::createDefault(any.type().name());
                        }
                        m_bufferFilter->write(*this, any, m_dims, len);
                    } catch (...) {
                        KARABO_RETHROW
                    }

                }

                void allocate(karabo::util::Hash & data) {

                    if (!data.has(m_key)) {
                        // if element is not set allocate memory
                        boost::shared_array<T> arr(new T[m_size]);
                        ArrayView<T> av(arr, m_dims);
                        data.set(m_key, av);
                    }
                    karabo::util::Hash::iterator it = data.find(m_key);
                    boost::any& any = data.getAny(it);
                    m_filter = FLArrayFilter<T>::createDefault(any.type().name());
                }

                void allocate(karabo::util::Hash& buffer, size_t len) {


                    boost::shared_array<T> arr(new T[m_size * len]);
                    std::vector<hsize_t> bufDims(m_dims.size() + 1, 0);
                    bufDims[0] = len;
                    for (size_t i = 0; i < m_dims.size(); ++i) {
                        bufDims[i + 1] = m_dims[i];
                    }
                    ArrayView<T> av(arr, bufDims);
                    
                    ArrayView<ArrayView<T> > bufferArrayView = av.indexable();
                    buffer.set(m_key, bufferArrayView);
                    //std::vector<ArrayView<T> >& vec = buffer.bindReference<std::vector<ArrayView<T> > >(m_key);
                    //av.getVectorOfArrayViews(vec);
                }

                void read(karabo::util::Hash& data, hsize_t recordId) {
                    selectFileRecord(recordId);
                    karabo::util::Hash::iterator it = data.find(m_key);
                    boost::any& any = data.getAny(it);
                    //tracer << "READING type=" << any.type().name() << std::endl;
                    m_filter->read(*this, any, m_dims);

                }

                // buffered reading

                void read(karabo::util::Hash& data, hsize_t recordId, hsize_t len) {

                    // "any" must contain a vector of ArrayView's of type T
                    // and it must be a continues block of memory to fit all elements of the vector (single buffer)
                    try {
                        selectFileRecord(recordId, len);
                        karabo::util::Hash::iterator it = data.find(m_key);
                        boost::any& any = data.getAny(it);
                        if (!m_bufferFilter) {
                            //std::cout << "creating read filter" << std::endl;
                            m_bufferFilter = FLArrayFilterBuffer<T>::createDefault(any.type().name());
                        }
                        m_bufferFilter->read(*this, any, m_dims, len);

                    } catch (...) {
                        KARABO_RETHROW
                    }


                }

                /*
                 * This function is not available via RecordElement interface
                 * To be used by filters only
                 */

                template<class U>
                inline void read(U* ptr) const {
                    try {
                        m_dataSet.read(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), m_memoryDataSpace, m_fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW
                    }
                }

                /*
                 * This function is not available via RecordElement interface
                 * To be used by filters only.
                 * Variant with two parameters. Used for strings (and possibly to be used for converters)
                 */
                template<class U, class V >
                inline void read(U* ptr, const V& p) const {
                    try {
                        m_dataSet.read(ptr, ArrayTypes::getHdf5NativeType<V > (m_dims), m_memoryDataSpace, m_fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW
                    }
                }

                /*
                 * This function is not available via RecordElement interface
                 * To be used by filters only
                 */

                template<class U>
                inline void readBuffer(U* ptr, size_t len) const {
                    try {
                        H5::DataSpace mds = this->getBufferDataSpace(len);
                        m_dataSet.read(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), mds, m_fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW
                    }
                }

                void readSpecificAttributes(karabo::util::Hash& attributes) {
                    attributes.setFromPath(m_key + ".rank", static_cast<int> (m_dims.size()));
                    attributes.setFromPath(m_key + ".dims", m_dims);
                    attributes.setFromPath(m_key + ".typeCategory", "FixedLengthArray");
                }



            protected:

                hsize_t m_size; // size of the array is fixed
                ArrayDimensions m_dims;

                hid_t m_dataAccessPropListId;

                boost::shared_ptr<FLArrayFilter<T> > m_filter;
                boost::shared_ptr<FLArrayFilterBuffer<T> > m_bufferFilter;


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
