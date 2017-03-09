/*
 * $Id: FixedLengthArray.cc 9370 2013-04-17 05:54:29Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "DatasetWriter.hh"
using namespace karabo::io;

namespace karabo {
    namespace io {
        namespace h5 {


            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<char>, DatasetVectorWriter<char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<signed char>, DatasetVectorWriter<signed char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<short>, DatasetVectorWriter<short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<int>, DatasetVectorWriter<int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<long long>, DatasetVectorWriter<long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned char>, DatasetVectorWriter<unsigned char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned short>, DatasetVectorWriter<unsigned short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned int>, DatasetVectorWriter<unsigned int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned long long>, DatasetVectorWriter<unsigned long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<double>, DatasetVectorWriter<double>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<float>, DatasetVectorWriter<float>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::string>, DatasetVectorWriter<std::string>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<bool>, DatasetVectorWriter<bool>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<float> >, DatasetVectorWriter< std::complex<float> >)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<double> >, DatasetVectorWriter< std::complex<double> >)

            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<char>, DatasetPointerWriter<char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<signed char>, DatasetPointerWriter<signed char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<short>, DatasetPointerWriter<short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<int>, DatasetPointerWriter<int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<long long>, DatasetPointerWriter<long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned char>, DatasetPointerWriter<unsigned char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned short>, DatasetPointerWriter<unsigned short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned int>, DatasetPointerWriter<unsigned int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned long long>, DatasetPointerWriter<unsigned long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<double>, DatasetPointerWriter<double>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<float>, DatasetPointerWriter<float>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::string>, DatasetPointerWriter<std::string>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<bool>, DatasetPointerWriter<bool>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<float> >, DatasetPointerWriter< std::complex<float> >)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<double> >, DatasetPointerWriter< std::complex<double> >)

            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<char>, DatasetNDArrayH5Writer<char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<signed char>, DatasetNDArrayH5Writer<signed char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<short>, DatasetNDArrayH5Writer<short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<int>, DatasetNDArrayH5Writer<int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<long long>, DatasetNDArrayH5Writer<long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned char>, DatasetNDArrayH5Writer<unsigned char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned short>, DatasetNDArrayH5Writer<unsigned short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned int>, DatasetNDArrayH5Writer<unsigned int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned long long>, DatasetNDArrayH5Writer<unsigned long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<double>, DatasetNDArrayH5Writer<double>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<float>, DatasetNDArrayH5Writer<float>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::string>, DatasetNDArrayH5Writer<std::string>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<bool>, DatasetNDArrayH5Writer<bool>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<float> >, DatasetNDArrayH5Writer< std::complex<float> >)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<double> >, DatasetNDArrayH5Writer< std::complex<double> >)


            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<char>, DatasetScalarWriter<char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<signed char>, DatasetScalarWriter<signed char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<short>, DatasetScalarWriter<short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<int>, DatasetScalarWriter<int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<long long>, DatasetScalarWriter<long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned char>, DatasetScalarWriter<unsigned char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned short>, DatasetScalarWriter<unsigned short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned int>, DatasetScalarWriter<unsigned int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned long long>, DatasetScalarWriter<unsigned long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<double>, DatasetScalarWriter<double>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<float>, DatasetScalarWriter<float>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::string>, DatasetScalarWriter<std::string>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<bool>, DatasetScalarWriter<bool>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::complex<float> >, DatasetScalarWriter<std::complex<float> >)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::complex<double> >, DatasetScalarWriter<std::complex<double> >)


#define _LOGGER_CATEGORY "karabo.io.h5.DatasetWriter"

            /**
            * Specializations for bool
            *
            * HDF5 does not support Boolean datatypes. For writing to HDF5 they are thus represented as chars.
            */

            /**
             * @class DatasetScalarWriter<bool>
             */
            template<>
            template<class HASH_ELEMENT>
            void DatasetScalarWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool)";
                const bool& value = node.template getValue<bool>();
                unsigned char converted = boost::numeric_cast<unsigned char>(value);
                hid_t tid = ScalarTypes::getHdf5NativeType<bool> ();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template<>
            template<class HASH_ELEMENT>
            void DatasetScalarWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
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

            /**
             * @class DatasetVectorWriter<bool>
             */
            template<>
            template<class HASH_ELEMENT>
            void DatasetVectorWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
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

            template<>
            template<class HASH_ELEMENT>
            void DatasetVectorWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {

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

            /**
             * @class DatasetPointerWriter<bool>
             */
            template<>
            template<class HASH_ELEMENT>
            void DatasetPointerWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
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

            template<>
            template<class HASH_ELEMENT>
            void DatasetPointerWriter<bool>::writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
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

            /**
             * @class DatasetNDArrayH5Writer<bool>
             */
            template<>
            template<class HASH_ELEMENT>
            void DatasetNDArrayH5Writer<bool>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, ptr)";

                hsize_t len = m_dims.size();
                const bool* ptr = node.template getValue<karabo::util::NDArray>().template getData<bool>();
                std::vector<unsigned char> converted(len, 0);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char > ();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));

            }

            template<>
            template<class HASH_ELEMENT>
            void DatasetNDArrayH5Writer<bool>::writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(bool, ptr, buffer)";

                hsize_t lenTotal = len * m_dims.size();
                const bool* ptr = node.template getValue<karabo::util::NDArray>().template getData<bool>();
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


            /**
            * Specializations for std::string
            *
            * std::string is not a char*. Care must be taken when writing strings.
            */

            /**
             * @class DatasetScalarWriter<std::string>
             */
            template<>
            template<class HASH_ELEMENT>
            void DatasetScalarWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string)";
                const std::string& value = node.template getValue<std::string>();
                const char* converted = value.c_str();
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string> ();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template<>
            template<class HASH_ELEMENT>
            void DatasetScalarWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, buffer) len=" << len;

                const std::string* ptr = node.template getValue<std::string*>();
                std::vector<const char*> converted(len, 0);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = ptr[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string> ();
                hid_t memoryDataSpace = Dataset::dataSpaceOneDim(len);
                herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(memoryDataSpace));
            }

            /**
             * @class DatasetVectorWriter<std::string>
             */
            template<>
            template<class HASH_ELEMENT>
            void DatasetVectorWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, vector)";
                const std::vector<std::string>& vec = node.template getValue < std::vector<std::string> > ();
                hsize_t len = vec.size();
                std::vector<const char*> converted(len, NULL);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = vec[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string> ();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template<>
            template<class HASH_ELEMENT>
            void DatasetVectorWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, vector, buffer)";
                const std::vector<std::string>& vec = node.template getValue < std::vector<std::string> >();
                hsize_t lenTotal = vec.size();
                std::vector<const char*> converted(lenTotal, NULL);
                for (size_t i = 0; i < lenTotal; ++i) {
                    converted[i] = vec[i].c_str();
                }

                hid_t tid = ScalarTypes::getHdf5NativeType<std::string> ();
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
            template<>
            template<class HASH_ELEMENT>
            void DatasetPointerWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, ptr)";

                hsize_t len = m_dims.size();
                const std::string* ptr = node.template getValue<std::string*>();
                std::vector<const char*> converted(len, NULL);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = ptr[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string> ();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template<>
            template<class HASH_ELEMENT>
            void DatasetPointerWriter<std::string>::writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
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
            template<>
            template<class HASH_ELEMENT>
            void DatasetNDArrayH5Writer<std::string>::writeHashElement(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, ptr)";

                hsize_t len = m_dims.size();
                const std::string* ptr = node.template getValue<karabo::util::NDArray>().template getData<std::string>();
                std::vector<const char*> converted(len, NULL);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = ptr[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string> ();
                herr_t status = H5Dwrite(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
            }

            template<>
            template<class HASH_ELEMENT>
            void DatasetNDArrayH5Writer<std::string>::writeHashElement(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                KARABO_LOG_FRAMEWORK_TRACE_C(_LOGGER_CATEGORY) << "entered write(std::string, ptr, buffer)";

                hsize_t lenTotal = len * m_dims.size();
                const std::string* ptr = node.template getValue<karabo::util::NDArray>().template getData<std::string>();
                std::vector<const char*> converted(lenTotal, NULL);
                for (size_t i = 0; i < lenTotal; ++i) {
                    converted[i] = ptr[i].c_str();
                }
                hid_t tid = ScalarTypes::getHdf5NativeType<std::string> ();

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

        }
    }
}
