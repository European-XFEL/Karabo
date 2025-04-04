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
#include "karabo/data/schema/StateElement.hh"
#include "karabo/data/types/State.hh"


namespace py = pybind11;
using namespace karabo::data;
using namespace std;
using namespace karabind;


void exportPyUtilStateElement(py::module_& m) {
    py::class_<StateElement> se(m, "STATE_ELEMENT");

    se.def(py::init<Schema&>(), py::arg("expected"));

    se.def(
          "alias",
          [](StateElement& self, const py::object& alias) {
              return wrapper::AliasAttributePy<StateElement>::setAlias(self, alias);
          },
          py::arg("alias"), py::return_value_policy::reference_internal);

    se.def("commit", &StateElement::commit);

    se.def("description", &StateElement::description, py::return_value_policy::reference_internal);

    se.def("displayedName", &StateElement::displayedName, py::return_value_policy::reference_internal);

    se.def("key", &StateElement::key, py::return_value_policy::reference_internal);

    se.def("tags", (StateElement & (StateElement::*)(std::string const&, std::string const&)) & StateElement::tags,
           py::arg("tags"), py::arg("sep") = " ,;", py::return_value_policy::reference_internal);

    se.def("tags", (StateElement & (StateElement::*)(std::vector<std::string> const&)) & StateElement::tags,
           py::arg("tags"), py::return_value_policy::reference_internal);

    se.def(
          "options",
          [](StateElement& self, py::args args) {
              std::vector<State> states;
              for (unsigned int i = 0; i < py::len(args); ++i) {
                  const auto state = args[i].attr("name").cast<std::string>();
                  states.push_back(State::fromString(state));
              }
              self.options(states);
              return self;
          },
          py::return_value_policy::reference_internal);

    se.def("daqPolicy", &StateElement::daqPolicy, py::return_value_policy::reference_internal);

    auto initialValue = [](StateElement& self, const py::object& value) {
        const std::string className = value.attr("__class__").attr("__name__").cast<std::string>();
        if (className != "State") throw KARABO_PYTHON_EXCEPTION("defaultValue expects parameter of type State.");
        const std::string state = value.attr("name").cast<std::string>();
        return self.initialValue(State::fromString(state));
    };

    se.def("defaultValue", initialValue, py::arg("value"), py::return_value_policy::reference_internal);

    se.def("initialValue", initialValue, py::arg("value"), py::return_value_policy::reference_internal);

    // py::implicitly_convertible<Schema &, StateElement>();
}
