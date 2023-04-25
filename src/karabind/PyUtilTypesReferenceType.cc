/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#include <pybind11/pybind11.h>

#include <karabo/util/FromLiteral.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>

#include "PyTypes.hh"


namespace py = pybind11;

void exportPyUtilTypesReferenceType(py::module_& m) {
    using namespace karabo::util;
    using namespace karabind;

    // Types
    py::enum_<PyTypes::ReferenceType>(m, "Types",
                                      "This enumeration describes reference types supported in configuration system.")
          .value("BOOL", PyTypes::BOOL)
          .value("VECTOR_BOOL", PyTypes::VECTOR_BOOL)
          .value("CHAR", PyTypes::CHAR)
          .value("VECTOR_CHAR", PyTypes::VECTOR_CHAR)
          .value("INT8", PyTypes::INT8)
          .value("VECTOR_INT8", PyTypes::VECTOR_INT8)
          .value("UINT8", PyTypes::UINT8)
          .value("VECTOR_UINT8", PyTypes::VECTOR_UINT8)
          .value("INT16", PyTypes::INT16)
          .value("VECTOR_INT16", PyTypes::VECTOR_INT16)
          .value("UINT16", PyTypes::UINT16)
          .value("VECTOR_UINT16", PyTypes::VECTOR_UINT16)
          .value("INT32", PyTypes::INT32)
          .value("VECTOR_INT32", PyTypes::VECTOR_INT32)
          .value("UINT32", PyTypes::UINT32)
          .value("VECTOR_UINT32", PyTypes::VECTOR_UINT32)
          .value("INT64", PyTypes::INT64)
          .value("VECTOR_INT64", PyTypes::VECTOR_INT64)
          .value("UINT64", PyTypes::UINT64)
          .value("VECTOR_UINT64", PyTypes::VECTOR_UINT64)
          .value("FLOAT", PyTypes::FLOAT)
          .value("VECTOR_FLOAT", PyTypes::VECTOR_FLOAT)
          .value("DOUBLE", PyTypes::DOUBLE)
          .value("VECTOR_DOUBLE", PyTypes::VECTOR_DOUBLE)
          .value("COMPLEX_FLOAT", PyTypes::COMPLEX_FLOAT)
          .value("VECTOR_COMPLEX_FLOAT", PyTypes::VECTOR_COMPLEX_FLOAT)
          .value("COMPLEX_DOUBLE", PyTypes::COMPLEX_DOUBLE)
          .value("VECTOR_COMPLEX_DOUBLE", PyTypes::VECTOR_COMPLEX_DOUBLE)
          .value("STRING", PyTypes::STRING)
          .value("VECTOR_STRING", PyTypes::VECTOR_STRING)
          .value("HASH", PyTypes::HASH)
          .value("VECTOR_HASH", PyTypes::VECTOR_HASH)
          .value("SCHEMA", PyTypes::SCHEMA)
          .value("ANY", PyTypes::ANY)
          .value("NONE", PyTypes::NONE)
          .value("VECTOR_NONE", PyTypes::VECTOR_NONE)
          .value("UNKNOWN", PyTypes::UNKNOWN)
          .value("SIMPLE", PyTypes::SIMPLE)
          .value("SEQUENCE", PyTypes::SEQUENCE)
          .value("POINTER", PyTypes::POINTER)
          .value("VECTOR_HASH_POINTER", PyTypes::VECTOR_HASH_POINTER)
          .export_values();

    py::class_<PyTypes>(m, "TypesClass")
          .def_static("toType", &PyTypes::to, py::arg("Python_types"))
          .def_static("fromType", &PyTypes::from, py::arg("C++_types"))
          .def_static("category", &PyTypes::category, py::arg("C++_types int"));
}
