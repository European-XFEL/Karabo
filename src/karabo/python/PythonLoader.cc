/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 * contributions by <Irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <string>
#include <iostream>
#include <exfel/util/Factory.hh>
//#include <boost/python.hpp>
#include <boost/filesystem.hpp>
#include <log4cpp/Category.hh>

#include "PythonLoader.hh"

using namespace std;
using namespace log4cpp;
using namespace boost::filesystem;
using namespace boost::python;
using namespace exfel::util;

namespace exfel {
  namespace pyexfel {

    void PythonLoader::expectedParameters(Schema & expected, const string & interface) {

      Category & log = Category::getInstance("exfel.pyexfel.PythonLoader");

      TRACE("entering PythonLoader::expectedParameters")

      string appendStr = "sys.path.append(\"" + interface + "\")";
      string removeStr = "sys.path.remove(\"" + interface + "\")";
           
      //Py_Initialize();
      PyRun_SimpleString("import sys");
      PyRun_SimpleString(appendStr.c_str());
#ifdef EXFEL_TRACE_FLAG
      PyRun_SimpleString("print sys.path");
#endif
      Schema listOfPythonModules;

      path interfacePath(interface);     
      try {
        if (exists(interfacePath)) {
          if (is_directory(interfacePath)) {
            typedef vector<path> vec;
            vec v;

            copy(directory_iterator(interfacePath), directory_iterator(), back_inserter(v));
            sort(v.begin(), v.end());

            for (vec::const_iterator it(v.begin()); it != v.end(); ++it) {
              string baseFileName = boost::filesystem::basename(*it);
              string fileExt = boost::filesystem::extension(*it);

              if (fileExt == ".py") {

                TRACE("processing filename " + baseFileName + fileExt)

                PyObject* pName = PyString_FromString(baseFileName.c_str());
                PyObject* pModule = PyImport_Import(pName);
                if (PyErr_Occurred() || pModule == NULL) {
                  //Occurrence case 'PyErr_Occurred' : class Sum(ModulePy7)
                  log << Priority::ERROR << "Failed to load Python module: " << *it;
                  throw PYTHON_EXCEPTION("Failed to load Python module: " + (*it).string());
                }

                Py_DECREF(pName);

                PyObject* pDict = PyModule_GetDict(pModule);
                PyObject* pClass = PyDict_GetItemString(pDict, baseFileName.c_str());
                if (PyErr_Occurred() || pClass == NULL) {
                  //Occurrence cases 'pClass == NULL' : 1) class Sum7(ModulePy), 2) class Sum(ModulePy) in Sum7.py module
                  log << Priority::ERROR << "Failure by loading Python module: " << *it;
                  throw PYTHON_EXCEPTION("Failure by loading Python module: " + (*it).string() +
                          "   (Check that class name and name of the module corresponds.)");
                }

                log << Priority::DEBUG << "class " << baseFileName << " loaded";

                
                exfel::util::Schema pyModuleExpected(expected.getAccessMode(),expected.getCurrentState());
                try {
                  boost::python::call_method<void>(pClass, "expectedParameters", boost::ref(pyModuleExpected));
                } catch (error_already_set) {
                  string errorString = retrievePythonError();
                  log << Priority::DEBUG << "Failure in Python code("+baseFileName+"expectedParameters). " << errorString;
                  throw PYTHON_EXCEPTION("Failure in Python code("+baseFileName+".expectedParameters). " + errorString);
                }
                string strElements = baseFileName + ".elements";
                listOfPythonModules.setFromPath(strElements.c_str(), pyModuleExpected);

                string strRoot = baseFileName + ".root";
                listOfPythonModules.setFromPath(strRoot.c_str(), baseFileName.c_str());

              }
            }
          } else {
            log << Priority::WARN << interfacePath << " exists, but is not a directory";
            return;
          }
        } else {
          log << Priority::DEBUG << interfacePath << " does not exist";
          return;
        }


        CHOICE_ELEMENT<Schema>(expected,listOfPythonModules)
                .key("python")
                .displayedName("python module")
                .description("Special parameters for python modules")
                .assignmentMandatory()
                .commit();

        //cout << "Expected:\n" << expected << "\nEnd of Expected---\n";

      } catch (error_already_set) {

        string errorString = retrievePythonError();
        log << Priority::DEBUG << "Failure in Python code. " << errorString;
        throw PYTHON_EXCEPTION("Failure in Python code. " + errorString);
      } catch (...) {
        RETHROW;
      }

      PyRun_SimpleString(removeStr.c_str());
      //PyRun_SimpleString("print sys.path");
      //Py_XDECREF(pClass);
      //Py_XDECREF(pModule);
    }

    std::string PythonLoader::retrievePythonError() {

      PyObject *e, *v, *t, *messagePyString, *namePyString;

      // get the error indicators
      PyErr_Fetch(&e, &v, &t);
      // exception name
      namePyString = PyObject_GetAttrString(e, "__name__");
      char* name = PyString_AsString(namePyString);

      // exception message
      messagePyString = PyObject_Str(v);
      char* message = PyString_AsString(messagePyString);

      // build the error message
      string errorString = string(name) + ": " + string(message);

      Py_XDECREF(e);
      Py_XDECREF(v);
      Py_XDECREF(t);
      Py_XDECREF(messagePyString);
      Py_XDECREF(namePyString);

      return errorString;
    }

    void PythonLoader::trace(const string& message) {
      cerr << "TRACE  " << message << endl;
    }

  }
}
