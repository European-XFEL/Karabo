/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <exfel/core/Device.hh>

#include "PythonMacros.hh"

namespace bp = boost::python;
using namespace exfel::util;
using namespace exfel::core;
using namespace std;

void exportListElement() {

  //C++: LIST_ELEMENT<exfel::core:Device>    Python: LIST_ELEMENT_DEVICE
  {
    //::exfel::util::GenericElement< exfel::util::LIST_ELEMENT< exfel::core::Device >, std::string >
    typedef LIST_ELEMENT< Device > U;
    typedef std::string EType;
    EXFEL_PYTHON_GENERIC_SIMPLE_TYPES(U, EType);
    typedef DefaultValue< U, EType > DefValue;
    
    bp::class_< DefValue, boost::noncopyable > ("DefaultValueLIST_ELEMENTDevice", bp::no_init)
            EXFEL_PYTHON_DEFAULT_VALUE
            ;

    bp::class_< GenericElement< U, string >, boost::noncopyable > ("GenericElementLIST_ELEMENTDevice", bp::init< Schema & >((bp::arg("expected"))))
            EXFEL_PYTHON_GENERIC_ELEMENT_DEFS
            ;

    //::exfel::util::LIST_ELEMENT< exfel::core::Device >  "LIST_ELEMENT_DEVICE"
    typedef bp::class_< U, bp::bases< GenericElement< U, string > >, boost::noncopyable > LIST_ELEMENT_DEVICE_exposer_t;
    bp::implicitly_convertible< Schema &, U > ();
    LIST_ELEMENT_DEVICE_exposer_t("LIST_ELEMENT_DEVICE", bp::init< Schema & >((bp::arg("expected"))));
  }

}//exportListElement()

