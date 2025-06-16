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
 * File:   ToLiteral.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013 9:12 PM
 *
 */

#include "ToType.hh"

#ifndef KARABO_DATA_TYPES_TOLITERAL_HH
#define KARABO_DATA_TYPES_TOLITERAL_HH

namespace karabo {

    namespace data {

        class ToLiteral {
           public:
            typedef std::string ReturnType;

            template <Types::ReferenceType RefType>
            static ReturnType to() {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
            }
        };

        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, BOOL, "BOOL")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_BOOL, "VECTOR_BOOL")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, CHAR, "CHAR")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_CHAR, "VECTOR_CHAR")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, INT8, "INT8")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_INT8, "VECTOR_INT8")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, UINT8, "UINT8")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_UINT8, "VECTOR_UINT8")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, INT16, "INT16")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_INT16, "VECTOR_INT16")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, UINT16, "UINT16")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_UINT16, "VECTOR_UINT16")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, INT32, "INT32")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_INT32, "VECTOR_INT32")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, UINT32, "UINT32")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_UINT32, "VECTOR_UINT32")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, INT64, "INT64")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_INT64, "VECTOR_INT64")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, UINT64, "UINT64")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_UINT64, "VECTOR_UINT64")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, FLOAT, "FLOAT")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_FLOAT, "VECTOR_FLOAT")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, DOUBLE, "DOUBLE")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_DOUBLE, "VECTOR_DOUBLE")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, STRING, "STRING")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_STRING, "VECTOR_STRING")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, HASH, "HASH")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_HASH, "VECTOR_HASH")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, SCHEMA, "SCHEMA")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, COMPLEX_FLOAT, "COMPLEX_FLOAT")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_COMPLEX_FLOAT, "VECTOR_COMPLEX_FLOAT")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, COMPLEX_DOUBLE, "COMPLEX_DOUBLE")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_COMPLEX_DOUBLE, "VECTOR_COMPLEX_DOUBLE")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, NONE, "NONE")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_NONE, "VECTOR_NONE")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, UNKNOWN, "UNKNOWN")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, HASH_POINTER, "HASH_POINTER")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, VECTOR_HASH_POINTER, "VECTOR_HASH_POINTER")
        KARABO_MAP_TO_REFERENCE_TYPE(ToLiteral, BYTE_ARRAY, "BYTE_ARRAY")

    } // namespace data
} // namespace karabo

#endif
