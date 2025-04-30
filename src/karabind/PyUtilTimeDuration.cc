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

#include "karabo/data/time/TimeDuration.hh"

namespace py = pybind11;
using namespace karabo::data;
using namespace std;


void exportPyUtilTimeDuration(py::module_& m) {
    py::class_<TimeDuration> t(m, "TimeDuration");

    t.def(py::init<>());

    t.def(py::init<Hash const&>(), py::arg("hash"));

    py::implicitly_convertible<Hash const&, TimeDuration>();

    t.def(py::init<long long unsigned int, long long unsigned int>(), py::arg("seconds"), py::arg("fractions"));

    t.def(py::init<int, int, int, long long unsigned int, long long unsigned int>(), py::arg("days"), py::arg("hours"),
          py::arg("minutes"), py::arg("seconds"), py::arg("fractions"));

    // t.def(py::self_ns::str(py::self));
    t.def("__str__", [](const TimeDuration& self) {
        std::ostringstream oss;
        oss << self;
        return oss.str();
    });

    t.def("set", (TimeDuration & (TimeDuration::*)(TimeValue const, TimeValue const)) & TimeDuration::set,
          py::arg("seconds"), py::arg("fractions"), py::return_value_policy::reference_internal);

    t.def("set",
          (TimeDuration & (TimeDuration::*)(int const, int const, int const, TimeValue const, TimeValue const)) &
                TimeDuration::set,
          py::arg("days"), py::arg("hours"), py::arg("minutes"), py::arg("seconds"), py::arg("fractions"),
          py::return_value_policy::reference_internal);

    t.def("add", (TimeDuration & (TimeDuration::*)(TimeValue const, TimeValue const)) & TimeDuration::add,
          py::arg("seconds"), py::arg("fractions"), py::return_value_policy::reference_internal);

    t.def("add",
          (TimeDuration & (TimeDuration::*)(int const, int const, int const, TimeValue const, TimeValue const)) &
                TimeDuration::add,
          py::arg("days"), py::arg("hours"), py::arg("minutes"), py::arg("seconds"), py::arg("fractions"),
          py::return_value_policy::reference_internal);

    t.def("sub", (TimeDuration & (TimeDuration::*)(TimeValue const, TimeValue const)) & TimeDuration::sub,
          py::arg("seconds"), py::arg("fractions"), py::return_value_policy::reference_internal);

    t.def("sub",
          (TimeDuration & (TimeDuration::*)(int const, int const, int const, TimeValue const, TimeValue const)) &
                TimeDuration::sub,
          py::arg("days"), py::arg("hours"), py::arg("minutes"), py::arg("seconds"), py::arg("fractions"),
          py::return_value_policy::reference_internal);

    t.def("format", &TimeDuration::format, py::arg("fmt"));

    t.def("fromHash", &TimeDuration::fromHash, py::arg("hash"));

    t.def("getDays", &TimeDuration::getDays);

    t.def("getFractions", &TimeDuration::getFractions, py::arg("unit") = TIME_UNITS::NANOSEC);

    t.def("getHours", &TimeDuration::getHours);

    t.def("getMinutes", &TimeDuration::getMinutes);

    t.def("getSeconds", &TimeDuration::getSeconds);

    t.def("getTotalHours", &TimeDuration::getTotalHours);

    t.def("getTotalMinutes", &TimeDuration::getTotalMinutes);

    t.def("getTotalSeconds", &TimeDuration::getTotalSeconds);

    t.def("isNull", &TimeDuration::isNull);

    t.def_static("setDefaultFormat", &TimeDuration::setDefaultFormat, py::arg("fmt"));

    t.def("toHash", &TimeDuration::toHash, py::arg("hash"));

    t.def(py::self != py::self);
    t.def(py::self + py::self);
    t.def(py::self += py::self);
    t.def(py::self - py::self);
    t.def(py::self -= py::self);
    t.def(py::self / py::self);
    t.def(py::self < py::self);
    t.def(py::self <= py::self);
    t.def(py::self == py::self);
    t.def(py::self > py::self);
    t.def(py::self >= py::self);
}
