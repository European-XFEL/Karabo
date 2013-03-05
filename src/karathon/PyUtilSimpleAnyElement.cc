/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"

#include <karabo/util/Factory.hh>
#include <karabo/util/SimpleElement.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;

void exportSimpleAnyElement() {

    //karabo::util::SimpleElement< karabo::util::Types::Any >
    bp::class_<SimpleElement<Types::Any> > anyElem("INTERNAL_ANY_ELEMENT", bp::init<Schema &>(bp::arg("expected")));

    bp::implicitly_convertible< Schema &, SimpleElement<Types::Any> >();

    anyElem.def("commit"
            , (void (SimpleElement<Types::Any>::*)())(&SimpleElement<Types::Any>::commit)
            , bp::return_internal_reference<> ());

    anyElem.def("description"
            , (SimpleElement<Types::Any> & (SimpleElement< Types::Any >::*)(string const &))(&SimpleElement<Types::Any>::description)
            , bp::arg("desc")
            , bp::return_internal_reference<> ());

    anyElem.def("init"
            , (SimpleElement<Types::Any> & (SimpleElement<Types::Any>::*)())(&SimpleElement<Types::Any>::init)
            , bp::return_internal_reference<> ());

    anyElem.def("key"
            , (SimpleElement<Types::Any> & (SimpleElement< Types::Any >::*)(string const &))(&SimpleElement<Types::Any>::key)
            , bp::arg("name")
            , bp::return_internal_reference<> ());

    anyElem.def("readOnly"
            , (SimpleElement<Types::Any> & (SimpleElement<Types::Any>::*)())(&SimpleElement<Types::Any>::readOnly)
            , bp::return_internal_reference<> ());

    anyElem.def("reconfigurable"
            , (SimpleElement<Types::Any> & (SimpleElement<Types::Any>::*)())(&SimpleElement<Types::Any >::reconfigurable)
            , bp::return_internal_reference<> ());

}

