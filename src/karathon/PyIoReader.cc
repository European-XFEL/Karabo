/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/util/Hash.hh>
#include <karabo/io/Reader.hh>
#include "PythonFactoryMacros.hh"

using namespace karabo::pyexfel;
using namespace karabo::io;
using namespace karabo::util;
using namespace std;
namespace bp = boost::python;


void exportPyIoReader() {

    //exposing karabo::io::Reader<karabo::util::Hash>
    typedef karabo::io::Reader<karabo::util::Hash> ReaderHash;

    KARABO_PYTHON_FACTORY_TYPEDEFS(ReaderHash);

    bp::class_< ReaderHash, boost::noncopyable > ("ReaderHash", bp::no_init)
            .def("read", &ReaderHash::read)
            KARABO_PYTHON_FACTORY_BINDING_BASE(ReaderHash)
            ;

    bp::register_ptr_to_python< boost::shared_ptr<ReaderHash> >();
}
