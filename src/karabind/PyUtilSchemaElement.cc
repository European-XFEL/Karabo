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

#include "PyUtilSchemaElement.hh"

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
#include "Wrapper.hh"


namespace py = pybind11;
using namespace karabo::util;
using namespace karabo::io;
using namespace std;
using namespace karabind;


void exportPyUtilSchemaElement(py::module_& m) {
    /////////////////////////////////////////////////////////////
    // DefaultValue<SimpleElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    // and DefaultValue<PathElement, std::string >
    // and DefaultValue<ByteArrayElement, ByteArray >
    /////////////////////////////////////////////////////////////

    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<int>, int, INT32)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<unsigned int>, unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<long long>, long long, INT64)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<unsigned long long>, unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<float>, float, FLOAT)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<double>, double, DOUBLE)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<string>, string, STRING)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(SimpleElement<bool>, bool, BOOL)
    KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(PathElement, string, PATH)
    // KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(ByteArrayElement, ByteArray, BYTEARRAY)
    {
        typedef DefaultValue<ByteArrayElement, ByteArray> DefValue;
        py::class_<DefValue>(m, "DefaultValueBYTEARRAY")
              .def(
                    "defaultValue",
                    [](DefValue& self, const py::object& val) -> ByteArrayElement& {
                        auto value = wrapper::copyPyToByteArray(val);
                        return self.defaultValue(value);
                    },
                    py::arg("defaultValue"))
              .def(
                    "defaultValueFromString",
                    [](DefValue& self, const std::string& valstr) -> ByteArrayElement& {
                        return self.defaultValueFromString(valstr);
                    },
                    py::arg("defaultValue"))
              .def("noDefaultValue", [](DefValue& self) -> ByteArrayElement& { return self.noDefaultValue(); });
    }


    ///////////////////////////////////////////////////////////////////////////////////////////
    // DefaultValue<VectorElement< EType, 1, std::vector >, std::vector< EType > > where EType:
    // BOOL, INT32, UINT32, INT64, UINT64, DOUBLE, STRING
    ///////////////////////////////////////////////////////////////////////////////////////////

    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(int, INT32)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(unsigned int, UINT32)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(long long, INT64)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(unsigned long long, UINT64)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(float, FLOAT)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(double, DOUBLE)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(std::string, STRING)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(bool, BOOL)
    KARABO_PYTHON_VECTOR_DEFAULT_VALUE(char, CHAR)

    ///////////////////////////////////////////////////////////////
    // AlarmSpecific<SimpleElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL

    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<int>, int, ReadOnlySpecific, INT32)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<unsigned int>, unsigned int, ReadOnlySpecific, UINT32)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<long long>, long long, ReadOnlySpecific, INT64)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<unsigned long long>, unsigned long long, ReadOnlySpecific, UINT64)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<float>, float, ReadOnlySpecific, FLOAT)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<double>, double, ReadOnlySpecific, DOUBLE)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<std::string>, std::string, ReadOnlySpecific, STRING)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<bool>, bool, ReadOnlySpecific, BOOL)

    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<int>, int, RollingStatsSpecific, INT32)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<unsigned int>, unsigned int, RollingStatsSpecific, UINT32)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<long long>, long long, RollingStatsSpecific, INT64)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<unsigned long long>, unsigned long long, RollingStatsSpecific,
                                        UINT64)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<float>, float, RollingStatsSpecific, FLOAT)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<double>, double, RollingStatsSpecific, DOUBLE)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<std::string>, std::string, RollingStatsSpecific, STRING)
    KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(SimpleElement<bool>, bool, RollingStatsSpecific, BOOL)

    ///////////////////////////////////////////////////////////////
    // RollingStatSpecific<SimpleElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL

    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<int>, int, INT32)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<unsigned int>, unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<long long>, long long, INT64)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<unsigned long long>, unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<float>, float, FLOAT)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<double>, double, DOUBLE)
    KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(SimpleElement<bool>, bool, BOOL)

    ///////////////////////////////////////////////////////////////
    // ReadOnlySpecific<SimpleElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    //  and ReadOnlySpecific<PathElement, std::string >
    //  and ReadOnlySpecific<ByteArrayElement, ByteArray >


    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<int>, int, INT32)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<unsigned int>, unsigned int, UINT32)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<long long>, long long, INT64)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<unsigned long long>, unsigned long long, UINT64)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<float>, float, FLOAT)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<double>, double, DOUBLE)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<std::string>, std::string, STRING)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(SimpleElement<bool>, bool, BOOL)
    KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(PathElement, std::string, PATH)
    // KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(ByteArrayElement, ByteArray, BYTEARRAY)
    {
        typedef ReadOnlySpecific<ByteArrayElement, ByteArray> ReadOnlySpec;
        typedef AlarmSpecific<ByteArrayElement, ByteArray, ReadOnlySpec> AlarmSpec;
        typedef RollingStatsSpecific<ByteArrayElement, ByteArray> RollingStatsSpec;
        py::class_<ReadOnlySpec>(m, "ReadOnlySpecificBYTEARRAY")
              .def("initialValue",
                   [](ReadOnlySpec& self, const py::object& o) -> ReadOnlySpec& {
                       auto value = wrapper::copyPyToByteArray(o);
                       return self.initialValue(value);
                   })
              .def("defaultValue",
                   [](ReadOnlySpec& self, const py::object& o) -> ReadOnlySpec& {
                       auto value = wrapper::copyPyToByteArray(o);
                       return self.initialValue(value);
                   })
              .def("archivePolicy",
                   [](ReadOnlySpec& self, const py::object& o) -> ReadOnlySpec& {
                       auto policy = o.cast<karabo::util::Schema::ArchivePolicy>();
                       return self.archivePolicy(policy);
                   })
              .def("commit", &ReadOnlySpec::commit);
    }

    ////////////////////////////////////////////////////////////////////
    // ReadOnlySpecific<VectorElement< EType >, EType >, where EType:
    // INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    ////////////////////////////////////////////////////////////////////

    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(int, INT32)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(unsigned int, UINT32)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(long long, INT64)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(unsigned long long, UINT64)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(float, FLOAT)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(double, DOUBLE)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(std::string, STRING)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(bool, BOOL)
    KARABO_PYTHON_VECTOR_READONLYSPECIFIC(char, CHAR)

    //////////////////////////////////////////////////////////////////////
    // Binding SimpleElement< EType >, where EType:
    // int, long long, double, string, bool
    // In Python: INT32_ELEMENT, UINT32_ELEMENT, INT64_ELEMENT, UINT64_ELEMENT, DOUBLE_ELEMENT,
    // STRING_ELEMENT, BOOL_ELEMENT

    KARABO_PYTHON_SIMPLE(int, INT32)
    KARABO_PYTHON_SIMPLE(unsigned int, UINT32)
    KARABO_PYTHON_SIMPLE(long long, INT64)
    KARABO_PYTHON_SIMPLE(unsigned long long, UINT64)
    KARABO_PYTHON_SIMPLE(float, FLOAT)
    KARABO_PYTHON_SIMPLE(double, DOUBLE)
    KARABO_PYTHON_SIMPLE(std::string, STRING)
    KARABO_PYTHON_SIMPLE(bool, BOOL)

    //////////////////////////////////////////////////////////////////////
    // Binding ByteArrayElement
    // In Python : BYTE_ARRAY
    {
        // py::implicitly_convertible<Schema&, ByteArrayElement>();
        py::class_<ByteArrayElement>(m, "BYTEARRAY_ELEMENT")
              .def(py::init<Schema&>(), py::arg("expected")) KARABO_PYTHON_COMMON_ATTRIBUTES(ByteArrayElement);
    }

    //////////////////////////////////////////////////////////////////////
    // Binding PathElement
    // In Python : PATH_ELEMENT
    {
        // py::implicitly_convertible<Schema&, PathElement>();
        typedef std::string EType;
        py::class_<PathElement>(m, "PATH_ELEMENT")
              .def(py::init<Schema&>(), py::arg("expected")) KARABO_PYTHON_COMMON_ATTRIBUTES(PathElement)
                    KARABO_PYTHON_OPTIONS_NONVECTOR(PathElement)
              .def("isInputFile", &PathElement::isInputFile, py::return_value_policy::reference_internal)
              .def("isOutputFile", &PathElement::isOutputFile, py::return_value_policy::reference_internal)
              .def("isDirectory", &PathElement::isDirectory, py::return_value_policy::reference_internal);
    }

    //////////////////////////////////////////////////////////////////////
    // Binding VectorElement< EType, std::vector >
    // In Python : VECTOR_INT32_ELEMENT, VECTOR_UINT32_ELEMENT,
    // VECTOR_INT64_ELEMENT, VECTOR_UINT64_ELEMENT, VECTOR_DOUBLE_ELEMENT,
    // VECTOR_STRING_ELEMENT, VECTOR_BOOL_ELEMENT, VECTOR_CHAR_ELEMENT


    KARABO_PYTHON_VECTOR(int, INT32)
    KARABO_PYTHON_VECTOR(unsigned int, UINT32)
    KARABO_PYTHON_VECTOR(long long, INT64)
    KARABO_PYTHON_VECTOR(unsigned long long, UINT64)
    KARABO_PYTHON_VECTOR(float, FLOAT)
    KARABO_PYTHON_VECTOR(double, DOUBLE)
    KARABO_PYTHON_VECTOR(string, STRING)
    KARABO_PYTHON_VECTOR(bool, BOOL)
    KARABO_PYTHON_VECTOR(char, CHAR)

    //////////////////////////////////////////////////////////////////////
    // Binding NDArrayElement
    // In Python : NDARRAY_ELEMENT
    {
        // py::implicitly_convertible<Schema&, NDArrayElement>();
        py::class_<NDArrayElement>(m, "NDARRAY_ELEMENT")
              .def(py::init<Schema&>(), py::arg("expected"))
              .def(
                    "dtype",
                    [](karabo::util::NDArrayElement& self, const py::object& otype) {
                        Types::ReferenceType reftype = wrapper::pyObjectToCppType(otype);
                        return self.dtype(reftype);
                    },
                    py::arg("type"), py::return_value_policy::reference_internal)
              .def(
                    "shape",
                    [](karabo::util::NDArrayElement& self, const py::object& obj) {
                        if (py::isinstance<py::str>(obj)) {
                            return self.shape(obj.cast<std::string>());
                        } else if (py::isinstance<py::list>(obj)) {
                            const std::vector<long long> v = obj.cast<std::vector<long long>>();
                            const std::string shapeStr = karabo::util::toString<long long>(v);
                            return self.shape(shapeStr);
                        } else {
                            throw KARABO_PYTHON_EXCEPTION(
                                  "Python type of the shape value of NDArrayElement must be a list or a string");
                        }
                    },
                    py::arg("shape"), py::return_value_policy::reference_internal)
              .def("unit", &NDArrayElement::unit, py::return_value_policy::reference_internal)
              .def("metricPrefix", &NDArrayElement::metricPrefix, py::return_value_policy::reference_internal)
              .def("observerAccess", &NDArrayElement::observerAccess, py::return_value_policy::reference_internal)
              .def("userAccess", &NDArrayElement::userAccess, py::return_value_policy::reference_internal)
              .def("operatorAccess", &NDArrayElement::operatorAccess, py::return_value_policy::reference_internal)
              .def("expertAccess", &NDArrayElement::expertAccess, py::return_value_policy::reference_internal)
              .def("adminAccess", &NDArrayElement::adminAccess, py::return_value_policy::reference_internal)
              .def("description", &NDArrayElement::description, py::return_value_policy::reference_internal)
              .def("displayedName", &NDArrayElement::displayedName, py::return_value_policy::reference_internal)
              .def("init", &NDArrayElement::init, py::return_value_policy::reference_internal)
              .def("key", &NDArrayElement::key, py::return_value_policy::reference_internal)
              .def("readOnly", &NDArrayElement::readOnly, py::return_value_policy::reference_internal)
              .def("reconfigurable", &NDArrayElement::reconfigurable, py::return_value_policy::reference_internal)
              .def(
                    "setAllowedActions",
                    [](NDArrayElement& self, const py::object& actions) -> NDArrayElement& {
                        self.setAllowedActions(wrapper::fromPySequenceToVectorString(actions));
                        return self;
                    },
                    py::arg("actions"), py::return_value_policy::reference_internal, R"pbdoc(
                        Specify one or more actions that are allowed on this node.
                        If a Karabo device specifies allowed actions for a node,
                        that means that it offers a specific slot interface to operate
                        on this node. Which allowed actions require which interface
                        is defined elsewhere
                    )pbdoc")
              .def("skipValidation", &NDArrayElement::skipValidation, py::return_value_policy::reference_internal)
              .def("commit", &NDArrayElement::commit, py::return_value_policy::reference_internal);
    }
}
