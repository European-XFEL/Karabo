/*
 * $Id: main.cc 3269 2011-04-03 16:04:37Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on March,2011,05:54AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <Python.h>
#include <exfel/core/Application.hh>
#include <boost/python.hpp>
#include <boost/python/call.hpp>

int main(int argc, char** argv) {
  Py_Initialize();
  return  exfel::core::Application::runModules(argc, argv);
}

