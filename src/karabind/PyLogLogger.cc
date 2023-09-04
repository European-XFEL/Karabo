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

#include <karabo/log/Logger.hh>

namespace py = pybind11;
using namespace karabo::log;
using namespace karabo::util;
using namespace krb_log4cpp;
using namespace std;


void exportPyLogLogger(py::module_& m) {
    {
        py::enum_<krb_log4cpp::Priority::PriorityLevel>(m, "PriorityLevel",
                                                        "This enumeration describes priority levels")
              .value("EMERG", krb_log4cpp::Priority::EMERG)
              .value("FATAL", krb_log4cpp::Priority::FATAL)
              .value("ALERT", krb_log4cpp::Priority::ALERT)
              .value("CRIT", krb_log4cpp::Priority::CRIT)
              .value("ERROR", krb_log4cpp::Priority::ERROR)
              .value("WARN", krb_log4cpp::Priority::WARN)
              .value("NOTICE", krb_log4cpp::Priority::NOTICE)
              .value("INFO", krb_log4cpp::Priority::INFO)
              .value("DEBUG", krb_log4cpp::Priority::DEBUG)
              .value("NOTSET", krb_log4cpp::Priority::NOTSET);

        py::class_<krb_log4cpp::Priority>(m, "Priority")
              .def(py::init<>())
              .def_static(
                    "getPriorityName",
                    [](int priority) { return py::cast(krb_log4cpp::Priority::getPriorityName(priority)); },
                    py::arg("priority"))
              .def_static("getPriorityValue", &krb_log4cpp::Priority::getPriorityValue, py::arg("priorityName"));
    }

    { // krb_log4cpp::Category
        py::class_<krb_log4cpp::Category>(m, "Category")
              .def_static("getInstance", &krb_log4cpp::Category::getInstance, py::arg("name"),
                          py::return_value_policy::reference_internal)
              .def("getName", &krb_log4cpp::Category::getName)
              .def("getAdditivity", &krb_log4cpp::Category::getAdditivity)
              .def("getPriority", &krb_log4cpp::Category::getPriority)
              .def_static("getRootPriority", &krb_log4cpp::Category::getRootPriority)
              .def("getChainedPriority", &krb_log4cpp::Category::getChainedPriority)
              .def("setPriority", &krb_log4cpp::Category::setPriority, py::arg("newprio"))
              .def_static("setRootPriority", &krb_log4cpp::Category::setRootPriority, py::arg("newprio"))
              .def("WARN", (void(krb_log4cpp::Category::*)(string const&)) & krb_log4cpp::Category::warn,
                   py::arg("message"))
              .def("DEBUG", (void(krb_log4cpp::Category::*)(string const&)) & krb_log4cpp::Category::debug,
                   py::arg("message"))
              .def("INFO", (void(krb_log4cpp::Category::*)(string const&)) & krb_log4cpp::Category::info,
                   py::arg("message"))
              .def("ERROR", (void(krb_log4cpp::Category::*)(string const&)) & krb_log4cpp::Category::error,
                   py::arg("message"));
    }

    { // karabo::log::Logger
        py::class_<Logger>(m, "Logger")
              .def_static("expectedParameters", &Logger::expectedParameters, py::arg("schema"))
              .def_static("configure", &Logger::configure, py::arg("config") = Hash())
              .def_static("useOstream", &Logger::useOstream, py::arg("category") = "",
                          py::arg("inheritAppenders") = true)
              .def_static("useFile", &Logger::useFile, py::arg("category") = "", py::arg("inheritAppenders") = true)
              .def_static("useCache", &Logger::useCache, py::arg("category") = "", py::arg("inheritAppenders") = true)
              .def_static("getCachedContent", &Logger::getCachedContent, py::arg("nMessages") = 100u)
              .def_static("reset", &Logger::reset)
              .def_static(
                    "logDebug",
                    [](const std::string& message, const std::string& category) {
                        Logger::logDebug(category) << message;
                    },
                    py::arg("message"), py::arg("category") = "")
              .def_static(
                    "logInfo",
                    [](const std::string& message, const std::string& category) {
                        Logger::logInfo(category) << message;
                    },
                    py::arg("message"), py::arg("category") = "")
              .def_static(
                    "logWarn",
                    [](const std::string& message, const std::string& category) {
                        Logger::logWarn(category) << message;
                    },
                    py::arg("message"), py::arg("category") = "")
              .def_static(
                    "logError",
                    [](const std::string& message, const std::string& category) {
                        Logger::logError(category) << message;
                    },
                    py::arg("message"), py::arg("category") = "")
              .def_static("setPriority", &Logger::setPriority, py::arg("priority"), py::arg("category") = "")
              .def_static("getPriority", &Logger::getPriority, py::arg("category") = "")
              .def_static("getCategory", (krb_log4cpp::Category & (*)(const string&)) & Logger::getCategory,
                          py::arg("logCategorie") = "", py::return_value_policy::reference_internal);
    }
}
