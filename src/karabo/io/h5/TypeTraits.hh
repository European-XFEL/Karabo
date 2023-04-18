/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_TYPETRAITS_HH
#define KARABO_IO_TYPETRAITS_HH

#include <hdf5/hdf5.h>

#include <complex>
#include <string>

namespace karabo {
    namespace io {
        namespace h5 {

            /**
             * @class ScalarTypes
             * @brief This class converts C++ types to native HDF5 types
             */
            class ScalarTypes {
               public:
                template <class U>
                static const hid_t getHdf5NativeType() {
                    U* a;
                    return NotImplementedType(a);
                }

                template <class U>
                static const hid_t getHdf5StandardType() {
                    U* a;
                    return NotImplementedType(a);
                }
            };

            template <>
            const hid_t ScalarTypes::getHdf5NativeType<char>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<signed char>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<short>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<int>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<long long>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<unsigned char>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<unsigned short>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<unsigned int>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<unsigned long long>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<float>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<std::complex<float> >();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<double>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<std::complex<double> >();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<bool>();
            template <>
            const hid_t ScalarTypes::getHdf5NativeType<std::string>();

            template <>
            const hid_t ScalarTypes::getHdf5StandardType<char>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<signed char>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<short>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<int>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<long long>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<unsigned char>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<unsigned short>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<unsigned int>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<unsigned long long>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<float>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<std::complex<float> >();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<double>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<std::complex<double> >();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<bool>();
            template <>
            const hid_t ScalarTypes::getHdf5StandardType<std::string>();

            struct Hdf5Types {};

        } // namespace h5
    }     // namespace io
} // namespace karabo


#endif /* KARABO_IO_TYPETRAITS_HH */
