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
 * File:   ToCppString.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013
 *
 */

#include "ToType.hh"

#ifndef KARABO_DATA_TYPES_TOCPPSTRING_HH
#define KARABO_DATA_TYPES_TOCPPSTRING_HH

namespace karabo {

    namespace data {

        class ToCppString {
           public:
            typedef std::string ReturnType;

            template <int RefType>
            static ReturnType to() {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
            }
        };

        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, BOOL, "bool")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_BOOL, "vector<bool>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, CHAR, "char")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_CHAR, "vector<char>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, INT8, "signed char")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_INT8, "vector<signed char>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UINT8, "unsigned char")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_UINT8, "vector<unsigned char>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, INT16, "short")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_INT16, "vector<short>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UINT16, "unsigned short")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_UINT16, "vector<unsigned short>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, INT32, "int")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_INT32, "vector<int>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UINT32, "unsigned int")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_UINT32, "vector<unsigned int>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, INT64, "long long")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_INT64, "vector<long long>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UINT64, "unsigned long long")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_UINT64, "vector<unsigned long long>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, FLOAT, "float")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_FLOAT, "vector<float>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, DOUBLE, "double")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_DOUBLE, "vector<double>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, STRING, "string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_STRING, "vector<string>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, HASH, "Hash")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_HASH, "vector<Hash>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, SCHEMA, "Schema")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, COMPLEX_FLOAT, "complex<float>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_COMPLEX_FLOAT, "vector<complex<float> >")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, COMPLEX_DOUBLE, "complex<double>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_COMPLEX_DOUBLE, "vector<complex<double> >")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_BOOL, "bool*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_CHAR, "char*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_INT8, "signed char*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_UINT8, "unsigned char*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_INT16, "short*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_UINT16, "unsigned short*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_INT32, "int*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_UINT32, "unsigned int*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_INT64, "long long*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_UINT64, "unsigned long long*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_FLOAT, "float*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_DOUBLE, "double*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_COMPLEX_FLOAT, "complex<float>*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_COMPLEX_DOUBLE, "complex<double>*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_STRING, "string*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UNKNOWN, "unknown")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, NONE, "None")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_NONE, "vector<None>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, HASH_POINTER, "Hash::Pointer")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_HASH_POINTER, "vector<Hash::Pointer>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, BYTE_ARRAY, "std::pair<std::shared_ptr<char>,size_t>")
    } // namespace data
} // namespace karabo

#endif
