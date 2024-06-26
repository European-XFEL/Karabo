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
 * File:   ToNumpy.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 26, 2014, 11:11 AM
 */

#ifndef KARATHON_TONUMPY_HH
#define KARATHON_TONUMPY_HH

#include <numpy/ndarraytypes.h>

#include <karabo/util/ToType.hh>

namespace karathon {

    class ToNumpy {
       public:
        typedef int ReturnType;

        template <int RefType>
        static ReturnType to() {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
        }
    };

    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, BOOL, NPY_BOOL)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, CHAR, NPY_BYTE)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, INT8, NPY_BYTE)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, UINT8, NPY_UBYTE)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, INT16, NPY_SHORT)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, UINT16, NPY_USHORT)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, INT32, NPY_INT)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, UINT32, NPY_UINT)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, INT64, NPY_LONGLONG)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, UINT64, NPY_ULONGLONG)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, FLOAT, NPY_FLOAT)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, DOUBLE, NPY_DOUBLE)

} // namespace karathon
#endif
