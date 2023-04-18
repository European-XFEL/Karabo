/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_DATASETREADER_HH
#define KARABO_IO_H5_DATASETREADER_HH


#include <boost/shared_array.hpp>
#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/VectorElement.hh>
#include <string>

#include "Dataset.hh"
#include "TypeTraits.hh"


namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class DatasetReader
             * @brief The DatasetReader provides reading access to HDF5 datasets.
             *        Specializations for string and Boolean data types exist.
             */
            template <typename T>
            class DatasetReader {
               public:
                KARABO_CLASSINFO(DatasetReader, "DatasetReader", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS

                virtual ~DatasetReader() {
                    KARABO_CHECK_HDF5_STATUS_NO_THROW(H5Sclose(m_memoryDataSpace));
                }

                static void expectedParameters(karabo::util::Schema& expected) {
                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                          .key("dims")
                          .displayedName("Dimensions")
                          .description("Array dimensions.")
                          .assignmentMandatory()
                          .init()
                          .commit();
                }

                DatasetReader(const karabo::util::Hash& input) {
                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));

                    m_memoryDataSpace = Dataset::dataSpace(m_dims);
#ifdef KARABO_ENABLE_TRACE_LOG
                    std::ostringstream oss;
                    Dataset::getDataSpaceInfo(this->m_memoryDataSpace, oss);
                    KARABO_LOG_FRAMEWORK_TRACE << oss.str();
#endif

                    std::vector<unsigned long long> bufferVector(m_dims.rank() + 1, 0);
                    bufferVector[0] = 0;
                    for (size_t i = 0; i < m_dims.rank(); ++i) {
                        bufferVector[i + 1] = m_dims.extentIn(i);
                    }
                    m_dimsBuffer.fromVector(bufferVector);
                }

                /**
                 * Read a dataset specfied by dataSet from a HDF5 data space
                 * @param dataSet
                 * @param fileDataSpace
                 */
                void read(hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE << "enter read T1*";
                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();
                    std::ostringstream oss;
                    Dataset::getDataSpaceInfo(fileDataSpace, oss);
                    KARABO_LOG_FRAMEWORK_TRACE << oss.str();
                    KARABO_CHECK_HDF5_STATUS(
                          H5Dread(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, m_readData));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                /**
                 * Batch read len datasets starting at dataSet from a HDF5 data space
                 * @param len
                 * @param dataSet
                 * @param fileDataSpace
                 */
                void read(hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE << "enter read T2*";

                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);
                    hid_t tid = ScalarTypes::getHdf5NativeType<T>();
                    KARABO_CHECK_HDF5_STATUS(H5Dread(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, m_readData));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid))
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(mds))
                }

                /**
                 * Bind a vector to the dataset in the HDF5, data will be read into this vector
                 * @param vec
                 */
                void bind(std::vector<T>& vec) {
                    m_readData = &vec[0];
                }

                /**
                 * Bind a pointer to the dataset in the HDF5, data will be read into this pointer
                 * @param vec
                 */
                void bind(T* ptr) {
                    m_readData = ptr;
                }


               protected:
                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsBuffer;
                hid_t m_memoryDataSpace;
                T* m_readData;
            };

            template <>
            class DatasetReader<std::string> {
               public:
                KARABO_CLASSINFO(DatasetReader, "DatasetReader", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS

                virtual ~DatasetReader() {}

                static void expectedParameters(karabo::util::Schema& expected) {
                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                          .key("dims")
                          .displayedName("Dimensions")
                          .description("Array dimensions.")
                          .assignmentMandatory()
                          .init()
                          .commit();
                }

                DatasetReader(const karabo::util::Hash& input) {
                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));

                    m_memoryDataSpace = Dataset::dataSpace(m_dims);
#ifdef KARABO_ENABLE_TRACE_LOG
                    std::ostringstream oss;
                    Dataset::getDataSpaceInfo(this->m_memoryDataSpace, oss);
                    KARABO_LOG_FRAMEWORK_TRACE << oss.str();
#endif

                    std::vector<unsigned long long> bufferVector(m_dims.rank() + 1, 0);
                    bufferVector[0] = 0;
                    for (size_t i = 0; i < m_dims.rank(); ++i) {
                        bufferVector[i + 1] = m_dims.extentIn(i);
                    }
                    m_dimsBuffer.fromVector(bufferVector);
                }

                void bind(std::vector<std::string>& vec) {
                    KARABO_LOG_FRAMEWORK_TRACE << " binding vector<string>";
                    m_readData = boost::shared_ptr<Mapping>(new Mapping(vec, vec.size()));
                }

                void bind(std::string* ptr) {
                    hsize_t len = 1;
                    if (m_dims.rank() > 0) {
                        len = m_dims.size();
                    }
                    std::vector<std::string> dummy(0);
                    m_readData = boost::shared_ptr<Mapping>(new Mapping(ptr, len, dummy));
                }

                void read(hid_t dataSet, hid_t fileDataSpace) {
                    m_readData->m_ch.resize(m_readData->m_len);
                    char** chPtr = &(m_readData->m_ch[0]);

                    hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();
                    KARABO_CHECK_HDF5_STATUS(
                          H5Dread(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, chPtr));

                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid))

                    if (m_readData->m_useVector) {
                        for (size_t i = 0; i < m_readData->m_len; ++i) {
                            m_readData->m_vec[i] = m_readData->m_ch[i];
                        }
                    } else {
                        for (size_t i = 0; i < m_readData->m_len; ++i) {
                            m_readData->m_ptr[i] = m_readData->m_ch[i];
                        }
                    }
                }

                void read(hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE << "enter read (string,buffer) len=" << len;

                    m_readData->m_ch.resize(m_readData->m_len);
                    char** chPtr = &(m_readData->m_ch[0]);


                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);

                    KARABO_LOG_FRAMEWORK_TRACE << "before H5Dread";
                    hid_t tid = ScalarTypes::getHdf5NativeType<std::string>();
                    KARABO_CHECK_HDF5_STATUS(H5Dread(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, chPtr));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid))

                    KARABO_LOG_FRAMEWORK_TRACE << "after H5Dread";
                    if (m_readData->m_useVector) {
                        for (size_t i = 0; i < m_readData->m_len; ++i) {
                            m_readData->m_vec[i] = m_readData->m_ch[i];
                        }
                    } else {
                        for (size_t i = 0; i < m_readData->m_len; ++i) {
                            m_readData->m_ptr[i] = m_readData->m_ch[i];
                        }
                    }
                }

               private:
                struct Mapping {
                    std::vector<std::string>& m_vec;
                    std::vector<char*> m_ch;
                    std::string* m_ptr;
                    hsize_t m_len;
                    bool m_useVector;

                    Mapping(std::vector<std::string>& vec, hsize_t len)
                        : m_vec(vec), m_ptr(0), m_len(vec.size()), m_useVector(true) {
                        m_ch = std::vector<char*>(0);
                    }

                    Mapping(std::string* ptr, hsize_t len, std::vector<std::string>& dummy)
                        : m_vec(dummy), m_ptr(ptr), m_len(len), m_useVector(false) {
                        m_ch = std::vector<char*>(0);
                    }
                };


                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsBuffer;
                hid_t m_memoryDataSpace;
                boost::shared_ptr<Mapping> m_readData;
            };

            template <>
            class DatasetReader<bool> {
                // Two issues with bool values exist:
                // 1) vector<bool> - cannot use reference to vector elements
                // 2) bool is not supported by hdf5 -> we will use unsigned char to store the bool values
               public:
                KARABO_CLASSINFO(DatasetReader, "DatasetReader", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS

               private:
                struct Mapping {
                    std::vector<bool>& m_vec;
                    std::vector<unsigned char> m_uch;
                    bool* m_ptr;
                    hsize_t m_len;
                    bool m_useVector;

                    Mapping(std::vector<bool>& vec, hsize_t len)
                        : m_vec(vec), m_ptr(0), m_len(vec.size()), m_useVector(true) {
                        m_uch = std::vector<unsigned char>(0);
                    }

                    Mapping(bool* ptr, hsize_t len, std::vector<bool>& dummy)
                        : m_vec(dummy), m_ptr(ptr), m_len(len), m_useVector(false) {
                        m_uch = std::vector<unsigned char>(0);
                    }
                };

               public:
                virtual ~DatasetReader() {}

                static void expectedParameters(karabo::util::Schema& expected) {
                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                          .key("dims")
                          .displayedName("Dimensions")
                          .description("Array dimensions.")
                          .assignmentMandatory()
                          .init()
                          .commit();
                }

                DatasetReader(const karabo::util::Hash& input) {
                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));

                    m_memoryDataSpace = Dataset::dataSpace(m_dims);
#ifdef KARABO_ENABLE_TRACE_LOG
                    std::ostringstream oss;
                    Dataset::getDataSpaceInfo(this->m_memoryDataSpace, oss);
                    KARABO_LOG_FRAMEWORK_TRACE << oss.str();
#endif

                    std::vector<unsigned long long> bufferVector(m_dims.rank() + 1, 0);
                    bufferVector[0] = 0;
                    for (size_t i = 0; i < m_dims.rank(); ++i) {
                        bufferVector[i + 1] = m_dims.extentIn(i);
                    }
                    m_dimsBuffer.fromVector(bufferVector);
                }

                void bind(std::vector<bool>& vec) {
                    m_readData = boost::shared_ptr<Mapping>(new Mapping(vec, vec.size()));
                }

                void bind(bool* ptr) {
                    hsize_t len = 1;
                    if (m_dims.rank() > 0) {
                        len = m_dims.size();
                    }
                    std::vector<bool> dummy(0);
                    m_readData = boost::shared_ptr<Mapping>(new Mapping(ptr, len, dummy));
                }

                void read(hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE << "enter read(bool)";
                    m_readData->m_uch.resize(m_readData->m_len);
                    unsigned char* uchPtr = &(m_readData->m_uch[0]);
                    hid_t tid = ScalarTypes::getHdf5NativeType<bool>();
                    KARABO_CHECK_HDF5_STATUS(
                          H5Dread(dataSet, tid, m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, uchPtr));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid))
                    if (m_readData->m_useVector) {
                        for (size_t i = 0; i < m_readData->m_len; ++i) {
                            m_readData->m_vec[i] = boost::numeric_cast<bool>(m_readData->m_uch[i]);
                        }
                    } else {
                        for (size_t i = 0; i < m_readData->m_len; ++i) {
                            m_readData->m_ptr[i] = boost::numeric_cast<bool>(m_readData->m_uch[i]);
                        }
                    }
                }

                void read(hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE << "enter read(bool, buffer) len=" << len;
                    m_readData->m_uch.resize(m_readData->m_len);
                    unsigned char* uchPtr = &(m_readData->m_uch[0]);


                    std::vector<hsize_t> vdims = this->m_dimsBuffer.toVector();
                    vdims[0] = len;
                    karabo::util::Dims memoryDims(vdims);
                    hid_t mds = Dataset::dataSpace(memoryDims);

                    hid_t tid = ScalarTypes::getHdf5NativeType<bool>();
                    KARABO_CHECK_HDF5_STATUS(H5Dread(dataSet, tid, mds, fileDataSpace, H5P_DEFAULT, uchPtr));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid))
                    if (m_readData->m_useVector) {
                        for (size_t i = 0; i < m_readData->m_len; ++i) {
                            m_readData->m_vec[i] = boost::numeric_cast<bool>(m_readData->m_uch[i]);
                        }
                    } else {
                        for (size_t i = 0; i < m_readData->m_len; ++i) {
                            m_readData->m_ptr[i] = boost::numeric_cast<bool>(m_readData->m_uch[i]);
                        }
                    }
                }


               private:
                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsBuffer;
                hid_t m_memoryDataSpace;
                boost::shared_ptr<Mapping> m_readData;
            };


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
