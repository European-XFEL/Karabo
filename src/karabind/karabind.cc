/*
 * Author: CONTROLS DEV group
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
