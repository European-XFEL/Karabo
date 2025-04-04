/*
 * File:   FromNumpy.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on May 23, 2014 6:34 PM
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
 *
 */

#include "FromNumpy.hh"

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "Wrapper.hh"

namespace py = pybind11;

namespace karabind {

    FromNumpy* FromNumpy::singleInstance;
    std::once_flag FromNumpy::staticFlag;

    // clang-format off
    FromNumpy::FromNumpy() {
#define _KARABO_HELPER_MACRO(fromType, refType) { fromType, karabo::data::Types::refType }
        // clang-format on
        _typeInfoMap = {_KARABO_HELPER_MACRO(py::detail::npy_api::NPY_BOOL_, BOOL),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_BYTE_, INT8),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_UBYTE_, UINT8),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_SHORT_, INT16),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_USHORT_, UINT16),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_INT_, INT32),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_UINT_, UINT32),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_LONG_, INT64),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_ULONG_, UINT64),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_LONGLONG_, INT64),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_ULONGLONG_, UINT64),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_FLOAT_, FLOAT),
                        _KARABO_HELPER_MACRO(py::detail::npy_api::NPY_DOUBLE_, DOUBLE)};
#undef _KARABO_HELPER_MACRO
    }

    FromNumpy::~FromNumpy() {
        delete singleInstance;
    }

    FromNumpy& FromNumpy::init() {
        std::call_once(staticFlag, [&]() { singleInstance = new FromNumpy(); });
        return *singleInstance;
    }
} // namespace karabind
