/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
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
#include <karabo/util/Timestamp.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;


void exportPyUtilTimestamp() {
    bp::class_<Timestamp> ts("Timestamp", bp::init<>());

    ts.def(bp::init<Epochstamp const&, Trainstamp const&>((bp::arg("e"), bp::arg("t"))));

    ts.def("getSeconds", (unsigned long long const& (Timestamp::*)() const)(&Timestamp::getSeconds),
           bp::return_value_policy<bp::copy_const_reference>());

    ts.def("getFractionalSeconds", (unsigned long long const& (Timestamp::*)() const)(&Timestamp::getFractionalSeconds),
           bp::return_value_policy<bp::copy_const_reference>());

    ts.def("getTrainId", (unsigned long long const& (Timestamp::*)() const)(&Timestamp::getTrainId),
           bp::return_value_policy<bp::copy_const_reference>());

    ts.def("fromHashAttributes", &Timestamp::fromHashAttributes, bp::arg("attributes"));
    ts.staticmethod("fromHashAttributes");

    ts.def("hashAttributesContainTimeInformation", &Timestamp::hashAttributesContainTimeInformation,
           bp::arg("attributes"));
    ts.staticmethod("hashAttributesContainTimeInformation");

    ts.def("toIso8601", (string(Timestamp::*)(karabo::util::TIME_UNITS, bool) const)(&Timestamp::toIso8601),
           (bp::arg("precision") = karabo::util::MICROSEC, bp::arg("extended") = (bool)(false)));

    ts.def("toIso8601Ext", (string(Timestamp::*)(karabo::util::TIME_UNITS, bool) const)(&Timestamp::toIso8601Ext),
           (bp::arg("precision") = karabo::util::MICROSEC, bp::arg("extended") = (bool)(false)));

    ts.def("toFormattedStringLocale",
           (string(Timestamp::*)(const string&, const string&, const string&)
                  const)(&Timestamp::toFormattedStringLocale),
           (bp::arg("localeName") = "", bp::arg("format") = "%Y-%b-%d %H:%M:%S", bp::arg("localTimeZone") = "Z"));

    ts.def("toFormattedString",
           (string(Timestamp::*)(const string&, const string&) const)(&Timestamp::toFormattedString),
           (bp::arg("format") = "%Y-%b-%d %H:%M:%S", bp::arg("localTimeZone") = "Z"));

    ts.def("toTimestamp", (double(Timestamp::*)() const)(&Timestamp::toTimestamp));

    ts.def("toHashAttributes", (void(Timestamp::*)(Hash::Attributes&) const)(&Timestamp::toHashAttributes),
           bp::arg("attributes"));
}
