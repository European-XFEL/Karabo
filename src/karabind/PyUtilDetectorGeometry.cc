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
#include <pybind11/stl.h>

#include <karabo/util/DetectorGeometry.hh>
#include <karabo/util/Hash.hh>


namespace py = pybind11;
using namespace karabo::util;


void exportPyUtilDetectorGeometry(py::module_& m) {
    py::class_<DetectorGeometry> d(m, "DetectorGeometry");
    d.def(py::init<>());
    d.def(py::init<Hash>());
    d.def("getOffsets", [](DetectorGeometry& self) -> py::list { return py::cast(self.getOffsets()); });
    d.def("getRotations", [](DetectorGeometry& self) -> py::list { return py::cast(self.getRotations()); });
    d.def("toHash", &DetectorGeometry::toHash);
    d.def("toSchema", &DetectorGeometry::toSchema, py::arg("topNode"), py::arg("schema"), py::arg("topMost") = true);
    d.def("setOffsets", &DetectorGeometry::setOffsets, py::arg("x"), py::arg("y"), py::arg("z"),
          py::return_value_policy::reference_internal);
    d.def("setRotations", &DetectorGeometry::setRotations, py::arg("theta"), py::arg("phi"), py::arg("omega"),
          py::return_value_policy::reference_internal);
    d.def("startSubAssembly", &DetectorGeometry::startSubAssembly, py::return_value_policy::reference_internal);
    d.def("endSubAssembly", &DetectorGeometry::endSubAssembly, py::return_value_policy::reference_internal);
    d.def("setPixelRegion", &DetectorGeometry::setPixelRegion, py::arg("x0"), py::arg("y0"), py::arg("x1"),
          py::arg("y1"), py::return_value_policy::reference_internal);
}
