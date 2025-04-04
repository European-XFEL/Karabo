/*
 * File:   ToNumpy.hh
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

#ifndef KARABIND_TONUMPY_HH
#define KARABIND_TONUMPY_HH

#include <pybind11/numpy.h>

#include "karabo/data/types/ToType.hh"

namespace py = pybind11;

namespace karabind {

    class ToNumpy {
       public:
        typedef int ReturnType;

        template <int RefType>
        static ReturnType to() {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
        }
    };

    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, BOOL, py::detail::npy_api::NPY_BOOL_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, CHAR, py::detail::npy_api::NPY_BYTE_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, INT8, py::detail::npy_api::NPY_BYTE_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, UINT8, py::detail::npy_api::NPY_UBYTE_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, INT16, py::detail::npy_api::NPY_SHORT_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, UINT16, py::detail::npy_api::NPY_USHORT_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, INT32, py::detail::npy_api::NPY_INT_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, UINT32, py::detail::npy_api::NPY_UINT_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, INT64, py::detail::npy_api::NPY_LONGLONG_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, UINT64, py::detail::npy_api::NPY_ULONGLONG_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, FLOAT, py::detail::npy_api::NPY_FLOAT_)
    KARABO_MAP_TO_REFERENCE_TYPE(ToNumpy, DOUBLE, py::detail::npy_api::NPY_DOUBLE_)

} // namespace karabind
#endif /*KARABIND_TONUMPY_HH*/
