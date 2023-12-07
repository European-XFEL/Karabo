/*
 * $Id$
 *
 * Author: <steffen.hauf@xfel.eu>
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

#include <karabo/util/RollingWindowStatistics.hh>


namespace py = pybind11;
using namespace karabo::util;


void exportPyUtilRollingWindowStatistics(py::module& m) {
    py::class_<RollingWindowStatistics>(m, "RollingWindowStatistics")

          .def(py::init<const unsigned int>())

          .def("update", &RollingWindowStatistics::update, py::arg("value"))

          .def("getRollingWindowVariance", &RollingWindowStatistics::getRollingWindowVariance)

          .def("getRollingWindowMean", &RollingWindowStatistics::getRollingWindowMean);
}
