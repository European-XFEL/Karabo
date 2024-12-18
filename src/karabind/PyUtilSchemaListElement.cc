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

#include <karabo/util/ByteArrayElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/Exception.hh>
#include <karabo/util/ListElement.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>

#include "PyTypes.hh"
#include "PyUtilSchemaElement.hh"
#include "Wrapper.hh"


namespace py = pybind11;
using namespace karabo::util;
using namespace std;
using namespace karabind;


void exportPyUtilSchemaListElement(py::module_& m) {
    ///////////////////////////////////////////////////////////////////////////
    //  DefaultValue<ListElement>
    ///////////////////////////////////////////////////////////////////////////

    typedef DefaultValue<ListElement, vector<string>> DefListElement;

    py::class_<DefListElement>(m, "DefaultValueListElement")
          .def(
                "defaultValue",
                [](DefListElement& self, const py::object& obj) -> ListElement& {
                    if (py::isinstance<py::list>(obj)) {
                        const std::vector<std::string>& v = obj.cast<std::vector<std::string>>();
                        return self.defaultValue(v);
                    } else {
                        throw KARABO_PYTHON_EXCEPTION(
                              "Python type of the defaultValue of LIST_ELEMENT must be a list of strings");
                    }
                },
                py::arg("pyList"), py::return_value_policy::reference_internal)
          .def("defaultValueFromString", &DefListElement::defaultValueFromString, py::arg("defValue"),
               py::return_value_policy::reference_internal)
          .def("noDefaultValue", &DefListElement::noDefaultValue, py::return_value_policy::reference_internal);


    //////////////////////////////////////////////////////////////////////
    // Binding ListElement
    // In Python : LIST_ELEMENT
    //////////////////////////////////////////////////////////////////////

    py::class_<ListElement>(m, "LIST_ELEMENT")
          .def(py::init<Schema&>(), py::arg("expected")) KARABO_PYTHON_NODE_CHOICE_LIST(ListElement)
          .def("assignmentMandatory", &ListElement::assignmentMandatory, py::return_value_policy::reference_internal)
          .def("assignmentOptional", &ListElement::assignmentOptional, py::return_value_policy::reference_internal)
          .def("min", &ListElement::min, py::return_value_policy::reference_internal)
          .def("max", &ListElement::max, py::return_value_policy::reference_internal)
          .def(
                "appendNodesOfConfigurationBase",
                [](ListElement& self, const py::object& classobj) -> ListElement& {
                    if (!py::isinstance<py::type>(classobj)) {
                        throw KARABO_PYTHON_EXCEPTION(
                              "Argument 'arg' given in 'appendNodesOfConfigurationBase(arg)' of LIST_ELEMENT must be "
                              "a class in Python");
                    }
                    if (!py::hasattr(classobj, "getSchema")) {
                        throw KARABO_PYTHON_EXCEPTION(
                              "Class given in 'appendNodesOfConfigurationBase' of LIST_ELEMENT has no 'getSchema' "
                              "method");
                    }
                    std::string classid;
                    if (py::hasattr(classobj, "__karabo_cpp_classid__")) {
                        classid = classobj.attr("__karabo_cpp_classid__").cast<std::string>();
                    } else {
                        classid = classobj.attr("__classid__").cast<std::string>();
                    }

                    if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
                    Hash& listOfNodes = self.getNode().getValue<Hash>();

                    py::object nodeNameList = classobj.attr("getRegisteredClasses")();
                    std::any any;
                    wrapper::castPyToAny(nodeNameList, any);

                    if (any.type() != typeid(std::vector<std::string>)) {
                        throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
                    }
                    const std::vector<std::string>& nodeNames = std::any_cast<std::vector<std::string>>(any);
                    for (size_t i = 0; i < nodeNames.size(); i++) {
                        const std::string& nodeName = nodeNames[i];
                        py::object schemaObj = classobj.attr("getSchema")(nodeName, Schema::AssemblyRules());
                        Schema& schema = schemaObj.cast<Schema&>();
                        Hash::Node& node = listOfNodes.set<Hash>(nodeName, schema.getParameterHash());
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
                [](ListElement& self, const py::object& classobj, const std::string& name) -> ListElement& {
                    if (!py::isinstance<py::type>(classobj)) {
                        throw KARABO_PYTHON_EXCEPTION(
                              "Argument 'classobj' given in 'appendAsNode(classobj, nodeName)' of LIST_ELEMENT must"
                              " be a class in Python");
                    }
                    if (!py::hasattr(classobj, "getSchema")) {
                        throw KARABO_PYTHON_EXCEPTION(
                              "Class given in 'appendAsNode(classobj, nodeName)' of LIST_ELEMENT has no 'getSchema' "
                              "method");
                    }
                    std::string classid;
                    if (py::hasattr(classobj, "__karabo_cpp_classid__")) {
                        classid = classobj.attr("__karabo_cpp_classid__").cast<std::string>();
                    } else {
                        classid = classobj.attr("__classid__").cast<std::string>();
                    }
                    if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
                    Hash& listOfNodes = self.getNode().getValue<Hash>();
                    std::string nodeName = name;
                    if (nodeName == "") nodeName = classid;
                    py::object schemaObj = classobj.attr("getSchema")(nodeName, Schema::AssemblyRules());
                    const Schema& schema = schemaObj.cast<const Schema&>();
                    Hash::Node& node = listOfNodes.set<Hash>(nodeName, schema.getParameterHash());
                    node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
                    node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, nodeName);
                    node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                    node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                    return self;
                },
                py::arg("python_class"), py::arg("nodeName") = "", py::return_value_policy::reference_internal)
          .def("init", &ListElement::init, py::return_value_policy::reference_internal)
          .def("reconfigurable", &ListElement::reconfigurable, py::return_value_policy::reference_internal)
          .def("setSpecialDisplayType", &ListElement::setSpecialDisplayType,
               py::return_value_policy::reference_internal);

    // TODO: evaluate if ListEelement and Schema should be implicitly convertible
    // py::implicitly_convertible<Schema&, ListElement>();
}
