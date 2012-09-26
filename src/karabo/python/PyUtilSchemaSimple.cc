/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>
#include <karabo/util/Schema.hh>

namespace bp = boost::python;
using namespace karabo::util;

void exportPyUtilSchemaSimple() {

    bp::class_< Schema, bp::bases< Hash > > s("Schema", bp::init< >());

    s.def(bp::self_ns::str(bp::self));

    s.def("help"
            , &Schema::help
            , (bp::arg("classId") = ""));

    s.def("validate"
            , &Schema::validate
            , (bp::arg("user"), bp::arg("injectDefaults") = true, bp::arg("allowUnrootedConfiguration") = false, bp::arg("allowAdditionalKeys") = false, bp::arg("allowMissingKeys") = false));

    s.def("mergeUserInput", &Schema::mergeUserInput);

    s.def("initParameterDescription"
            , &Schema::initParameterDescription
            , (bp::arg("key"), bp::arg("accessMode") = INIT | WRITE, bp::arg("currentState") = "")
            , bp::return_internal_reference<> ());
    
    s.def("addExternalSchema"
                , &Schema::addExternalSchema
                , (bp::arg("params")) 
                , bp::return_internal_reference<> () );

} //exportPyUtilSchemaSimple()


