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
#include <karabo/log/Logger.hh>

#include "Dataset.hh"
#include "TypeTraits.hh"



namespace karabo {

    namespace io {

        namespace h5 {

            template< typename T>
            class ScalarReader {

            public:

                KARABO_CLASSINFO(ScalarReader, "ScalarReader", "1.0")

                static void read(T* value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {

                    KARABO_LOG_FRAMEWORK_TRACE << "enter read T*";
                    KARABO_CHECK_HDF5_STATUS(
                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<T > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, value)
                                             );
                }

                static void read(std::vector<T>& value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {

                    KARABO_LOG_FRAMEWORK_TRACE << "enter read vector<T>";
                    KARABO_CHECK_HDF5_STATUS(
                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<T > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, &value[0])
                                             );
                }

                static T* getPointerFromVector(std::vector<T>& vec) {
                    return &vec[0];
                }

                inline static T* getPointerFromRaw(T* ptr, hsize_t len) {
                    return ptr;
                }


            };

            template<>
            class ScalarReader<std::string> {

            public:

                KARABO_CLASSINFO(ScalarReader, "ScalarReader", "1.0")


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

                static void read(std::string* value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {

                    KARABO_LOG_FRAMEWORK_TRACE << "enter read string*";
                    char* ptr[1];
                    KARABO_CHECK_HDF5_STATUS(
                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<std::string> (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr)
                                             );
                    *value = ptr[0];
                }

                static void read(boost::shared_ptr< ScalarReader<std::string>::Mapping > ptrMap, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {

                    KARABO_LOG_FRAMEWORK_TRACE << "vector size: ";

                    ptrMap->m_ch.resize(ptrMap->m_len);
                    char** chPtr = &(ptrMap->m_ch[0]);

                    KARABO_CHECK_HDF5_STATUS(
                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<std::string> (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, chPtr)
                                             );
                   
                    if (ptrMap->m_useVector) {
                        for (size_t i = 0; i < ptrMap->m_len; ++i) {
                            ptrMap->m_vec[i] = ptrMap->m_ch[i];
                        }
                    } else {
                        for (size_t i = 0; i < ptrMap->m_len; ++i) {
                            ptrMap->m_ptr[i] = ptrMap->m_ch[i];
                        }
                    }
                }

                static boost::shared_ptr<Mapping> getPointerFromVector(std::vector<std::string>& vec) {
                    return boost::shared_ptr<Mapping > (new Mapping(vec, vec.size()));
                }

                inline static boost::shared_ptr<Mapping> getPointerFromRaw(std::string* ptr, hsize_t len) {
                    std::vector<std::string> dummy(0);
                    return boost::shared_ptr<Mapping > (new Mapping(ptr, len, dummy));
                }

            };

            template<>
            class ScalarReader<bool> {

                // Two issues with bool values exist:
                // 1) vector<bool> - cannot use reference to vector elements
                // 2) bool is not supported by hdf5 -> we will use unsigned char to store the bool values
            public:

                KARABO_CLASSINFO(ScalarReader, "ScalarReader", "1.0")

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

                static void read(bool* value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {

                    KARABO_LOG_FRAMEWORK_TRACE << "enter read bool*";
                    unsigned char tmp;
                    KARABO_CHECK_HDF5_STATUS(
                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<bool > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, &tmp)
                                             );
                    *value = boost::numeric_cast<bool>(tmp);
                }

                static void read(boost::shared_ptr< ScalarReader<bool>::Mapping > ptrMap, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {

                    KARABO_LOG_FRAMEWORK_TRACE << "enter read boost::shared_ptr< ScalarReader<bool>::Mapping >";
                    ptrMap->m_uch.resize(ptrMap->m_len);
                    unsigned char* uchPtr = &(ptrMap->m_uch[0]);

                    KARABO_CHECK_HDF5_STATUS(
                                             H5Dread(dataSet, ScalarTypes::getHdf5NativeType<bool > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, uchPtr));

                    if (ptrMap->m_useVector) {
                        for (size_t i = 0; i < ptrMap->m_len; ++i) {
                            ptrMap->m_vec[i] = boost::numeric_cast<bool>(ptrMap->m_uch[i]);
                        }
                    } else {
                        for (size_t i = 0; i < ptrMap->m_len; ++i) {
                            ptrMap->m_ptr[i] = boost::numeric_cast<bool>(ptrMap->m_uch[i]);
                        }
                    }

                }

                static boost::shared_ptr<Mapping> getPointerFromVector(std::vector<bool>& vec) {
                    return boost::shared_ptr<Mapping > (new Mapping(vec, vec.size()));
                }

                inline static boost::shared_ptr<Mapping> getPointerFromRaw(bool* ptr, hsize_t len) {
                    std::vector<bool> dummy(0);
                    return boost::shared_ptr<Mapping > (new Mapping(ptr, len, dummy));
                }

            };



        }
    }
}

#endif	
