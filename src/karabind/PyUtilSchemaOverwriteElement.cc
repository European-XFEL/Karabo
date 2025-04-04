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
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "PyTypes.hh"
#include "PyUtilSchemaElement.hh"
#include "Wrapper.hh"
#include "karabo/data/schema/OverwriteElement.hh"
#include "karabo/data/types/Exception.hh"


namespace py = pybind11;
using namespace karabo::data;
using namespace std;
using namespace karabind;


void exportPyUtilSchemaOverwriteElement(py::module_& m) {
    ///////////////////////////////////////////////////////////////////////////
    //  Binding OverwriteElement
    //  In Python : OVERWRITE_ELEMENT
    ///////////////////////////////////////////////////////////////////////////
    // py::implicitly_convertible<Schema&, OverwriteElement>();
    py::class_<OverwriteElement>(m, "OVERWRITE_ELEMENT")
          .def(py::init<Schema&>(), py::arg("expected"))
          .def("key", &OverwriteElement::key, py::arg("key"), py::return_value_policy::reference_internal)
          .def("setNewDisplayedName", &OverwriteElement::setNewDisplayedName, py::arg("name"),
               py::return_value_policy::reference_internal)
          .def("setNewDescription", &OverwriteElement::setNewDescription, py::arg("description"),
               py::return_value_policy::reference_internal)
          .def(
                "setNewAlias",
                [](OverwriteElement& self, const py::object& alias) {
                    std::any any;
                    wrapper::castPyToAny(alias, any);
                    return self.setNewAlias(any);
                },
                py::arg("alias"), py::return_value_policy::reference_internal)
          .def(
                "setNewTags",
                [](OverwriteElement& self, const py::object& tags) -> OverwriteElement& {
                    if (py::isinstance<py::str>(tags)) {
                        std::string tag = tags.cast<std::string>();
                        return self.setNewTags({tag});
                    }
                    std::vector<std::string> vtags = wrapper::fromPySequenceToVectorString(tags);
                    return self.setNewTags(vtags);
                },
                py::arg("tags"), py::return_value_policy::reference_internal,
                "Overwrite tags, 'tags' can be a str or a sequence of str.")
          .def("setNewAssignmentMandatory", &OverwriteElement::setNewAssignmentMandatory,
               py::return_value_policy::reference_internal)
          .def("setNewAssignmentOptional", &OverwriteElement::setNewAssignmentOptional,
               py::return_value_policy::reference_internal)
          .def("setNewAssignmentInternal", &OverwriteElement::setNewAssignmentInternal,
               py::return_value_policy::reference_internal)
          .def("setNowInit", &OverwriteElement::setNowInit, py::return_value_policy::reference_internal)
          .def("setNowReconfigurable", &OverwriteElement::setNowReconfigurable,
               py::return_value_policy::reference_internal)
          .def("setNowReadOnly", &OverwriteElement::setNowReadOnly, py::return_value_policy::reference_internal)
          .def("setNowValidate", &OverwriteElement::setNowValidate, py::return_value_policy::reference_internal)
          .def("setNowSkipValidation", &OverwriteElement::setNowSkipValidation,
               py::return_value_policy::reference_internal)
          .def(
                "setNewDefaultValue",
                [](OverwriteElement& self, const py::object& value) {
                    const std::string className = value.attr("__class__").attr("__name__").cast<std::string>();
                    if (className == "State") {
                        const std::string state = value.attr("name").cast<std::string>();
                        return self.setNewDefaultValue(karabo::data::State::fromString(state));
                    } else if (className == "AlarmCondition") {
                        const std::string condition = value.attr("value").cast<std::string>();

                        return self.setNewDefaultValue(karabo::data::AlarmCondition::fromString(condition));
                    } else {
                        std::any any;
                        if (className == "AccessLevel") {
                            // Special treatment for Enum: convert to int first, like int(value)
                            // TODO: should we implement Enum conversion to 'int' in `castPyToAny'?
                            wrapper::castPyToAny(value.cast<py::int_>(), any);
                        } else {
                            wrapper::castPyToAny(value, any);
                        }
                        return self.setNewDefaultValue(any);
                    }
                },
                py::arg("value"), py::return_value_policy::reference_internal)
          .def(
                "setNewMinInc",
                [](OverwriteElement& self, const py::object& value) {
                    std::any any;
                    wrapper::castPyToAny(value, any);
                    return self.setNewMinInc(any);
                },
                py::arg("value"), py::return_value_policy::reference_internal)
          .def(
                "setNewMaxInc",
                [](OverwriteElement& self, const py::object& value) {
                    std::any any;
                    wrapper::castPyToAny(value, any);
                    return self.setNewMaxInc(any);
                },
                py::arg("value"), py::return_value_policy::reference_internal)
          .def(
                "setNewMinExc",
                [](OverwriteElement& self, const py::object& value) {
                    std::any any;
                    wrapper::castPyToAny(value, any);
                    return self.setNewMinExc(any);
                },
                py::arg("value"), py::return_value_policy::reference_internal)
          .def(
                "setNewMaxExc",
                [](OverwriteElement& self, const py::object& value) {
                    std::any any;
                    wrapper::castPyToAny(value, any);
                    return self.setNewMaxExc(any);
                },
                py::arg("value"), py::return_value_policy::reference_internal)
          .def(
                "setNewMinSize",
                [](OverwriteElement& self, const py::object& obj) {
                    try {
                        return self.setNewMinSize(wrapper::toInteger<unsigned int>(obj));
                    } catch (const karabo::data::CastException& e) {
                        KARABO_RETHROW_AS(e);
                        return self; // please compiler
                    }
                },
                py::arg("value"), py::return_value_policy::reference_internal)
          .def(
                "setNewMaxSize",
                [](OverwriteElement& self, const py::object& obj) {
                    try {
                        return self.setNewMaxSize(wrapper::toInteger<unsigned int>(obj));
                    } catch (const karabo::data::CastException& e) {
                        KARABO_RETHROW_AS(e);
                        return self; // please compiler
                    }
                },
                py::arg("value"), py::return_value_policy::reference_internal)
          .def(
                "setNewOptions",
                [](OverwriteElement& self, py::args args) {
                    // Check the type of the first arg
                    auto arg0 = *args.begin();
                    if (py::isinstance<py::str>(arg0)) {
                        self.setNewOptions(arg0.cast<std::string>(), ",;");
                        return self;
                    }
                    // try states
                    std::vector<State> states;
                    for (auto arg : args) {
                        const auto className = arg.attr("__class__").attr("__name__").cast<std::string>();
                        if (className == "State") {
                            const std::string state = arg.attr("name").cast<std::string>();
                            states.push_back(State::fromString(state));
                        } else {
                            throw KARABO_PYTHON_EXCEPTION(
                                  "setNewOptions expects either a string or an arbitrary number of arguments "
                                  "of type State.");
                        }
                    }
                    self.setNewOptions(states);
                    return self;
                },
                py::return_value_policy::reference_internal)
          .def(
                "setNewAllowedStates",
                [](OverwriteElement& self, py::args args) {
                    std::vector<State> states;
                    for (auto arg : args) {
                        if (py::isinstance<py::str>(arg)) {
                            const std::string state = arg.attr("name").cast<std::string>();
                            states.push_back(State::fromString(state));
                        } else if (py::isinstance<State>(arg)) {
                            states.push_back(arg.cast<State>());
                        }
                    }
                    self.setNewAllowedStates(states);
                    return self;
                },
                py::return_value_policy::reference_internal)
          .def("setNowObserverAccess", &OverwriteElement::setNowObserverAccess,
               py::return_value_policy::reference_internal)
          .def("setNowUserAccess", &OverwriteElement::setNowUserAccess, py::return_value_policy::reference_internal)
          .def("setNowOperatorAccess", &OverwriteElement::setNowOperatorAccess,
               py::return_value_policy::reference_internal)
          .def("setNowExpertAccess", &OverwriteElement::setNowExpertAccess, py::return_value_policy::reference_internal)
          .def("setNowAdminAccess", &OverwriteElement::setNowAdminAccess, py::return_value_policy::reference_internal)
          .def("setNewUnit", &OverwriteElement::setNewUnit, py::arg("value"),
               py::return_value_policy::reference_internal)
          .def("setNewMetricPrefix", &OverwriteElement::setNewMetricPrefix, py::arg("value"),
               py::return_value_policy::reference_internal)
          .def("commit", &OverwriteElement::commit);
}
