/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
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

#ifndef PYTHONFACTORYMACROS_HH
#define PYTHONFACTORYMACROS_HH

#include <boost/python.hpp>

namespace bp = boost::python;

#define KARABO_PYTHON_FACTORY_CONFIGURATOR(baseClass)                                                                 \
    .def("classInfo", (karabo::util::ClassInfo(*)())(&karabo::util::Configurator<baseClass>::classInfo),              \
         "Returns C++ introspection info for \"" #baseClass "\"")                                                     \
          .staticmethod("classInfo")                                                                                  \
          .def("create",                                                                                              \
               (boost::shared_ptr<baseClass>(*)(karabo::util::Hash const&, bool const))(                              \
                     &karabo::util::Configurator<baseClass>::create),                                                 \
               (bp::arg("input"), bp::arg("validate") = (bool const)(true)),                                          \
               "The factory method to create instance of C++ class derived from C++ base class \"" #baseClass         \
               "\" using \"input\" configuration. The configuration should have \"classId\" of class to be "          \
               "created\nas a root element.  The last argument is a flag to "                                         \
               "determine if the input configuration should be validated.")                                           \
          .def("create",                                                                                              \
               (boost::shared_ptr<baseClass>(*)(std::string const&, karabo::util::Hash const&, bool const))(          \
                     &karabo::util::Configurator<baseClass>::create),                                                 \
               (bp::arg("classId"), bp::arg("input") = karabo::util::Hash(),                                          \
                bp::arg("validate") = (bool const)(true)),                                                            \
               "The factory method to create the instance of C++ class with \"classId\" derived from C++ base class " \
               "\"" #baseClass                                                                                        \
               "\" using \"input\" configuration.\n"                                                                  \
               "The last argument is a flag to determine if the input configuration should be validated.")            \
          .staticmethod("create")                                                                                     \
          .def("createNode",                                                                                          \
               (boost::shared_ptr<baseClass>(*)(const std::string&, std::string const&, karabo::util::Hash const&,    \
                                                bool const))(&karabo::util::Configurator<baseClass>::createNode),     \
               (bp::arg("nodeName"), bp::arg("classId"), bp::arg("input") = karabo::util::Hash(),                     \
                bp::arg("validate") = (bool const)(true)),                                                            \
               "The factory method to create the instance of C++ class with \"classId\" derived from C++ base class " \
               "\"" #baseClass                                                                                        \
               "\" using \"input\" configuration.\n"                                                                  \
               "The last argument is a flag to determine if the input configuration should be validated.")            \
          .staticmethod("createNode")                                                                                 \
          .def("createChoice",                                                                                        \
               (boost::shared_ptr<baseClass>(*)(std::string const&, karabo::util::Hash const&, bool const))(          \
                     &karabo::util::Configurator<baseClass>::createChoice),                                           \
               (bp::arg("choiceName"), bp::arg("input") = karabo::util::Hash(),                                       \
                bp::arg("validate") = (bool const)(true)),                                                            \
               "The factory method to create the instance of C++ class with \"classId\" derived from C++ base class " \
               "\"" #baseClass                                                                                        \
               "\" using \"input\" configuration.\n"                                                                  \
               "The last argument is a flag to determine if the input configuration should be validated.")            \
          .staticmethod("createChoice")                                                                               \
          .def("createList",                                                                                          \
               (boost::shared_ptr<baseClass>(*)(std::string const&, karabo::util::Hash const&, bool const))(          \
                     &karabo::util::Configurator<baseClass>::createList),                                             \
               (bp::arg("listName"), bp::arg("input") = karabo::util::Hash(),                                         \
                bp::arg("validate") = (bool const)(true)),                                                            \
               "The factory method to create the instance of C++ class with \"classId\" derived from C++ base class " \
               "\"" #baseClass                                                                                        \
               "\" using \"input\" configuration.\n"                                                                  \
               "The last argument is a flag to determine if the input configuration should be validated.")            \
          .staticmethod("createList")                                                                                 \
          .def("getClassInfo", (karabo::util::ClassInfo(karabo::util::Configurator<baseClass>::*)()                   \
                                      const)(&karabo::util::Configurator<baseClass>::getClassInfo))                   \
          .def("getRegisteredClasses",                                                                                \
               (std::vector<std::string>(*)())(&karabo::util::Configurator<baseClass>::getRegisteredClasses),         \
               "Get list of classIds of all C++ classes derived from given C++ base class \"" #baseClass "\".")       \
          .staticmethod("getRegisteredClasses")                                                                       \
          .def("getSchema",                                                                                           \
               (karabo::util::Schema(*)(std::string const&, karabo::util::Schema::AssemblyRules const&))(             \
                     &karabo::util::Configurator<baseClass>::getSchema),                                              \
               (bp::arg("classId"), bp::arg("rules") = karabo::util::Schema::AssemblyRules()),                        \
               "Get schema for C++ class with \"classId\" derived from C++ base class \"" #baseClass                  \
               "\" using assembly \"rules\"")                                                                         \
          .staticmethod("getSchema")                                                                                  \
          .attr("__karabo_cpp_classid__") = baseClass::classInfo().getClassId()

#endif /* PYTHONFACTORYMACROS_HH */
