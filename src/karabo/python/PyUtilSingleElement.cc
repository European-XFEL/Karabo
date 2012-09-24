/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <exfel/util/Factory.hh>
#include <exfel/net/Connection.hh>
#include <exfel/util/PluginLoader.hh>

#include "PythonMacros.hh"

namespace bp = boost::python;
using namespace exfel::util;
using namespace exfel::net;
using namespace std;

void exportSingleElement() {

  //C++: SINGLE_ELEMENT<exfel::util::Connection>    Python: SINGLE_ELEMENT_CONNECTION
  {
    //::exfel::util::GenericElement< exfel::util::SINGLE_ELEMENT< exfel::util::Connection >, std::string >
    typedef std::string EType;
    typedef SINGLE_ELEMENT< Connection > U;
    EXFEL_PYTHON_GENERIC_SIMPLE_TYPES(U, EType);
    typedef DefaultValue< U , EType > DefValue; 
    
    bp::class_< DefValue, boost::noncopyable > ("DefaultValueSINGLE_ELEMENTConnection", bp::no_init)
            EXFEL_PYTHON_DEFAULT_VALUE
            ;

    bp::class_< GenericElement< U , string >, boost::noncopyable > ("GenericElementSINGLE_ELEMENTConnection", bp::init< Schema & >((bp::arg("expected"))))
            EXFEL_PYTHON_GENERIC_ELEMENT_DEFS
            ;

    //::exfel::util::SINGLE_ELEMENT< exfel::util::Connection >    "SINGLE_ELEMENT_CONNECTION"
    typedef bp::class_< U, bp::bases< GenericElement< U, std::string > >, boost::noncopyable > SINGLE_ELEMENT_CONNECTION_exposer_t;
    bp::implicitly_convertible< Schema &, U > ();
    typedef U & (U::*classId_function_type)(::std::string const &);
    SINGLE_ELEMENT_CONNECTION_exposer_t("SINGLE_ELEMENT_CONNECTION", bp::init< Schema & >((bp::arg("expected"))));

  }

  //C++: SINGLE_ELEMENT<exfel::util::PluginLoader>    Python: SINGLE_ELEMENT_PLUGIN_LOADER
    {
        //::exfel::util::GenericElement< exfel::util::SINGLE_ELEMENT< exfel::util::PluginLoader >, std::string >
        typedef std::string EType;
        typedef SINGLE_ELEMENT< PluginLoader > U;
        EXFEL_PYTHON_GENERIC_SIMPLE_TYPES(U, EType);
        typedef DefaultValue< U, EType > DefValue;

        bp::class_< DefValue, boost::noncopyable > ("DefaultValueSINGLE_ELEMENTPluginLoader", bp::no_init)
                EXFEL_PYTHON_DEFAULT_VALUE
                ;

        bp::class_< GenericElement< U, string >, boost::noncopyable > ("GenericElementSINGLE_ELEMENTPluginLoader", bp::init< Schema & >((bp::arg("expected"))))
                EXFEL_PYTHON_GENERIC_ELEMENT_DEFS
                ;

        //::exfel::util::SINGLE_ELEMENT< exfel::util::PluginLoader >    "SINGLE_ELEMENT_PLUGIN_LOADER"
        typedef bp::class_< U, bp::bases< GenericElement< U, std::string > >, boost::noncopyable > SINGLE_ELEMENT_PLUGIN_LOADER_exposer_t;
        bp::implicitly_convertible< Schema &, U > ();
        typedef U & (U::*classId_function_type)(::std::string const &);
        SINGLE_ELEMENT_PLUGIN_LOADER_exposer_t("SINGLE_ELEMENT_PLUGIN_LOADER", bp::init< Schema & >((bp::arg("expected"))));

    }

}//exportSingleElement()

