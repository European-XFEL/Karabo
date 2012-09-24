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

void exportNonEmptyListElement() {

  //C++: NON_EMPTY_LIST_ELEMENT<exfel::core:Device>    Python: NON_EMPTY_LIST_ELEMENT_DEVICE
  { //::exfel::util::GenericElement< exfel::util::NON_EMPTY_LIST_ELEMENT< exfel::core::Device >, std::string >
    typedef NON_EMPTY_LIST_ELEMENT< Device > U;
    typedef std::string EType;
    EXFEL_PYTHON_GENERIC_SIMPLE_TYPES(U, EType);
    typedef DefaultValue< U, EType > DefValue; 
    bp::class_< DefValue, boost::noncopyable > ("DefaultValueNONEMPTYLIST_ELEMENTDevice", bp::no_init)
            EXFEL_PYTHON_DEFAULT_VALUE
            ;

    bp::class_< GenericElement< U, string >, boost::noncopyable > ("GenericElementNONEMPTYLIST_ELEMENTDevice", bp::init< Schema & >((bp::arg("expected"))))
            EXFEL_PYTHON_GENERIC_ELEMENT_DEFS
            ;

    //::exfel::util::NON_EMPTY_LIST_ELEMENT< exfel::core::Device >  "NON_EMPTY_LIST_ELEMENT_DEVICE"
    typedef bp::class_< U, bp::bases< GenericElement< U, string > >, boost::noncopyable > NON_EMPTY_LIST_ELEMENT_DEVICE_exposer_t;
    bp::implicitly_convertible< Schema &, U > ();
    NON_EMPTY_LIST_ELEMENT_DEVICE_exposer_t("NON_EMPTY_LIST_ELEMENT_DEVICE", bp::init< Schema & >((bp::arg("expected"))));
  }

}//exportNonEmptyListElement()
