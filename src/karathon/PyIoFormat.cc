/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"

#include <karabo/util/Hash.hh>
#include <karabo/io/Format.hh>

#include "PythonFactoryMacros.hh"

using namespace karabo::util;
using namespace std;
namespace bp = boost::python;

void exportPyIoFormat() {

    typedef karabo::io::Format<karabo::util::Hash> FormatHash;

    KARABO_PYTHON_FACTORY_TYPEDEFS(FormatHash);
    
    bp::class_< FormatHash, boost::noncopyable > ("FormatHash", bp::no_init)
            .def("unserialize", &FormatHash::unserialize)
            .def("serialize", &FormatHash::serialize)
            KARABO_PYTHON_FACTORY_BINDING_BASE(FormatHash)   
            ;

    bp::register_ptr_to_python< boost::shared_ptr<FormatHash> >();
}


