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

#include <karabo/log/Logger.hh>

#include "PythonFactoryMacros.hh"
#include "boost/python.hpp"

namespace bp = boost::python;
using namespace karabo::log;
using namespace karabo::util;
using namespace krb_log4cpp;
using namespace std;


namespace karathon {

    struct PriorityWrap {
        static bp::object getPriorityNamePy(int priority) {
            return bp::object(krb_log4cpp::Priority::getPriorityName(priority));
        }
    };

    struct LoggerWrap {
        static void logDebug(const std::string& message, const std::string& category) {
            karabo::log::Logger::logDebug(category) << message;
        }

        static void logInfo(const std::string& message, const std::string& category) {
            karabo::log::Logger::logInfo(category) << message;
        }

        static void logWarn(const std::string& message, const std::string& category) {
            karabo::log::Logger::logWarn(category) << message;
        }

        static void logError(const std::string& message, const std::string& category) {
            karabo::log::Logger::logError(category) << message;
        }
    };

} // namespace karathon


void exportPyLogLogger() {
    {
        bp::enum_<krb_log4cpp::Priority::PriorityLevel>("PriorityLevel", "This enumeration describes priority levels")
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

        bp::class_<krb_log4cpp::Priority> p("Priority", bp::init<>());

        p.def("getPriorityName", &karathon::PriorityWrap::getPriorityNamePy, (bp::arg("priority")))
              .staticmethod("getPriorityName");

        p.def("getPriorityValue", (int (*)(string const&))(&krb_log4cpp::Priority::getPriorityValue),
              (bp::arg("priorityName")))
              .staticmethod("getPriorityValue");
    }

    { // krb_log4cpp::Category
        bp::class_<krb_log4cpp::Category, boost::noncopyable> ct("Category", bp::no_init);

        ct.def("getInstance", (krb_log4cpp::Category & (*)(string const&))(&krb_log4cpp::Category::getInstance),
               bp::arg("name"), bp::return_internal_reference<>())
              .staticmethod("getInstance");

        ct.def("getName", (string const& (krb_log4cpp::Category::*)() const)(&krb_log4cpp::Category::getName),
               bp::return_value_policy<bp::copy_const_reference>());

        ct.def("getAdditivity", (bool(krb_log4cpp::Category::*)() const)(&krb_log4cpp::Category::getAdditivity));

        ct.def("getPriority", (int(krb_log4cpp::Category::*)() const)(&krb_log4cpp::Category::getPriority));

        ct.def("getRootPriority", (int (*)())(&krb_log4cpp::Category::getRootPriority)).staticmethod("getRootPriority");

        ct.def("getChainedPriority",
               (int(krb_log4cpp::Category::*)() const)(&krb_log4cpp::Category::getChainedPriority));

        ct.def("setPriority", (void(krb_log4cpp::Category::*)(int))(&krb_log4cpp::Category::setPriority),
               bp::arg("newprio"));

        ct.def("setRootPriority", (void (*)(int))(&krb_log4cpp::Category::setRootPriority), bp::arg("newprio"))
              .staticmethod("setRootPriority");

        ct.def("WARN", (void(krb_log4cpp::Category::*)(string const&))(&krb_log4cpp::Category::warn),
               bp::arg("message"));

        ct.def("DEBUG", (void(krb_log4cpp::Category::*)(string const&))(&krb_log4cpp::Category::debug),
               bp::arg("message"));

        ct.def("INFO", (void(krb_log4cpp::Category::*)(string const&))(&krb_log4cpp::Category::info),
               bp::arg("message"));

        ct.def("ERROR", (void(krb_log4cpp::Category::*)(string const&))(&krb_log4cpp::Category::error),
               bp::arg("message"));
    }

    { // karabo::log::Logger
        bp::class_<Logger>("Logger", bp::no_init)
              .def("expectedParameters", &Logger::expectedParameters, (bp::arg("schema")))
              .staticmethod("expectedParameters")
              .def("configure", &Logger::configure, (bp::arg("config") = Hash()))
              .staticmethod("configure")
              .def("useOstream", &Logger::useOstream, (bp::arg("category") = "", bp::arg("inheritAppenders") = true))
              .staticmethod("useOstream")
              .def("useFile", &Logger::useFile, (bp::arg("category") = "", bp::arg("inheritAppenders") = true))
              .staticmethod("useFile")
              .def("useCache", &Logger::useCache, (bp::arg("category") = "", bp::arg("inheritAppenders") = true))
              .staticmethod("useCache")
              .def("getCachedContent", &Logger::getCachedContent, (bp::arg("nMessages") = 100u))
              .staticmethod("getCachedContent")
              .def("reset", &Logger::reset)
              .staticmethod("reset")
              .def("logDebug", &karathon::LoggerWrap::logDebug, (bp::arg("message"), bp::arg("category") = ""))
              .staticmethod("logDebug")
              .def("logInfo", &karathon::LoggerWrap::logInfo, (bp::arg("message"), bp::arg("category") = ""))
              .staticmethod("logInfo")
              .def("logWarn", &karathon::LoggerWrap::logWarn, (bp::arg("message"), bp::arg("category") = ""))
              .staticmethod("logWarn")
              .def("logError", &karathon::LoggerWrap::logError, (bp::arg("message"), bp::arg("category") = ""))
              .staticmethod("logError")
              .def("setPriority", &Logger::setPriority, (bp::arg("priority"), bp::arg("category") = ""))
              .staticmethod("setPriority")
              .def("getPriority", &Logger::getPriority, (bp::arg("category") = ""),
                   bp::return_value_policy<bp::copy_const_reference>())
              .staticmethod("getPriority")
              .def("getCategory", (krb_log4cpp::Category & (*)(const string&))(&Logger::getCategory),
                   (bp::arg("logCategorie") = ""), bp::return_internal_reference<>())
              .staticmethod("getCategory") KARABO_PYTHON_FACTORY_CONFIGURATOR(Logger);
        bp::register_ptr_to_python<boost::shared_ptr<Logger> >();
    }
}
