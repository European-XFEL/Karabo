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

#include <pybind11/pybind11.h>

namespace py = pybind11;

// util
void exportPyUtilTypesReferenceType(py::module_ &);
void exportPyUtilHashAttributes(py::module_ &);
void exportPyUtilHashNode(py::module_ &);
void exportPyUtilHash(py::module_ &);

// io

// xms

// core

// log

// net

// Build one big module, 'karabind.so', similar to how we build 'karathon' module

PYBIND11_MODULE(karabind, m) {
    // util
    exportPyUtilTypesReferenceType(m);
    exportPyUtilHashAttributes(m);
    exportPyUtilHashNode(m);
    exportPyUtilHash(m);

    // io

    // xms

    // core

    // log

    // net
}
