/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */
#include <karabo/util/TimeDuration.hh>

#include "boost/python.hpp"

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;


void exportPyUtilTimeDuration() {
    bp::class_<TimeDuration> t("TimeDuration", bp::init<>());
    t.def(bp::init<Hash const &>((bp::arg("hash"))));
    bp::implicitly_convertible<Hash const &, TimeDuration>();
    t.def(bp::init<long long unsigned int, long long unsigned int>((bp::arg("seconds"), bp::arg("fractions"))));
    t.def(bp::init<int, int, int, long long unsigned int, long long unsigned int>(
          (bp::arg("days"), bp::arg("hours"), bp::arg("minutes"), bp::arg("seconds"), bp::arg("fractions"))));
    t.def(bp::self_ns::str(bp::self));
    t.def("set", (TimeDuration & (TimeDuration::*)(TimeValue const, TimeValue const))(&TimeDuration::set),
          (bp::arg("seconds"), bp::arg("fractions")), bp::return_internal_reference<>());

    t.def("set",
          (TimeDuration &
           (TimeDuration::*)(int const, int const, int const, TimeValue const, TimeValue const))(&TimeDuration::set),
          (bp::arg("days"), bp::arg("hours"), bp::arg("minutes"), bp::arg("seconds"), bp::arg("fractions")),
          bp::return_internal_reference<>());

    t.def("add", (TimeDuration & (TimeDuration::*)(TimeValue const, TimeValue const))(&TimeDuration::add),
          (bp::arg("seconds"), bp::arg("fractions")), bp::return_internal_reference<>());

    t.def("add",
          (TimeDuration &
           (TimeDuration::*)(int const, int const, int const, TimeValue const, TimeValue const))(&TimeDuration::add),
          (bp::arg("days"), bp::arg("hours"), bp::arg("minutes"), bp::arg("seconds"), bp::arg("fractions")),
          bp::return_internal_reference<>());

    t.def("sub", (TimeDuration & (TimeDuration::*)(TimeValue const, TimeValue const))(&TimeDuration::sub),
          (bp::arg("seconds"), bp::arg("fractions")), bp::return_internal_reference<>());

    t.def("sub",
          (TimeDuration &
           (TimeDuration::*)(int const, int const, int const, TimeValue const, TimeValue const))(&TimeDuration::sub),
          (bp::arg("days"), bp::arg("hours"), bp::arg("minutes"), bp::arg("seconds"), bp::arg("fractions")),
          bp::return_internal_reference<>());

    t.def("format", (string(TimeDuration::*)(string const &) const)(&TimeDuration::format), (bp::arg("fmt")));

    t.def("fromHash", (void(TimeDuration::*)(Hash const &))(&TimeDuration::fromHash), (bp::arg("hash")));

    t.def("getDays", (unsigned long int (TimeDuration::*)() const)(&TimeDuration::getDays));

    t.def("getFractions",
          (TimeValue(TimeDuration::*)(karabo::util::TIME_UNITS const) const)(&TimeDuration::getFractions),
          (bp::arg("unit") = karabo::util::NANOSEC));

    t.def("getHours", (unsigned long int (TimeDuration::*)() const)(&TimeDuration::getHours));

    t.def("getMinutes", (unsigned long int (TimeDuration::*)() const)(&TimeDuration::getMinutes));

    t.def("getSeconds", (TimeValue(TimeDuration::*)() const)(&TimeDuration::getSeconds));

    t.def("getTotalHours", (TimeValue(TimeDuration::*)() const)(&TimeDuration::getTotalHours));

    t.def("getTotalMinutes", (TimeValue(TimeDuration::*)() const)(&TimeDuration::getTotalMinutes));

    t.def("getTotalSeconds", (TimeValue(TimeDuration::*)() const)(&TimeDuration::getTotalSeconds));

    t.def("isNull", (bool(TimeDuration::*)() const)(&TimeDuration::isNull));

    t.def("setDefaultFormat", (void (*)(string const &))(&TimeDuration::setDefaultFormat), (bp::arg("fmt")));
    t.staticmethod("setDefaultFormat");

    t.def("toHash", (void(TimeDuration::*)(Hash &))(&TimeDuration::toHash), (bp::arg("hash")));

    t.def(bp::self != bp::self);
    t.def(bp::self + bp::self);
    t.def(bp::self += bp::self);
    t.def(bp::self - bp::self);
    t.def(bp::self -= bp::self);
    t.def(bp::self / bp::self);
    t.def(bp::self < bp::self);
    t.def(bp::self <= bp::self);
    t.def(bp::self == bp::self);
    t.def(bp::self > bp::self);
    t.def(bp::self >= bp::self);
}
