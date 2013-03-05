/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/util/Factory.hh>
#include <karabo/net/Connection.hh>
#include <karabo/util/PluginLoader.hh>

#include "PythonMacros.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace karabo::net;
using namespace std;

void exportSingleElement() {

  //C++: SINGLE_ELEMENT<karabo::util::Connection>    Python: SINGLE_ELEMENT_CONNECTION
  {
    //::karabo::util::GenericElement< karabo::util::SINGLE_ELEMENT< karabo::util::Connection >, std::string >
    typedef std::string EType;
    typedef SINGLE_ELEMENT< Connection > U;
    KARABO_PYTHON_GENERIC_SIMPLE_TYPES(U, EType);
    typedef DefaultValue< U , EType > DefValue; 
    
    bp::class_< DefValue, boost::noncopyable > ("DefaultValueSINGLE_ELEMENTConnection", bp::no_init)
            KARABO_PYTHON_DEFAULT_VALUE
            ;

    bp::class_< GenericElement< U , string >, boost::noncopyable > ("GenericElementSINGLE_ELEMENTConnection", bp::init< Schema & >((bp::arg("expected"))))
            KARABO_PYTHON_GENERIC_ELEMENT_DEFS
            ;

    //::karabo::util::SINGLE_ELEMENT< karabo::util::Connection >    "SINGLE_ELEMENT_CONNECTION"
    typedef bp::class_< U, bp::bases< GenericElement< U, std::string > >, boost::noncopyable > SINGLE_ELEMENT_CONNECTION_exposer_t;
    bp::implicitly_convertible< Schema &, U > ();
    typedef U & (U::*classId_function_type)(::std::string const &);
    SINGLE_ELEMENT_CONNECTION_exposer_t("SINGLE_ELEMENT_CONNECTION", bp::init< Schema & >((bp::arg("expected"))));

  }

  //C++: SINGLE_ELEMENT<karabo::util::PluginLoader>    Python: SINGLE_ELEMENT_PLUGIN_LOADER
    {
        //::karabo::util::GenericElement< karabo::util::SINGLE_ELEMENT< karabo::util::PluginLoader >, std::string >
        typedef std::string EType;
        typedef SINGLE_ELEMENT< PluginLoader > U;
        KARABO_PYTHON_GENERIC_SIMPLE_TYPES(U, EType);
        typedef DefaultValue< U, EType > DefValue;

        bp::class_< DefValue, boost::noncopyable > ("DefaultValueSINGLE_ELEMENTPluginLoader", bp::no_init)
                KARABO_PYTHON_DEFAULT_VALUE
                ;

        bp::class_< GenericElement< U, string >, boost::noncopyable > ("GenericElementSINGLE_ELEMENTPluginLoader", bp::init< Schema & >((bp::arg("expected"))))
                KARABO_PYTHON_GENERIC_ELEMENT_DEFS
                ;

        //::karabo::util::SINGLE_ELEMENT< karabo::util::PluginLoader >    "SINGLE_ELEMENT_PLUGIN_LOADER"
        typedef bp::class_< U, bp::bases< GenericElement< U, std::string > >, boost::noncopyable > SINGLE_ELEMENT_PLUGIN_LOADER_exposer_t;
        bp::implicitly_convertible< Schema &, U > ();
        typedef U & (U::*classId_function_type)(::std::string const &);
        SINGLE_ELEMENT_PLUGIN_LOADER_exposer_t("SINGLE_ELEMENT_PLUGIN_LOADER", bp::init< Schema & >((bp::arg("expected"))));

    }

}//exportSingleElement()

