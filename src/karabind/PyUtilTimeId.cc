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

#include "karabo/data/time/TimeId.hh"

namespace py = pybind11;
using namespace karabo::data;
using namespace std;


void exportPyUtilTimeId(py::module_& m) {
    py::class_<TimeId> t(m, "TimeId");

    t.def(py::init<>());

    t.def(py::init<const unsigned long long>(), py::arg("trainId"));

    t.def("getTid", (unsigned long long const& (TimeId::*)() const) & TimeId::getTid,
          py::return_value_policy::reference);

    t.def_static("hashAttributesContainTimeInformation", &TimeId::hashAttributesContainTimeInformation,
                 py::arg("attributes"));

    t.def_static("fromHashAttributes", (TimeId(*)(Hash::Attributes const)) & TimeId::fromHashAttributes,
                 py::arg("attributes"));

    t.def("toHashAttributes", (void(TimeId::*)(Hash::Attributes&) const) & TimeId::toHashAttributes,
          py::arg("attributes"));
}
