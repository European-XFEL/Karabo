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

#include <karabo/io/InputElement.hh>
#include <karabo/io/OutputElement.hh>
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
using namespace karabo::io;
using namespace karabo::util;
using namespace std;
using namespace karabind;


namespace karabind {

    struct InputElementWrap {
        static InputElement& setInputType(InputElement& self, const py::object& classobj) {
            if (!PyType_Check(classobj.ptr())) {
                throw KARABO_PYTHON_EXCEPTION(
                      "Argument 'classobj' given in 'setInputType(classobj)' of INPUT_ELEMENT must be a class in "
                      "Python");
            }
            if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
                throw KARABO_PYTHON_EXCEPTION(
                      "Class given in 'setInputType(classobj)' of INPUT_ELEMENT has no 'getSchema' method");
            }
            std::string classid;
            if (PyObject_HasAttrString(classobj.ptr(), "__karabo_cpp_classid__")) {
                classid = classobj.attr("__karabo_cpp_classid__").cast<std::string>();
            } else {
                classid = classobj.attr("__classid__").cast<std::string>();
            }
            if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
            Hash& choiceOfNodes = self.getNode().getValue<Hash>();

            py::object nodeNameList = classobj.attr("getRegisteredClasses")();
            std::any any;
            karabind::wrapper::castPyToAny(nodeNameList, any);

            if (any.type() != typeid(vector<string>))
                throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
            const vector<string>& nodeNames = std::any_cast<vector<string>>(any);
            for (size_t i = 0; i < nodeNames.size(); i++) {
                std::string nodeName = nodeNames[i];
                py::object schemaObj = classobj.attr("getSchema")(nodeName);
                const Schema& schema = schemaObj.cast<const Schema&>();
                Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
                node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
                node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Input-" + nodeName);
                node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
            }
            return self;
        }
    };


    struct OutputElementWrap {
        static OutputElement& setOutputType(OutputElement& self, const py::object& classobj) {
            if (!PyType_Check(classobj.ptr())) {
                throw KARABO_PYTHON_EXCEPTION(
                      "Argument 'classobj' given in 'setOutputType(classobj)' of OUTPUT_ELEMENT must be a class in "
                      "Python");
            }
            if (!PyObject_HasAttrString(classobj.ptr(), "getSchema")) {
                throw KARABO_PYTHON_EXCEPTION(
                      "Class given in 'setOutputType(classobj)' of OUTPUT_ELEMENT has no 'getSchema' method");
            }
            std::string classid;
            if (PyObject_HasAttrString(classobj.ptr(), "__karabo_cpp_classid__")) {
                classid = classobj.attr("__karabo_cpp_classid__").cast<std::string>();
            } else {
                classid = classobj.attr("__classid__").cast<std::string>();
            }
            if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
            // Retrieve reference for filling
            Hash& choiceOfNodes = self.getNode().getValue<Hash>();

            py::object nodeNameList = classobj.attr("getRegisteredClasses")();
            std::any any;
            karabind::wrapper::castPyToAny(nodeNameList, any);

            if (any.type() != typeid(vector<string>))
                throw KARABO_PYTHON_EXCEPTION("getRegisteredClasses() doesn't return vector<string>!");
            const vector<string>& nodeNames = std::any_cast<vector<string>>(any);
            for (size_t i = 0; i < nodeNames.size(); i++) {
                std::string nodeName = nodeNames[i];
                py::object schemaObj = classobj.attr("getSchema")(nodeName);
                const Schema& schema = schemaObj.cast<const Schema&>();
                Hash::Node& node = choiceOfNodes.set<Hash>(nodeName, schema.getParameterHash());
                node.setAttribute(KARABO_SCHEMA_CLASS_ID, nodeName);
                node.setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Output-" + nodeName);
                node.setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::NODE);
                node.setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
            }
            return self;
        }
    };
} // namespace karabind


void exportPyUtilSchemaNodeElement(py::module_& m) {
    //////////////////////////////////////////////////////////////////////
    // Binding NodeElement
    // In Python : NODE_ELEMENT
    //////////////////////////////////////////////////////////////////////
    py::class_<NodeElement>(m, "NODE_ELEMENT")
          .def(py::init<Schema&>(), py::arg("expected")) KARABO_PYTHON_NODE_CHOICE_LIST(NodeElement)
          .def(
                "appendParametersOf",
                [](NodeElement& self, const py::object& obj) -> NodeElement& {
                    if (!PyType_Check(obj.ptr())) {
                        throw KARABO_PYTHON_EXCEPTION(
                              "Argument 'arg' given in 'appendParametersOf(arg)' of "
                              "NODE_ELEMENT must be a class in Python");
                    }
                    std::string classId(obj.attr("__name__").cast<std::string>());
                    auto schema = std::make_shared<Schema>();
                    obj.attr("expectedParameters")(schema);
                    self.getNode().setValue<Hash>(schema->getParameterHash());
                    self.getNode().setAttribute(KARABO_SCHEMA_CLASS_ID, classId);
                    self.getNode().setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, classId);
                    return self;
                },
                py::arg("python_class"), py::return_value_policy::reference_internal)
          .def(
                "appendParametersOfConfigurableClass",
                [](NodeElement& self, const py::object& baseobj, const std::string& classId) -> NodeElement& {
                    if (!py::isinstance<py::type>(baseobj)) {
                        throw KARABO_PYTHON_EXCEPTION(
                              "Argument 'arg1' given in 'appendParametersOfConfigurableClass(arg1, arg2)' of "
                              "NODE_ELEMENT must be a class in Python registered as base class in Configurator");
                    }
                    if (!py::hasattr(baseobj, "getSchema")) {
                        throw KARABO_PYTHON_EXCEPTION("Class with classid = '" + classId +
                                                      "' given in 'appendParametersOfConfigurableClass"
                                                      "(base, classid)' of NODE_ELEMENT has no 'getSchema' method.");
                    }
                    std::string baseClassId;
                    if (py::hasattr(baseobj, "__karabo_cpp_classid__")) {
                        // baseobj is object of C++ base class
                        baseClassId = baseobj.attr("__karabo_cpp_classid__").cast<std::string>();
                    } else {
                        baseClassId = baseobj.attr("__classid__").cast<std::string>();
                    }
                    if (self.getNode().getType() != Types::HASH) self.getNode().setValue(Hash());
                    self.getNode().setAttribute(KARABO_SCHEMA_CLASS_ID, classId);
                    self.getNode().setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, baseClassId);

                    py::object schemaObj = baseobj.attr("getSchema")(classId);

                    const Schema schema = schemaObj.cast<Schema>();
                    const Hash h = schema.getParameterHash();
                    self.getNode().setValue<Hash>(h);
                    return self;
                },
                py::arg("python_base_class"), py::arg("classid"), py::return_value_policy::reference_internal)
          .def(
                "appendSchema",
                [](NodeElement& self, const Schema& schemaPy) -> NodeElement& {
                    const Hash h = schemaPy.getParameterHash();
                    self.getNode().setValue<Hash>(h);
                    return self;
                },
                py::arg("schema"), py::return_value_policy::reference_internal)
          .def(
                "setDaqDataType",
                [](NodeElement& self, const DaqDataType& dataType) -> NodeElement& {
                    return self.setDaqDataType(dataType);
                },
                py::arg("dataType"), py::return_value_policy::reference_internal)
          .def(
                "setSpecialDisplayType",
                [](NodeElement& self, const std::string& displayType) -> NodeElement& {
                    self.setSpecialDisplayType(displayType);
                    return self;
                },
                py::arg("displayType"), py::return_value_policy::reference_internal)
          .def(
                "setAllowedActions",
                [](NodeElement& self, const py::object& actions) -> NodeElement& {
                    // Accept any Python sequence (list, tuple, set, etc) that provides strings
                    self.setAllowedActions(wrapper::fromPySequenceToVectorString(actions));
                    return self;
                },
                py::arg("actions"), py::return_value_policy::reference_internal, R"pbdoc(
                    Specify one or more actions that are allowed on this node.
                    If a Karabo device specifies allowed actions for a node,
                    that means that it offers a specific slot interface to operate
                    on this node. Which allowed actions require which interface
                    is defined elsewhere
                )pbdoc");

    // TODO: evaluate if implicitly_convertible should be applied
    // py::implicitly_convertible<Schema&, NodeElement>();


    //////////////////////////////////////////////////////////////////////
    // Binding InputElement
    // In Python : INPUT_ELEMENT
    {
        // bp::implicitly_convertible<Schema&, InputElement>();
        py::class_<InputElement>(m, "INPUT_ELEMENT")
              .def(py::init<Schema&>(), py::arg("expected")) KARABO_PYTHON_NODE_CHOICE_LIST(InputElement)
              .def("setInputType", &InputElementWrap::setInputType, py::arg("python_base_class"),
                   py::return_value_policy::reference_internal);
    }

    //////////////////////////////////////////////////////////////////////
    // Binding OutputElement
    // In Python : OUTPUT_ELEMENT
    {
        // py::implicitly_convertible<Schema&, OutputElement>();
        py::class_<OutputElement>(m, "OUTPUT_ELEMENT")
              .def(py::init<Schema&>(), py::arg("expected")) KARABO_PYTHON_NODE_CHOICE_LIST(OutputElement)
              .def("setOutputType", &OutputElementWrap::setOutputType, py::arg("python_base_class"),
                   py::return_value_policy::reference_internal);
    }
}
