/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   FromType.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013 9:12 PM
 *
 */

#include "FromTypeInfo.hh"

#include "Hash.hh"
#include "Schema.hh"

namespace karabo {
    namespace data {

#define _KARABO_HELPER_MACRO1(ReferenceType, CppType)                                   \
    {std::string(typeid(CppType).name()), Types::ReferenceType}, {                      \
        std::string(typeid(std::vector<CppType>).name()), Types::VECTOR_##ReferenceType \
    }


#define _KARABO_HELPER_MACRO(ReferenceType, CppType)                                    \
    {std::string(typeid(CppType).name()), Types::ReferenceType}, {                      \
        std::string(typeid(std::vector<CppType>).name()), Types::VECTOR_##ReferenceType \
    }


        FromTypeInfo::FromTypeInfo() {
            _typeInfoMap = {_KARABO_HELPER_MACRO(BOOL, bool),
                            _KARABO_HELPER_MACRO(CHAR, char),
                            _KARABO_HELPER_MACRO(INT8, signed char),
                            _KARABO_HELPER_MACRO(UINT8, unsigned char),
                            _KARABO_HELPER_MACRO(INT16, short),
                            _KARABO_HELPER_MACRO(UINT16, unsigned short),
                            _KARABO_HELPER_MACRO(INT32, int),
                            _KARABO_HELPER_MACRO(UINT32, unsigned int),
                            _KARABO_HELPER_MACRO(INT64, long long),
                            _KARABO_HELPER_MACRO(UINT64, unsigned long long),
                            _KARABO_HELPER_MACRO(FLOAT, float),
                            _KARABO_HELPER_MACRO(DOUBLE, double),
                            _KARABO_HELPER_MACRO(COMPLEX_FLOAT, std::complex<float>),
                            _KARABO_HELPER_MACRO(COMPLEX_DOUBLE, std::complex<double>),
                            _KARABO_HELPER_MACRO(STRING, std::string)

                                  ,
                            _KARABO_HELPER_MACRO1(HASH, Hash),
                            _KARABO_HELPER_MACRO1(SCHEMA, Schema),
                            _KARABO_HELPER_MACRO1(NONE, CppNone)

                                  ,
                            {std::string(typeid(Hash::Pointer).name()), Types::HASH_POINTER},
                            {std::string(typeid(std::vector<Hash::Pointer>).name()), Types::VECTOR_HASH_POINTER},
                            {std::string(typeid(ByteArray).name()), Types::BYTE_ARRAY}

            };
#undef _KARABO_HELPER_MACRO
#undef _KARABO_HELPER_MACRO1
        }
    } // namespace data
} // namespace karabo
