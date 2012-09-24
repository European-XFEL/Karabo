/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <exfel/util/Hash.hh>
#include <exfel/io/Writer.hh>

#include "PythonLoader.hh"

using namespace exfel::util;
using namespace std;
namespace bp = boost::python;

void exportPyIoWriterSimple() {
    {//exposing exfel::io::Writer<exfel::util::Hash>
    typedef exfel::io::Writer<exfel::util::Hash> WriterHash;

    EXFEL_PYTHON_FACTORY_TYPEDEFS(WriterHash);

    bp::class_<WriterHash, boost::noncopyable > ("WriterHash", bp::no_init)
            .def("write", &WriterHash::write)
            EXFEL_PYTHON_FACTORY_BINDING_BASE(WriterHash)
            ;

    bp::register_ptr_to_python< boost::shared_ptr<WriterHash> >();
    }
    /////////////////////////////////////////////////
    {//exposing exfel::io::Writer<exfel::util::Schema>
    typedef exfel::io::Writer<exfel::util::Schema> WriterSchema;
    
    EXFEL_PYTHON_FACTORY_TYPEDEFS(WriterSchema);
    
    bp::class_<WriterSchema, boost::noncopyable > ("WriterSchema", bp::no_init)
            .def("write", &WriterSchema::write)
            EXFEL_PYTHON_FACTORY_BINDING_BASE(WriterSchema)
            ;

    bp::register_ptr_to_python< boost::shared_ptr<WriterSchema> >();
    }
}

