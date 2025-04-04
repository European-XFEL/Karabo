/*
 * File: PyUtilClassInfo.cc
 *
 * Author: CONTROLS DEV group
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

#include "karabo/data/types/ClassInfo.hh"

using namespace karabo::data;
using namespace std;
namespace py = pybind11;


void exportPyUtilClassInfo(py::module_& m) {
    py::class_<ClassInfo>(m, "ClassInfo")
          .def(py::init<string const&, string const&, string const&>(), py::arg("classId"), py::arg("signature"),
               py::arg("classVersion"))
          .def("getClassId", (string const& (ClassInfo::*)() const) & ClassInfo::getClassId)
          .def("getClassName", (string const& (ClassInfo::*)() const) & ClassInfo::getClassName)
          .def("getVersion", (string const& (ClassInfo::*)() const) & ClassInfo::getVersion)
          .def("getLogCategory", (string const& (ClassInfo::*)() const) & ClassInfo::getLogCategory)
          .def("getNamespace", (string const& (ClassInfo::*)() const) & ClassInfo::getNamespace);
}
