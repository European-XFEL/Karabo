/*
  * File:   ReconfigurableFsmWrap.hh
  * Author: irinak
  *
  * Created on April 12, 2012, 3:14 PM
  */

#ifndef EXFEL_PYEXFEL_RECONFIGURABLEFSMWRAP_HH
#define EXFEL_PYEXFEL_RECONFIGURABLEFSMWRAP_HH

#include <boost/python.hpp>
#include <exfel/core/ReconfigurableFsm.hh>
#include <exfel/core/Device.hh>
#include <exfel/util/Schema.hh>
#include <exfel/xms/SignalSlotable.hh>
#include "PythonLoader.hh"

namespace bp = boost::python;

namespace exfel {
     namespace pyexfel {

         class ReconfigurableFsmWrap : public exfel::core::ReconfigurableFsm {
             
         public:
            template <class Derived>
              ReconfigurableFsmWrap(Derived* derived): Device(derived) {
             }

            virtual ~ReconfigurableFsmWrap() {
             }
            
            static void registerReconfigurableFsmDeviceClass(const std::string& fileName){
                
                exfel::util::Schema listOfPythonDevices; //now not a list, just ONE single device
                
                PyRun_SimpleString("import sys");
                
                //fileName - name of file containing Device class in Python; fileName.py
                PyObject* pName = PyString_FromString(fileName.c_str());
                PyObject* pModule = PyImport_Import(pName);
                if (PyErr_Occurred() || pModule == NULL) {
                  throw PYTHON_EXCEPTION("Failed to load Python module: " + fileName );
                }

                Py_DECREF(pName);

                PyObject* pDict = PyModule_GetDict(pModule);
                PyObject* pClass = PyDict_GetItemString(pDict, fileName.c_str());
                if (PyErr_Occurred() || pClass == NULL) {
                  throw PYTHON_EXCEPTION("Failure by loading Python module: " + fileName +
                          "   (Check that class name and name of the module corresponds.)");
                }

                std::cout << "@@@ class " << fileName << " loaded" << std::endl;
                
                exfel::util::Schema expectedTEST;
                exfel::core::ReconfigurableFsm::expectedParameters(expectedTEST);
                std::cout << "@@@ Schema expectedTEST :\n " << expectedTEST << std::endl;
                
                exfel::util::Schema pyDeviceExpected;
                try {
                 bp::call_method<void>(pClass, "expectedParameters", boost::ref(pyDeviceExpected));
                } catch (bp::error_already_set) {
                  std::string errorString = PythonLoader::retrievePythonError();
                  throw PYTHON_EXCEPTION("Failure in Python code(" + fileName + ".expectedParameters). " + errorString);
                }

               std::cout << "@@@ pyDeviceExpected :\n " << pyDeviceExpected << std::endl;
                
                std::string strElements = fileName + ".*elements";
                listOfPythonDevices.setFromPath(strElements.c_str(), pyDeviceExpected);

                std::string strRoot = fileName + ".*root";
                listOfPythonDevices.setFromPath(strRoot.c_str(), fileName.c_str());
               
                std::cout << "@@@ listOfPythonDevices :\n " << listOfPythonDevices << std::endl;
                
            }
            
            
         private: 
            
     };
   }
}

#endif    /* EXFEL_PYEXFEL_RECONFIGURABLEFSMWRAP_HH */

