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
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "karabo/data/time/Timestamp.hh"

namespace py = pybind11;
using namespace karabo::data;
using namespace std;


void exportPyUtilTimestamp(py::module_& m) {
    py::class_<Timestamp> ts(m, "Timestamp");

    ts.def(py::init<>());

    ts.def(py::init<Epochstamp const&, Trainstamp const&>(), py::arg("e"), py::arg("t"));

    ts.def("getSeconds", &Timestamp::getSeconds);

    ts.def("getFractionalSeconds", &Timestamp::getFractionalSeconds);

    ts.def("getTrainId", &Timestamp::getTrainId);

    ts.def_static("fromHashAttributes", &Timestamp::fromHashAttributes, py::arg("attributes"));

    ts.def_static("hashAttributesContainTimeInformation", &Timestamp::hashAttributesContainTimeInformation,
                  py::arg("attributes"));

    ts.def("toIso8601", &Timestamp::toIso8601, py::arg("precision") = karabo::data::MICROSEC,
           py::arg("extended") = (bool)(false));

    ts.def("toIso8601Ext", &Timestamp::toIso8601Ext, py::arg("precision") = karabo::data::MICROSEC,
           py::arg("extended") = (bool)(false));

    ts.def("toFormattedStringLocale", &Timestamp::toFormattedStringLocale, py::arg("localeName") = "",
           py::arg("format") = "%Y-%b-%d %H:%M:%S", py::arg("localTimeZone") = "Z");

    ts.def("toFormattedString", &Timestamp::toFormattedString, py::arg("format") = "%Y-%b-%d %H:%M:%S",
           py::arg("localTimeZone") = "Z");

    ts.def("toTimestamp", &Timestamp::toTimestamp);

    ts.def("toHashAttributes", &Timestamp::toHashAttributes, py::arg("attributes"));

    ts.def(py::self == py::self);

    ts.def(py::self != py::self);
}
