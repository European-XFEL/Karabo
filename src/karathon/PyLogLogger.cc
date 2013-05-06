/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"
#include <karabo/log/Logger.hh>

namespace bp = boost::python;
using namespace karabo::log;
using namespace karabo::util;
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
        bp::class_< Logger > log("Logger", bp::init<Hash const &>((bp::arg("input"))));
            log.def("configure"
                , &Logger::configure
                , (bp::arg("config") = Hash())).staticmethod("configure");
            
            log.def("reset", &Logger::reset).staticmethod("reset");
            
            log.def("getLogger"
                , (log4cpp::Category& (*)(string const &))(&Logger::getLogger)
                , (bp::arg("logCategorie"))
                , bp::return_internal_reference<> ()).staticmethod("getLogger");
                
        bp::register_ptr_to_python< boost::shared_ptr< Logger > >();
    }
    
}
