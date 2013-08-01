/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/util/Timestamp.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

void exportPyUtilTimestamp() {
       
bp::class_<Timestamp> ts("Timestamp", bp::init<>());
    ts.def(bp::init<Epochstamp const &, Trainstamp const &>((bp::arg("e"), bp::arg("t"))));
    
    ts.def("getSeconds"
           , (unsigned long long const & (Timestamp::*)() const)(&Timestamp::getSeconds)
           , bp::return_value_policy< bp::copy_const_reference >() );
    
    ts.def("getFractionalSeconds"
           , (unsigned long long const & (Timestamp::*)() const)(&Timestamp::getFractionalSeconds)
           , bp::return_value_policy< bp::copy_const_reference >() );
    
    ts.def("getTrainId"
           , (unsigned long long const & (Timestamp::*)() const)(&Timestamp::getTrainId)
           , bp::return_value_policy< bp::copy_const_reference >() );
    
    ts.def("fromHashAttributes"
           , &Timestamp::fromHashAttributes
           , bp::arg("attributes") );
    ts.staticmethod("fromHashAttributes");
    
    ts.def("hashAttributesContainTimeInformation"
           , &Timestamp::hashAttributesContainTimeInformation
           , bp::arg("attributes") );
    ts.staticmethod("hashAttributesContainTimeInformation");
    
    ts.def("fromIso8601"
          , &Timestamp::toIso8601
          , bp::arg("timePoint"));
    ts.staticmethod("fromIso8601");
    
    ts.def("fromIso8601Ext"
          , &Timestamp::toIso8601Ext
          , bp::arg("timePoint"));
    ts.staticmethod("fromIso8601Ext");
    
    ts.def("toIso8601"
           , (string (Timestamp::*)() const)(&Timestamp::toIso8601)
           , bp::arg("precision")
           , bp::arg("extended"));
    
    ts.def("toIso8601Ext"
           , (string (Timestamp::*)() const)(&Timestamp::toIso8601Ext)
           , bp::arg("precision")
           , bp::arg("extended"));
    
    ts.def("toFormattedString"
           , (string (Timestamp::*)(const string &) const)(&Timestamp::toFormattedString)
           , bp::arg("format") );
    
    ts.def("toHashAttributes"
           , (void (Timestamp::*)(Hash::Attributes &) const)(&Timestamp::toHashAttributes)
           , bp::arg("attributes") );
    
}