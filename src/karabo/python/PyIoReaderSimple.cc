/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"

#include <exfel/util/Hash.hh>
#include <exfel/io/Reader.hh>

#include "PythonLoader.hh"

using namespace exfel::util;
using namespace std;
namespace bp = boost::python;

void exportPyIoReaderSimple() {

    typedef exfel::io::Reader<exfel::util::Hash> ReaderHash;

    EXFEL_PYTHON_FACTORY_TYPEDEFS(ReaderHash);

    bp::class_< ReaderHash, boost::noncopyable > ("ReaderHash", bp::no_init)
            .def("read", &ReaderHash::read)
            EXFEL_PYTHON_FACTORY_BINDING_BASE(ReaderHash)   
            ;

    bp::register_ptr_to_python< boost::shared_ptr<ReaderHash> >();
}

