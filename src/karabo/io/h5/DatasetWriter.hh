/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_DATASETWRITER_HH
#define KARABO_IO_H5_DATASETWRITER_HH

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/NDArray.hh>
#include <karabo/util/VectorElement.hh>
#include <string>

#include "Dataset.hh"
#include "ErrorHandler.hh"
#include "TypeTraits.hh"

namespace karabo {

    namespace io {

        namespace h5 {

#define _LOGGER_CATEGORY "karabo.io.h5.DatasetWriter"

            /**
             * @class DatasetWriter
             * @brief The dataset writer is used write Karabo data structures to HDF5 files
             *
             * The dataset writer is used write Karabo data structures to HDF5 files. It
             * supports bool types via specialization. HDF5 does not support bool and we need to specialize
             * this class. Bool values are stored as unsigned chars (1byte).
             *
             * Implementations for scalar (pod and complex), vector (pod and complex), pointer (pod and complex)
             * (deprecated) and karabo::util::NDArray data types exist for this class. If data does not match one of
             * these categories it cannot be written.
             */
            template <typename T>
            class DatasetWriter {
               public:
                KARABO_CLASSINFO(DatasetWriter,
                                 "DatasetWriter" +
                                       karabo::util::ToType<karabo::util::ToLiteral>::to(
                                             karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))),
                                 "1.0");

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

                /**
                 * Create a Dataset writer for a dataset with specified input diminsions.
                 *
                 * @param input should contain a key dims of std::vector<unsigned long long> type specifying dataset
                 * dimension.
                 */
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
                    KARABO_CHECK_HDF5_STATUS_NO_THROW(H5Sclose(m_memoryDataSpace))
                }

                /**
                 * Write data contained in a hash node to a dataset in an HDf5 data space
                 * @param node to write
                 * @param dataSet identifying the data set to write to
                 * @param fileDataSpace HDF5 data space to write to
                 */
                virtual void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) = 0;

                /**
                 * Batch write data contained in a hash node to a dataset in an HDf5 data space
                 * @param node to write
                 * @param len: number of elements to write
                 * @param dataSet identifying the data set to write to
                 * @param fileDataSpace HDF5 data space to write to
                 */
                virtual void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet,
                                   hid_t fileDataSpace) = 0;

                /**
                 * Write data contained in a karabo::util::Element with string keys
                 * @param node to write
                 * @param dataSet identifying the data set to write to
                 * @param fileDataSpace HDF5 data space to write to
                 */
                virtual void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                }

                /**
                 * Batch write data contained in a karabo::util::Element with string keys
                 * @param node to write
                 * @param len: number of elements to write
                 * @param dataSet identifying the data set to write to
                 * @param fileDataSpace HDF5 data space to write to
                 */
                virtual void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet,
                                   hid_t fileDataSpace) {}

               protected:
                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsBuffer;
                hid_t m_memoryDataSpace;
            };

            /**
             * @class DatasetScalarWriter
             * @brief Implementation of DatasetWriter for writing scalar data types
             */
            template <typename T>
            class DatasetScalarWriter : public DatasetWriter<T> {
               public:
                KARABO_CLASSINFO(DatasetScalarWriter,
                                 "DatasetWriter_" +
                                       karabo::util::ToType<karabo::util::ToLiteral>::to(
                                             karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))),
                                 "1.0")

                DatasetScalarWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {}

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet,
                           hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

               private:
                template <class HASH_ELEMENT>
                inline void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(scalar)";
                    const T& value = node.template getValue<T>();
                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();
                    herr_t status = H5Dwrite(dataSet, tid, this->m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &value);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                template <class HASH_ELEMENT>
                inline void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet,
                                             hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(scalar, buffer)";
                    const T* ptr = 0;


                    if (node.template is<T*>()) {
                        KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "buffer is raw pointer";
                        ptr = node.template getValue<T*>();
                    } else if (node.template is<std::vector<T> >()) {
                        KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "buffer is a vector";
                        const std::vector<T>& vec = node.template getValue<std::vector<T> >();
                        ptr = &vec[0];
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("buffer type not supported. Use vector or raw pointer");
                    }

                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();

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

            /**
             * @class DatasetVectorWriter
             * @brief Implementation of DatasetWriter for writing vector data types
             */
            template <typename T>
            class DatasetVectorWriter : public DatasetWriter<T> {
               public:
                KARABO_CLASSINFO(DatasetVectorWriter,
                                 "DatasetWriter_" + karabo::util::ToType<karabo::util::ToLiteral>::to(
                                                          karabo::util::FromType<karabo::util::FromTypeInfo>::from(
                                                                typeid(std::vector<T>))),
                                 "1.0")


                DatasetVectorWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {}

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet,
                           hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }


               private:
                template <class HASH_ELEMENT>
                inline void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(vector)";
                    const std::vector<T>& vec = node.template getValue<std::vector<T> >();
                    const T* ptr = &vec[0];
                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();
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

                template <class HASH_ELEMENT>
                inline void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet,
                                             hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(vector, buffer)";
                    const std::vector<T>& vec = node.template getValue<std::vector<T> >();
                    const T* ptr = &vec[0];
                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();

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

            /**
             * @class DatasetPointerWriter
             * @brief Implementation of DatasetWriter for writing pointer data types
             *
             * @deprecated This interface is deprecated. Karabo::util::NDArray should be used for multi-dimensional data
             */
            template <typename T>
            class DatasetPointerWriter : public DatasetWriter<T> {
               public:
                KARABO_CLASSINFO(DatasetPointerWriter,
                                 "DatasetWriter_" +
                                       karabo::util::ToType<karabo::util::ToLiteral>::to(
                                             karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T*))),
                                 "1.0")

                DatasetPointerWriter(const karabo::util::Hash& input) : DatasetWriter<T>(input) {}

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet,
                           hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

               private:
                template <class HASH_ELEMENT>
                inline void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(pointer)";
                    const T* ptr = node.template getValue<T*>();
                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();
                    herr_t status = H5Dwrite(dataSet, tid, this->m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                template <class HASH_ELEMENT>
                inline void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet,
                                             hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(pointer, buffer)";
                    const T* ptr = node.template getValue<T*>();

                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();

                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);
                    //                    Dataset::getDataSpaceInfo(mds, "DatasetWriter::write (pointer, buffer) mds");
                    //                    Dataset::getDataSpaceInfo(fileDataSpace, "DatasetWriter::write
                    //                    (pointer,buffer) fileDataSpace");

                    herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
                }
            };

            /**
             * @class DatasetNDArrayH5Writer
             * @brief Implementation of DatasetWriter for writing karabo::util::NDArray multidimensional data
             *
             * This interface should be used instead of DatasetPointerWriter
             */
            template <typename T>
            class DatasetNDArrayH5Writer : public DatasetWriter<T> {
               public:
                KARABO_CLASSINFO(DatasetNDArrayH5Writer,
                                 "DatasetWriter_NDArrayH5" +
                                       karabo::util::ToType<karabo::util::ToLiteral>::to(
                                             karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))),
                                 "1.0")

                DatasetNDArrayH5Writer(const karabo::util::Hash& input) : DatasetWriter<T>(input) {}

                void write(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeHashElement(node, dataSet, fileDataSpace);
                }

                void write(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet,
                           hid_t fileDataSpace) {
                    writeHashElement(node, len, dataSet, fileDataSpace);
                }

               private:
                template <class HASH_ELEMENT>
                inline void writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(pointer)";

                    const T* ptr = node.template getValue<const karabo::util::NDArray>().template getData<T>();
                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();
                    herr_t status = H5Dwrite(dataSet, tid, this->m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                template <class HASH_ELEMENT>
                inline void writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet,
                                             hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(pointer, buffer)";
                    const T* ptr = node.template getValue<const karabo::util::NDArray>().template getData<T>();

                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();

                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);
                    //                    Dataset::getDataSpaceInfo(mds, "DatasetWriter::write (pointer, buffer) mds");
                    //                    Dataset::getDataSpaceInfo(fileDataSpace, "DatasetWriter::write
                    //                    (pointer,buffer) fileDataSpace");

                    herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
                }
            };

            /**
             * Specializations for bool
             *
             * HDF5 does not support Boolean datatypes. For writing to HDF5 they are thus represented as chars.
             */

            /**
             * @class DatasetScalarWriter<bool>
             */
            template <>
            template <class HASH_ELEMENT>
            inline void DatasetScalarWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet,
                                                                    hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool)";
                const bool& value = node.template getValue<bool>();
                unsigned char converted = boost::numeric_cast<unsigned char>(value);
                hid_t tid = ScalarTypes::getHdf5NativeType<bool>();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template <>
            template <class HASH_ELEMENT>
            inline void DatasetScalarWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hsize_t len,
                                                                    hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, buffer) len=" << len;

                const bool* ptr = node.template getValue<bool*>();
                std::vector<unsigned char> converted(len, 0);
                std::ostringstream oss;
                for (size_t i = 0; i < len; ++i) {
                    oss << " [" << i << "] b:" << ptr[i];
                    converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                    oss << " c:" << (int)converted[i];
                }
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << oss.str();
                hid_t tid = ScalarTypes::getHdf5NativeType<bool>();
                hid_t memoryDataSpace = Dataset::dataSpaceOneDim(len);
                herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(memoryDataSpace));
            }

            /**
             * @class DatasetVectorWriter<bool>
             */
            template <>
            template <class HASH_ELEMENT>
            inline void DatasetVectorWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet,
                                                                    hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, vector)";
                const std::vector<bool>& vec = node.template getValue<std::vector<bool> >();
                hsize_t len = vec.size();
                std::vector<unsigned char> converted(len, 0);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(vec[i]);
                }
                const unsigned char* ptr = &converted[0];
                hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char>();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template <>
            template <class HASH_ELEMENT>
            inline void DatasetVectorWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hsize_t len,
                                                                    hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, vector, buffer)";
                const std::vector<bool>& vec = node.template getValue<std::vector<bool> >();
                hsize_t lenTotal = vec.size();
                std::vector<unsigned char> converted(lenTotal, 0);
                for (size_t i = 0; i < lenTotal; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(vec[i]);
                }
                const unsigned char* ptr = &converted[0];

                hid_t tid = ScalarTypes::getHdf5NativeType<bool>();

                std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                vdims[0] = len;
                karabo::util::Dims memoryDims(vdims);
                hid_t mds = Dataset::dataSpace(memoryDims);

                herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, ptr);

                KARABO_CHECK_HDF5_STATUS(status)
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
            }

            /**
             * @class DatasetPointerWriter<bool>
             */
            template <>
            template <class HASH_ELEMENT>
            inline void DatasetPointerWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet,
                                                                     hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, ptr)";

                hsize_t len = m_dims.size();
                const bool* ptr = node.template getValue<bool*>();
                std::vector<unsigned char> converted(len, 0);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char>();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template <>
            template <class HASH_ELEMENT>
            inline void DatasetPointerWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hsize_t len,
                                                                     hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, ptr, buffer)";

                hsize_t lenTotal = len * m_dims.size();
                const bool* ptr = node.template getValue<bool*>();
                std::vector<unsigned char> converted(lenTotal, 0);
                for (size_t i = 0; i < lenTotal; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char>();

                std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                vdims[0] = len;
                karabo::util::Dims memoryDims(vdims);
                hid_t mds = Dataset::dataSpace(memoryDims);

                herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
            }

            /**
             * @class DatasetNDArrayH5Writer<bool>
             */
            template <>
            template <class HASH_ELEMENT>
            inline void DatasetNDArrayH5Writer<bool>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet,
                                                                       hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, ptr)";

                hsize_t len = m_dims.size();
                const bool* ptr = node.template getValue<karabo::util::NDArray>().template getData<bool>();
                std::vector<unsigned char> converted(len, 0);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char>();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template <>
            template <class HASH_ELEMENT>
            inline void DatasetNDArrayH5Writer<bool>::writeHashElement(const HASH_ELEMENT& node, hsize_t len,
                                                                       hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, ptr, buffer)";

                hsize_t lenTotal = len * m_dims.size();
                const bool* ptr = node.template getValue<karabo::util::NDArray>().template getData<bool>();
                std::vector<unsigned char> converted(lenTotal, 0);
                for (size_t i = 0; i < lenTotal; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char>();

                std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                vdims[0] = len;
                karabo::util::Dims memoryDims(vdims);
                hid_t mds = Dataset::dataSpace(memoryDims);

                herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
            }


            /**
             * Specializations for std::string
             *
             * std::string is not a char*. Care must be taken when writing strings.
             */

            /**
             * @class DatasetScalarWriter<std::string>
             */
            template <>
            template <class HASH_ELEMENT>
            inline void DatasetScalarWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet,
                                                                           hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string)";
                const std::string& value = node.template getValue<std::string>();
                const char* converted = value.c_str();
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template <>
            template <class HASH_ELEMENT>
            inline void DatasetScalarWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hsize_t len,
                                                                           hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, buffer) len=" << len;

                const std::string* ptr = node.template getValue<std::string*>();
                std::vector<const char*> converted(len, 0);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = ptr[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();
                hid_t memoryDataSpace = Dataset::dataSpaceOneDim(len);
                herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(memoryDataSpace));
            }

            /**
             * @class DatasetVectorWriter<std::string>
             */
            template <>
            template <class HASH_ELEMENT>
            inline void DatasetVectorWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet,
                                                                           hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, vector)";
                const std::vector<std::string>& vec = node.template getValue<std::vector<std::string> >();
                hsize_t len = vec.size();
                std::vector<const char*> converted(len, NULL);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = vec[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template <>
            template <class HASH_ELEMENT>
            inline void DatasetVectorWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hsize_t len,
                                                                           hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, vector, buffer)";
                const std::vector<std::string>& vec = node.template getValue<std::vector<std::string> >();
                hsize_t lenTotal = vec.size();
                std::vector<const char*> converted(lenTotal, NULL);
                for (size_t i = 0; i < lenTotal; ++i) {
                    converted[i] = vec[i].c_str();
                }

                hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();
                std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                vdims[0] = len;
                karabo::util::Dims memoryDims(vdims);
                hid_t mds = Dataset::dataSpace(memoryDims);
                herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status)
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
            }

            /**
             * @class DatasetPointerWriter<std::string>
             */
            template <>
            template <class HASH_ELEMENT>
            inline void DatasetPointerWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet,
                                                                            hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, ptr)";

                hsize_t len = m_dims.size();
                const std::string* ptr = node.template getValue<std::string*>();
                std::vector<const char*> converted(len, NULL);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = ptr[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template <>
            template <class HASH_ELEMENT>
            inline void DatasetPointerWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hsize_t len,
                                                                            hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, ptr, buffer)";

                hsize_t lenTotal = len * m_dims.size();
                const std::string* ptr = node.template getValue<std::string*>();
                std::vector<const char*> converted(lenTotal, NULL);
                for (size_t i = 0; i < lenTotal; ++i) {
                    converted[i] = ptr[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();

                std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                vdims[0] = len;
                karabo::util::Dims memoryDims(vdims);
                hid_t mds = Dataset::dataSpace(memoryDims);

                herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
            }

            /**
             * @class DatasetNDArrayH5Writer<std::string>
             */
            template <>
            template <class HASH_ELEMENT>
            inline void DatasetNDArrayH5Writer<std::string>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet,
                                                                              hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, ptr)";

                hsize_t len = m_dims.size();
                const std::string* ptr =
                      node.template getValue<karabo::util::NDArray>().template getData<std::string>();
                std::vector<const char*> converted(len, NULL);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = ptr[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template <>
            template <class HASH_ELEMENT>
            inline void DatasetNDArrayH5Writer<std::string>::writeHashElement(const HASH_ELEMENT& node, hsize_t len,
                                                                              hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, ptr, buffer)";

                hsize_t lenTotal = len * m_dims.size();
                const std::string* ptr =
                      node.template getValue<karabo::util::NDArray>().template getData<std::string>();
                std::vector<const char*> converted(lenTotal, NULL);
                for (size_t i = 0; i < lenTotal; ++i) {
                    converted[i] = ptr[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();

                std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                vdims[0] = len;
                karabo::util::Dims memoryDims(vdims);
                hid_t mds = Dataset::dataSpace(memoryDims);

                herr_t status = H5Dwrite(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(mds));
            }

#undef _LOGGER_CATEGORY

        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
