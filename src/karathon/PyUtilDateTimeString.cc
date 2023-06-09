/*
 * $Id$
 *
 * Author: <luis.maia@xfel.eu>
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
#include <boost/python.hpp>
#include <karabo/util/DateTimeString.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;


void exportPyUtilDateTimeString() {
    bp::class_<DateTimeString> dts("DateTimeString", bp::init<>());

    dts.def(bp::init<string const &>((bp::arg("timePoint"))));

    dts.def(bp::init<string const &, string const &, string const &, string const &>(
          (bp::arg("inputDateStr"), bp::arg("inputTimeStr"), bp::arg("inputFractionSecondStr"),
           bp::arg("inputTimeZoneStr"))));

    bp::implicitly_convertible<std::string const &, karabo::util::DateTimeString>();

    dts.def("getDate", (const std::string &(DateTimeString::*)() const)(&DateTimeString::getDate),
            bp::return_value_policy<bp::copy_const_reference>());

    dts.def("getTime", (const std::string &(DateTimeString::*)() const)(&DateTimeString::getTime),
            bp::return_value_policy<bp::copy_const_reference>());

    dts.def("getFractionalSeconds",
            (const std::string (DateTimeString::*)() const)(&DateTimeString::getFractionalSeconds));

    dts.def("getFractionalSeconds",
            (const unsigned long long (DateTimeString::*)() const)(&DateTimeString::getFractionalSeconds));

    dts.def("getTimeZone", (const std::string &(DateTimeString::*)() const)(&DateTimeString::getTimeZone),
            bp::return_value_policy<bp::copy_const_reference>());

    dts.def("getDateTime", (const std::string &(DateTimeString::*)() const)(&DateTimeString::getDateTime),
            bp::return_value_policy<bp::copy_const_reference>());

    dts.def("isStringValidIso8601", (const bool (*)(string const &))(&DateTimeString::isStringValidIso8601),
            (bp::arg("timePoint")));
    dts.staticmethod("isStringValidIso8601");

    dts.def("isStringKaraboValidIso8601", (const bool (*)(string const &))(&DateTimeString::isStringKaraboValidIso8601),
            (bp::arg("timePoint")));
    dts.staticmethod("isStringKaraboValidIso8601");

    dts.def("getSecondsSinceEpoch",
            (const unsigned long long (DateTimeString::*)())(&DateTimeString::getSecondsSinceEpoch));
}
