/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_TYPETRAITS_HH
#define	KARABO_IO_TYPETRAITS_HH

#include <string>
#include <hdf5/hdf5.h>

namespace karabo {
    namespace io {
        namespace h5 {

            class ScalarTypes {
            public:

                template <class U>
                static const hid_t getHdf5NativeType(const U& var = U()) {
                    return NotImplementedType(var);
                }

                template <class U>
                static const hid_t getHdf5StandardType(const U& var = U()) {
                    return NotImplementedType(var);
                }
            };

            template<> const hid_t ScalarTypes::getHdf5NativeType(const char&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const signed char&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const short&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const int&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const long long&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const unsigned char&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const unsigned short&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const unsigned int&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const unsigned long long&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const float&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const double&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const bool&);
            template<> const hid_t ScalarTypes::getHdf5NativeType(const std::string&);

            template<> const hid_t ScalarTypes::getHdf5StandardType(const char&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const signed char&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const short&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const int&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const long long&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const unsigned char&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const unsigned short&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const unsigned int&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const unsigned long long&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const float&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const double&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const bool&);
            template<> const hid_t ScalarTypes::getHdf5StandardType(const std::string&);


        }
    }
}


#endif	/* KARABO_IO_TYPETRAITS_HH */

