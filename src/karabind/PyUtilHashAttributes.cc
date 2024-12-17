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
#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <karabo/util/FromLiteral.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <string>

#include "PyTypes.hh"
#include "Wrapper.hh"

namespace py = pybind11;

using namespace karabo::util;


void exportPyUtilHashAttributes(py::module_& m) {
    using namespace karabo::util;
    using namespace karabind;

    py::class_<Hash::Attributes::Node, std::shared_ptr<Hash::Attributes::Node>> an(m, "HashAttributesNode");

    an.def(
          "getKey", [](const Hash::Attributes::Node& self) -> std::string { return self.getKey(); },
          "Get key of current node in attribute's container");

    an.def("__str__", [](const Hash::Attributes::Node& self) -> std::string { return self.getKey(); });

    an.def(
          "setValue",
          [](Hash::Attributes::Node& self, const py::object& o) {
              boost::any any;
              wrapper::castPyToAny(o, any);
              self.setValue(any);
          },
          py::arg("value"), "Set value for current node in attribute's container");

    an.def(
          "getValue", [](const Hash::Attributes::Node& self) { return wrapper::castAnyToPy(self.getValueAsAny()); },
          "Get value for current node in attribute's container");

    an.def(
          "getValueAs",
          [](const Hash::Attributes::Node& self, const py::object& otype) {
              auto type = wrapper::pyObjectToCppType(otype);
              return wrapper::detail::castElementToPy(self, type);
          },
          py::arg("type"), "Get value as a type given as an argument for current node");

    an.def(
          "getType",
          [](const Hash::Attributes::Node& node) {
              using namespace karabo::util;
              PyTypes::ReferenceType type = static_cast<PyTypes::ReferenceType>(node.getType());
              return py::cast(type);
          },
          "Get type of the value kept in current node");

    an.def(
          "setType",
          [](Hash::Attributes::Node& node, const py::object& otype) {
              Types::ReferenceType type = wrapper::pyObjectToCppType(otype);
              node.setType(type);
          },
          py::arg("type"), "Set type for value kept in current node");


    py::class_<Hash::Attributes> a(
          m, "HashAttributes",
          "The HashAttributes class is a heterogeneous container with string key and \"any object\" value\n"
          "that preserves insertion order, i.e. it is behaving like an ordered map");
    a.def(py::init<>());

    a.def(
          "has",
          [](karabo::util::Hash::Attributes& self, const std::string& key) -> py::bool_ { return self.has(key); },
          (py::arg("key")), "Returns True if HashAttributes container contains given \"key\"");

    a.def(
          "__contains__",
          [](karabo::util::Hash::Attributes& self, const std::string& key) -> py::bool_ { return self.has(key); },
          py::arg("key"), "Returns True if HashAttributes container contains given \"key\"");

    a.def(
          "isType",
          [](karabo::util::Hash::Attributes& self, const std::string& key, const py::object& otype) -> py::bool_ {
              Types::ReferenceType targetType = wrapper::pyObjectToCppType(otype);
              Types::ReferenceType type = self.getNode(key).getType();
              return (type == targetType);
          },
          py::arg("key"), py::arg("type"),
          "Returns True if HashAttributes container has given \"key\" of reference \"type\"..");

    a.def(
          "getType",
          [](karabo::util::Hash::Attributes& self, const std::string& key) {
              using namespace karabo::util;
              auto node = self.getNode(key);
              PyTypes::ReferenceType type = static_cast<PyTypes::ReferenceType>(node.getType());
              return py::cast(type);
          },
          py::arg("key"), "Returns ReferenceType for given attribute \"key\"");

    a.def(
          "erase", [](karabo::util::Hash::Attributes& self, const std::string& key) { self.erase(key); },
          py::arg("key"), "Erase \"key\" attribute");

    a.def(
          "__delitem__", [](karabo::util::Hash::Attributes& self, const std::string& key) { self.erase(key); },
          py::arg("key"), "Erase \"key\" attribute");

    a.def(
          "size", [](karabo::util::Hash::Attributes& self) { return self.size(); },
          "Returns number of entries in HashAttributes container");

    a.def(
          "__len__", [](karabo::util::Hash::Attributes& self) { return self.size(); },
          "Returns number of entries in HashAttributes container");

    a.def(
          "empty", [](karabo::util::Hash::Attributes& self) { return self.empty(); },
          "Returns True if HashAttributes container is empty.");

    a.def(
          "bool", [](karabo::util::Hash::Attributes& self) { return self.size() > 0; },
          "This function automatically called when HashAttributes object checked in \"if\" expression. \"False\" means "
          "that container is empty.");

    a.def("clear", [](karabo::util::Hash::Attributes& self) { self.clear(); }, "Make HashAttributes container empty.");

    a.def(
          "getNode",
          [](karabo::util::Hash::Attributes& self, const std::string& key) {
              using namespace karabo::util;
              Hash::Attributes::Node& nodeRef = self.getNode(key);
              boost::optional<Hash::Attributes::Node&> node(nodeRef);
              return py::cast(std::shared_ptr<Hash::Attributes::Node>(&(*node), [](const void*) {}));
          },
          py::arg("key"), "Returns HashAttributesNode object associated with \"key\" attribute.");

    a.def(
          "get",
          [](karabo::util::Hash::Attributes& self, const std::string& key) {
              return wrapper::castAnyToPy(self.getAny(key));
          },
          py::arg("key"), "Returns value for \"key\" attribute.");

    a.def(
          "__getitem__",
          [](karabo::util::Hash::Attributes& self, const std::string& key) {
              return wrapper::castAnyToPy(self.getAny(key));
          },
          py::arg("key"), "Pythonic style for getting value of attribute: x = attrs['abc']");

    a.def(
          "getAs",
          [](karabo::util::Hash::Attributes& self, const std::string& key, const py::object& otype) {
              Types::ReferenceType rtype = wrapper::pyObjectToCppType(otype);
              const auto& node = self.getNode(key);
              return wrapper::detail::castElementToPy(node, rtype);
          },
          py::arg("key"), py::arg("type"), "Get the value of the \"key\" attribute and convert it to type \"type\".");

    a.def(
          "set",
          [](karabo::util::Hash::Attributes& self, const std::string& key, const py::object& value) {
              boost::any a;
              wrapper::castPyToAny(value, a);
              return py::cast(self.set(key, a));
          },
          py::arg("key"), py::arg("value"), "Set the \"value\" for \"key\" attribute.");

    a.def(
          "__setitem__",
          [](karabo::util::Hash::Attributes& self, const std::string& key, const py::object& value) {
              boost::any a;
              wrapper::castPyToAny(value, a);
              return py::cast(self.set(key, a));
          },
          py::arg("key"), py::arg("value"), "Pythonic style for setting value of attribute: attrs['abc'] = 123");

    a.def(
          "__iter__", [](Hash::Attributes& self) { return py::make_iterator(self.begin(), self.end()); },
          py::keep_alive<0, 1>());
}
