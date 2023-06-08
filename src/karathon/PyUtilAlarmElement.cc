/*
 * $Id$
 *
 * Author: <steffen.hauf@xfel.eu>
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

#include <karabo/util/AlarmConditionElement.hh>
#include <karabo/util/AlarmConditions.hh>

#include "PythonMacros.hh"
#include "boost/python.hpp"
#include "boost/python/raw_function.hpp"


namespace bp = boost::python;
using namespace karabo::util;
using namespace std;


class AlarmConditionElementWrap {
   public:
    static AlarmConditionElement &initialValuePy(AlarmConditionElement &self, const bp::object &value) {
        const std::string className = bp::extract<std::string>(value.attr("__class__").attr("__name__"));
        if (className == "AlarmCondition") {
            const std::string condition = bp::extract<std::string>(value.attr("asString")());
            return self.initialValue(karabo::util::AlarmCondition::fromString(condition));
        } else {
            throw KARABO_PYTHON_EXCEPTION("initialValue() expects parameter of type AlarmCondition.");
        }
    }
};


void exportPyUtilAlarmConditionElement() {
    bp::implicitly_convertible<Schema &, AlarmConditionElement>();
    bp::class_<AlarmConditionElement>("ALARM_ELEMENT", bp::init<Schema &>((bp::arg("expected"))))
          .def("alias", &AliasAttributeWrap<AlarmConditionElement>::aliasPy, bp::return_internal_reference<>())
          .def("commit", &AlarmConditionElement::commit, bp::return_internal_reference<>())
          .def("commit",
               (AlarmConditionElement &
                (AlarmConditionElement::*)(karabo::util::Schema &))(&AlarmConditionElement::commit),
               bp::arg("expected"), bp::return_internal_reference<>())
          .def("description", &AlarmConditionElement::description, bp::return_internal_reference<>())
          .def("displayedName", &AlarmConditionElement::displayedName, bp::return_internal_reference<>())
          .def("key", &AlarmConditionElement::key, bp::return_internal_reference<>())
          .def("tags",
               (AlarmConditionElement &
                (AlarmConditionElement::*)(std::string const &, std::string const &))(&AlarmConditionElement::tags),
               (bp::arg("tags"), bp::arg("sep") = " ,;"), bp::return_internal_reference<>())
          .def("tags",
               (AlarmConditionElement &
                (AlarmConditionElement::*)(std::vector<std::string> const &))(&AlarmConditionElement::tags),
               (bp::arg("tags")), bp::return_internal_reference<>())
          .def("defaultValue", &AlarmConditionElementWrap::initialValuePy, (bp::arg("value")),
               bp::return_internal_reference<>())
          .def("initialValue", &AlarmConditionElementWrap::initialValuePy, (bp::arg("value")),
               bp::return_internal_reference<>());
}
