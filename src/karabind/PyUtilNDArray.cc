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

#include <pybind11/pybind11.h>

#include "Wrapper.hh"

namespace py = pybind11;
using namespace karabind;

void exportPyUtilNDArray(py::module_& m) {
    py::class_<ArrayDataPtrBase, std::shared_ptr<ArrayDataPtrBase>>(m, "_ArrayDataPtrBase_")
          .def(py::init<ArrayDataPtrBase>());
}
