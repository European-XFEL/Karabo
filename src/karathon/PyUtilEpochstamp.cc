/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/util/Epochstamp.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

void exportPyUtilEpochstamp() {

    bp::class_<Epochstamp> e("Epochstamp", bp::init<>());
    e.def(bp::init<const unsigned long long&, const unsigned long long&>((bp::arg("seconds"), bp::arg("fraction"))));

    e.def("getSeconds"
          , (unsigned long long const & (Epochstamp::*)() const)(&Epochstamp::getSeconds)
          , bp::return_value_policy< bp::copy_const_reference >());

    e.def("getFractionalSeconds"
          , (unsigned long long const & (Epochstamp::*)() const)(&Epochstamp::getFractionalSeconds)
          , bp::return_value_policy< bp::copy_const_reference >());

    e.def("fromIso8601"
          , &Epochstamp::toIso8601
          , bp::arg("timePoint"));
    e.staticmethod("fromIso8601");
    
    e.def("fromHashAttributes"
           , &Epochstamp::fromHashAttributes
           , bp::arg("attributes") );
    e.staticmethod("fromHashAttributes");
    
    e.def("hashAttributesContainTimeInformation"
           , &Epochstamp::hashAttributesContainTimeInformation
           , bp::arg("attributes") );
    e.staticmethod("hashAttributesContainTimeInformation");
    
    e.def("toIso8601"
          , (string(Epochstamp::*)() const) (&Epochstamp::toIso8601));


}


