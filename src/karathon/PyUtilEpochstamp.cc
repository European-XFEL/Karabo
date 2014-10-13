/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/util/Epochstamp.hh>
#include <karabo/util/TimeDuration.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

void exportPyUtilEpochstamp() {

#if __LP64__ == 1
    bp::enum_< karabo::util::TIME_UNITS>("TIME_UNITS")
            .value("ATTOSEC", karabo::util::ATTOSEC)
            .value("FEMTOSEC", karabo::util::FEMTOSEC)
            .value("PICOSEC", karabo::util::PICOSEC)
            .value("NANOSEC", karabo::util::NANOSEC)
            .value("MICROSEC", karabo::util::MICROSEC)
            .value("MILLISEC", karabo::util::MILLISEC)
            .value("ONESECOND", karabo::util::ONESECOND)
            .value("SECOND", karabo::util::SECOND)
            .value("MINUTE", karabo::util::MINUTE)
            .value("HOUR", karabo::util::HOUR)
            .value("DAY", karabo::util::DAY)
            .export_values()
            ;
#endif
    
    bp::class_<Epochstamp> e("Epochstamp", bp::init<>());
    e.def(bp::init<const unsigned long long&, const unsigned long long&>((bp::arg("seconds"), bp::arg("fractions"))));
    e.def(bp::init< time_t const & >((bp::arg("tm"))));
    bp::implicitly_convertible< time_t const &, Epochstamp >();

    e.def(bp::init< timeval const & >((bp::arg("tv"))));
    bp::implicitly_convertible< timeval const &, Epochstamp >();

    e.def(bp::init< timespec const & >((bp::arg("ts"))));
    bp::implicitly_convertible< timespec const &, Epochstamp >();

    e.def(bp::init< string const & >((bp::arg("pTimeStr"))));
    bp::implicitly_convertible< string const &, Epochstamp >();

    e.def("getSeconds"
            , (unsigned long long const & (Epochstamp::*)() const) (&Epochstamp::getSeconds)
            , bp::return_value_policy< bp::copy_const_reference >());

    e.def("getFractionalSeconds"
            , (unsigned long long const & (Epochstamp::*)() const) (&Epochstamp::getFractionalSeconds)
            , bp::return_value_policy< bp::copy_const_reference >());

    e.def("getTime"
            , (time_t(Epochstamp::*)() const) (&Epochstamp::getTime));

    e.def("getTimeOfDay"
            , (timeval(Epochstamp::*)() const) (&Epochstamp::getTimeOfDay));

    e.def("getClockTime"
            , (timespec(Epochstamp::*)() const) (&Epochstamp::getClockTime));

    typedef void ( ::karabo::util::Epochstamp::*now_function_type)();

    e.def("now"
            , (void (Epochstamp::*)())(&Epochstamp::now));

    e.def("elapsed"
            , (TimeDuration(Epochstamp::*)(Epochstamp const &) const) (&Epochstamp::elapsed)
            , (bp::arg("other") = karabo::util::Epochstamp()));

    e.def("fromHashAttributes"
            , &Epochstamp::fromHashAttributes
            , bp::arg("attributes"));
    e.staticmethod("fromHashAttributes");

    e.def("toHashAttributes"
            , &Epochstamp::toHashAttributes
            , bp::arg("attributes"));

    e.def("hashAttributesContainTimeInformation"
            , &Epochstamp::hashAttributesContainTimeInformation
            , bp::arg("attributes"));
    e.staticmethod("hashAttributesContainTimeInformation");

    e.def("toIso8601"
            , (string(Epochstamp::*)(karabo::util::TIME_UNITS, bool) const) (&Epochstamp::toIso8601)
            , (bp::arg("precision") = karabo::util::MICROSEC, bp::arg("extended") = (bool)(false)));

    e.def("toIso8601Ext"
            , (string(Epochstamp::*)(karabo::util::TIME_UNITS, bool) const) (&Epochstamp::toIso8601Ext)
            , (bp::arg("precision") = karabo::util::MICROSEC, bp::arg("extended") = (bool)(false)));

    e.def("toTimestamp"
            , (double (Epochstamp::*)() const) (&Epochstamp::toTimestamp));

    e.def("toFormattedString"
            , (string(Epochstamp::*)(const string &, const string &) const) (&Epochstamp::toFormattedString)
            , (bp::arg("format") = "%Y-%b-%d %H:%M:%S", bp::arg("localTimeZone") = "Z"));

    e.def("toFormattedStringLocale"
            , (string(Epochstamp::*)(const string &, const string &, const string &) const) (&Epochstamp::toFormattedStringLocale)
            , (bp::arg("localeName") = "", bp::arg("format") = "%Y-%b-%d %H:%M:%S", bp::arg("localTimeZone") = "Z"));

    e.def(bp::self != bp::self);
    e.def(bp::self + bp::other< karabo::util::TimeDuration >());
    e.def(bp::self += bp::other< karabo::util::TimeDuration >());
    e.def(bp::self - bp::other< karabo::util::TimeDuration >());
    e.def(bp::self - bp::self);
    e.def(bp::self -= bp::other< karabo::util::TimeDuration >());
    e.def(bp::self < bp::self);
    e.def(bp::self <= bp::self);

    //karabo::util::Epochstamp::operator=       
    e.def("assign"
            , (Epochstamp & (Epochstamp::*)(time_t const &))(&Epochstamp::operator=)
            , (bp::arg("tm"))
            , bp::return_self< >());


    //karabo::util::Epochstamp::operator=    
    e.def("assign"
            , (Epochstamp & (Epochstamp::*)(timeval const &)) (&Epochstamp::operator=)
            , (bp::arg("tv"))
            , bp::return_self< >());


    //::karabo::util::Epochstamp::operator=        
    e.def("assign"
            , (Epochstamp & (Epochstamp::*)(timespec const &))(&Epochstamp::operator=)
            , (bp::arg("ts"))
            , bp::return_self< >());

    e.def(bp::self == bp::self);
    e.def(bp::self > bp::self);
    e.def(bp::self >= bp::self);

}


