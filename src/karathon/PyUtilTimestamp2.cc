/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/util/Timestamp2.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

void exportPyUtilTimestamp2() {
       
bp::class_<Timestamp2> ts("Timestamp2", bp::init<>());
    ts.def(bp::init<Epochstamp const &, Trainstamp const &>((bp::arg("e"), bp::arg("t"))));
    
    ts.def("getSeconds"
           , (unsigned long long const & (Timestamp2::*)() const)(&Timestamp2::getSeconds)
           , bp::return_value_policy< bp::copy_const_reference >() );
    
    ts.def("getFractionalSeconds"
           , (unsigned long long const & (Timestamp2::*)() const)(&Timestamp2::getFractionalSeconds)
           , bp::return_value_policy< bp::copy_const_reference >() );
    
    ts.def("getTrainId"
           , (unsigned long long const & (Timestamp2::*)() const)(&Timestamp2::getTrainId)
           , bp::return_value_policy< bp::copy_const_reference >() );
    
    ts.def("fromHashAttributes"
           , &Timestamp2::fromHashAttributes
           , bp::arg("attributes") );
    ts.staticmethod("fromHashAttributes");
    
    ts.def("hashAttributesContainTimeInformation"
           , &Timestamp2::hashAttributesContainTimeInformation
           , bp::arg("attributes") );
    ts.staticmethod("hashAttributesContainTimeInformation");
    
    ts.def("toIso8601"
           , (string (Timestamp2::*)() const)(&Timestamp2::toIso8601));
    
    ts.def("toHashAttributes"
           , (void (Timestamp2::*)(Hash::Attributes &) const)(&Timestamp2::toHashAttributes)
           , bp::arg("attributes") );
    
    ts.def("toFormattedString"
           , (string (Timestamp2::*)(const string &) const)(&Timestamp2::toFormattedString)
           , bp::arg("format") );
}