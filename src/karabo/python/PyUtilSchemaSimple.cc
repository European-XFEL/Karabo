/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>
#include <exfel/util/Schema.hh>

namespace bp = boost::python;
using namespace exfel::util;

void exportPyUtilSchemaSimple() {

    bp::class_< Schema, bp::bases< Hash > > s("Schema", bp::init< >());

    s.def(bp::self_ns::str(bp::self));

    s.def("help"
            , &Schema::help
            , (bp::arg("classId") = ""));

    s.def("validate"
            , &Schema::validate
            , (bp::arg("user"), bp::arg("ignoreAdditionalKeys") = false, bp::arg("ignoreMissingKeys") = false, bp::arg("ignoreDefaults") = false));

    s.def("mergeUserInput"
            , &Schema::mergeUserInput
            , (bp::arg("user"), bp::arg("ignoreUnknownKeys") = false));

    s.def("initParameterDescription"
            , &Schema::initParameterDescription
            , (bp::arg("key"), bp::arg("accessMode") = INIT | WRITE, bp::arg("currentState") = "")
            , bp::return_internal_reference<> ());
    
    s.def("addExternalSchema"
                , &Schema::addExternalSchema
                , (bp::arg("params")) 
                , bp::return_internal_reference<> () );

} //exportPyUtilSchemaSimple()


