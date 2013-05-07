/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <karabo/core/Device.hh>

#include "PythonMacros.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace karabo::core;
using namespace std;

void exportListElement() {

  //C++: LIST_ELEMENT<karabo::core:Device>    Python: LIST_ELEMENT_DEVICE
  {
    //::karabo::util::GenericElement< karabo::util::LIST_ELEMENT< karabo::core::Device >, std::string >
    typedef LIST_ELEMENT< Device > U;
    typedef std::string EType;
    KARABO_PYTHON_GENERIC_SIMPLE_TYPES(U, EType);
    typedef DefaultValue< U, EType > DefValue;
    
    bp::class_< DefValue, boost::noncopyable > ("DefaultValueLIST_ELEMENTDevice", bp::no_init)
            KARABO_PYTHON_DEFAULT_VALUE
            ;

    bp::class_< GenericElement< U, string >, boost::noncopyable > ("GenericElementLIST_ELEMENTDevice", bp::init< Schema & >((bp::arg("expected"))))
            KARABO_PYTHON_GENERIC_ELEMENT_DEFS
            ;

    //::karabo::util::LIST_ELEMENT< karabo::core::Device >  "LIST_ELEMENT_DEVICE"
    typedef bp::class_< U, bp::bases< GenericElement< U, string > >, boost::noncopyable > LIST_ELEMENT_DEVICE_exposer_t;
    bp::implicitly_convertible< Schema &, U > ();
    LIST_ELEMENT_DEVICE_exposer_t("LIST_ELEMENT_DEVICE", bp::init< Schema & >((bp::arg("expected"))));
  }

}//exportListElement()

