/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "TypeTraits.hh"

#include <iostream>
#include <karabo/util/Exception.hh>
#include <string>

using namespace std;

namespace karabo {
    namespace io {
        namespace h5 {


            // native hdf5 types used in memory


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<char>() {
                return H5Tcopy(H5T_NATIVE_CHAR);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<signed char>() {
                return H5Tcopy(H5T_NATIVE_SCHAR);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<short>() {
                return H5Tcopy(H5T_NATIVE_SHORT);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<int>() {
                return H5Tcopy(H5T_NATIVE_INT);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<long long>() {
                return H5Tcopy(H5T_NATIVE_LLONG);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<unsigned char>() {
                return H5Tcopy(H5T_NATIVE_UCHAR);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<unsigned short>() {
                return H5Tcopy(H5T_NATIVE_USHORT);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<unsigned int>() {
                return H5Tcopy(H5T_NATIVE_INT);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<unsigned long long>() {
                return H5Tcopy(H5T_NATIVE_ULLONG);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<bool>() {
                return H5Tcopy(H5T_NATIVE_UCHAR);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<float>() {
                return H5Tcopy(H5T_NATIVE_FLOAT);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<std::complex<float> >() {
                return H5Tcopy(H5T_NATIVE_FLOAT);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<double>() {
                return H5Tcopy(H5T_NATIVE_DOUBLE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<std::complex<double> >() {
                return H5Tcopy(H5T_NATIVE_DOUBLE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5NativeType<std::string>() {
                hid_t tid = H5Tcopy(H5T_C_S1);
                herr_t status = H5Tset_size(tid, H5T_VARIABLE);
                if (status < 0) {
                    throw KARABO_HDF_IO_EXCEPTION("Could not create variable string data type");
                }
                return tid;
            }


            // standard hdf5 types used in file


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<char>() {
                return H5Tcopy(H5T_STD_I8LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<signed char>() {
                return H5Tcopy(H5T_STD_I8LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<short>() {
                return H5Tcopy(H5T_STD_I16LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<int>() {
                return H5Tcopy(H5T_STD_I32LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<long long>() {
                return H5Tcopy(H5T_STD_I64LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<unsigned char>() {
                return H5Tcopy(H5T_STD_U8LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<unsigned short>() {
                return H5Tcopy(H5T_STD_U16LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<unsigned int>() {
                return H5Tcopy(H5T_STD_U32LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<unsigned long long>() {
                return H5Tcopy(H5T_STD_U64LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<bool>() {
                return H5Tcopy(H5T_STD_U8LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<float>() {
                return H5Tcopy(H5T_IEEE_F32LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<std::complex<float> >() {
                return H5Tcopy(H5T_IEEE_F32LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<double>() {
                return H5Tcopy(H5T_IEEE_F64LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<std::complex<double> >() {
                return H5Tcopy(H5T_IEEE_F64LE);
            }


            template <>
            const hid_t ScalarTypes::getHdf5StandardType<std::string>() {
                hid_t tid = H5Tcopy(H5T_C_S1);
                herr_t status = H5Tset_size(tid, H5T_VARIABLE);
                if (status < 0) {
                    throw KARABO_HDF_IO_EXCEPTION("Could not create variable string data type");
                }
                return tid;
            }

        } // namespace h5
    }     // namespace io
} // namespace karabo
