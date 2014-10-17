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



            //         DatasetWriter class is used to support bool type via specialization. 
            //         HDF5 does not support bool and we need to specialize
            //         this class. bool values are stored as unsigned chars (1byte)


            #define _LOGGER_CATEGORY "karabo.io.h5.DatasetWriter"

            template <typename T>
            class DatasetWriter {

            public:
                KARABO_CLASSINFO(DatasetWriter,
                                 "DatasetWriter" +
                                 karabo::util::ToType<karabo::util::ToLiteral>::to
                                 (karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0");

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
                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));

                    m_memoryDataSpace = Dataset::dataSpace(m_dims);
                    #ifdef KARABO_ENABLE_TRACE_LOG
                    std::ostringstream oss;
                    Dataset::getDataSpaceInfo(this->m_memoryDataSpace, oss);
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "constr. m_memoryDataSpace:" << oss.str();
                    #endif

                    std::vector<unsigned long long> bufferVector(m_dims.rank() + 1, 0);
                    bufferVector[0] = 0;
                    for (size_t i = 0; i < m_dims.rank(); ++i) {
                        bufferVector[i + 1] = m_dims.extentIn(i);
                    }
                    m_dimsBuffer.fromVector(bufferVector);

                }

                virtual ~DatasetWriter() {
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(m_memoryDataSpace))
                }
                virtual void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) = 0;

                virtual void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) = 0;

                virtual void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace){}

                virtual void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace){}

            protected:
                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsBuffer;
                hid_t m_memoryDataSpace;


            };

            template< typename T >
            class DatasetScalarWriter : public DatasetWriter<T> {

            public:

                KARABO_CLASSINFO(DatasetScalarWriter,
                                 "DatasetWriter_" +
                                 karabo::util::ToType<karabo::util::ToLiteral>::to
                                 (karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0")

                DatasetScalarWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

            private:

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(scalar)";
                    const T& value = node.template getValue<T>();
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    herr_t status = H5Dwrite(dataSet, tid, this->m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &value);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(scalar, buffer)";
                    const T* ptr = 0;


                    if (node.template is<T*>()) {
                        KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "buffer is raw pointer";
                        ptr = node.template getValue<T*>();
                    } else if (node.template is< std::vector<T> >()) {
                        KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "buffer is a vector";
                        const std::vector<T>& vec = node.template getValue<std::vector<T> >();
                        ptr = &vec[0];
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("buffer type not supported. Use vector or raw pointer");
                    }

                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();

                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);

                    herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }


            };

            template< typename T >
            class DatasetVectorWriter : public DatasetWriter<T> {

            public:

                KARABO_CLASSINFO(DatasetVectorWriter,
                                 "DatasetWriter_" +
                                 karabo::util::ToType<karabo::util::ToLiteral>::to
                                 (karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (std::vector<T>))), "1.0")


                DatasetVectorWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }


            private:

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(vector)";
                    const std::vector<T>& vec = node.template getValue < std::vector<T> >();
                    const T* ptr = &vec[0];
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    #ifdef KARABO_ENABLE_TRACE_LOG
                    std::ostringstream oss;
                    Dataset::getDataSpaceInfo(this->m_memoryDataSpace, oss);
                    KARABO_LOG_FRAMEWORK_TRACE << "memory space: " << oss.str();
                    oss.str("");
                    Dataset::getDataSpaceInfo(fileDataSpace, oss);
                    KARABO_LOG_FRAMEWORK_TRACE << "  file space: " << oss.str();
                    #endif
                    herr_t status = H5Dwrite(dataSet, tid, this->m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {

                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(vector, buffer)";
                    const std::vector<T>& vec = node.template getValue< std::vector<T> >();
                    const T* ptr = &vec[0];
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();

                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);
                    #ifdef KARABO_ENABLE_TRACE_LOG
                    std::ostringstream oss;
                    Dataset::getDataSpaceInfo(mds, oss);
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "memory space: " << oss.str();
                    oss.str("");
                    Dataset::getDataSpaceInfo(fileDataSpace, oss);
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "  file space: " << oss.str();

                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "vec[0]=" << vec[0];
                    #endif

                    herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
                }

            };

            template< typename T >
            class DatasetPointerWriter : public DatasetWriter<T> {

            public:

                KARABO_CLASSINFO(DatasetPointerWriter,
                                 "DatasetWriter_" +
                                 karabo::util::ToType<karabo::util::ToLiteral>::to
                                 (karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T*))), "1.0")

                DatasetPointerWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

            private:

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(pointer)";
                    const T* ptr = node.template getValue<T*>();
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    herr_t status = H5Dwrite(dataSet, tid, this->m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(pointer, buffer)";
                    const T* ptr = node.template getValue<T* >();

                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();

                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);
                    //                    Dataset::getDataSpaceInfo(mds, "DatasetWriter::write (pointer, buffer) mds");
                    //                    Dataset::getDataSpaceInfo(fileDataSpace, "DatasetWriter::write (pointer,buffer) fileDataSpace");

                    herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
                }



            };

          template< typename T >
            class DatasetArrayWriter : public DatasetWriter<T> {

            public:

                KARABO_CLASSINFO(DatasetArrayWriter,
                                 "DatasetWriter_" +
                                 karabo::util::ToType<karabo::util::ToLiteral>::to
                                 (karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (std::pair<const T*, size_t>))), "1.0")

                DatasetArrayWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

            private:

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(array)";
                    const T* ptr = node.template getValue<std::pair<const T*, size_t> >().first;
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    herr_t status = H5Dwrite(dataSet, tid, this->m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(array, buffer)";
                    const T* ptr = node.template getValue<std::pair<const T*,size_t> >().first;

                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    
                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);
                    
                    #ifdef KARABO_ENABLE_TRACE_LOG
                    std::ostringstream oss;
                    Dataset::getDataSpaceInfo(mds, oss);
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "memory space: " << oss.str();
                    oss.str("");
                    Dataset::getDataSpaceInfo(fileDataSpace, oss);
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "  file space: " << oss.str();

                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "ptr[2]=" << ptr[2];
                    #endif

                    herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
                }



            };







            //      bool specializations            

            template<>
            class DatasetScalarWriter<bool> : public DatasetWriter<bool> {

            public:

                KARABO_CLASSINFO(DatasetScalarWriter,
                                 "DatasetWriter_" +
                                 karabo::util::ToType<karabo::util::ToLiteral>::to
                                 (karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (bool))), "1.0")

                DatasetScalarWriter(const karabo::util::Hash& input) : DatasetWriter<bool>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

            private:

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool)";
                    const bool& value = node.template getValue<bool>();
                    unsigned char converted = boost::numeric_cast<unsigned char>(value);
                    hid_t tid = ScalarTypes::getHdf5NativeType<bool> ();
                    herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, buffer) len=" << len;

                    const bool* ptr = node.template getValue<bool*>();
                    std::vector<unsigned char> converted(len, 0);
                    std::ostringstream oss;
                    for (size_t i = 0; i < len; ++i) {
                        oss << " [" << i << "] b:" << ptr[i];
                        converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                        oss << " c:" << (int) converted[i];
                    }
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << oss.str();
                    hid_t tid = ScalarTypes::getHdf5NativeType<bool> ();
                    hid_t memoryDataSpace = Dataset::dataSpaceOneDim(len);                                        
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(memoryDataSpace));
                }

            };

            template<>
            class DatasetVectorWriter<bool> : public DatasetWriter<bool> {

            public:

                KARABO_CLASSINFO(DatasetVectorWriter,
                                 "DatasetWriter_" +
                                 karabo::util::ToType<karabo::util::ToLiteral>::to
                                 (karabo::util::FromType<karabo::util::FromTypeInfo>::from
                                  (typeid (std::vector<bool>))), "1.0"
                                 )


                DatasetVectorWriter(const karabo::util::Hash& input) : DatasetWriter<bool>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

            private:

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, vector)";
                    const std::vector<bool>& vec = node.template getValue < std::vector<bool> > ();
                    hsize_t len = vec.size();
                    std::vector<unsigned char> converted(len, 0);
                    for (size_t i = 0; i < len; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(vec[i]);
                    }
                    const unsigned char* ptr = &converted[0];
                    hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char > ();
                    herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {

                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, vector, buffer)";
                    const std::vector<bool>& vec = node.template getValue < std::vector<bool> >();
                    hsize_t lenTotal = vec.size();
                    std::vector<unsigned char> converted(lenTotal, 0);
                    for (size_t i = 0; i < lenTotal; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(vec[i]);
                    }
                    const unsigned char* ptr = &converted[0];


                    hid_t tid = ScalarTypes::getHdf5NativeType<bool> ();

                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);

                    herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, ptr);

                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));

                }


            };

            template<>
            class DatasetPointerWriter<bool> : public DatasetWriter<bool> {

            public:

                KARABO_CLASSINFO(DatasetPointerWriter,
                                 "DatasetWriter_" +
                                 karabo::util::ToType<karabo::util::ToLiteral>::to
                                 (karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (bool*))), "1.0")


                DatasetPointerWriter(const karabo::util::Hash& input) : DatasetWriter<bool>(input) {
                }

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

            private:

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, ptr)";

                    hsize_t len = m_dims.size();
                    const bool* ptr = node.template getValue<bool*>();
                    std::vector<unsigned char> converted(len, 0);
                    for (size_t i = 0; i < len; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                    }
                    hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char > ();
                    herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));

                }

                template<class HASH_ELEMENT>
                void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, ptr, buffer)";

                    hsize_t lenTotal = len * m_dims.size();
                    const bool* ptr = node.template getValue<bool*>();
                    std::vector<unsigned char> converted(lenTotal, 0);
                    for (size_t i = 0; i < lenTotal; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                    }
                    hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char > ();

                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);

                    herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, &converted[0]);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));

                }


            };


            #undef _LOGGER_CATEGORY

        }
    }
}

#endif	
