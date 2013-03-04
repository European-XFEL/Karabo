/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "TypeTraits.hh"
#include <karabo/util/Exception.hh>
#include <string>
#include <iostream>

using namespace std;

namespace karabo {
    namespace io {
        namespace h5 {


            // native hdf5 types used in memory

            template <> const hid_t ScalarTypes::getHdf5NativeType(const char&) {
                return H5Tcopy(H5T_NATIVE_CHAR);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const signed char&) {
                return H5Tcopy(H5T_NATIVE_CHAR);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const short&) {
                return H5Tcopy(H5T_NATIVE_SHORT);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const int&) {
                return H5Tcopy(H5T_NATIVE_INT);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const long long&) {
                return H5Tcopy(H5T_NATIVE_LLONG);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const unsigned char&) {
                return H5Tcopy(H5T_NATIVE_UCHAR);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const unsigned short&) {
                return H5Tcopy(H5T_NATIVE_USHORT);

            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const unsigned int&) {
                return H5Tcopy(H5T_NATIVE_INT);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const unsigned long long&) {
                return H5Tcopy(H5T_NATIVE_ULLONG);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const bool&) {
                return H5Tcopy(H5T_NATIVE_UCHAR);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const float&) {
                return H5Tcopy(H5T_NATIVE_FLOAT);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const double&) {
                return H5Tcopy(H5T_NATIVE_DOUBLE);
            }

            template <> const hid_t ScalarTypes::getHdf5NativeType(const std::string&) {
                hid_t tid = H5Tcopy(H5T_C_S1);
                herr_t status = H5Tset_size(tid, H5T_VARIABLE);
                if (status < 0) {
                    throw KARABO_HDF_IO_EXCEPTION("Could not create variable string data type");
                }
                return tid;
            }


            // standard hdf5 types used in file

            template <> const hid_t ScalarTypes::getHdf5StandardType(const char&) {
                return H5Tcopy(H5T_STD_I8LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const signed char&) {
                return H5Tcopy(H5T_STD_I8LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const short&) {
                return H5Tcopy(H5T_STD_I16LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const int&) {
                return H5Tcopy(H5T_STD_I32LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const long long&) {
                return H5Tcopy(H5T_STD_I64LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const unsigned char&) {
                return H5Tcopy(H5T_STD_U8LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const unsigned short&) {
                return H5Tcopy(H5T_STD_U16LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const unsigned int&) {
                return H5Tcopy(H5T_STD_U32LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const unsigned long long&) {
                return H5Tcopy(H5T_STD_U64LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const bool&) {
                return H5Tcopy(H5T_STD_U8LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const float&) {
                return H5Tcopy(H5T_IEEE_F32LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const double&) {
                return H5Tcopy(H5T_IEEE_F64LE);
            }

            template <> const hid_t ScalarTypes::getHdf5StandardType(const std::string&) {
                hid_t tid = H5Tcopy(H5T_C_S1);
                herr_t status = H5Tset_size(tid, H5T_VARIABLE);
                if (status < 0) {
                    throw KARABO_HDF_IO_EXCEPTION("Could not create variable string data type");
                }
                return tid;
            }

        }
    }
}
