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
#include "WriterPy.hh"
#include "PythonLoader.hh"
#include "Wrapper.hh"

using namespace exfel::pyexfel;
using namespace exfel::util;
using namespace exfel::io;
using namespace std;
namespace bp = boost::python;

void exportPyIoWriter() {
  {//exposing exfel::io::Writer<exfel::util::Hash>
  typedef exfel::io::Writer<exfel::util::Hash> WriterHash;
  typedef WriterPy<exfel::util::Hash> WriterHashPy;
  typedef WriterPyWrapper<exfel::util::Hash> WriterHashPyWrapper;

  EXFEL_PYTHON_FACTORY_TYPEDEFS(WriterHash);
  typedef void ( WriterHashPy::*configure_function_type)(Hash const &);
  
  bp::class_<WriterHash, Wrapper<WriterHash>, boost::noncopyable > ("WriterHash", bp::no_init)
          .def("write", &WriterHash::write)
          EXFEL_PYTHON_FACTORY_BINDING_BASE(WriterHash)
          ;

  bp::class_<WriterHashPy, bp::bases<WriterHash>, WriterHashPyWrapper > ("WriterHashPy")
          .def("write", &WriterHashPy::write, &WriterHashPyWrapper::default_write)
          EXFEL_PYTHON_FACTORY_DERIVED_BINDING(WriterHash)
          ;

  bp::register_ptr_to_python< boost::shared_ptr<WriterHash> >();
}
  /////////////////////////////////////////////////
  {//exposing exfel::io::Writer<exfel::util::Schema>
  typedef exfel::io::Writer<exfel::util::Schema> WriterSchema;
  typedef WriterPy<exfel::util::Schema> WriterSchemaPy;
  typedef WriterPyWrapper<exfel::util::Schema> WriterSchemaPyWrapper;
  
  EXFEL_PYTHON_FACTORY_TYPEDEFS(WriterSchema);
  
  bp::class_<WriterSchema, Wrapper<WriterSchema>, boost::noncopyable > ("WriterSchema", bp::no_init)
          .def("write", &WriterSchema::write)
          EXFEL_PYTHON_FACTORY_BINDING_BASE(WriterSchema)
          ;

  bp::class_<WriterSchemaPy, bp::bases<WriterSchema>, WriterSchemaPyWrapper > ("WriterSchemaPy")
          .def("write", &WriterSchemaPy::write, &WriterSchemaPyWrapper::default_write)
          EXFEL_PYTHON_FACTORY_DERIVED_BINDING(WriterSchema)
          ;

  bp::register_ptr_to_python< boost::shared_ptr<WriterSchema> >();
  }
}
