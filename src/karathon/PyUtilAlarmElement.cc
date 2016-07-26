/*
 * $Id$
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"
#include "boost/python/raw_function.hpp"


#include "PythonMacros.hh"
#include <karabo/util/AlarmConditions.hh>
#include <karabo/util/AlarmConditionElement.hh>


namespace bp = boost::python;
using namespace karabo::util;
using namespace std;



class AlarmConditionElementWrap{
public:
    
    
    static AlarmConditionElement & initialValuePy(AlarmConditionElement& self, const bp::object& value) {
        const std::string className = bp::extract<std::string>(value.attr("__class__").attr("__name__"));
        if(className == "AlarmCondition" ){
            const std::string condition = bp::extract<std::string>(value.attr("name"));
            return self.initialValue(karabo::util::AlarmCondition::fromString(condition));
        } else {
            throw KARABO_PYTHON_EXCEPTION("initialValue() expects parameter of type AlarmCondition.");
        }
    }
    
    
    
};


void exportPyUtilAlarmConditionElement() {
    bp::implicitly_convertible< Schema &, AlarmConditionElement >();
        bp::class_<AlarmConditionElement> ("ALARM_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))
                .def("alias"
                     , &AliasAttributeWrap<AlarmConditionElement>::aliasPy
                     , bp::return_internal_reference<> ())
                .def("commit", &AlarmConditionElement::commit
                     , bp::return_internal_reference<> ())
                .def("commit"
                     , (AlarmConditionElement & (AlarmConditionElement::*)(karabo::util::Schema &))(&AlarmConditionElement::commit)
                     , bp::arg("expected")
                     , bp::return_internal_reference<> ())
                .def("description", &AlarmConditionElement::description
                     , bp::return_internal_reference<> ())
                .def("displayedName", &AlarmConditionElement::displayedName
                     , bp::return_internal_reference<> ())
                .def("key", &AlarmConditionElement::key
                     , bp::return_internal_reference<> ())
                .def("tags"
                     , (AlarmConditionElement & (AlarmConditionElement::*)(std::string const &, std::string const &))(&AlarmConditionElement::tags)
                     , (bp::arg("tags"), bp::arg("sep") = " ,;")
                     , bp::return_internal_reference<> ())
                .def("tags"
                     , (AlarmConditionElement & (AlarmConditionElement::*)(std::vector<std::string> const &))(&AlarmConditionElement::tags)
                     , (bp::arg("tags"))
                     , bp::return_internal_reference<> ())
                .def("initialValue", &AlarmConditionElementWrap::initialValuePy
                     , (bp::arg("value"))
                     , bp::return_internal_reference<> ());
               

}
