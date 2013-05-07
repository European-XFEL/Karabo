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

void exportNonEmptyListElement() {

  //C++: NON_EMPTY_LIST_ELEMENT<karabo::core:Device>    Python: NON_EMPTY_LIST_ELEMENT_DEVICE
  { //karabo::util::GenericElement< karabo::util::NON_EMPTY_LIST_ELEMENT< karabo::core::Device >, std::string >
    typedef NON_EMPTY_LIST_ELEMENT< Device > U;
    typedef std::string EType;
    KARABO_PYTHON_GENERIC_SIMPLE_TYPES(U, EType);
    typedef DefaultValue< U, EType > DefValue; 
    bp::class_< DefValue, boost::noncopyable > ("DefaultValueNONEMPTYLIST_ELEMENTDevice", bp::no_init)
            KARABO_PYTHON_DEFAULT_VALUE
            ;

    bp::class_< GenericElement< U, string >, boost::noncopyable > ("GenericElementNONEMPTYLIST_ELEMENTDevice", bp::init< Schema & >((bp::arg("expected"))))
            KARABO_PYTHON_GENERIC_ELEMENT_DEFS
            ;

    //karabo::util::NON_EMPTY_LIST_ELEMENT< karabo::core::Device >  "NON_EMPTY_LIST_ELEMENT_DEVICE"
    bp::class_< U, bp::bases< GenericElement< U, string > >, boost::noncopyable > ("NON_EMPTY_LIST_ELEMENT_DEVICE", bp::init< Schema & >(bp::arg("expected")));
    bp::implicitly_convertible< Schema &, U > ();   
  }

}//exportNonEmptyListElement()
