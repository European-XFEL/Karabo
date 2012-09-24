/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <exfel/util/Hash.hh>
#include <exfel/core/Module.hh>
#include "ModulePy.hh"
#include "PythonLoader.hh"

using namespace exfel::pyexfel;
using namespace exfel::core;
using namespace exfel::util;
using namespace std;
namespace bp = boost::python;

void exportPyCoreModule() { // exposing exfel::core::Module

  EXFEL_PYTHON_FACTORY_TYPEDEFS(Module);

  bp::class_<Module, ModuleWrapper, boost::noncopyable > ("Module", bp::no_init)
          .def("compute", &Module::compute)
          .def("getName", &Module::getName)
          EXFEL_PYTHON_FACTORY_BINDING_BASE(Module)
          ;

  bp::class_<ModulePy, bp::bases<Module>, ModulePyWrapper > ("ModulePy")
          .def("compute", &ModulePy::compute, &ModulePyWrapper::default_compute)
          EXFEL_PYTHON_FACTORY_DERIVED_BINDING(Module)
          ;

  bp::register_ptr_to_python< boost::shared_ptr<Module> >();
}

