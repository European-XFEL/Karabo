/*
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

#include <pybind11/pybind11.h>

#include "Wrapper.hh"
#include "karabo/data/schema/AlarmConditionElement.hh"
#include "karabo/data/types/AlarmCondition.hh"


namespace py = pybind11;
using namespace karabo::data;
using namespace std;
using namespace karabind;


void exportPyUtilAlarmConditionElement(py::module_& m) {
    // py::implicitly_convertible<Schema &, AlarmConditionElement>();

    py::class_<AlarmConditionElement> a(m, "ALARM_ELEMENT");

    a.def(py::init<Schema&>(), py::arg("expected"));

    a.def(
          "alias",
          [](AlarmConditionElement& self, const py::object& alias) {
              return wrapper::AliasAttributePy<AlarmConditionElement>::setAlias(self, alias);
          },
          py::arg("alias"), py::return_value_policy::reference_internal);

    a.def("commit", &AlarmConditionElement::commit);

    a.def("description", &AlarmConditionElement::description, py::return_value_policy::reference_internal);

    a.def("displayedName", &AlarmConditionElement::displayedName, py::return_value_policy::reference_internal);

    a.def("key", &AlarmConditionElement::key, py::arg("name"), py::arg("strict") = true,
          py::return_value_policy::reference_internal);

    a.def("tags",
          (AlarmConditionElement & (AlarmConditionElement::*)(std::string const&, std::string const&)) &
                AlarmConditionElement::tags,
          py::arg("tags"), py::arg("sep") = " ,;", py::return_value_policy::reference_internal);

    a.def("tags",
          (AlarmConditionElement & (AlarmConditionElement::*)(std::vector<std::string> const&)) &
                AlarmConditionElement::tags,
          py::arg("tags"), py::return_value_policy::reference_internal);

    a.def(
          "defaultValue",
          [](AlarmConditionElement& self, const py::object& value) {
              const std::string className = value.attr("__class__").attr("__name__").cast<std::string>();
              if (className != "AlarmCondition") {
                  throw KARABO_PYTHON_EXCEPTION("initialValue() expects parameter of type AlarmCondition.");
              }
              const std::string condition = value.attr("asString")().cast<std::string>();
              return self.initialValue(AlarmCondition::fromString(condition));
          },
          py::arg("value"), py::return_value_policy::reference_internal);

    a.def(
          "initialValue",
          [](AlarmConditionElement& self, const py::object& value) {
              const std::string className = value.attr("__class__").attr("__name__").cast<std::string>();
              if (className != "AlarmCondition") {
                  throw KARABO_PYTHON_EXCEPTION("initialValue() expects parameter of type AlarmCondition.");
              }
              const std::string condition = value.attr("asString")().cast<std::string>();
              return self.initialValue(AlarmCondition::fromString(condition));
          },
          py::arg("value"), py::return_value_policy::reference_internal);
}
