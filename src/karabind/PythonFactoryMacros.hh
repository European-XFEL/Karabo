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

#ifndef PYTHONFACTORYMACROS_HH
#define PYTHONFACTORYMACROS_HH

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#define KARABO_PYTHON_FACTORY_CONFIGURATOR_NOCREATE(baseClass)                                                   \
    .def("getClassInfo", &baseClass::getClassInfo)                                                               \
          .def_static("classInfo", &karabo::data::Configurator<baseClass>::classInfo,                            \
                      "Returns C++ introspection info for \"" #baseClass "\"")                                   \
          .def_static(                                                                                           \
                "getRegisteredClasses",                                                                          \
                []() -> py::list {                                                                               \
                    std::vector<std::string> v = baseClass::getRegisteredClasses();                              \
                    return py::cast(v);                                                                          \
                },                                                                                               \
                "Get list of classIds of all C++ classes derived from given C++ base class \"" #baseClass "\".") \
          .def_static("getSchema", &karabo::data::Configurator<baseClass>::getSchema, py::arg("classId"),        \
                      py::arg("rules") = karabo::data::Schema::AssemblyRules(),                                  \
                      "Get schema for C++ class with \"classId\" derived from C++ base class \"" #baseClass      \
                      "\" using assembly \"rules\"")

#define KARABO_PYTHON_FACTORY_CONFIGURATOR(baseClass)                                                                  \
    .def_static("create",                                                                                              \
                (std::shared_ptr<baseClass>(*)(karabo::data::Hash const&, bool const)) &                               \
                      karabo::data::Configurator<baseClass>::create,                                                   \
                py::arg("input"), py::arg("validate") = true,                                                          \
                "The factory method to create instance of C++ class derived from C++ base class \"" #baseClass         \
                "\" using \"input\" configuration. The configuration should have \"classId\" of class to be "          \
                "created\nas a root element.  The last argument is a flag to "                                         \
                "determine if the input configuration should be validated.")                                           \
          .def_static(                                                                                                 \
                "create",                                                                                              \
                (std::shared_ptr<baseClass>(*)(std::string const&, karabo::data::Hash const&, bool const)) &           \
                      karabo::data::Configurator<baseClass>::create,                                                   \
                py::arg("classId"), py::arg("input") = karabo::data::Hash(), py::arg("validate") = true,               \
                "The factory method to create the instance of C++ class with \"classId\" derived from C++ base class " \
                "\"" #baseClass                                                                                        \
                "\" using \"input\" configuration.\n"                                                                  \
                "The last argument is a flag to determine if the input configuration should be validated.")            \
          .def_static(                                                                                                 \
                "createNode",                                                                                          \
                (std::shared_ptr<baseClass>(*)(const std::string&, std::string const&, karabo::data::Hash const&,      \
                                               bool const))(&karabo::data::Configurator<baseClass>::createNode),       \
                py::arg("nodeName"), py::arg("classId"), py::arg("input") = karabo::data::Hash(),                      \
                py::arg("validate") = true,                                                                            \
                "The factory method to create the instance of C++ class with \"classId\" derived from C++ base class " \
                "\"" #baseClass                                                                                        \
                "\" using \"input\" configuration.\n"                                                                  \
                "The last argument is a flag to determine if the input configuration should be validated.")            \
                KARABO_PYTHON_FACTORY_CONFIGURATOR_NOCREATE(baseClass)

#endif /* PYTHONFACTORYMACROS_HH */
