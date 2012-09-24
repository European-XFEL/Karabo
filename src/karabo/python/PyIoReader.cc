/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <exfel/util/Hash.hh>
#include <exfel/io/Reader.hh>
#include "ReaderPy.hh"
#include "PythonLoader.hh"
#include "Wrapper.hh"

using namespace exfel::pyexfel;
using namespace exfel::io;
using namespace exfel::util;
using namespace std;
namespace bp = boost::python;

void exportPyIoReader() {

  //exposing exfel::io::Reader<exfel::util::Hash>
  typedef exfel::io::Reader<exfel::util::Hash> ReaderHash;
  typedef ReaderPy<exfel::util::Hash> ReaderHashPy;
  typedef ReaderPyWrapper<exfel::util::Hash> ReaderHashPyWrapper;

  EXFEL_PYTHON_FACTORY_TYPEDEFS(ReaderHash);
  typedef void ( ReaderHashPy::*configure_function_type)(Hash const &);
  
  bp::class_< ReaderHash, Wrapper<ReaderHash>, boost::noncopyable > ("ReaderHash", bp::no_init)
          .def("read", &ReaderHash::read)
          EXFEL_PYTHON_FACTORY_BINDING_BASE(ReaderHash)
          ;

  bp::class_<ReaderHashPy, bp::bases<ReaderHash>, ReaderHashPyWrapper > ("ReaderHashPy")
          .def("read", &ReaderHashPy::read, &ReaderHashPyWrapper::default_read)
          EXFEL_PYTHON_FACTORY_DERIVED_BINDING(ReaderHash)
          ;

  bp::register_ptr_to_python< boost::shared_ptr<ReaderHash> >();
}
