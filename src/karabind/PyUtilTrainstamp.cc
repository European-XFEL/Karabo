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

#include "karabo/data/time/Trainstamp.hh"

namespace py = pybind11;
using namespace karabo::data;
using namespace std;


void exportPyUtilTrainstamp(py::module_& m) {
    py::class_<Trainstamp> t(m, "Trainstamp");

    t.def(py::init<>());

    t.def(py::init<const unsigned long long>(), py::arg("trainId"));

    t.def("getTrainId", (unsigned long long const& (Trainstamp::*)() const) & Trainstamp::getTrainId,
          py::return_value_policy::reference);

    t.def_static("hashAttributesContainTimeInformation", &Trainstamp::hashAttributesContainTimeInformation,
                 py::arg("attributes"));

    t.def_static("fromHashAttributes", (Trainstamp(*)(Hash::Attributes const)) & Trainstamp::fromHashAttributes,
                 py::arg("attributes"));

    t.def("toHashAttributes", (void(Trainstamp::*)(Hash::Attributes&) const) & Trainstamp::toHashAttributes,
          py::arg("attributes"));
}
