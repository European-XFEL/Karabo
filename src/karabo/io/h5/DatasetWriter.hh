/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_DATASETWRITER_HH
#define	KARABO_IO_H5_DATASETWRITER_HH

#include <string>

#include "Dataset.hh"
#include "TypeTraits.hh"
#include "ErrorHandler.hh"
#include <karabo/util/Configurator.hh>
#include <karabo/log/Logger.hh>
#include <karabo/util/VectorElement.hh>

namespace karabo {

    namespace io {

        namespace h5 {

            template <typename T>
            class DatasetWriter {

            public:
                KARABO_CLASSINFO(DatasetWriter, "DatasetWriter" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0");

                KARABO_CONFIGURATION_BASE_CLASS


                static void expectedParameters(karabo::util::Schema& expected) {

                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                            .key("dims")
                            .displayedName("Dimensions")
                            .description("Array dimensions.")
                            .assignmentMandatory()
                            .init()
                            .commit();
                }

                DatasetWriter(const karabo::util::Hash& input) {
                    m_dims = input.get<std::vector<unsigned long long> >("dims");
                    m_memoryDataSpace = Dataset::dataSpace(m_dims);

                    std::vector<unsigned long long> bufferVector(m_dims.rank() + 1, 0);
                    bufferVector[0] = 0;
                    for (size_t i = 0; i < m_dims.rank(); ++i) {
                        bufferVector[i + 1] = m_dims.extentIn(i);
                    }
                    m_dimsBuffer.fromVector(bufferVector);

                }


                //                virtual void write(const T& value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) = 0;
                virtual void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) = 0;

                virtual void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                }

            protected:
                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsBuffer;
                hid_t m_memoryDataSpace;


            };

            /**
             * DatasetWriter is needed to support bool type. HDF5 does not support bool and we need to specialize
             * this class. bool values are stored as unsigned chars (1byte)
             */
            template< typename T >
            class DatasetVectorWriter : public DatasetWriter<T> {

            public:

                DatasetVectorWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {
                }

                KARABO_CLASSINFO(DatasetVectorWriter, "DatasetWriter_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (std::vector<T>))), "1.0")



                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entered (vector)";
                    const std::vector<T>& vec = node.getValue < std::vector<T> >();
                    const T* ptr = &vec[0];
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    
                    const std::vector<T>& vec = node.getValue< std::vector<T> >();
                    const T* ptr = &vec[0];
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();

                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);
                    
                    herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, ptr);
                    
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));

                }




            };

            template< typename T >
            class DatasetPointerWriter : public DatasetWriter<T> {

            public:

                KARABO_CLASSINFO(DatasetPointerWriter, "DatasetWriter_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T*))), "1.0")

                DatasetPointerWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entered (pointer)";
                    const T* ptr = node.getValue<T*>();
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

            };

            template<>
            class DatasetVectorWriter<bool> : public DatasetWriter<bool> {

            public:

                KARABO_CLASSINFO(DatasetVectorWriter, "DatasetWriter_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (std::vector<bool>))), "1.0")


                //                static void expectedParameters(karabo::util::Schema& expected) {
                //
                //                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                //                            .key("dims")
                //                            .displayedName("Dimensions")
                //                            .description("Array dimensions.")
                //                            .assignmentMandatory()
                //                            .init()
                //                            .commit();
                //                }

                DatasetVectorWriter(const karabo::util::Hash& input) : DatasetWriter<bool>(input) {
                    //                    m_len = input.get<std::vector<unsigned long long> >("dims").size();
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entered (vector)";
                    const std::vector<bool>& vec = node.getValue < std::vector<bool> > ();
                    hsize_t len = vec.size();
                    std::vector<unsigned char> converted(len, 0);
                    for (size_t i = 0; i < len; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(vec[i]);
                        // std::clog << converted[i];
                    }
                    //std::clog << std::endl;
                    const unsigned char* ptr = &converted[0];
                    hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

            };

            template<>
            class DatasetPointerWriter<bool> : public DatasetWriter<bool> {

            public:

                KARABO_CLASSINFO(DatasetPointerWriter, "DatasetWriter_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (bool*))), "1.0")


                //                static void expectedParameters(karabo::util::Schema& expected) {
                //
                //                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                //                            .key("dims")
                //                            .displayedName("Dimensions")
                //                            .description("Array dimensions.")
                //                            .assignmentMandatory()
                //                            .init()
                //                            .commit();
                //                }

                DatasetPointerWriter(const karabo::util::Hash& input) : DatasetWriter<bool>(input) {
                    //                    m_len = input.get<std::vector<unsigned long long> >("dims").size();
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entered (ptr)";

                    hsize_t len = m_dims.size();
                    const bool* ptr = node.getValue<bool*>();
                    std::vector<unsigned char> converted(len, 0);
                    for (size_t i = 0; i < len; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                    }
                    hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));

                }



            };

            template< typename T >
            class DatasetScalarWriter : public DatasetWriter<T> {

            public:

                KARABO_CLASSINFO(DatasetScalarWriter, "DatasetWriter_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0")

                DatasetScalarWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entered (scalar)";
                    const T& value = node.getValue<T>();
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &value);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entered (scalar, buffer)";
                    const T* ptr = node.getValue<T*>();
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    hid_t memoryDataSpace = Dataset::dataSpace1dim(len);
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }


            };

            template<>
            class DatasetScalarWriter<bool> : public DatasetWriter<bool> {

            public:

                KARABO_CLASSINFO(DatasetScalarWriter, "DatasetWriter_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (bool))), "1.0")

                DatasetScalarWriter(const karabo::util::Hash& input) : DatasetWriter<bool>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entered (bool)";
                    const bool& value = node.getValue<bool>();
                    unsigned char converted = boost::numeric_cast<unsigned char>(value);
                    hid_t tid = ScalarTypes::getHdf5NativeType<bool> ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entered (bool)";

                    const bool* ptr = node.getValue<bool*>();
                    std::vector<unsigned char> converted(len, 0);
                    for (size_t i = 0; i < len; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                    }
                    hid_t tid = ScalarTypes::getHdf5NativeType<bool> ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

            };


        }
    }
}

#endif	
