/*
 * Author: CONTROLS DEV group
 *
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

#ifndef KARABIND_PYTYPES_HH
#define KARABIND_PYTYPES_HH

#include <pybind11/pybind11.h>

#include <sstream>

#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Types.hh"


namespace karabind {

    class PyTypes {
       public:
        enum ReferenceType {

            BOOL = karabo::data::Types::BOOL,               // boolean
            VECTOR_BOOL = karabo::data::Types::VECTOR_BOOL, // std::vector<std::bool>

            CHAR = karabo::data::Types::CHAR,                 // char
            VECTOR_CHAR = karabo::data::Types::VECTOR_CHAR,   // std::vector<char>
            INT8 = karabo::data::Types::INT8,                 // signed char
            VECTOR_INT8 = karabo::data::Types::VECTOR_INT8,   // std::vector<std::signed char>
            UINT8 = karabo::data::Types::UINT8,               // unsigned char
            VECTOR_UINT8 = karabo::data::Types::VECTOR_UINT8, // std::vector<std::unsigned char>

            INT16 = karabo::data::Types::INT16,                 // signed short
            VECTOR_INT16 = karabo::data::Types::VECTOR_INT16,   // std::vector<std::signed short>
            UINT16 = karabo::data::Types::UINT16,               // unsigned short
            VECTOR_UINT16 = karabo::data::Types::VECTOR_UINT16, // std::vector<std::unsigned short>

            INT32 = karabo::data::Types::INT32,                 // signed int
            VECTOR_INT32 = karabo::data::Types::VECTOR_INT32,   // std::vector<std::int>
            UINT32 = karabo::data::Types::UINT32,               // unsigned int
            VECTOR_UINT32 = karabo::data::Types::VECTOR_UINT32, // std::vector<std::unsigned int>

            INT64 = karabo::data::Types::INT64,                 // signed long long
            VECTOR_INT64 = karabo::data::Types::VECTOR_INT64,   // std::vector<std::signed long long>
            UINT64 = karabo::data::Types::UINT64,               // unsigned long long
            VECTOR_UINT64 = karabo::data::Types::VECTOR_UINT64, // std::vector<std::unsigned long long>

            FLOAT = karabo::data::Types::FLOAT,               // float
            VECTOR_FLOAT = karabo::data::Types::VECTOR_FLOAT, // std::vector<std::float>

            DOUBLE = karabo::data::Types::DOUBLE,               // double
            VECTOR_DOUBLE = karabo::data::Types::VECTOR_DOUBLE, // std::vector<std::double>

            COMPLEX_FLOAT = karabo::data::Types::COMPLEX_FLOAT,               // std::complex<float>
            VECTOR_COMPLEX_FLOAT = karabo::data::Types::VECTOR_COMPLEX_FLOAT, // std::vector<std::complex<float>

            COMPLEX_DOUBLE = karabo::data::Types::COMPLEX_DOUBLE,               // std::complex<double>
            VECTOR_COMPLEX_DOUBLE = karabo::data::Types::VECTOR_COMPLEX_DOUBLE, // std::vector<std::complex<double>

            STRING = karabo::data::Types::STRING,               // std::string
            VECTOR_STRING = karabo::data::Types::VECTOR_STRING, // std::vector<std::string>

            HASH = karabo::data::Types::HASH,               // Hash
            VECTOR_HASH = karabo::data::Types::VECTOR_HASH, // std::vector<Hash>

            // ...
            // Removed PTR-... types that are not used
            // ...

            SCHEMA = karabo::data::Types::SCHEMA, // Schema

            ANY = karabo::data::Types::ANY,   // unspecified type
            NONE = karabo::data::Types::NONE, // CppNone type used during serialization/de-serialization
            VECTOR_NONE = karabo::data::Types::VECTOR_NONE,

            BYTE_ARRAY = karabo::data::Types::BYTE_ARRAY,

            UNKNOWN = karabo::data::Types::UNKNOWN, // unknown type
            SIMPLE = karabo::data::Types::SIMPLE,
            SEQUENCE = karabo::data::Types::SEQUENCE,
            POINTER = karabo::data::Types::POINTER,

            HASH_POINTER = karabo::data::Types::HASH_POINTER,
            VECTOR_HASH_POINTER = karabo::data::Types::VECTOR_HASH_POINTER,
            LAST_CPP_TYPE = karabo::data::Types::VECTOR_HASH_POINTER + 1,
            // Removed NUMPY flags here: not used
        };

        static const ReferenceType from(const karabo::data::Types::ReferenceType& input) {
            switch (input) {
                case karabo::data::Types::BOOL:
                    return BOOL;
                case karabo::data::Types::VECTOR_BOOL:
                    return VECTOR_BOOL;
                case karabo::data::Types::CHAR:
                    return CHAR;
                case karabo::data::Types::VECTOR_CHAR:
                    return VECTOR_CHAR;
                case karabo::data::Types::INT8:
                    return INT8;
                case karabo::data::Types::VECTOR_INT8:
                    return VECTOR_INT8;
                case karabo::data::Types::UINT8:
                    return UINT8;
                case karabo::data::Types::VECTOR_UINT8:
                    return VECTOR_UINT8;
                case karabo::data::Types::INT16:
                    return INT16;
                case karabo::data::Types::VECTOR_INT16:
                    return VECTOR_INT16;
                case karabo::data::Types::UINT16:
                    return UINT16;
                case karabo::data::Types::VECTOR_UINT16:
                    return VECTOR_UINT16;
                case karabo::data::Types::INT32:
                    return INT32;
                case karabo::data::Types::VECTOR_INT32:
                    return VECTOR_INT32;
                case karabo::data::Types::UINT32:
                    return UINT32;
                case karabo::data::Types::VECTOR_UINT32:
                    return VECTOR_UINT32;
                case karabo::data::Types::INT64:
                    return INT64;
                case karabo::data::Types::VECTOR_INT64:
                    return VECTOR_INT64;
                case karabo::data::Types::UINT64:
                    return UINT64;
                case karabo::data::Types::VECTOR_UINT64:
                    return VECTOR_UINT64;
                case karabo::data::Types::FLOAT:
                    return FLOAT;
                case karabo::data::Types::VECTOR_FLOAT:
                    return VECTOR_FLOAT;
                case karabo::data::Types::DOUBLE:
                    return DOUBLE;
                case karabo::data::Types::VECTOR_DOUBLE:
                    return VECTOR_DOUBLE;
                case karabo::data::Types::COMPLEX_FLOAT:
                    return COMPLEX_FLOAT;
                case karabo::data::Types::VECTOR_COMPLEX_FLOAT:
                    return VECTOR_COMPLEX_FLOAT;
                case karabo::data::Types::COMPLEX_DOUBLE:
                    return COMPLEX_DOUBLE;
                case karabo::data::Types::VECTOR_COMPLEX_DOUBLE:
                    return VECTOR_COMPLEX_DOUBLE;
                case karabo::data::Types::STRING:
                    return STRING;
                case karabo::data::Types::VECTOR_STRING:
                    return VECTOR_STRING;
                case karabo::data::Types::HASH:
                    return HASH;
                case karabo::data::Types::VECTOR_HASH:
                    return VECTOR_HASH;
                // ...
                // Removed PTR-... types that are not used
                // ...
                case karabo::data::Types::SCHEMA:
                    return SCHEMA;
                case karabo::data::Types::ANY:
                    return ANY;
                case karabo::data::Types::NONE:
                    return NONE;
                case karabo::data::Types::VECTOR_NONE:
                    return VECTOR_NONE;
                case karabo::data::Types::BYTE_ARRAY:
                    return BYTE_ARRAY;
                case karabo::data::Types::UNKNOWN:
                    return UNKNOWN;
                case karabo::data::Types::SIMPLE:
                    return SIMPLE;
                case karabo::data::Types::SEQUENCE:
                    return SEQUENCE;
                case karabo::data::Types::POINTER:
                    return POINTER;
                case karabo::data::Types::HASH_POINTER:
                    return HASH_POINTER;
                case karabo::data::Types::VECTOR_HASH_POINTER:
                    return VECTOR_HASH_POINTER;
                default:
                    std::ostringstream oss;
                    oss << "Unknown type " << int(input) << " encountered while converting from Types to PyTypes.";
                    throw KARABO_PYTHON_EXCEPTION(oss.str());
            }
        }

        static const karabo::data::Types::ReferenceType to(const ReferenceType& input) {
            switch (input) {
                case BOOL:
                    return karabo::data::Types::BOOL;
                case VECTOR_BOOL:
                    return karabo::data::Types::VECTOR_BOOL;
                case CHAR:
                    return karabo::data::Types::CHAR;
                case VECTOR_CHAR:
                    return karabo::data::Types::VECTOR_CHAR;
                case INT8:
                    return karabo::data::Types::INT8;
                case VECTOR_INT8:
                    return karabo::data::Types::VECTOR_INT8;
                case UINT8:
                    return karabo::data::Types::UINT8;
                case VECTOR_UINT8:
                    return karabo::data::Types::VECTOR_UINT8;
                case INT16:
                    return karabo::data::Types::INT16;
                case VECTOR_INT16:
                    return karabo::data::Types::VECTOR_INT16;
                case UINT16:
                    return karabo::data::Types::UINT16;
                case VECTOR_UINT16:
                    return karabo::data::Types::VECTOR_UINT16;
                case INT32:
                    return karabo::data::Types::INT32;
                case VECTOR_INT32:
                    return karabo::data::Types::VECTOR_INT32;
                case UINT32:
                    return karabo::data::Types::UINT32;
                case VECTOR_UINT32:
                    return karabo::data::Types::VECTOR_UINT32;
                case INT64:
                    return karabo::data::Types::INT64;
                case VECTOR_INT64:
                    return karabo::data::Types::VECTOR_INT64;
                case UINT64:
                    return karabo::data::Types::UINT64;
                case VECTOR_UINT64:
                    return karabo::data::Types::VECTOR_UINT64;
                case FLOAT:
                    return karabo::data::Types::FLOAT;
                case VECTOR_FLOAT:
                    return karabo::data::Types::VECTOR_FLOAT;
                case DOUBLE:
                    return karabo::data::Types::DOUBLE;
                case VECTOR_DOUBLE:
                    return karabo::data::Types::VECTOR_DOUBLE;
                case COMPLEX_FLOAT:
                    return karabo::data::Types::COMPLEX_FLOAT;
                case VECTOR_COMPLEX_FLOAT:
                    return karabo::data::Types::VECTOR_COMPLEX_FLOAT;
                case COMPLEX_DOUBLE:
                    return karabo::data::Types::COMPLEX_DOUBLE;
                case VECTOR_COMPLEX_DOUBLE:
                    return karabo::data::Types::VECTOR_COMPLEX_DOUBLE;
                case STRING:
                    return karabo::data::Types::STRING;
                case VECTOR_STRING:
                    return karabo::data::Types::VECTOR_STRING;
                case HASH:
                    return karabo::data::Types::HASH;
                case VECTOR_HASH:
                    return karabo::data::Types::VECTOR_HASH;
                case SCHEMA:
                    return karabo::data::Types::SCHEMA;
                case ANY:
                    return karabo::data::Types::ANY;
                case NONE:
                    return karabo::data::Types::NONE;
                case VECTOR_NONE:
                    return karabo::data::Types::VECTOR_NONE;
                case BYTE_ARRAY:
                    return karabo::data::Types::BYTE_ARRAY;
                case UNKNOWN:
                    return karabo::data::Types::UNKNOWN;
                case SIMPLE:
                    return karabo::data::Types::SIMPLE;
                case SEQUENCE:
                    return karabo::data::Types::SEQUENCE;
                case POINTER:
                    return karabo::data::Types::POINTER;
                case HASH_POINTER:
                    return karabo::data::Types::HASH_POINTER;
                case VECTOR_HASH_POINTER:
                    return karabo::data::Types::VECTOR_HASH_POINTER;
                default:
                    std::ostringstream oss;
                    oss << "Unsupported type " << int(input) << " encountered while converting from PyTypes to Types.";
                    throw KARABO_PYTHON_EXCEPTION(oss.str());
            }
        }

        static const ReferenceType category(int type) {
            return from(karabo::data::Types::category(karabo::data::Types::ReferenceType(type)));
        }
    };

} // namespace karabind

#endif /* KARABIND_PYTYPES_HH */
