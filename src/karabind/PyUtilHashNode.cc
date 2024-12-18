/*
 * File: PyUtilHash.cc
 * Author: CONTROLS DEV group
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

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <karabo/util/FromLiteral.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <sstream>
#include <string>

#include "PyTypes.hh"
#include "Wrapper.hh"


namespace py = pybind11;


using namespace karabo::util;
using namespace std;
using namespace karabind;


void exportPyUtilHashNode(py::module_& m) {
    typedef std::shared_ptr<karabo::util::Hash::Node> Pointer;

    py::class_<Hash::Node, std::shared_ptr<Hash::Node>> n(m, "HashNode");

    n.def("__repr__", [](const Hash::Node& self) { return py::cast(self.getKey()); });

    n.def("__str__", [](const Hash::Node& self) { return py::cast(self.getKey()); });

    n.def("getKey", [](const Hash::Node& self) { return py::cast(self.getKey()); }, "Returns the key of current node.");

    n.def(
          "setValue",
          [](Hash::Node& self, const py::object& o) {
              std::any any;
              wrapper::castPyToAny(o, any);
              self.setValue(std::move(any));
          },
          py::arg("value"), "Sets the new value of current node.");

    n.def(
          "getValue",
          [](Hash::Node& self) {
              using namespace karabo::util;
              std::any& a = self.getValueAsAny();
              // handle Hash differently returning reference to Hash
              if (std::any_cast<Hash>(&a)) {
                  Hash* hp = &std::any_cast<Hash&>(a);
                  return py::cast(hp);
              }
              return wrapper::castAnyToPy(a);
          },
          "Gets the value of current node.");

    n.def(
          "getValueAs",
          [](const Hash::Node& self, const py::object& otype) {
              auto targetType = wrapper::pyObjectToCppType(otype);
              return wrapper::detail::castElementToPy(self, targetType);
          },
          py::arg("type"), "Gets the value of current node converted to given reference type");

    n.def(
          "setAttribute",
          [](Hash::Node& self, const std::string& key, const py::object& o) {
              std::any value;
              wrapper::castPyToAny(o, value);
              self.setAttribute(key, std::move(value));
          },
          py::arg("key"), py::arg("value"), "Sets the \"key\" attribute to some \"value\" in current node.");

    n.def(
          "getAttribute",
          [](Hash::Node& self, const std::string& key) { return wrapper::castAnyToPy(self.getAttributeAsAny(key)); },
          py::arg("key"), "Gets the value of \"key\" attribute  in current node.");

    n.def(
          "getAttributeAs",
          [](const Hash::Node& self, const std::string& key, const py::object& otype) {
              auto targetType = wrapper::pyObjectToCppType(otype);
              const auto& anode = self.getAttributeNode(key);
              return wrapper::detail::castElementToPy(anode, targetType);
          },
          py::arg("key"), py::arg("type"), "Gets the value of \"key\" attribute converted to type \"type\".");

    n.def(
          "hasAttribute", [](const Hash::Node& self, const std::string& key) { return self.hasAttribute(key); },
          py::arg("key"), "Check that current node has the \"key\" attribute.");

    n.def(
          "setAttributes",
          [](Hash::Node& self, const py::object& o) {
              if (py::isinstance<Hash::Attributes>(o)) {
                  self.setAttributes(o.cast<Hash::Attributes>());
              } else {
                  throw KARABO_PYTHON_EXCEPTION("Python object is not of a Hash.Attributes type");
              }
          },
          py::arg("attributes"), "Sets new set of attributes in current node.");

    n.def(
          "getAttributes", [](const Hash::Node& self) { return &self.getAttributes(); },
          py::return_value_policy::reference_internal,
          "Gets all attributes in current node as HashAttributes object. This object is internal reference not a "
          "copy.");

    n.def(
          "copyAttributes", [](const Hash::Node& self) { return self.getAttributes(); },
          "Gets a copy of all attributes in current node as HashAttributes object.");

    n.def(
          "getType", [](const Hash::Node& self) { return py::cast(PyTypes::from(self.getType())); },
          "Gets the value type as a reference type");

    n.def(
          "setType",
          [](Hash::Node& self, const py::object& otype) {
              if (py::isinstance<py::str>(otype)) {
                  const std::string& stype = otype.cast<std::string>();
                  self.setType(Types::from<FromLiteral>(stype));
              } else if (py::isinstance<PyTypes::ReferenceType>(otype)) {
                  PyTypes::ReferenceType ptype = otype.cast<PyTypes::ReferenceType>();
                  self.setType(PyTypes::to(ptype));
              } else {
                  throw KARABO_PARAMETER_EXCEPTION("Argument type is not supported. Valid types: 'str' and 'Types'");
              }
          },
          py::arg("type"), "Sets the value type as a reference \"type\".");
}
