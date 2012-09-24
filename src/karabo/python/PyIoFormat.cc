/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"

#include <exfel/util/Hash.hh>
#include <exfel/io/Format.hh>

#include "PythonLoader.hh"

using namespace exfel::util;
using namespace std;
namespace bp = boost::python;

void exportPyIoFormat() {

    typedef exfel::io::Format<exfel::util::Hash> FormatHash;

    EXFEL_PYTHON_FACTORY_TYPEDEFS(FormatHash);
    
    bp::class_< FormatHash, boost::noncopyable > ("FormatHash", bp::no_init)
            .def("unserialize", &FormatHash::unserialize)
            .def("serialize", &FormatHash::serialize)
            EXFEL_PYTHON_FACTORY_BINDING_BASE(FormatHash)   
            ;

    bp::register_ptr_to_python< boost::shared_ptr<FormatHash> >();
}


