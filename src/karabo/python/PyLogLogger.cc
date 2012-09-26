/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"

#include <log4cpp/Category.hh>
#include <log4cpp/Priority.hh>
#include <karabo/log/Logger.hh>
#include "PythonFactoryMacros.hh"

namespace bp = boost::python;
using namespace karabo::log;
using namespace log4cpp;
using namespace std;

void exportPyLogLogger() {
    
    {//log4cpp::Category
        bp::class_< log4cpp::Category, boost::noncopyable > ct("Category", bp::no_init);

        ct.def("getInstance"
                , (log4cpp::Category & (*)(string const &))(&log4cpp::Category::getInstance)
                , bp::arg("name")
                , bp::return_internal_reference<> ()).staticmethod("getInstance");
 
        ct.def("getName"
                , (string const & (log4cpp::Category::*)()const) (&log4cpp::Category::getName)
                , bp::return_value_policy<bp::copy_const_reference > ());

        ct.def("getAdditivity"
                , (bool (log4cpp::Category::*)()const) (&log4cpp::Category::getAdditivity));

        ct.def("getChainedPriority"
                , (int (log4cpp::Category::*)()const) (&log4cpp::Category::getChainedPriority));

        ct.def("WARN"
                , (void (log4cpp::Category::*)(string const &))(&log4cpp::Category::warn)
                , bp::arg("message"));

        ct.def("DEBUG"
                , (void (log4cpp::Category::*)(string const &))(&log4cpp::Category::debug)
                , bp::arg("message"));

        ct.def("INFO"
                , (void (log4cpp::Category::*)(string const &))(&log4cpp::Category::info)
                , bp::arg("message"));

        ct.def("ERROR"
                , (void (log4cpp::Category::*)(string const &))(&log4cpp::Category::error)
                , bp::arg("message"));
    }

    {//karabo::log::Logger
        
        KARABO_PYTHON_FACTORY_TYPEDEFS(Logger);
        
        bp::class_< Logger > ("Logger")
            .def("initialize"
                , (void (Logger::*)())(&Logger::initialize))

            .def("logger"
                , (log4cpp::Category & (*)(string const &))(&Logger::logger)
                , bp::arg("logCategory")
                , bp::return_internal_reference<> ()).staticmethod("logger")

            .def("configure"
                , (void (Logger::*)(karabo::util::Hash const &))(&Logger::configure)
                , (bp::arg("input")))
                
            KARABO_PYTHON_FACTORY_BINDING_BASE(Logger)
           ;
        
        bp::register_ptr_to_python< boost::shared_ptr< Logger > >();
    }
    
    { //log4cpp::Priority
        bp::class_<log4cpp::Priority> p("Priority");
        
        bp::enum_<log4cpp::Priority::PriorityLevel>("PriorityLevel")
            .value("EMERG", log4cpp::Priority::EMERG)
            .value("FATAL", log4cpp::Priority::FATAL)
            .value("ALERT", log4cpp::Priority::ALERT)
            .value("CRIT", log4cpp::Priority::CRIT)
            .value("ERROR", log4cpp::Priority::ERROR)
            .value("WARN", log4cpp::Priority::WARN)
            .value("NOTICE", log4cpp::Priority::NOTICE)
            .value("INFO", log4cpp::Priority::INFO)
            .value("DEBUG", log4cpp::Priority::DEBUG)
            .value("NOTSET", log4cpp::Priority::NOTSET)
            .export_values();

            p.def("getPriorityName"
                , (string const & (*)(int))(&log4cpp::Priority::getPriorityName)
                , bp::arg("priority")
                , bp::return_value_policy< bp::copy_const_reference >()).staticmethod("getPriorityName");

            p.def("getPriorityValue"
                , (int (*)(string const &))(&log4cpp::Priority::getPriorityValue)
                , bp::arg("priorityName")).staticmethod("getPriorityValue");

    }
}
