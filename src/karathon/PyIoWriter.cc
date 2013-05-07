/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/util/Hash.hh>
#include <karabo/io/Writer.hh>
#include "PythonFactoryMacros.hh"

using namespace karabo::pyexfel;
using namespace karabo::util;
using namespace karabo::io;
using namespace std;
namespace bp = boost::python;

void exportPyIoWriter() {
  {//exposing karabo::io::Writer<karabo::util::Hash>
  typedef karabo::io::Writer<karabo::util::Hash> WriterHash;

  KARABO_PYTHON_FACTORY_TYPEDEFS(WriterHash);
  
  bp::class_<WriterHash, boost::noncopyable > ("WriterHash", bp::no_init)
          .def("write", &WriterHash::write)
          KARABO_PYTHON_FACTORY_BINDING_BASE(WriterHash)
          ;

  bp::register_ptr_to_python< boost::shared_ptr<WriterHash> >();
}
  /////////////////////////////////////////////////
  {//exposing karabo::io::Writer<karabo::util::Schema>
  typedef karabo::io::Writer<karabo::util::Schema> WriterSchema;
  
  KARABO_PYTHON_FACTORY_TYPEDEFS(WriterSchema);
  
  bp::class_<WriterSchema, boost::noncopyable > ("WriterSchema", bp::no_init)
          .def("write", &WriterSchema::write)
          KARABO_PYTHON_FACTORY_BINDING_BASE(WriterSchema)
          ;

  bp::register_ptr_to_python< boost::shared_ptr<WriterSchema> >();
  }
}
