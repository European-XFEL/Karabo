/* 
 * File:   ReaderPy.hh
 * Author: irinak
 *
 * Created on April 26, 2011, 4:06 PM
 */

#ifndef EXFEL_PYEXFEL_READERPY_HH
#define	EXFEL_PYEXFEL_READERPY_HH

#include "Python.h"
#include <string>
#include <set>
#include <vector>
#include <iterator>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/python.hpp>
#include <boost/python/call_method.hpp>
#include <exfel/io/Reader.hh>
#include <exfel/util/Hash.hh>
#include "PythonLoader.hh"
#include "Wrapper.hh"

namespace exfel {

  namespace pyexfel {

    template <class T>
    class ReaderPy : public exfel::io::Reader<T> {
    public:

      EXFEL_CLASSINFO(ReaderPy<T>, "ReaderPy", "1.0")

      ReaderPy() {
      }

      virtual ~ReaderPy() {
      }

      virtual void configure(const exfel::util::Hash & conf) {
        TRACE("ReaderPy::configure(conf)");

        std::string modname = getModuleName(conf);
        TRACE("python module name = " + modname);
        m_pymod = create(modname);
        std::string configPath = "python." + modname;
        m_pymod->configure(conf.getFromPath<exfel::util::Hash > (configPath));
      }

      static void expectedParameters(exfel::util::Schema & expected) {
        TRACE("ReaderPy::expectedParameters(Schema&)");
        PythonLoader::expectedParameters(expected, "ReaderSchema");
      }

      static boost::shared_ptr<ReaderPy<T> > create(const std::string& className) {
        return PythonLoader::createInstance<ReaderPy<T> >(className);
      }

      void read(T& conf) {
        TRACE("ReadePy::read()");
        try {
          m_pymod->read(conf);
        } catch (boost::python::error_already_set const &) {
          TRACE("ReaderPy::read() catch(error_already_set)")
          std::string error = PythonLoader::retrievePythonError();
          TRACE("ReaderPy::read() error extracted)")
                  throw PYTHON_EXCEPTION("Failure in Python code. " + error);
        } catch (...) {
          TRACE("ReaderPy::read() catch(...)")
          RETHROW;
        }
      }

    private:

      boost::shared_ptr<ReaderPy<T> > m_pymod;

      std::string getModuleName(const exfel::util::Hash& conf) const {
        exfel::util::Hash pymoduleConf = conf.get<exfel::util::Hash > ("python");
        std::set<std::string> keys = pymoduleConf.getKeysAsSet();
        std::set<std::string>::const_iterator it = keys.begin();
        return *it;
      }
    };

    template <class T>
    class ReaderPyWrapper : public ReaderPy<T>, Wrapper<exfel::io::Reader<T> > {
    public:

      ReaderPyWrapper(PyObject * self_) : Wrapper<exfel::io::Reader<T> >(self_) {
        TRACE("Constr(self) ReaderPyWrapper")
        Py_INCREF(this->self);
      }

      ReaderPyWrapper(PyObject*& self_, const ReaderPy<T> & a) : Wrapper<exfel::io::Reader<T> >(self_, a) {
        TRACE("Constr(self,copy) ReaderPyWrapper")
        Py_INCREF(this->self);
      }

      ~ReaderPyWrapper() {
        TRACE("Destr ReaderPyWrapper")
        Py_DECREF(this->self);
      }

      void configure(const exfel::util::Hash & conf) {
        TRACE("ReaderPyWrapper::configure(conf)")
        try {
          boost::python::call_method<void>(this->self, "configure", conf);
        } catch (boost::python::error_already_set const &) {
          std::string error = PythonLoader::retrievePythonError();
          throw PYTHON_EXCEPTION("Failure in Python code. " + error);
        } catch (...) {
          RETHROW;
        }
      }

      void read(T& conf) {
        TRACE("ReaderPyWrapper::read")
        try {
          boost::python::call_method<void>(this->self, "read");
        } catch (boost::python::error_already_set const &) {
          std::string error = PythonLoader::retrievePythonError();
          throw PYTHON_EXCEPTION("Failure in Python code. " + error);
        } catch (...) {
          throw PYTHON_EXCEPTION("Unknown exception in Python code.");
        }
      }

      void default_read(T& conf) {
        TRACE("ReaderPyWrapper::default_read")
                throw PYTHON_EXCEPTION("ReaderSchema.read() method not implemented");
      }
    };

  }
}

#endif

