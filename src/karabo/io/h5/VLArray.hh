/*
 * $Id: VLArray.hh 9577 2013-04-30 16:06:45Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_VLARRAY_HH
#define KARABO_IO_H5_VLARRAY_HH


#include <karabo/util/Configurator.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>
#include <string>
#include <vector>

#include "Dataset.hh"
#include "DatasetReader.hh"
#include "DatasetWriter.hh"
#include "ErrorHandler.hh"
#include "TypeTraits.hh"


namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class VLArray
             * @brief The VLArray class is an implementation of Dataset for variable length arrays
             */
            template <typename T>
            class VLArray : public Dataset {
               public:
                KARABO_CLASSINFO(VLArray,
                                 "VLARRAY_" +
                                       karabo::util::ToType<karabo::util::ToLiteral>::to(
                                             karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))),
                                 "1.0")


                VLArray(const karabo::util::Hash& input) : Dataset(input, this), m_readVector(0) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "classId " << Self::classInfo().getClassId();
                    karabo::util::Hash config("dims", dims().toVector());
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "config " << config;
                }

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                karabo::util::Types::ReferenceType getMemoryType() const {
                    return m_memoryType;
                }

                virtual ~VLArray() {}

                static void expectedParameters(karabo::util::Schema& expected) {}

                void close() {
                    Dataset::close();
                }

                hid_t getDatasetTypeId() {
                    return H5Tvlen_create(ScalarTypes::getHdf5StandardType<T>());
                }

                void writeNode(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray") << "writing one record of " << m_key;
                    try {
                        hvl_t v;
                        if (node.is<std::vector<T> >()) {
                            const std::vector<T>& vec = node.getValue<std::vector<T> >();
                            v.p = (void*)&vec[0];
                            v.len = vec.size();
                        } else if (node.is<T*>()) {
                            const T* value = node.getValue<T*>();
                            v.p = (void*)value;
                            v.len = node.getAttribute<int>("size");
                        }
                        hid_t tid = H5Tvlen_create(ScalarTypes::getHdf5NativeType<T>());
                        hid_t ms = Dataset::dataSpace(karabo::util::Dims());
                        herr_t status = H5Dwrite(dataSet, tid, ms, fileDataSpace, H5P_DEFAULT, &v);
                        KARABO_CHECK_HDF5_STATUS(status);
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));

                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key +
                                                                      " to VL dataset /" + m_h5PathName));
                    }
                }

                void writeNode(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray")
                          << "writing " << len << " records of " << m_key;
                    try {
                        std::vector<hvl_t> v(len);

                        if (node.is<std::vector<T> >()) {
                            const std::vector<T>& vec = node.getValue<std::vector<T> >();
                            const std::vector<unsigned long long>& l =
                                  node.getAttribute<std::vector<unsigned long long> >("size");
                            size_t idx = 0;
                            for (size_t i = 0; i < len; ++i) {
                                v[i].p = (void*)&vec[idx];
                                v[i].len = l[i];
                                idx += l[i];
                            }
                        } else if (node.is<T*>()) {
                        }
                        hid_t tid = H5Tvlen_create(ScalarTypes::getHdf5NativeType<T>());
                        hid_t ms = Dataset::dataSpace(karabo::util::Dims(len));
                        herr_t status = H5Dwrite(dataSet, tid, ms, fileDataSpace, H5P_DEFAULT, &v[0]);
                        KARABO_CHECK_HDF5_STATUS(status);
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));

                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key +
                                                                      " to dataset /" + m_h5PathName));
                    }
                }

                void bind(karabo::util::Hash& data) {
                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        std::clog << "bind VL" << std::endl;
                        std::vector<T>& vec = data.bindReference<std::vector<T> >(m_key, '/');
                        m_readVector = &vec;
                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            std::vector<T>& vec = node->getValue<std::vector<T> >();
                            m_readVector = &vec;
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            throw KARABO_HDF_IO_EXCEPTION("Pointer type not supported for variable length arrays");
                        } else {
                            throw KARABO_HDF_IO_EXCEPTION(
                                  "Type " + karabo::util::Types::to<karabo::util::ToLiteral>(node->getType()) +
                                  " not supported");
                        }
                    }
                }

                void bind(karabo::util::Hash& data, hsize_t len) {
                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        std::vector<T>& vec = data.bindReference<std::vector<T> >(m_key, '/');
                        m_readVector = &vec;
                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            std::vector<T>& vec = node->getValue<std::vector<T> >();
                            m_readVector = &vec;
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            throw KARABO_HDF_IO_EXCEPTION("Pointer type not supported for variable length arrays");
                        } else {
                            throw KARABO_HDF_IO_EXCEPTION(
                                  "Type " + karabo::util::Types::to<karabo::util::ToLiteral>(node->getType()) +
                                  " not supported");
                        }
                    }
                }

                void readRecord(const hid_t& dataSet, const hid_t& fileDataSpace) {
                    try {
                        KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray")
                              << "reading single record of dataset: " << m_key;
                        hid_t tid = H5Tvlen_create(ScalarTypes::getHdf5NativeType<T>());
                        KARABO_CHECK_HDF5_STATUS(tid);
                        hid_t ms = Dataset::dataSpace(karabo::util::Dims());
                        hsize_t size;
                        KARABO_CHECK_HDF5_STATUS(H5Dvlen_get_buf_size(dataSet, tid, fileDataSpace, &size));
                        m_readVector->resize(size / sizeof(T));
                        KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray")
                              << "Size after resize(): " << m_readVector->size();

                        hid_t xferPid = H5Pcreate(H5P_DATASET_XFER);
                        KARABO_CHECK_HDF5_STATUS(xferPid);
                        KARABO_CHECK_HDF5_STATUS(H5Pset_vlen_mem_manager(xferPid, VLArray<T>::vltypes_alloc_custom,
                                                                         m_readVector, VLArray<T>::vltypes_free_custom,
                                                                         m_readVector));

                        T* ptr = &((*m_readVector)[0]);
                        KARABO_CHECK_HDF5_STATUS(H5Dread(dataSet, tid, ms, fileDataSpace, xferPid, &ptr));

                        // KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                        //                         KARABO_CHECK_HDF5_STATUS(H5Sclose(ms));
                    } catch (...) {
                        KARABO_RETHROW;
                    }
                }

                void readRecords(hsize_t len, const hid_t& dataSet, const hid_t& fileDataSpace) {
                    try {
                        KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray")
                              << "reading " << len << " records of dataset: " << m_key;
                        hid_t tid = H5Tvlen_create(ScalarTypes::getHdf5NativeType<T>());
                        KARABO_CHECK_HDF5_STATUS(tid);
                        //                        hid_t ms = Dataset::dataSpace(karabo::util::Dims(len));

                        hssize_t numBlocks = H5Sget_select_hyper_nblocks(fileDataSpace);
                        KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray") << "num blocks" << numBlocks;
                        std::vector<hsize_t> buf(2 * numBlocks);
                        H5Sget_select_hyper_blocklist(fileDataSpace, 0, numBlocks, &buf[0]);
                        for (ssize_t j = 0; j < numBlocks; j += 2) {
                            KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray")
                                  << "size: " << buf[j] << " " << buf[j + 1];
                        }

                        KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray") << "Finished with blocks";
                        //                        std::vector<hsize_t> sizes(len);
                        //
                        //
                        //
                        //                        int ndims = m_dataSetExtent.size();
                        //                        std::vector<hsize_t> start(ndims, 0);
                        //                        start[0] = 1;
                        //
                        //                        std::vector<hsize_t> count = m_dataSetExtent;
                        //                        count[0] = 1;
                        //                        KARABO_CHECK_HDF5_STATUS(H5Sselect_hyperslab(fileDataSpace,
                        //                                                                     H5S_SELECT_SET,
                        //                                                                     &start[0],
                        //                                                                     NULL,
                        //                                                                     &count[0],
                        //                                                                     NULL));
                        //
                        //                        KARABO_CHECK_HDF5_STATUS(H5Dvlen_get_buf_size(dataSet, tid,
                        //                        fileDataSpace, &sizes[0])); for (size_t i = 0; i < len; ++i) {
                        //                            std::clog << "sizes[" << i << "] = " << sizes[i] << std::endl;
                        //                        }

                        //                        if (node.is< std::vector<T> >()) {
                        //                            //const std::vector<T>& vec = node.getValue< std::vector<T> >();
                        //                            //const std::vector<unsigned long long>& l =
                        //                            node.getAttribute<std::vector<unsigned long long> >("size");
                        //                            size_t idx = 0;
                        //                            for (size_t i = 0; i < len; ++i) {
                        //                                sizes[i]
                        //                            }
                        //                                v[i].p = (void *) &vec[idx];
                        //                                v[i].len = l[i];
                        //                                idx += l[i];
                        //                            }


                        //                        m_readVector->resize(size / sizeof (T));
                        //                        KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray") << "Size after
                        //                        resize(): " << m_readVector->size();
                        //
                        //                        hid_t xferPid = H5Pcreate(H5P_DATASET_XFER);
                        //                        KARABO_CHECK_HDF5_STATUS(xferPid);
                        //                        KARABO_CHECK_HDF5_STATUS(H5Pset_vlen_mem_manager(xferPid,
                        //                        VLArray<T>::vltypes_alloc_custom,
                        //                                                                         m_readVector,
                        //                                                                         VLArray<T>::vltypes_free_custom,
                        //                                                                         m_readVector));
                        //
                        //                        T* ptr = &((*m_readVector)[0]);
                        //                        KARABO_CHECK_HDF5_STATUS(H5Dread(dataSet, tid, ms, fileDataSpace,
                        //                        xferPid, &ptr));

                        // KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                        //                         KARABO_CHECK_HDF5_STATUS(H5Sclose(ms));
                    } catch (...) {
                        KARABO_RETHROW;
                    }
                }


               protected:
                //                typename karabo::io::h5::DatasetWriter<T>::Pointer m_datasetWriter;
                //                typename karabo::io::h5::DatasetReader<T>::Pointer m_datasetReader;
                karabo::util::Types::ReferenceType m_memoryType;

                std::vector<T>* m_readVector;

               private:
                static void* vltypes_alloc_custom(size_t size, void* info) {
                    std::clog << "ALOCATE: " << size << std::endl;
                    std::vector<T>* vec = (std::vector<T>*)info; /* Get the pointer to the vector */
                    // vec->resize(size/sizeof(int),100);
                    //                for (size_t i =0; i< 20;++i) std::clog << "vec[" << i << "] = " <<
                    //                vec->operator[](i) << std::endl; return (void*) (*vec)[0];
                    return (void*)&(vec->operator[](0));
                }

                /******************************************************************
                 **  vltypes_free_custom(): VL datatype custom memory
                 **      allocation routine.
                 ** ****************************************************************/
                static void vltypes_free_custom(void* _mem, void* info) {}
            };

            // typedefs
            typedef VLArray<signed char> Int8VLArrayElement;
            typedef VLArray<short> Int16VLArrayElement;
            typedef VLArray<int> Int32VLArrayElement;
            typedef VLArray<long long> Int64VLArrayElement;
            typedef VLArray<unsigned char> UInt8VLArrayElement;
            typedef VLArray<unsigned short> UInt16VLArrayElement;
            typedef VLArray<unsigned int> UInt32VLArrayElement;
            typedef VLArray<unsigned long long> UInt64VLArrayElement;
            typedef VLArray<double> DoubleVLArrayElement;
            typedef VLArray<float> FloatVLArrayElement;

            // NOTE: Do not typedef complex types here (std::string, std::complex)
            // As the writing code assumes that bare pointers to objects of type T
            // can be handed directly to the HDF5 C-API.

        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
