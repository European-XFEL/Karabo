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

#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeDuration.hh"

namespace py = pybind11;
using namespace karabo::data;
using namespace std;


void exportPyUtilEpochstamp(py::module_& m) {
    py::enum_<TIME_UNITS>(m, "TIME_UNITS")
          .value("ATTOSEC", TIME_UNITS::ATTOSEC)
          .value("FEMTOSEC", TIME_UNITS::FEMTOSEC)
          .value("PICOSEC", TIME_UNITS::PICOSEC)
          .value("NANOSEC", TIME_UNITS::NANOSEC)
          .value("MICROSEC", TIME_UNITS::MICROSEC)
          .value("MILLISEC", TIME_UNITS::MILLISEC)
          .value("ONESECOND", TIME_UNITS::ONESECOND)
          .value("SECOND", TIME_UNITS::SECOND)
          .value("MINUTE", TIME_UNITS::MINUTE)
          .value("HOUR", TIME_UNITS::HOUR)
          .value("DAY", TIME_UNITS::DAY)
          .export_values();

    py::class_<Epochstamp> e(m, "Epochstamp");

    e.def(py::init<>());

    e.def(py::init<const unsigned long long&, const unsigned long long&>(), py::arg("seconds"), py::arg("fractions"));

    e.def(py::init<time_t const&>(), py::arg("tm"));
    py::implicitly_convertible<time_t const&, Epochstamp>();

    e.def(py::init<timeval const&>(), py::arg("tv"));
    py::implicitly_convertible<timeval const&, Epochstamp>();

    e.def(py::init<timespec const&>(), py::arg("ts"));
    py::implicitly_convertible<timespec const&, Epochstamp>();

    e.def(py::init<string const&>(), py::arg("pTimeStr"));
    py::implicitly_convertible<string const&, Epochstamp>();

    e.def("getSeconds", &Epochstamp::getSeconds);

    e.def("getFractionalSeconds", &Epochstamp::getFractionalSeconds);

    e.def("getTime", &Epochstamp::getTime);

    e.def("getTimeOfDay", &Epochstamp::getTimeOfDay);

    e.def("getClockTime", &Epochstamp::getClockTime);

    e.def("now", &Epochstamp::now);

    e.def("elapsed", &Epochstamp::elapsed, py::arg("other") = Epochstamp());

    e.def_static("fromHashAttributes", &Epochstamp::fromHashAttributes, py::arg("attributes"));

    e.def("toHashAttributes", &Epochstamp::toHashAttributes, py::arg("attributes"));

    e.def_static("hashAttributesContainTimeInformation", &Epochstamp::hashAttributesContainTimeInformation,
                 py::arg("attributes"));

    e.def("toIso8601", &Epochstamp::toIso8601, py::arg("precision") = TIME_UNITS::MICROSEC,
          py::arg("extended") = false);

    e.def("toIso8601Ext", &Epochstamp::toIso8601Ext, py::arg("precision") = TIME_UNITS::MICROSEC,
          py::arg("extended") = (bool)(false));

    e.def("toTimestamp", &Epochstamp::toTimestamp);

    e.def("toFormattedString", &Epochstamp::toFormattedString, py::arg("format") = "%Y-%b-%d %H:%M:%S",
          py::arg("localTimeZone") = "Z");

    e.def("toFormattedStringLocale", &Epochstamp::toFormattedStringLocale, py::arg("localeName") = "",
          py::arg("format") = "%Y-%b-%d %H:%M:%S", py::arg("localTimeZone") = "Z");

    e.def(py::self != py::self);
    e.def(py::self + TimeDuration());
    e.def(py::self += TimeDuration());
    e.def(py::self - py::self);
    e.def(py::self - TimeDuration());
    e.def(py::self -= TimeDuration());
    e.def(py::self < py::self);
    e.def(py::self <= py::self);

    // Epochstamp::operator=
    e.def("assign", (Epochstamp & (Epochstamp::*)(time_t const&))(&Epochstamp::operator=), py::arg("tm"));

    // Epochstamp::operator=
    e.def("assign", (Epochstamp & (Epochstamp::*)(timeval const&))(&Epochstamp::operator=), py::arg("tv"));

    //::Epochstamp::operator=
    e.def("assign", (Epochstamp & (Epochstamp::*)(timespec const&))(&Epochstamp::operator=), py::arg("ts"));

    e.def(py::self == py::self);
    e.def(py::self > py::self);
    e.def(py::self >= py::self);
}
