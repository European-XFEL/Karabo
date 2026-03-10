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
#include <pybind11/native_enum.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeDuration.hh"
#include "karabo/data/time/TimeId.hh"
#include "karabo/data/time/Timestamp.hh"

namespace py = pybind11;
using namespace karabo::data;
using namespace std;


void exportPyUtilTimestamp(py::module_& m) {
    py::class_<Epochstamp> e(m, "Epochstamp");

    py::class_<TimeDuration> td(m, "TimeDuration");

    py::class_<TimeId> ti(m, "TimeId");

    py::class_<Timestamp> ts(m, "Timestamp");

    py::native_enum<TIME_UNITS>(e, "TIME_UNITS", "enum.Enum")
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
          .export_values()
          .finalize();

    // ------------------- TimeDuration

    td.def(py::init<>());

    td.def(py::init<Hash const&>(), py::arg("hash"));

    py::implicitly_convertible<Hash const&, TimeDuration>();

    td.def(py::init<long long unsigned int, long long unsigned int>(), py::arg("seconds"), py::arg("fractions"));

    td.def(py::init<int, int, int, long long unsigned int, long long unsigned int>(), py::arg("days"), py::arg("hours"),
           py::arg("minutes"), py::arg("seconds"), py::arg("fractions"));

    // td.def(py::self_ns::str(py::self));
    td.def("__str__", [](const TimeDuration& self) {
        std::ostringstream oss;
        oss << self;
        return oss.str();
    });

    td.def("set", (TimeDuration & (TimeDuration::*)(TimeValue const, TimeValue const)) & TimeDuration::set,
           py::arg("seconds"), py::arg("fractions"), py::return_value_policy::reference_internal);

    td.def("set",
           (TimeDuration & (TimeDuration::*)(int const, int const, int const, TimeValue const, TimeValue const)) &
                 TimeDuration::set,
           py::arg("days"), py::arg("hours"), py::arg("minutes"), py::arg("seconds"), py::arg("fractions"),
           py::return_value_policy::reference_internal);

    td.def("add", (TimeDuration & (TimeDuration::*)(TimeValue const, TimeValue const)) & TimeDuration::add,
           py::arg("seconds"), py::arg("fractions"), py::return_value_policy::reference_internal);

    td.def("add",
           (TimeDuration & (TimeDuration::*)(int const, int const, int const, TimeValue const, TimeValue const)) &
                 TimeDuration::add,
           py::arg("days"), py::arg("hours"), py::arg("minutes"), py::arg("seconds"), py::arg("fractions"),
           py::return_value_policy::reference_internal);

    td.def("sub", (TimeDuration & (TimeDuration::*)(TimeValue const, TimeValue const)) & TimeDuration::sub,
           py::arg("seconds"), py::arg("fractions"), py::return_value_policy::reference_internal);

    td.def("sub",
           (TimeDuration & (TimeDuration::*)(int const, int const, int const, TimeValue const, TimeValue const)) &
                 TimeDuration::sub,
           py::arg("days"), py::arg("hours"), py::arg("minutes"), py::arg("seconds"), py::arg("fractions"),
           py::return_value_policy::reference_internal);

    td.def("format", &TimeDuration::format, py::arg("fmt"));

    td.def("fromHash", &TimeDuration::fromHash, py::arg("hash"));

    td.def("getDays", &TimeDuration::getDays);

    td.def("getFractions", &TimeDuration::getFractions, py::arg_v("unit", TIME_UNITS::NANOSEC, "TIME_UNITS.NANOSEC"));

    td.def("getHours", &TimeDuration::getHours);

    td.def("getMinutes", &TimeDuration::getMinutes);

    td.def("getSeconds", &TimeDuration::getSeconds);

    td.def("getTotalHours", &TimeDuration::getTotalHours);

    td.def("getTotalMinutes", &TimeDuration::getTotalMinutes);

    td.def("getTotalSeconds", &TimeDuration::getTotalSeconds);

    td.def("isNull", &TimeDuration::isNull);

    td.def_static("setDefaultFormat", &TimeDuration::setDefaultFormat, py::arg("fmt"));

    td.def("toHash", &TimeDuration::toHash, py::arg("hash"));

    td.def(py::self != py::self);
    td.def(py::self + py::self);
    td.def(py::self += py::self);
    td.def(py::self - py::self);
    td.def(py::self -= py::self);
    td.def(py::self / py::self);
    td.def(py::self < py::self);
    td.def(py::self <= py::self);
    td.def(py::self == py::self);
    td.def(py::self > py::self);
    td.def(py::self >= py::self);

    // ------------------- Epochstamp

    e.def(py::init<>());

    e.def(py::init<const unsigned long long&, const unsigned long long&>(), py::arg("seconds"), py::arg("fractions"));

    e.def(py::init<string const&>(), py::arg("pTimeStr"));
    py::implicitly_convertible<string const&, Epochstamp>();

    e.def("getSeconds", &Epochstamp::getSeconds);

    e.def("getFractionalSeconds", &Epochstamp::getFractionalSeconds);

    e.def("getTime", &Epochstamp::getTime);

    e.def("now", &Epochstamp::now);

    e.def(
          "elapsed", [](const Epochstamp& self, const Epochstamp& other) { return py::cast(self.elapsed(other)); },
          py::arg_v("other", Epochstamp(), "Epochstamp()"));

    e.def_static("fromHashAttributes", &Epochstamp::fromHashAttributes, py::arg("attributes"));

    e.def("toHashAttributes", &Epochstamp::toHashAttributes, py::arg("attributes"));

    e.def_static("hashAttributesContainTimeInformation", &Epochstamp::hashAttributesContainTimeInformation,
                 py::arg("attributes"));

    e.def("toIso8601", &Epochstamp::toIso8601, py::arg_v("precision", TIME_UNITS::MICROSEC, "TIME_UNITS.MICROSEC"),
          py::arg("extended") = false);

    e.def("toIso8601Ext", &Epochstamp::toIso8601Ext,
          py::arg_v("precision", TIME_UNITS::MICROSEC, "TIME_UNITS.MICROSEC"), py::arg("extended") = false);

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

    e.def(py::self == py::self);
    e.def(py::self > py::self);
    e.def(py::self >= py::self);


    // ------------------- TimeId

    ti.def(py::init<>());

    ti.def(py::init<const unsigned long long>(), py::arg("trainId"));

    ti.def("getTid", (unsigned long long const& (TimeId::*)() const) & TimeId::getTid,
           py::return_value_policy::reference);

    ti.def_static("hashAttributesContainTimeInformation", &TimeId::hashAttributesContainTimeInformation,
                  py::arg("attributes"));

    ti.def_static("fromHashAttributes", (TimeId(*)(Hash::Attributes const)) & TimeId::fromHashAttributes,
                  py::arg("attributes"));

    ti.def("toHashAttributes", (void(TimeId::*)(Hash::Attributes&) const) & TimeId::toHashAttributes,
           py::arg("attributes"));


    // ------------------- Timestamp

    ts.def(py::init<>());

    ts.def(py::init<Epochstamp const&, TimeId const&>(), py::arg("e"), py::arg("t"));

    ts.def("getSeconds", &Timestamp::getSeconds);

    ts.def("getFractionalSeconds", &Timestamp::getFractionalSeconds);

    ts.def("getTid", &Timestamp::getTid);

    ts.def_static("fromHashAttributes", &Timestamp::fromHashAttributes, py::arg("attributes"));

    ts.def_static("hashAttributesContainTimeInformation", &Timestamp::hashAttributesContainTimeInformation,
                  py::arg("attributes"));

    ts.def("toIso8601", &Timestamp::toIso8601, py::arg_v("precision", TIME_UNITS::MICROSEC, "TIME_UNITS.MICROSEC"),
           py::arg("extended") = false);

    ts.def("toIso8601Ext", &Timestamp::toIso8601Ext,
           py::arg_v("precision", TIME_UNITS::MICROSEC, "TIME_UNITS.MICROSEC"), py::arg("extended") = false);

    ts.def("toFormattedStringLocale", &Timestamp::toFormattedStringLocale, py::arg("localeName") = "",
           py::arg("format") = "%Y-%b-%d %H:%M:%S", py::arg("localTimeZone") = "Z");

    ts.def("toFormattedString", &Timestamp::toFormattedString, py::arg("format") = "%Y-%b-%d %H:%M:%S",
           py::arg("localTimeZone") = "Z");

    ts.def("toTimestamp", &Timestamp::toTimestamp);

    ts.def("toHashAttributes", &Timestamp::toHashAttributes, py::arg("attributes"));

    ts.def(py::self == py::self);

    ts.def(py::self != py::self);
}
