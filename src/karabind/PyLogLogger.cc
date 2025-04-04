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
using namespace karabo::data;
using namespace std;


void exportPyLogLogger(py::module_& m) {
    {
        py::enum_<spdlog::level::level_enum>(m, "PriorityLevel", "This enumeration describes priority levels")
              .value("FATAL", spdlog::level::critical)
              .value("ERROR", spdlog::level::err)
              .value("WARN", spdlog::level::warn)
              .value("INFO", spdlog::level::info)
              .value("DEBUG", spdlog::level::debug)
              .value("NOTSET", spdlog::level::off)
              .value("OFF", spdlog::level::off);
    }

    { // spdlog::logger

        py::class_<spdlog::logger, std::shared_ptr<spdlog::logger>>(m, "Category")

              .def_static("getInstance", &spdlog::get, py::arg("name"), py::return_value_policy::reference_internal)
              .def("getName", &spdlog::logger::name)
              .def(
                    "DEBUG",
                    [](std::shared_ptr<spdlog::logger>& self, const std::string& message) {
                        self->debug("{}", message);
                    },
                    py::arg("message"))
              .def(
                    "INFO",
                    [](std::shared_ptr<spdlog::logger>& self, const std::string& message) {
                        self->info("{}", message);
                    },
                    py::arg("message"))
              .def(
                    "WARN",
                    [](std::shared_ptr<spdlog::logger>& self, const std::string& message) {
                        self->warn("{}", message);
                    },
                    py::arg("message"))
              .def(
                    "ERROR",
                    [](std::shared_ptr<spdlog::logger>& self, const std::string& message) {
                        self->error("{}", message);
                    },
                    py::arg("message"));
    }

    { // karabo::log::Logger
        py::class_<Logger>(m, "Logger")
              .def_static("expectedParameters", &Logger::expectedParameters, py::arg("schema"))
              .def_static("configure", &Logger::configure, py::arg("config") = Hash())
              .def_static("useConsole", &Logger::useConsole, py::arg("logger") = "", py::arg("inheritSinks") = true)
              .def_static("useOstream", &Logger::useOstream, py::arg("logger") = "", py::arg("inheritSinks") = true)
              .def_static("useFile", &Logger::useFile, py::arg("logger") = "", py::arg("inheritSinks") = true)
              .def_static("useCache", &Logger::useCache, py::arg("logger") = "", py::arg("inheritSinks") = true)
              .def_static("getCachedContent", &Logger::getCachedContent, py::arg("nmessages") = 0)
              .def_static("reset", &Logger::reset)
              .def_static(
                    "logDebug",
                    [](const std::string& message, const std::string& logger) { Logger::debug(logger, "{}", message); },
                    py::arg("message"), py::arg("logger") = "")
              .def_static(
                    "logInfo",
                    [](const std::string& message, const std::string& logger) { Logger::info(logger, "{}", message); },
                    py::arg("message"), py::arg("logger") = "")
              .def_static(
                    "logWarn",
                    [](const std::string& message, const std::string& logger) { Logger::warn(logger, "{}", message); },
                    py::arg("message"), py::arg("logger") = "")
              .def_static(
                    "logError",
                    [](const std::string& message, const std::string& logger) { Logger::error(logger, "{}", message); },
                    py::arg("message"), py::arg("logger") = "")
              .def_static("setPriority", &Logger::setPriority, py::arg("priority"), py::arg("logger") = "")
              .def_static("getPriority", &Logger::getPriority, py::arg("logger") = "")
              .def_static("getCategory", &Logger::getCategory, py::arg("logger") = "");
    }
}
