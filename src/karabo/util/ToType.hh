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
 * File:   ToType.hpp
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013
 *
 */

#ifndef KARABO_UTIL_TOTYPE_HH
#define KARABO_UTIL_TOTYPE_HH

#include <string>

#include "Exception.hh"
#include "Types.hh"

namespace karabo {

    namespace util {

        template <class Impl>
        class ToType {
#define KARABO_MAP_TO_REFERENCE_TYPE(ClassName, ReferenceType, ToType)                 \
    template <>                                                                        \
    inline ClassName::ReturnType ClassName::to<karabo::util::Types::ReferenceType>() { \
        return ToType;                                                                 \
    }


            typedef typename Impl::ReturnType ReturnType;

            ToType();
            virtual ~ToType();

           public:
            // Concept the must be implemented

            template <Types::ReferenceType RefType>
            inline static ReturnType to() {
                return Impl::template to<RefType>();
            }

#define _KARABO_HELPER_MACRO(ReferenceType) \
    case Types::ReferenceType:              \
        return Impl::template to<Types::ReferenceType>();

            static ReturnType to(const Types::ReferenceType& type) {
                switch (type) {
                    _KARABO_HELPER_MACRO(BOOL)
                    _KARABO_HELPER_MACRO(VECTOR_BOOL)
                    _KARABO_HELPER_MACRO(CHAR)
                    _KARABO_HELPER_MACRO(VECTOR_CHAR)
                    _KARABO_HELPER_MACRO(INT8)
                    _KARABO_HELPER_MACRO(VECTOR_INT8)
                    _KARABO_HELPER_MACRO(UINT8)
                    _KARABO_HELPER_MACRO(VECTOR_UINT8)
                    _KARABO_HELPER_MACRO(INT16)
                    _KARABO_HELPER_MACRO(VECTOR_INT16)
                    _KARABO_HELPER_MACRO(UINT16)
                    _KARABO_HELPER_MACRO(VECTOR_UINT16)
                    _KARABO_HELPER_MACRO(INT32)
                    _KARABO_HELPER_MACRO(VECTOR_INT32)
                    _KARABO_HELPER_MACRO(UINT32)
                    _KARABO_HELPER_MACRO(VECTOR_UINT32)
                    _KARABO_HELPER_MACRO(INT64)
                    _KARABO_HELPER_MACRO(VECTOR_INT64)
                    _KARABO_HELPER_MACRO(UINT64)
                    _KARABO_HELPER_MACRO(VECTOR_UINT64)
                    _KARABO_HELPER_MACRO(FLOAT)
                    _KARABO_HELPER_MACRO(VECTOR_FLOAT)
                    _KARABO_HELPER_MACRO(DOUBLE)
                    _KARABO_HELPER_MACRO(VECTOR_DOUBLE)
                    _KARABO_HELPER_MACRO(STRING)
                    _KARABO_HELPER_MACRO(VECTOR_STRING)
                    _KARABO_HELPER_MACRO(HASH)
                    _KARABO_HELPER_MACRO(VECTOR_HASH)
                    _KARABO_HELPER_MACRO(SCHEMA)
                    _KARABO_HELPER_MACRO(COMPLEX_FLOAT)
                    _KARABO_HELPER_MACRO(VECTOR_COMPLEX_FLOAT)
                    _KARABO_HELPER_MACRO(COMPLEX_DOUBLE)
                    _KARABO_HELPER_MACRO(VECTOR_COMPLEX_DOUBLE)
                    _KARABO_HELPER_MACRO(UNKNOWN)
                    _KARABO_HELPER_MACRO(PTR_BOOL)
                    _KARABO_HELPER_MACRO(PTR_CHAR)
                    _KARABO_HELPER_MACRO(PTR_INT8)
                    _KARABO_HELPER_MACRO(PTR_UINT8)
                    _KARABO_HELPER_MACRO(PTR_INT16)
                    _KARABO_HELPER_MACRO(PTR_UINT16)
                    _KARABO_HELPER_MACRO(PTR_INT32)
                    _KARABO_HELPER_MACRO(PTR_UINT32)
                    _KARABO_HELPER_MACRO(PTR_INT64)
                    _KARABO_HELPER_MACRO(PTR_UINT64)
                    _KARABO_HELPER_MACRO(PTR_FLOAT)
                    _KARABO_HELPER_MACRO(PTR_DOUBLE)
                    _KARABO_HELPER_MACRO(PTR_COMPLEX_FLOAT)
                    _KARABO_HELPER_MACRO(PTR_COMPLEX_DOUBLE)
                    _KARABO_HELPER_MACRO(PTR_STRING)
                    _KARABO_HELPER_MACRO(NONE)
                    _KARABO_HELPER_MACRO(VECTOR_NONE)
                    _KARABO_HELPER_MACRO(HASH_POINTER)
                    _KARABO_HELPER_MACRO(VECTOR_HASH_POINTER)
                    _KARABO_HELPER_MACRO(BYTE_ARRAY)

                    default:
                        throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Requested datatype (" + std::to_string(type) +=
                                                               ") not known");
                }
            }
#undef _KARABO_HELPER_MACRO
        };
    } // namespace util
} // namespace karabo
#endif
