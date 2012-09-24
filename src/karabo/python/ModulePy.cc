/* 
 * File:   ModulePy.cc
 * Author: irinak
 * 
 * Created on February 15, 2011, 5:03 PM
 */

#include <string>
#include <vector>
#include <set>
#include <iterator>
#include <boost/filesystem.hpp>

#include "ModulePy.hh"
#include "PythonLoader.hh"

using namespace boost::filesystem;
using namespace boost::python;
using namespace boost;
using namespace exfel::util;
using namespace exfel::core;
using namespace std;

namespace exfel {
  namespace pyexfel {

    EXFEL_REGISTER_FACTORY_CC(Module, ModulePy)

    ModulePy::ModulePy() {
      TRACE("Constr() ModulePy")
    }

    ModulePy::~ModulePy() {
      TRACE("Destructor() ModulePy")
    }

    void ModulePy::compute() {
      TRACE("ModulePy::compute()")
      try {
        m_pymod->compute();
      } catch (error_already_set) {
        TRACE("ModulePy::compute() catch(error_already_set)")
        string error = PythonLoader::retrievePythonError();
        TRACE("ModulePy::compute() error extracted)")
                throw PYTHON_EXCEPTION("Failure in Python code. " + error);
      } catch (...) {
        TRACE("ModulePy::compute() catch(...)")
        RETHROW;
      }
    }

    void ModulePy::configure(const Hash & conf) {

      TRACE("ModulePy::configure(conf)")
      string modname = getModuleName(conf);
      TRACE("python module name = " + modname)

      m_pymod = create(modname);
      string configPath = "python." + modname;
      m_pymod->configure(conf.getFromPath<Hash > (configPath));
    }

    boost::shared_ptr<ModulePy> ModulePy::create(const string & className) {
      return PythonLoader::createInstance<ModulePy>(className);
    }

    void ModulePy::expectedParameters(Schema & expected) {

      TRACE("ModulePy::expectedParameters(Schema&)")
      PythonLoader::expectedParameters(expected, "Module");
    }

    string ModulePy::getModuleName(const Hash& conf) const {

      Hash pymoduleConf = conf.get<Hash > ("python");
      set<string> keys = pymoduleConf.getKeysAsSet();
      set<string>::const_iterator it = keys.begin();
      return *it;
    }

    // ModulePyWrapper

    ModulePyWrapper::ModulePyWrapper(PyObject * self_) : ModuleWrapper(self_) {
      TRACE("Constr(self) ModulePyWrapper")
      Py_INCREF(self);
    }

    ModulePyWrapper::ModulePyWrapper(PyObject*& self_, const ModulePy & a) : ModuleWrapper(self_, a) {
      TRACE("Constr(self,copy) ModulePyWrapper")
      Py_INCREF(self);
    }

    ModulePyWrapper::~ModulePyWrapper() {
      TRACE("Destr ModulePyWrapper")
      Py_DECREF(self);
    }

    void ModulePyWrapper::configure(const Hash & conf) {
      TRACE("ModulePyWrapper::configure(conf)")
      try {
        boost::python::call_method<void>(self, "configure", conf);
      } catch (error_already_set) {
        string error = PythonLoader::retrievePythonError();
        throw PYTHON_EXCEPTION("Failure in Python code. " + error);
      } catch (...) {
        RETHROW;
      }
    }

    void ModulePyWrapper::compute() {
      TRACE("ModulePyWrapper::compute")
      try {
        boost::python::call_method<void>(self, "compute");
      } catch (error_already_set) {
        string error = PythonLoader::retrievePythonError();
        throw PYTHON_EXCEPTION("Failure in Python code. " + error);
      } catch (...) {
        throw PYTHON_EXCEPTION("Unknown exception in Python code.");
      }
    }

    void ModulePyWrapper::default_compute() {
      TRACE("ModulePyWrapper::default_compute")
              throw PYTHON_EXCEPTION("Module.compute() method not implemented");
    }

  }//namespace pyexfel
}//namespace exfel
