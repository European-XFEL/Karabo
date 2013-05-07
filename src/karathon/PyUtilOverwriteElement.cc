/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"

#include <karabo/util/OverwriteElement.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

void exportOverwriteElement() {
    //karabo::util::OverwriteElement , in Python: OVERWRITE_ELEMENT
    bp::class_<OverwriteElement > elem("OVERWRITE_ELEMENT", bp::init< >());
    elem.def(bp::init< Schema & >(bp::arg("expected")));
    bp::implicitly_convertible< Schema &, OverwriteElement > ();

    elem.def("commit"
            , (OverwriteElement & (OverwriteElement::*)())(&OverwriteElement::commit)
            , bp::return_internal_reference<> ());

    elem.def("commit"
            , (OverwriteElement & (OverwriteElement::*)(Schema &))(&OverwriteElement::commit)
            , bp::arg("expected")
            , bp::return_internal_reference<>());

    elem.def("key"
            , (OverwriteElement & (OverwriteElement::*)(string const &))(&OverwriteElement::key)
            , bp::arg("name")
            , bp::return_internal_reference<>());

}
