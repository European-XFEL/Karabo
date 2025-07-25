/*
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
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <iostream>

#include "PyTypes.hh"
#include "Wrapper.hh"
#include "karabo/data/schema/ByteArrayElement.hh"
#include "karabo/data/schema/Factory.hh"
#include "karabo/data/schema/LeafElement.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/OverwriteElement.hh"
#include "karabo/data/schema/TableElement.hh"
#include "karabo/data/schema/Validator.hh"
#include "karabo/data/types/AlarmCondition.hh"
#include "karabo/data/types/HashFilter.hh"


namespace py = pybind11;
using namespace karabo::data;
using namespace std;
using namespace karabind;


struct ConvertOptions {
    inline ConvertOptions(const string& path, const Schema& schema) : m_path(path), m_schema(schema) {}

    template <class T>
    inline void operator()(T*) {
        const Hash& h = m_schema.getParameterHash();
        result = wrapper::castAnyToPy(h.getAttributeAsAny(m_path, KARABO_SCHEMA_OPTIONS));
    }

    const string& m_path;
    const Schema& m_schema;
    py::object result;
};


void exportPyUtilSchema(py::module_& m) {
    py::enum_<AccessType>(m, "AccessType")
          .value("INIT", AccessType::INIT)
          .value("READ", AccessType::READ)
          .value("WRITE", AccessType::WRITE)
          .export_values();

    py::enum_<DaqDataType>(m, "DaqDataType").value("PULSE", DaqDataType::PULSE).value("TRAIN", DaqDataType::TRAIN);

    {
        py::enum_<MetricPrefix>(m, "MetricPrefix")
              .value("YOTTA", MetricPrefix::YOTTA)
              .value("ZETTA", MetricPrefix::ZETTA)
              .value("EXA", MetricPrefix::EXA)
              .value("PETA", MetricPrefix::PETA)
              .value("TERA", MetricPrefix::TERA)
              .value("GIGA", MetricPrefix::GIGA)
              .value("MEGA", MetricPrefix::MEGA)
              .value("KILO", MetricPrefix::KILO)
              .value("HECTO", MetricPrefix::HECTO)
              .value("DECA", MetricPrefix::DECA)
              .value("NONE", MetricPrefix::NONE)
              .value("DECI", MetricPrefix::DECI)
              .value("CENTI", MetricPrefix::CENTI)
              .value("MILLI", MetricPrefix::MILLI)
              .value("MICRO", MetricPrefix::MICRO)
              .value("NANO", MetricPrefix::NANO)
              .value("PICO", MetricPrefix::PICO)
              .value("FEMTO", MetricPrefix::FEMTO)
              .value("ATTO", MetricPrefix::ATTO)
              .value("ZEPTO", MetricPrefix::ZEPTO)
              .value("YOCTO", MetricPrefix::YOCTO)
              .export_values();

        py::enum_<Unit>(m, "Unit")
              .value("NUMBER", Unit::NUMBER)
              .value("COUNT", Unit::COUNT)
              .value("METER", Unit::METER)
              .value("GRAM", Unit::GRAM)
              .value("SECOND", Unit::SECOND)
              .value("AMPERE", Unit::AMPERE)
              .value("KELVIN", Unit::KELVIN)
              .value("MOLE", Unit::MOLE)
              .value("CANDELA", Unit::CANDELA)
              .value("HERTZ", Unit::HERTZ)
              .value("RADIAN", Unit::RADIAN)
              .value("DEGREE", Unit::DEGREE)
              .value("STERADIAN", Unit::STERADIAN)
              .value("NEWTON", Unit::NEWTON)
              .value("PASCAL", Unit::PASCAL)
              .value("JOULE", Unit::JOULE)
              .value("ELECTRONVOLT", Unit::ELECTRONVOLT)
              .value("WATT", Unit::WATT)
              .value("COULOMB", Unit::COULOMB)
              .value("VOLT", Unit::VOLT)
              .value("FARAD", Unit::FARAD)
              .value("OHM", Unit::OHM)
              .value("SIEMENS", Unit::SIEMENS)
              .value("WEBER", Unit::WEBER)
              .value("TESLA", Unit::TESLA)
              .value("HENRY", Unit::HENRY)
              .value("DEGREE_CELSIUS", Unit::DEGREE_CELSIUS)
              .value("LUMEN", Unit::LUMEN)
              .value("LUX", Unit::LUX)
              .value("BECQUEREL", Unit::BECQUEREL)
              .value("GRAY", Unit::GRAY)
              .value("SIEVERT", Unit::SIEVERT)
              .value("KATAL", Unit::KATAL)
              .value("MINUTE", Unit::MINUTE)
              .value("HOUR", Unit::HOUR)
              .value("DAY", Unit::DAY)
              .value("YEAR", Unit::YEAR)
              .value("BAR", Unit::BAR)
              .value("PIXEL", Unit::PIXEL)
              .value("BYTE", Unit::BYTE)
              .value("BIT", Unit::BIT)
              .value("METER_PER_SECOND", Unit::METER_PER_SECOND)
              .value("VOLT_PER_SECOND", Unit::VOLT_PER_SECOND)
              .value("AMPERE_PER_SECOND", Unit::AMPERE_PER_SECOND)
              .value("PERCENT", Unit::PERCENT)
              .value("NOT_ASSIGNED", Unit::NOT_ASSIGNED)
              .value("REVOLUTIONS_PER_MINUTE", Unit::REVOLUTIONS_PER_MINUTE)
              .export_values();
    }

    { // exposing ::Schema

        py::class_<Schema::AssemblyRules>(m, "AssemblyRules")
              .def(py::init<AccessType const&, std::string const&, const int>(),
                   py::arg("accessMode") = INIT | WRITE | READ, py::arg("state") = "", py::arg("accessLevel") = -1)
              .def_readwrite("m_accessMode", &Schema::AssemblyRules::m_accessMode)
              .def_readwrite("m_accessLevel", &Schema::AssemblyRules::m_accessLevel)
              .def_readwrite("m_state", &Schema::AssemblyRules::m_state)
              .def("__str__", [](const Schema::AssemblyRules& self) -> py::str {
                  std::ostringstream oss;
                  oss << "AssemblyRules(mode: " << self.m_accessMode << ", level: " << self.m_accessLevel
                      << ", state: '" << self.m_state << "')";
                  return oss.str();
              });

        py::class_<Schema, std::shared_ptr<Schema>> s(m, "Schema");

        s.def(py::init<>());

        s.def(py::init<std::string const&, Schema::AssemblyRules const&>(), py::arg("root"),
              py::arg("rules") = Schema::AssemblyRules());

        py::enum_<Schema::AssignmentType>(m, "AssignmentType")
              .value("OPTIONAL", Schema::OPTIONAL_PARAM)
              .value("MANDATORY", Schema::MANDATORY_PARAM)
              .value("INTERNAL", Schema::INTERNAL_PARAM)
              .export_values();

        py::enum_<Schema::AccessLevel>(m, "AccessLevel")
              .value("OBSERVER", Schema::OBSERVER)
              .value("OPERATOR", Schema::OPERATOR)
              .value("EXPERT", Schema::EXPERT)
              .export_values();

        py::enum_<Schema::NodeType>(m, "NodeType")
              .value("LEAF", Schema::LEAF)
              .value("NODE", Schema::NODE)
              .export_values();

        py::enum_<Schema::ArchivePolicy>(m, "ArchivePolicy")
              .value("EVERY_EVENT", Schema::EVERY_EVENT)
              .value("NO_ARCHIVING", Schema::NO_ARCHIVING)
              .export_values();

        // s.def(py::self_ns::str(py::self));
        s.def("__str__", [](const Schema& self) {
            std::ostringstream oss;
            oss << "Schema for: " << self.getRootName() << "\n" << self.getParameterHash();
            return oss.str();
        });

        //********* General functions on Schema *******

        s.def("has", [](const Schema& self, string const& path) -> bool { return self.has(path); }, py::arg("path"));

        s.def(
              "__contains__", [](const Schema& self, const std::string& path) -> bool { return self.has(path); },
              py::arg("path"));

        s.def("empty", [](const Schema& self) { return self.empty(); });

        s.def(
              "merge",
              [](Schema& self, const Schema& other) {
                  self.merge(other);
                  return py::cast(self);
              },
              py::arg("schema"));

        s.def(
              "__iadd__",
              [](Schema& self, const Schema& other) {
                  self.merge(other);
                  return py::cast(self);
              },
              py::arg("schema"));

        s.def(
              "copy",
              [](Schema& self, const Schema& other) {
                  self = other;
                  return self;
              },
              py::arg("schema"));

        //********* 'get'-methods *********************
        s.def(
              "getRequiredAccessLevel",
              [](const Schema& self, const std::string& path) {
                  return py::cast(Schema::AccessLevel(self.getRequiredAccessLevel(path)));
              },
              py::arg("path"));

        s.def("getParameterHash", [](const Schema& self) { return py::cast(self.getParameterHash()); });

        s.def("getAccessMode",
              [](const Schema& self, const std::string& path) { return py::cast(self.getAccessMode(path)); });

        s.def("getAssemblyRules", [](const Schema& self) { return py::cast(self.getAssemblyRules()); });

        s.def(
              "getAssignment",
              [](const Schema& self, const std::string& path) { return py::cast(self.getAssignment(path)); },
              py::arg("path"));

        s.def(
              "getDescription",
              [](const Schema& self, const std::string& path) -> py::str { return self.getDescription(path); },
              py::arg("path"), py::return_value_policy::reference_internal);

        s.def(
              "getDisplayType",
              [](const Schema& self, const std::string& path) -> py::str { return self.getDisplayType(path); },
              py::arg("path"), py::return_value_policy::reference_internal);

        s.def(
              "getClassId",
              [](const Schema& self, const std::string& path) -> py::str { return self.getClassId(path); },
              py::arg("path"), py::return_value_policy::reference_internal);

        s.def(
              "getDisplayedName",
              [](const Schema& self, const std::string& path) -> py::str { return self.getDisplayedName(path); },
              py::arg("path"), py::return_value_policy::reference_internal);

        s.def(
              "getUnit", [](const Schema& self, const std::string& path) { return py::cast(self.getUnit(path)); },
              py::arg("path"));

        s.def(
              "getUnitName",
              [](const Schema& self, const std::string& path) -> py::str { return self.getUnitName(path); },
              py::arg("path"), py::return_value_policy::reference_internal);

        s.def(
              "getUnitSymbol",
              [](const Schema& self, const std::string& path) -> py::str { return self.getUnitSymbol(path); },
              py::arg("path"), py::return_value_policy::reference_internal);

        s.def(
              "getMetricPrefix",
              [](const Schema& self, const std::string& path) { return py::cast(self.getMetricPrefix(path)); },
              py::arg("path"));

        s.def(
              "getMetricPrefixName",
              [](const Schema& self, const std::string& path) -> py::str { return self.getMetricPrefixName(path); },
              py::arg("path"), py::return_value_policy::reference_internal);

        s.def(
              "getMetricPrefixSymbol",
              [](const Schema& self, const std::string& path) -> py::str { return self.getMetricPrefixSymbol(path); },
              py::arg("path"), py::return_value_policy::reference_internal);

        s.def(
              "getRootName", [](const Schema& self) -> py::str { return self.getRootName(); },
              py::return_value_policy::reference_internal);

        s.def(
              "getValueType",
              [](const Schema& self, const std::string& path) {
                  Types::ReferenceType type = self.getValueType(path);
                  return PyTypes::from(type);
              },
              py::arg("path"));

        s.def(
              "getNodeType",
              [](const Schema& self, const std::string& path) { return py::cast(self.getNodeType(path)); },
              py::arg("path"));

        s.def(
              "getMinInc",
              [](const Schema& self, const std::string& path) {
                  const Hash& h = self.getParameterHash();
                  const std::string attribute = KARABO_SCHEMA_MIN_INC;
                  return wrapper::castAnyToPy(h.getAttributeAsAny(path, attribute));
              },
              py::arg("path"));

        s.def(
              "getMaxInc",
              [](const Schema& self, const std::string& path) {
                  const Hash& h = self.getParameterHash();
                  const std::string attribute = KARABO_SCHEMA_MAX_INC;
                  return wrapper::castAnyToPy(h.getAttributeAsAny(path, attribute));
              },
              py::arg("path"));

        s.def(
              "getMinExc",
              [](const Schema& self, const std::string& path) {
                  const Hash& h = self.getParameterHash();
                  const std::string attribute = KARABO_SCHEMA_MIN_EXC;
                  return wrapper::castAnyToPy(h.getAttributeAsAny(path, attribute));
              },
              py::arg("path"));

        s.def(
              "getMaxExc",
              [](const Schema& self, const std::string& path) {
                  const Hash& h = self.getParameterHash();
                  const std::string attribute = KARABO_SCHEMA_MAX_EXC;
                  return wrapper::castAnyToPy(h.getAttributeAsAny(path, attribute));
              },
              py::arg("path"));

        s.def(
              "getMinIncAs",
              [](const Schema& self, const py::object& path, const py::object& otype) {
                  py::object h = py::cast(self).attr("getParameterHash")();
                  return h.attr("getAttributeAs")(path, KARABO_SCHEMA_MIN_INC, otype);
              },
              py::arg("path"), py::arg("pytype"));

        s.def(
              "getMaxIncAs",
              [](const Schema& self, const py::object& path, const py::object& otype) {
                  py::object h = py::cast(self).attr("getParameterHash")();
                  return h.attr("getAttributeAs")(path, KARABO_SCHEMA_MAX_INC, otype);
              },
              py::arg("path"), py::arg("pytype"));

        s.def(
              "getMinExcAs",
              [](const Schema& self, const py::object& path, const py::object& otype) {
                  py::object h = py::cast(self).attr("getParameterHash")();
                  return h.attr("getAttributeAs")(path, KARABO_SCHEMA_MIN_EXC, otype);
              },
              py::arg("path"), py::arg("pytype"));

        s.def(
              "getMaxExcAs",
              [](const Schema& self, const py::object& path, const py::object& otype) {
                  py::object h = py::cast(self).attr("getParameterHash")();
                  return h.attr("getAttributeAs")(path, KARABO_SCHEMA_MAX_EXC, otype);
              },
              py::arg("path"), py::arg("pytype"));

        s.def(
              "getAliasAsString",
              [](const Schema& self, const std::string& path) { return self.getAliasAsString(path); }, py::arg("path"));

        s.def(
              "getKeys",
              [](const Schema& self, const std::string& path) -> py::list {
                  std::vector<std::string> v = self.getKeys(path);
                  return py::cast(v);
              },
              py::arg("path") = "");

        s.def("getPaths", [](const Schema& self) -> py::list {
            std::vector<std::string> v = self.getPaths();
            return py::cast(v);
        });

        s.def(
              "getOptions",
              [](const Schema& self, const std::string& path) -> py::list {
                  ConvertOptions convertOptions(path, self);
                  templatize(self.getValueType(path), convertOptions);
                  return convertOptions.result;
              },
              py::arg("path"));

        s.def(
              "getTags",
              [](const Schema& self, const std::string& path) -> py::list {
                  std::vector<std::string> v = self.getTags(path);
                  return py::cast(v);
              },
              py::arg("path"));

        s.def(
              "getAllowedStates",
              [](const Schema& self, const std::string& path) {
                  const std::string& allowed = karabo::data::toString(self.getAllowedStates(path));
                  const vector<string> v = karabo::data::fromString<std::string, std::vector>(allowed);
                  py::list states;
                  py::module_ ms = py::module_::import("karabo.common.states");
                  for (std::vector<std::string>::const_iterator it = v.begin(); it != v.end(); ++it) {
                      states.append(ms.attr("State")(*it));
                  }
                  return states;
              },
              py::arg("path"));

        s.def(
              "getDefaultValue",
              [](const Schema& self, const std::string& path) {
                  const Hash& h = self.getParameterHash();
                  return wrapper::castAnyToPy(h.getAttributeAsAny(path, KARABO_SCHEMA_DEFAULT_VALUE));
              },
              py::arg("path"));

        s.def(
              "getDefaultValueAs",
              [](const Schema& self, const py::object& path, const py::object& otype) {
                  py::object h = py::cast(self).attr("getParameterHash")();
                  return h.attr("getAttributeAs")(path, KARABO_SCHEMA_DEFAULT_VALUE, otype);
              },
              py::arg("path"), py::arg("pytype"));

        s.def("getMinSize", [](const Schema& self, const py::object& path) {
            py::object h = py::cast(self).attr("getParameterHash")();
            return h.attr("getAttribute")(path, KARABO_SCHEMA_MIN_SIZE);
        });

        s.def("getMaxSize", [](const Schema& self, const py::object& path) {
            py::object h = py::cast(self).attr("getParameterHash")();
            return h.attr("getAttribute")(path, KARABO_SCHEMA_MAX_SIZE);
        });

        s.def("getArchivePolicy",
              [](const Schema& self, const std::string& path) { return py::cast(self.getArchivePolicy(path)); });

        //********* 'has'-methods ****************

        s.def(
              "keyHasAlias",
              [](const Schema& self, const std::string& path) { return py::cast(self.keyHasAlias(path)); },
              py::arg("key"));

        s.def(
              "aliasHasKey",
              [](const Schema& self, const py::object& alias) -> py::bool_ {
                  if (py::isinstance<py::str>(alias)) return self.aliasHasKey(alias.cast<std::string>());
                  if (py::isinstance<py::int_>(alias)) return self.aliasHasKey(alias.cast<int>());
                  if (py::isinstance<py::float_>(alias)) return self.aliasHasKey(alias.cast<double>());
                  if (py::isinstance<py::list>(alias)) {
                      const std::vector<py::object>& vo = alias.cast<std::vector<py::object>>();
                      if (vo.size() == 0) return self.aliasHasKey(std::vector<py::object>());
                      py::object list0 = vo[0];
                      if (py::isinstance<py::str>(list0))
                          return self.aliasHasKey(alias.cast<std::vector<std::string>>());
                      if (py::isinstance<py::int_>(list0)) return self.aliasHasKey(alias.cast<std::vector<int>>());
                      if (py::isinstance<py::float_>(list0)) return self.aliasHasKey(alias.cast<std::vector<double>>());
                  }
                  throw KARABO_PARAMETER_EXCEPTION("The 'alias' type is not supported");
              },
              py::arg("alias"));

        s.def(
              "getAliasFromKey",
              [](const Schema& self, const py::object& path) {
                  py::object h = py::cast(self).attr("getParameterHash")();
                  return h.attr("getAttribute")(path, KARABO_SCHEMA_ALIAS);
              },
              py::arg("key"));

        s.def(
              "getKeyFromAlias",
              [](const Schema& self, const py::object& alias) -> py::str {
                  if (py::isinstance<py::str>(alias)) return self.getKeyFromAlias(alias.cast<std::string>());
                  if (py::isinstance<py::int_>(alias)) return self.getKeyFromAlias(alias.cast<int>());
                  if (py::isinstance<py::float_>(alias)) return self.getKeyFromAlias(alias.cast<double>());
                  if (py::isinstance<py::list>(alias)) {
                      const std::vector<py::object>& vo = alias.cast<std::vector<py::object>>();
                      if (vo.size() == 0) return self.getKeyFromAlias(std::vector<std::string>());
                      py::object list0 = vo[0];
                      if (py::isinstance<py::str>(list0))
                          return self.getKeyFromAlias(alias.cast<std::vector<std::string>>());
                      if (py::isinstance<py::int_>(list0)) return self.getKeyFromAlias(alias.cast<std::vector<int>>());
                      if (py::isinstance<py::float_>(list0))
                          return self.getKeyFromAlias(alias.cast<std::vector<double>>());
                  }
                  throw KARABO_PARAMETER_EXCEPTION("The 'alias' type is not supported");
              },
              py::arg("alias"));

        s.def(
              "hasAccessMode",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasAccessMode(path); },
              py::arg("path"));

        s.def(
              "hasAssignment",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasAssignment(path); },
              py::arg("path"));

        s.def("hasAllowedStates", &Schema::hasAllowedStates, py::arg("path"));

        s.def(
              "hasDefaultValue",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasDefaultValue(path); },
              py::arg("path"));

        s.def(
              "hasOptions",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasOptions(path); },
              py::arg("path"));

        s.def(
              "hasTags", [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasTags(path); },
              py::arg("path"));

        s.def(
              "hasUnit", [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasUnit(path); },
              py::arg("path"));

        s.def(
              "hasMetricPrefix",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasMetricPrefix(path); },
              py::arg("path"));

        s.def(
              "hasMinInc",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasMinInc(path); },
              py::arg("path"));

        s.def(
              "hasMaxInc",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasMaxInc(path); },
              py::arg("path"));

        s.def(
              "hasMinExc",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasMinExc(path); },
              py::arg("path"));

        s.def(
              "hasMaxExc",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasMaxExc(path); },
              py::arg("path"));

        s.def(
              "hasArchivePolicy",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasArchivePolicy(path); },
              py::arg("path"));

        s.def(
              "hasDisplayedName",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasDisplayedName(path); },
              py::arg("path"));

        s.def(
              "hasDisplayType",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasDisplayType(path); },
              py::arg("path"));

        s.def(
              "hasClassId",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasClassId(path); },
              py::arg("path"));

        s.def(
              "hasDescription",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasDescription(path); },
              py::arg("path"));

        s.def(
              "hasMinSize",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasMinSize(path); },
              py::arg("path"));

        s.def(
              "hasMaxSize",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasMaxSize(path); },
              py::arg("path"));

        //********* 'is'-methods ****************

        s.def(
              "isAccessInitOnly",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.isAccessInitOnly(path); },
              py::arg("path"));

        s.def(
              "isAccessReadOnly",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.isAccessReadOnly(path); },
              py::arg("path"));

        s.def(
              "isAccessReconfigurable",
              [](const Schema& self, const std::string& path) -> py::bool_ {
                  return self.isAccessReconfigurable(path);
              },
              py::arg("path"));

        s.def(
              "isAssignmentInternal",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.isAssignmentInternal(path); },
              py::arg("path"));

        s.def(
              "isAssignmentMandatory",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.isAssignmentMandatory(path); },
              py::arg("path"));

        s.def(
              "isAssignmentOptional",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.isAssignmentOptional(path); },
              py::arg("path"));

        s.def(
              "isLeaf", [](const Schema& self, const std::string& path) -> py::bool_ { return self.isLeaf(path); },
              py::arg("path"));

        s.def(
              "isNode", [](const Schema& self, const std::string& path) -> py::bool_ { return self.isNode(path); },
              py::arg("path"));

        s.def(
              "isCommand",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.isCommand(path); },
              py::arg("path"));

        s.def(
              "isProperty",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.isProperty(path); },
              py::arg("path"));


        //********* Help function to show all parameters *******

        s.def("help", [](Schema& self, const std::string& classId) { self.help(classId); }, py::arg("classId") = "");

        s.def("getClassInfo", [](const Schema& self) { return self.getClassInfo(); });

        s.def_static("classInfo", (ClassInfo(*)()) & Schema::classInfo);

        //********* set-methods *********

        s.def(
              "setAssemblyRules",
              [](Schema& self, const Schema::AssemblyRules& rules) { self.setAssemblyRules(rules); }, py::arg("rules"));

        s.def(
              "setAccessMode",
              [](Schema& self, const std::string& path, const AccessType& value) { self.setAccessMode(path, value); },
              py::arg("path"), py::arg("value"));

        s.def(
              "setDisplayedName",
              [](Schema& self, const std::string& path, const std::string& value) {
                  self.setDisplayedName(path, value);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setDescription",
              [](Schema& self, const std::string& path, const std::string& value) { self.setDescription(path, value); },
              py::arg("path"), py::arg("value"));

        s.def(
              "setTags",
              [](Schema& self, const std::string& path, const std::string& value, const std::string& sep) {
                  self.setTags(path, value, sep);
              },
              py::arg("path"), py::arg("value"), py::arg("sep") = " ,;");

        s.def(
              "setDisplayType",
              [](Schema& self, const std::string& path, const std::string& value) { self.setDisplayType(path, value); },
              py::arg("path"), py::arg("value"));

        s.def(
              "setAssignment",
              [](Schema& self, const std::string& path, const Schema::AssignmentType& value) {
                  self.setAssignment(path, value);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setOptions",
              [](Schema& self, const std::string& path, const std::string& value, const std::string& sep) {
                  self.setOptions(path, value, sep);
              },
              py::arg("path"), py::arg("value"), py::arg("sep") = " ,;");

        s.def("setAllowedStates", [](Schema& self, const std::string& path, py::args args) {
            using namespace karabo::data;
            py::sequence tstates = (args.size() == 1) ? args[0] : args.cast<py::tuple>();
            std::vector<State> states;
            for (size_t i = 0; i < py::len(tstates); ++i) {
                const std::string state = tstates[i].attr("name").cast<std::string>();
                states.push_back(State::fromString(state));
            }
            self.setAllowedStates(path, states);
        });

        s.def(
              "setDefaultValue",
              [](Schema& self, const std::string& path, const py::object& ovalue) {
                  Hash& h = self.getParameterHash();
                  wrapper::setAttributeAsPy(h, path, KARABO_SCHEMA_DEFAULT_VALUE, ovalue);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setAlias",
              [](Schema& self, const std::string& path, const py::object& alias) {
                  Hash& h = self.getParameterHash();
                  wrapper::setAttributeAsPy(h, path, KARABO_SCHEMA_ALIAS, alias);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setUnit", [](Schema& self, const std::string& path, const Unit& value) { self.setUnit(path, value); },
              py::arg("path"), py::arg("value"));

        s.def(
              "setMetricPrefix",
              [](Schema& self, const std::string& path, const MetricPrefix& value) {
                  self.setMetricPrefix(path, value);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setMinInc",
              [](Schema& self, const std::string& path, const py::object& value) {
                  Hash& h = self.getParameterHash();
                  wrapper::setAttributeAsPy(h, path, KARABO_SCHEMA_MIN_INC, value);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setMaxInc",
              [](Schema& self, const std::string& path, const py::object& value) {
                  Hash& h = self.getParameterHash();
                  wrapper::setAttributeAsPy(h, path, KARABO_SCHEMA_MAX_INC, value);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setMinExc",
              [](Schema& self, const std::string& path, const py::object& value) {
                  Hash& h = self.getParameterHash();
                  wrapper::setAttributeAsPy(h, path, KARABO_SCHEMA_MIN_EXC, value);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setMaxExc",
              [](Schema& self, const std::string& path, const py::object& value) {
                  Hash& h = self.getParameterHash();
                  wrapper::setAttributeAsPy(h, path, KARABO_SCHEMA_MAX_EXC, value);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setMinSize",
              [](Schema& self, const std::string& path, const unsigned int& value) { self.setMinSize(path, value); },
              py::arg("path"), py::arg("value"));

        s.def(
              "setMaxSize",
              [](Schema& self, const std::string& path, const unsigned int& value) { self.setMaxSize(path, value); },
              py::arg("path"), py::arg("value"));

        s.def(
              "setArchivePolicy",
              [](Schema& self, const std::string& path, const Schema::ArchivePolicy& value) {
                  self.setArchivePolicy(path, value);
              },
              py::arg("path"), py::arg("value"));

        s.def(
              "setRequiredAccessLevel",
              [](Schema& self, const std::string& path, const Schema::AccessLevel& value) {
                  self.setRequiredAccessLevel(path, value);
              },
              py::arg("path"), py::arg("value"));

        // s.def("", &Schema::, ());     // overwrite<>(default) not implemented

        s.def("updateAliasMap", [](Schema& self) { self.updateAliasMap(); });

        s.def(
              "subSchema",
              [](Schema& self, const std::string& subNodePath, const std::string& filterTags) {
                  return self.subSchema(subNodePath, filterTags);
              },
              py::arg("subNodePath"), py::arg("filterTags") = "");

        s.def(
              "subSchemaByRules",
              [](Schema& self, const Schema::AssemblyRules& rules) { return self.subSchemaByRules(rules); },
              py::arg("assemblyRules"));

        s.def(
              "setDaqDataType",
              [](Schema& self, const std::string& path, const DaqDataType& dataType) {
                  self.setDaqDataType(path, dataType);
              },
              py::arg("path"), py::arg("dataType"));

        s.def(
              "getDaqDataType", [](const Schema& self, const std::string& path) { return self.getDaqDataType(path); },
              py::arg("path"));

        s.def(
              "hasDaqDataType",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.hasDaqDataType(path); },
              py::arg("path"));

        s.def(
              "isCustomNode",
              [](const Schema& self, const std::string& path) -> py::bool_ { return self.isCustomNode(path); },
              py::arg("path"));

        s.def(
              "getCustomNodeClass",
              [](const Schema& self, const std::string& path) -> py::str { return self.getCustomNodeClass(path); },
              py::arg("path"), py::return_value_policy::reference);

    } // end Schema


    py::class_<HashFilter>(m, "HashFilter")

          .def_static(
                "byTag",
                [](const Schema& schema, const Hash& config, const std::string& tags, const std::string& sep = ",") {
                    std::shared_ptr<Hash> result(new Hash);
                    HashFilter::byTag(schema, config, *result, tags, sep);
                    return result;
                },
                py::arg("schema"), py::arg("config"), py::arg("tags"), py::arg("sep") = ",")

          .def_static(
                "byAccessMode",
                [](const Schema& schema, const Hash& config, const AccessType& value) {
                    std::shared_ptr<Hash> result(new Hash);
                    HashFilter::byAccessMode(schema, config, *result, value);
                    return result;
                },
                py::arg("schema"), py::arg("config"), py::arg("accessMode"));

} // end  exportPyUtilSchema
