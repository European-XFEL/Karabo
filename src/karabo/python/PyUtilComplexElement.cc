/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"
#include <exfel/util/ComplexElement.hh>

namespace bp = boost::python;
using namespace exfel::util;
using namespace std;

void exportComplexElement() {
    //exfel::util::ComplexElement    
    bp::class_<ComplexElement, boost::noncopyable> complElem("ComplexElement", bp::init< Schema & >(bp::arg("expected")));
    bp::implicitly_convertible< Schema &, ComplexElement > ();

    complElem.def("advanced"
            , (ComplexElement & (ComplexElement::*)())(&ComplexElement::advanced)
            , bp::return_internal_reference<>());

    complElem.def("allowedStates"
            , (ComplexElement & (ComplexElement::*)(string const &))(&ComplexElement::allowedStates)
            , bp::arg("states")
            , bp::return_internal_reference<>());

    complElem.def("assignmentInternal"
            , (ComplexElement & (ComplexElement::*)())(&ComplexElement::assignmentInternal)
            , bp::return_internal_reference<>());

    complElem.def("assignmentMandatory"
            , (ComplexElement & (ComplexElement::*)())(&ComplexElement::assignmentMandatory)
            , bp::return_internal_reference<>());

    complElem.def("assignmentOptional"
            , (ComplexElement & (ComplexElement::*)())(&ComplexElement::assignmentOptional)
            , bp::return_internal_reference<>());

    complElem.def("commit"
            , (Schema & (ComplexElement::*)()) (&ComplexElement::commit)
            , bp::return_internal_reference<>());

    complElem.def("description"
            , (ComplexElement & (ComplexElement::*)(string const &))(&ComplexElement::description)
            , bp::arg("desc")
            , bp::return_internal_reference<>());

    complElem.def("displayType"
            , (ComplexElement & (ComplexElement::*)(string const &))(&ComplexElement::displayType)
            , bp::arg("type")
            , bp::return_internal_reference<>());

    complElem.def("displayedName"
            , (ComplexElement & (ComplexElement::*)(string const &))(&ComplexElement::displayedName)
            , bp::arg("displayedName")
            , bp::return_internal_reference<>());

    complElem.def("init"
            , (ComplexElement & (ComplexElement::*)())(&::exfel::util::ComplexElement::init)
            , bp::return_internal_reference<>());

    complElem.def("initAndRead"
            , (ComplexElement & (ComplexElement::*)())(&::exfel::util::ComplexElement::initAndRead)
            , bp::return_internal_reference<>());

    complElem.def("key"
            , (ComplexElement & (ComplexElement::*)(string const &))(&ComplexElement::key)
            , bp::arg("key")
            , bp::return_internal_reference<>());

    complElem.def("readOnly"
            , (ComplexElement & (ComplexElement::*)())(&::exfel::util::ComplexElement::readOnly)
            , bp::return_internal_reference<>());

    complElem.def("reconfigurable"
            , (ComplexElement & (ComplexElement::*)())(&ComplexElement::reconfigurable)
            , bp::return_internal_reference<>());

    complElem.def("reconfigureAndRead"
            , (ComplexElement & (ComplexElement::*)())(&ComplexElement::reconfigureAndRead)
            , bp::return_internal_reference<>());

}