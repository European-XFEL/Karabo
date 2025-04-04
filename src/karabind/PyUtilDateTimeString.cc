/*
 *
 * Author: CTRL DEV
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

#include "karabo/data/time/DateTimeString.hh"

namespace py = pybind11;
using namespace karabo::data;
using namespace std;


void exportPyUtilDateTimeString(py::module_& m) {
    py::class_<DateTimeString> dts(m, "DateTimeString");

    dts.def(py::init<>());

    dts.def(py::init<std::string const&>(), py::arg("timePoint"));

    dts.def(py::init<std::string const&, std::string const&, std::string const&, std::string const&>(),
            py::arg("inputDateStr"), py::arg("inputTimeStr"), py::arg("inputFractionSecondStr"),
            py::arg("inputTimeZoneStr"));

    py::implicitly_convertible<std::string const&, karabo::data::DateTimeString>();

    dts.def("getDate", &DateTimeString::getDate);

    dts.def("getTime", &DateTimeString::getTime);

    dts.def("getFractionalSeconds",
            (const std::string (DateTimeString::*)() const) & DateTimeString::getFractionalSeconds);

    dts.def("getFractionalSeconds",
            (const unsigned long long (DateTimeString::*)() const) & DateTimeString::getFractionalSeconds);

    dts.def("getTimeZone", &DateTimeString::getTimeZone);

    dts.def("getDateTime", &DateTimeString::getDateTime);

    dts.def_static("isStringValidIso8601", &DateTimeString::isStringValidIso8601, py::arg("timePoint"));

    dts.def_static("isStringKaraboValidIso8601", &DateTimeString::isStringKaraboValidIso8601, py::arg("timePoint"));

    dts.def("getSecondsSinceEpoch", &DateTimeString::getSecondsSinceEpoch);
}
