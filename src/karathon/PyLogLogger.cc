/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"
#include <karabo/log/Logger.hh>
#include "PythonFactoryMacros.hh"

namespace bp = boost::python;
using namespace karabo::log;
using namespace karabo::util;
using namespace krb_log4cpp;
using namespace std;


namespace karathon {
    
    // make 'create' to be public
    struct AppenderConfiguratorWrap1 : AppenderConfigurator {
        krb_log4cpp::Appender* create() = 0;
    };
    
    struct AppenderConfiguratorWrap : AppenderConfiguratorWrap1, bp::wrapper<AppenderConfiguratorWrap1> {
        krb_log4cpp::Appender* create() {
            return this->get_override("create")();
        }
    };
}

void exportPyLogLogger() {
    { // AppenderConfigurator
        bp::class_<karathon::AppenderConfiguratorWrap, boost::noncopyable>("AppenderConfigurator", bp::no_init)
                .def("create", bp::pure_virtual((krb_log4cpp::Appender* (karathon::AppenderConfiguratorWrap1::*)())&karathon::AppenderConfiguratorWrap1::create), bp::return_internal_reference<>())
                KARABO_PYTHON_FACTORY_CONFIGURATOR(AppenderConfigurator)
                ;
    }
    
    { // CategoryConfigurator
        bp::class_<CategoryConfigurator>("CategoryConfigurator", bp::init<const Hash&>())
                .def("setup", &CategoryConfigurator::setup)
                KARABO_PYTHON_FACTORY_CONFIGURATOR(CategoryConfigurator)
                ;
    }
    

    {//krb_log4cpp::Category
        bp::class_< krb_log4cpp::Category, boost::noncopyable > ct("Category", bp::no_init)
                ;

        ct.def("getInstance"
               , (krb_log4cpp::Category & (*)(string const &))(&krb_log4cpp::Category::getInstance)
               , bp::arg("name")
               , bp::return_internal_reference<> ()).staticmethod("getInstance");

        ct.def("getName"
               , (string const & (krb_log4cpp::Category::*)()const) (&krb_log4cpp::Category::getName)
               , bp::return_value_policy<bp::copy_const_reference > ());

        ct.def("getAdditivity"
               , (bool (krb_log4cpp::Category::*)()const) (&krb_log4cpp::Category::getAdditivity));

        ct.def("getChainedPriority"
               , (int (krb_log4cpp::Category::*)()const) (&krb_log4cpp::Category::getChainedPriority));

        ct.def("WARN"
               , (void (krb_log4cpp::Category::*)(string const &))(&krb_log4cpp::Category::warn)
               , bp::arg("message"));

        ct.def("DEBUG"
               , (void (krb_log4cpp::Category::*)(string const &))(&krb_log4cpp::Category::debug)
               , bp::arg("message"));

        ct.def("INFO"
               , (void (krb_log4cpp::Category::*)(string const &))(&krb_log4cpp::Category::info)
               , bp::arg("message"));

        ct.def("ERROR"
               , (void (krb_log4cpp::Category::*)(string const &))(&krb_log4cpp::Category::error)
               , bp::arg("message"));

    }

    {//karabo::log::Logger
        bp::class_< Logger >("Logger", bp::init<Hash const &>((bp::arg("input"))))
                .def("configure"
                     , &Logger::configure
                     , (bp::arg("config") = Hash())).staticmethod("configure")
                .def("reset", &Logger::reset).staticmethod("reset")
                .def("getLogger"
                     , (krb_log4cpp::Category & (*)(string const &))(&Logger::getLogger)
                     , (bp::arg("logCategorie"))
                     , bp::return_internal_reference<> ()).staticmethod("getLogger")
                KARABO_PYTHON_FACTORY_CONFIGURATOR(Logger)
                ;

        bp::register_ptr_to_python< boost::shared_ptr< Logger > >();
    }

}
