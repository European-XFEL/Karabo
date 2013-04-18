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

namespace karabo {

    namespace io {

        namespace h5 {



#define _KARABO_IO_H5_TYPE(type)\
            karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (type)))

            /**
             * DatasetWriter is needed to support bool type. HDF5 does not support bool and we need to specialize
             * this class. bool values are stored as unsigned chars (1byte)
             */
            template< typename T>
            class DatasetWriter {
            public:

                static void write(const T& value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &value);
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid) );

                }

                static void write(const T* ptr, hsize_t len, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid) );


                }

                static void write(const std::vector<T>& vec, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    const T* ptr = &vec[0];
                    hid_t tid = ScalarTypes::getHdf5NativeType<T > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, ptr);
                    KARABO_CHECK_HDF5_STATUS(status)
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

            };

            template<>
            class DatasetWriter<bool> {
            public:

                static void write(const bool& value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    unsigned char converted = boost::numeric_cast<unsigned char>(value);
                    hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

                static void write(const bool* ptr, hsize_t len, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {

                    std::vector<unsigned char> converted(len, 0);
                    for (size_t i = 0; i < len; ++i) {
                        converted[i] = boost::numeric_cast<unsigned char>(ptr[i]);
                    }
                    hid_t tid = ScalarTypes::getHdf5NativeType<unsigned char > ();
                    herr_t status = H5Dwrite(dataSet, tid, memoryDataSpace, fileDataSpace, H5P_DEFAULT, &converted);
                    KARABO_CHECK_HDF5_STATUS(status);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));

                }

                static void write(const std::vector<bool>& vec, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {

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

#undef _KARABO_IO_H5_TYPE
        }
    }
}

#endif	
