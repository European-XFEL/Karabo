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
#include "karabo/data/schema/ChoiceElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/data/types/Exception.hh"


namespace py = pybind11;
using namespace karabo::data;
using namespace std;
using namespace karabind;


void exportPyUtilSchemaChoiceElement(py::module_& m) {
    ///////////////////////////////////////////////////////////////////////////
    //  DefaultValue<ChoiceElement>
    ///////////////////////////////////////////////////////////////////////////
    {
        typedef DefaultValue<ChoiceElement, string> DefChoiceElement;
        py::class_<DefChoiceElement>(m, "DefaultValueChoiceElement")
              .def("defaultValue", &DefChoiceElement::defaultValue, py::arg("defValue"),
                   py::return_value_policy::reference_internal)
              .def("defaultValueFromString", &DefChoiceElement::defaultValueFromString, py::arg("defValue"),
                   py::return_value_policy::reference_internal)
              .def("noDefaultValue", &DefChoiceElement::noDefaultValue, py::return_value_policy::reference_internal);
    }

    //////////////////////////////////////////////////////////////////////
    // Binding ChoiceElement
    // In Python : CHOICE_ELEMENT
    //////////////////////////////////////////////////////////////////////
    {
        py::class_<ChoiceElement>(m, "CHOICE_ELEMENT")
              .def(py::init<Schema&>(), py::arg("expected")) KARABO_PYTHON_NODE_CHOICE(ChoiceElement)
              .def("assignmentMandatory", &ChoiceElement::assignmentMandatory,
                   py::return_value_policy::reference_internal)
              .def("assignmentOptional", &ChoiceElement::assignmentOptional,
                   py::return_value_policy::reference_internal)
              .def(
                    "appendNodesOfConfigurationBase",
                    [](ChoiceElement& self, const py::object& classobj) -> ChoiceElement& {
                        if (!py::isinstance<py::type>(classobj)) {
                            throw KARABO_PYTHON_EXCEPTION(
                                  "Argument 'arg' given in 'appendNodesOfConfigurationBase(arg)' of CHOICE_ELEMENT "
                                  "must be a class in Python");
                        }
                        if (!py::hasattr(classobj, "getSchema")) {
                            throw KARABO_PYTHON_EXCEPTION(
                                  "Class given in 'appendNodesOfConfigurationBase' of CHOICE_ELEMENT must have "
                                  "'getSchema' "
                                  "function");
                        }
                        std::string classid;
                        if (py::hasattr(classobj, "__karabo_cpp_classid__")) {
                            classid = classobj.attr("__karabo_cpp_classid__").cast<std::string>();
                        } else {
                            classid = classobj.attr("__classid__").cast<std::string>();
                        }

                        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
                        Hash& choiceOfNodes = self.getNode().getValue<Hash>();

                        py::object nodeNameList = classobj.attr("getRegisteredClasses")();
                        std::any any;
                        wrapper::castPyToAny(nodeNameList, any);

                        if (any.type() != typeid(std::vector<std::string>)) {
                            throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
                        }
                        const vector<string>& nodeNames = std::any_cast<vector<string>>(any);
                        for (size_t i = 0; i < nodeNames.size(); i++) {
                            std::string nodeName = nodeNames[i];
                            py::object schemaObj = classobj.attr("getSchema")(nodeName, Schema::AssemblyRules());
                            const Schema& schema = schemaObj.cast<const Schema&>();
                            Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
                            node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
                            node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
                            node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                            node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                        }
                        return self;
                    },
                    py::arg("python_base_class"), py::return_value_policy::reference_internal)
              .def(
                    "appendAsNode",
                    [](ChoiceElement& self, const py::object& classobj,
                       const std::string& nodeNameObj) -> ChoiceElement& {
                        if (!py::isinstance<py::type>(classobj)) {
                            throw KARABO_PYTHON_EXCEPTION(
                                  "Argument 'classobj' given in 'appendAsNode(classobj, nodeName)' of CHOICE_ELEMENT "
                                  "must be a class in Python");
                        }
                        if (!py::hasattr(classobj, "getSchema")) {
                            throw KARABO_PYTHON_EXCEPTION(
                                  "Class given in 'appendAsNode(classobj, nodeName)' of CHOICE_ELEMENT has "
                                  "no 'getSchema' method");
                        }
                        std::string classid;
                        if (py::hasattr(classobj, "__karabo_cpp_classid__")) {
                            classid = classobj.attr("__karabo_cpp_classid__").cast<std::string>();
                        } else {
                            classid = classobj.attr("__classid__").cast<std::string>();
                        }
                        if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
                        Hash& choiceOfNodes = self.getNode().getValue<Hash>();
                        string nodeName = nodeNameObj;
                        if (nodeNameObj == "") nodeName = classid;
                        py::object schemaObj = classobj.attr("getSchema")(nodeName, Schema::AssemblyRules());
                        const Schema& schema = schemaObj.cast<const Schema&>();
                        Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
                        node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
                        node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
                        node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                        node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                        return self;
                    },
                    py::arg("python_class"), py::arg("nodeName") = "", py::return_value_policy::reference_internal)
              .def("reconfigurable", &ChoiceElement::reconfigurable, py::return_value_policy::reference_internal)
              .def("init", &ChoiceElement::init, py::return_value_policy::reference_internal);
    }

    // TODO: evaluate if ChoiceElement and Schema should be implicitly convertible
    // py::implicitly_convertible<Schema&, ChoiceElement>();
}
