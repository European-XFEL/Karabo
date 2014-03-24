/*
 * $Id$
 *
 * Author: <luis.maia@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/util/DateTimeString.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

void exportPyUtilDateTimeString() {

    bp::class_<DateTimeString> dts("DateTimeString", bp::init<>());

    dts.def(bp::init<string const &>((bp::arg("timePoint"))));

    dts.def(bp::init<string const &, string const &, string const &, string const &>((bp::arg("inputDateStr"), bp::arg("inputTimeStr"), bp::arg("inputFractionSecondStr"), bp::arg("inputTimeZoneStr"))));

    bp::implicitly_convertible< std::string const &, karabo::util::DateTimeString >();

    dts.def("getDateString"
            , (const std::string & (DateTimeString::*)() const) (&DateTimeString::getDateString)
            , bp::return_value_policy< bp::copy_const_reference >());

    dts.def("getTimeString"
            , (const std::string & (DateTimeString::*)() const) (&DateTimeString::getTimeString)
            , bp::return_value_policy< bp::copy_const_reference >());

    dts.def("getFractionalSecondString"
            , (const std::string(DateTimeString::*)() const) (&DateTimeString::getFractionalSecondString));

    dts.def("getFractionalSecondString"
            , (const unsigned long long (DateTimeString::*)() const) (&DateTimeString::getFractionalSecondString));

    dts.def("getTimeZoneString"
            , (const std::string & (DateTimeString::*)() const) (&DateTimeString::getTimeZoneString)
            , bp::return_value_policy< bp::copy_const_reference >());

    dts.def("getDateTimeString"
            , (const std::string & (DateTimeString::*)() const) (&DateTimeString::getDateTimeString)
            , bp::return_value_policy< bp::copy_const_reference >());

    dts.def("isStringValidIso8601"
            , (const bool (*)(string const &))(&DateTimeString::isStringValidIso8601)
            , (bp::arg("timePoint")));
    dts.staticmethod("isStringValidIso8601");

    dts.def("isStringKaraboValidIso8601"
            , (const bool (*)(string const &))(&DateTimeString::isStringKaraboValidIso8601)
            , (bp::arg("timePoint")));
    dts.staticmethod("isStringKaraboValidIso8601");

    dts.def("getSecondsSinceEpoch"
            , (const unsigned long long (DateTimeString::*)()) (&DateTimeString::getSecondsSinceEpoch));

}
