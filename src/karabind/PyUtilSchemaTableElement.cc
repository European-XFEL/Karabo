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
#include "karabo/data/schema/TableElement.hh"


namespace py = pybind11;
using namespace karabo::data;
using namespace std;
using namespace karabind;


void exportPyUtilSchemaTableElement(py::module_& m) {
    ///////////////////////////////////////////////////////////////////////////
    //  TableDefaultValue<TableElement>
    ///////////////////////////////////////////////////////////////////////////
    {
        typedef TableDefaultValue<TableElement> DefTableElement;

        py::class_<DefTableElement>(m, "DefaultValueTableElement")
              .def(
                    "defaultValue",
                    [](DefTableElement& self, const py::object& obj) -> TableElement& {
                        if (py::isinstance<py::sequence>(obj)) {
                            std::vector<Hash> v;
                            auto vo = obj.cast<std::vector<py::object>>();
                            for (auto item : vo) {
                                if (py::isinstance<Hash>(item)) {
                                    v.push_back(item.cast<Hash>());
                                } else {
                                    throw KARABO_PYTHON_EXCEPTION(
                                          "Default value for TableElement should sequence of Hash or VectorHash");
                                }
                            }
                            return self.defaultValue(v);
                        } else {
                            throw KARABO_PYTHON_EXCEPTION(
                                  "Python type of the defaultValue of TableElement must be a list");
                        }
                    },
                    py::arg("pyList"), py::return_value_policy::reference_internal)
              .def("noDefaultValue", &DefTableElement::noDefaultValue, py::return_value_policy::reference_internal);
    }

    {
        typedef ReadOnlySpecific<TableElement, std::vector<Hash>> ReadOnlySpec;

        auto readOnlySpecificTableFunc = [](ReadOnlySpec& self, const py::object& obj) -> ReadOnlySpec& {
            if (!py::isinstance<py::sequence>(obj)) {
                throw KARABO_PYTHON_EXCEPTION(
                      "Python type of initialValue for a read-only table must be a list of Hashes");
            }
            std::vector<Hash> v;
            for (auto item : obj) {
                if (!py::isinstance<Hash>(item)) {
                    throw KARABO_PYTHON_EXCEPTION(
                          "Initial value for TableElement should be a sequence of Hash or VectorHash");
                }
                v.push_back(item.cast<Hash>());
            }
            return self.initialValue(v);
        };

        py::class_<ReadOnlySpec>(m, "ReadOnlySpecificTABLE")
              .def("initialValue", readOnlySpecificTableFunc, py::arg("pyList"),
                   py::return_value_policy::reference_internal)
              .def("defaultValue", readOnlySpecificTableFunc, py::arg("pyList"),
                   py::return_value_policy::reference_internal)
              .def("archivePolicy",
                   (ReadOnlySpec & (ReadOnlySpec::*)(Schema::ArchivePolicy const&)) & ReadOnlySpec::archivePolicy,
                   py::return_value_policy::reference_internal)
              .def("commit", &ReadOnlySpec::commit);
    }

    //////////////////////////////////////////////////////////////////////
    // Binding TableElement
    // In Python : TABLE_ELEMENT
    {
        // py::implicitly_convertible<Schema&, TableElement>();
        py::class_<TableElement>(m, "TABLE_ELEMENT")
              .def(py::init<Schema&>(), py::arg("expected"))
              .def("observerAccess", &TableElement::observerAccess, py::return_value_policy::reference_internal)
              .def("operatorAccess", &TableElement::operatorAccess, py::return_value_policy::reference_internal)
              .def("expertAccess", &TableElement::expertAccess, py::return_value_policy::reference_internal)
              .def(
                    "allowedStates",
                    [](TableElement& self, py::args args) -> TableElement& {
                        std::vector<State> states;
                        for (auto arg : args) {
                            const std::string state = arg.attr("name").cast<std::string>();
                            states.push_back(State::fromString(state));
                        }
                        self.allowedStates(states);
                        return self;
                    },
                    py::return_value_policy::reference_internal)
              .def("assignmentInternal", &TableElement::assignmentInternal, py::return_value_policy::reference_internal)
              .def("assignmentMandatory", &TableElement::assignmentMandatory,
                   py::return_value_policy::reference_internal)
              .def("assignmentOptional", &TableElement::assignmentOptional, py::return_value_policy::reference_internal)
              .def("alias", &AliasAttributeWrap<TableElement>::aliasPy, py::return_value_policy::reference_internal)
              .def("commit", &TableElement::commit)
              .def("description", &TableElement::description, py::return_value_policy::reference_internal)
              .def("displayedName", &TableElement::displayedName, py::return_value_policy::reference_internal)
              .def("init", &TableElement::init, py::return_value_policy::reference_internal)
              .def("key", &TableElement::key, py::arg("name"), py::arg("strict") = true,
                   py::return_value_policy::reference_internal)
              .def("reconfigurable", &TableElement::reconfigurable, py::return_value_policy::reference_internal)
              .def("tags",
                   (TableElement & (TableElement::*)(std::string const&, std::string const&)) & TableElement::tags,
                   py::arg("tags"), py::arg("sep") = " ,;", py::return_value_policy::reference_internal)
              .def("tags", (TableElement & (TableElement::*)(std::vector<std::string> const&)) & TableElement::tags,
                   py::arg("tags"), py::return_value_policy::reference_internal)
              .def("maxSize", &TableElement::maxSize, py::return_value_policy::reference_internal)
              .def("minSize", &TableElement::minSize, py::return_value_policy::reference_internal)
              .def("setNodeSchema", &TableElement::setColumns, py::arg("nodeSchema"),
                   py::return_value_policy::reference_internal, "DEPRECATED - use 'setColumns' instead")
              .def("setColumns", &TableElement::setColumns, py::arg("schema"),
                   py::return_value_policy::reference_internal, "Set Schema describing the columns")
              .def("readOnly", &TableElement::readOnly, py::return_value_policy::reference_internal)
              // .def("addRow", (TableElement & (TableElement::*)(const Hash&))&TableElement::addRow,
              //      py::arg("nodeHash") = Hash(), py::return_internal_reference<> ())
              // .def("addRow" , (TableElement & (TableElement::*)(const Schema&, const Hash&))&TableElement::addRow,
              //      py::arg("nodeSchema"), py::arg("nodeHash") = Hash(), py::return_value_policy::reference_internal)
              ;
    }
}
