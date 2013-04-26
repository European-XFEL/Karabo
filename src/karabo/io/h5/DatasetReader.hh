/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_DATASETREADER_HH
#define	KARABO_IO_H5_DATASETREADER_HH


#include <string>
#include <boost/shared_array.hpp>

#include <karabo/util/Configurator.hh>
#include <karabo/util/VectorElement.hh>

#include <karabo/log/Logger.hh>

#include "Dataset.hh"
#include "TypeTraits.hh"



namespace karabo {

    namespace io {

        namespace h5 {

            template< typename T>
            class DatasetReader {

            public:

                KARABO_CLASSINFO(DatasetReader, "DatasetReader", "1.0")
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

                DatasetReader(const karabo::util::Hash& input) {

                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));

                    m_memoryDataSpace = Dataset::dataSpace(m_dims);
                    #ifdef KARABO_ENABLE_LOG_TARCE
                    Dataset::getDataSpaceInfo(this->m_memoryDataSpace, "DatasetWriter:constr m_memoryDataSpace");
                    #endif

                    std::vector<unsigned long long> bufferVector(m_dims.rank() + 1, 0);
                    bufferVector[0] = 0;
                    for (size_t i = 0; i < m_dims.rank(); ++i) {
                        bufferVector[i + 1] = m_dims.extentIn(i);
                    }
                    m_dimsBuffer.fromVector(bufferVector);


                }

                void read(hid_t dataSet, hid_t fileDataSpace) {

                    KARABO_LOG_FRAMEWORK_TRACE << "enter read T*";
                    KARABO_CHECK_HDF5_STATUS(
                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<T > (), m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, m_readData)
                                             );
                }

                void bind(std::vector<T>& vec) {
                    m_readData = &vec[0];
                }

                void bind(T* ptr) {
                    m_readData = ptr;
                }





                // begin old
//
//                static void read(T* value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
//
//                    KARABO_LOG_FRAMEWORK_TRACE << "enter read T*";
//                    KARABO_CHECK_HDF5_STATUS(
//                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<T > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, value)
//                                             );
//                }
//
//                static void read(std::vector<T>& value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
//
//                    KARABO_LOG_FRAMEWORK_TRACE << "enter read vector<T>";
//                    KARABO_CHECK_HDF5_STATUS(
//                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<T > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, &value[0])
//                                             );
//                }
//
//                static T* getPointerFromVector(std::vector<T>& vec) {
//                    //This function is needed. See specializations below.
//                    return &vec[0];
//                }
//
//                static T* getPointerFromRaw(T* ptr, hsize_t len) {
//                    //This function is needed. See specializations below.
//                    return ptr;
//                }

                //end old


            protected:
                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsBuffer;
                hid_t m_memoryDataSpace;
                T* m_readData;
            };

            template<>
            class DatasetReader<std::string> {

            public:

                KARABO_CLASSINFO(DatasetReader, "DatasetReader", "1.0")
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

                DatasetReader(const karabo::util::Hash& input) {

                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));

                    m_memoryDataSpace = Dataset::dataSpace(m_dims);
                    #ifdef KARABO_ENABLE_LOG_TARCE
                    Dataset::getDataSpaceInfo(this->m_memoryDataSpace, "DatasetWriter:constr m_memoryDataSpace");
                    #endif

                    std::vector<unsigned long long> bufferVector(m_dims.rank() + 1, 0);
                    bufferVector[0] = 0;
                    for (size_t i = 0; i < m_dims.rank(); ++i) {
                        bufferVector[i + 1] = m_dims.extentIn(i);
                    }
                    m_dimsBuffer.fromVector(bufferVector);


                }

                void bind(std::vector<std::string>& vec) {
                    m_readData = boost::shared_ptr<Mapping > (new Mapping(vec, vec.size()));

                }

                void bind(std::string* ptr) {
                    hsize_t len = 1;
                    if (m_dims.rank() > 0) {
                        len = m_dims.size();
                    }
                    std::vector<std::string> dummy(0);
                    m_readData = boost::shared_ptr<Mapping > (new Mapping(ptr, len, dummy));
                }

                void read(hid_t dataSet, hid_t fileDataSpace) {


                    KARABO_LOG_FRAMEWORK_TRACE << "vector size: ";

                    m_readData->m_ch.resize(m_readData->m_len);
                    char** chPtr = &(m_readData->m_ch[0]);

                    KARABO_CHECK_HDF5_STATUS(
                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<std::string> (), m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, chPtr)
                                             );

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

                    Mapping(std::vector<std::string>& vec, hsize_t len) : m_vec(vec), m_ptr(0), m_len(vec.size()), m_useVector(true) {
                        m_ch = std::vector<char*>(0);
                    }

                    Mapping(std::string* ptr, hsize_t len, std::vector<std::string>& dummy) : m_vec(dummy), m_ptr(ptr), m_len(len), m_useVector(false) {
                        m_ch = std::vector<char*>(0);
                    }

                };

                //                static void read(std::string* value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                //
                //                    KARABO_LOG_FRAMEWORK_TRACE << "enter read string*";
                //                    char* ptr[1];
                //                    KARABO_CHECK_HDF5_STATUS(
                //                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<std::string> (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr)
                //                                             );
                //                    *value = ptr[0];
                //                }
                //
                //                static void read(boost::shared_ptr< DatasetReader<std::string>::Mapping > ptrMap, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                //
                //                    KARABO_LOG_FRAMEWORK_TRACE << "vector size: ";
                //
                //                    ptrMap->m_ch.resize(ptrMap->m_len);
                //                    char** chPtr = &(ptrMap->m_ch[0]);
                //
                //                    KARABO_CHECK_HDF5_STATUS(
                //                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<std::string> (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, chPtr)
                //                                             );
                //
                //                    if (ptrMap->m_useVector) {
                //                        for (size_t i = 0; i < ptrMap->m_len; ++i) {
                //                            ptrMap->m_vec[i] = ptrMap->m_ch[i];
                //                        }
                //                    } else {
                //                        for (size_t i = 0; i < ptrMap->m_len; ++i) {
                //                            ptrMap->m_ptr[i] = ptrMap->m_ch[i];
                //                        }
                //                    }
                //                }
                //
                //                static boost::shared_ptr<Mapping> getPointerFromVector(std::vector<std::string>& vec) {
                //                    return boost::shared_ptr<Mapping > (new Mapping(vec, vec.size()));
                //                }
                //
                //                inline static boost::shared_ptr<Mapping> getPointerFromRaw(std::string* ptr, hsize_t len) {
                //                    std::vector<std::string> dummy(0);
                //                    return boost::shared_ptr<Mapping > (new Mapping(ptr, len, dummy));
                //                }


                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsBuffer;
                hid_t m_memoryDataSpace;
                //T* m_readData;
                boost::shared_ptr<Mapping > m_readData;

            };

            template<>
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

                    Mapping(std::vector<bool>& vec, hsize_t len) : m_vec(vec), m_ptr(0), m_len(vec.size()), m_useVector(true) {
                        m_uch = std::vector<unsigned char>(0);
                    }

                    Mapping(bool* ptr, hsize_t len, std::vector<bool>& dummy) : m_vec(dummy), m_ptr(ptr), m_len(len), m_useVector(false) {
                        m_uch = std::vector<unsigned char>(0);
                    }

                };

            public:

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
                    #ifdef KARABO_ENABLE_LOG_TARCE
                    Dataset::getDataSpaceInfo(this->m_memoryDataSpace, "DatasetWriter:constr m_memoryDataSpace");
                    #endif

                    std::vector<unsigned long long> bufferVector(m_dims.rank() + 1, 0);
                    bufferVector[0] = 0;
                    for (size_t i = 0; i < m_dims.rank(); ++i) {
                        bufferVector[i + 1] = m_dims.extentIn(i);
                    }
                    m_dimsBuffer.fromVector(bufferVector);


                }

                void bind(std::vector<bool>& vec) {
                    m_readData = boost::shared_ptr<Mapping > (new Mapping(vec, vec.size()));

                }

                void bind(bool* ptr) {
                    hsize_t len = 1;
                    if (m_dims.rank() > 0) {
                        len = m_dims.size();
                    }
                    std::vector<bool> dummy(0);
                    m_readData = boost::shared_ptr<Mapping > (new Mapping(ptr, len, dummy));
                }

                void read(hid_t dataSet, hid_t fileDataSpace) {


                    KARABO_LOG_FRAMEWORK_TRACE << "enter read boost::shared_ptr< ScalarReader<bool>::Mapping >";
                    m_readData->m_uch.resize(m_readData->m_len);
                    unsigned char* uchPtr = &(m_readData->m_uch[0]);

                    KARABO_CHECK_HDF5_STATUS(
                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<bool > (), m_memoryDataSpace, fileDataSpace, H5P_DEFAULT, uchPtr));

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

                //                static void read(bool* value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                //
                //                    KARABO_LOG_FRAMEWORK_TRACE << "enter read bool*";
                //                    unsigned char tmp;
                //                    KARABO_CHECK_HDF5_STATUS(
                //                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<bool > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, &tmp)
                //                                             );
                //                    *value = boost::numeric_cast<bool>(tmp);
                //                }
                //
                //                static void read(boost::shared_ptr< DatasetReader<bool>::Mapping > ptrMap, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                //
                //                    KARABO_LOG_FRAMEWORK_TRACE << "enter read boost::shared_ptr< ScalarReader<bool>::Mapping >";
                //                    ptrMap->m_uch.resize(ptrMap->m_len);
                //                    unsigned char* uchPtr = &(ptrMap->m_uch[0]);
                //
                //                    KARABO_CHECK_HDF5_STATUS(
                //                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<bool > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, uchPtr));
                //
                //                    if (ptrMap->m_useVector) {
                //                        for (size_t i = 0; i < ptrMap->m_len; ++i) {
                //                            ptrMap->m_vec[i] = boost::numeric_cast<bool>(ptrMap->m_uch[i]);
                //                        }
                //                    } else {
                //                        for (size_t i = 0; i < ptrMap->m_len; ++i) {
                //                            ptrMap->m_ptr[i] = boost::numeric_cast<bool>(ptrMap->m_uch[i]);
                //                        }
                //                    }
                //
                //                }
                //
                //                static boost::shared_ptr<Mapping> getPointerFromVector(std::vector<bool>& vec) {
                //                    return boost::shared_ptr<Mapping > (new Mapping(vec, vec.size()));
                //                }
                //
                //                static boost::shared_ptr<Mapping> getPointerFromRaw(bool* ptr, hsize_t len) {
                //                    std::vector<bool> dummy(0);
                //                    return boost::shared_ptr<Mapping > (new Mapping(ptr, len, dummy));
                //                }
                //

            private:
                karabo::util::Dims m_dims;
                karabo::util::Dims m_dimsBuffer;
                hid_t m_memoryDataSpace;
                //T* m_readData;
                boost::shared_ptr<Mapping > m_readData;

            };



        }
    }
}

#endif	
