/* 
 * File:   WriterPy.hh
 * Author: irinak
 *
 * Created on April 28, 2011, 11:41 AM
 */

#ifndef EXFEL_PYEXFEL_WRITERPY_HH
#define	EXFEL_PYEXFEL_WRITERPY_HH

#include "Python.h"
#include <boost/python.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <set>
#include <string>

#include <exfel/io/Writer.hh>
#include <exfel/core/Module.hh>
#include <exfel/util/Hash.hh>

#include "PythonLoader.hh"
#include "Wrapper.hh"

namespace exfel {

  namespace pyexfel {

    template <class T>
    class WriterPy : public exfel::io::Writer<T> {
    public:

      EXFEL_CLASSINFO(WriterPy<T>, "WriterPy", "1.0")

      WriterPy() {
        TRACE("Constructor() WriterPy")
      };

      virtual ~WriterPy() {
        TRACE("Destructor() WriterPy")
      };

      virtual void configure(const exfel::util::Hash & conf) {
        TRACE("WriterPy::configure(conf)");

        std::string modname = getModuleName(conf);
        TRACE("python module name = " + modname);
        m_pymod = create(modname);
        std::string configPath = "python." + modname;
        m_pymod->configure(conf.getFromPath<exfel::util::Hash > (configPath));
      }

      static void expectedParameters(exfel::util::Schema & expected) {
        TRACE("WriterPy::expectedParameters(Schema&)");
        PythonLoader::expectedParameters(expected, "WriterSchema");
      }

      static boost::shared_ptr<WriterPy<T> > create(const std::string& className) {
        return PythonLoader::createInstance<WriterPy<T> >(className);
      }

      //virtual void write(const exfel::util::Hash& conf) {
      virtual void write(const T& conf) {
        TRACE("WriterPy::write()");
        try {
          m_pymod->write(conf);
        } catch (boost::python::error_already_set const &) {
          TRACE("WriterPy::write() catch(error_already_set)")
          std::string error = PythonLoader::retrievePythonError();
          TRACE("WriterPy::write() error extracted)")
                  throw PYTHON_EXCEPTION("Failure in Python code. " + error);
        } catch (...) {
          TRACE("WriterPy::write() catch(...)")
          RETHROW;
        }
      }
      
    private:

      boost::shared_ptr<WriterPy<T> > m_pymod;

      std::string getModuleName(const exfel::util::Hash& conf) const {
        exfel::util::Hash pymoduleConf = conf.get<exfel::util::Hash > ("python");
        std::set<std::string> keys = pymoduleConf.getKeysAsSet();
        std::set<std::string>::const_iterator it = keys.begin();
        return *it;
      }

    };

    template <class T>
    class WriterPyWrapper : public WriterPy<T>, Wrapper<exfel::io::Writer<T> > {
    public:

      WriterPyWrapper(PyObject * self_) : Wrapper<exfel::io::Writer<T> >(self_) {
        TRACE("Constr(self) WriterPyWrapper")
        Py_INCREF(this->self);
      }

      WriterPyWrapper(PyObject*& self_, const WriterPy<T> & a) : Wrapper<exfel::io::Writer<T> >(self_, a) {
        TRACE("Constr(self,copy) WriterPyWrapper")
        Py_INCREF(this->self);
      }

      ~WriterPyWrapper() {
        TRACE("Destructor() WriterPyWrapper");
        Py_DECREF(this->self);
      }

      void configure(const exfel::util::Hash& conf) {
        TRACE("WriterPyWrapper::configure(conf)")
        try {
          boost::python::call_method<void>(this->self, "configure", conf);
        } catch (boost::python::error_already_set const &) {
          std::string error = PythonLoader::retrievePythonError();
          throw PYTHON_EXCEPTION("Failure in Python code. " + error);
        } catch (...) {
          RETHROW;
        }
      }

      void write(const T& conf) {
        TRACE("WriterPyWrapper::write")
        try {
          boost::python::call_method<void>(this->self, "write");
        } catch (boost::python::error_already_set const &) {
          std::string error = PythonLoader::retrievePythonError();
          throw PYTHON_EXCEPTION("Failure in Python code. " + error);
        } catch (...) {
          throw PYTHON_EXCEPTION("Unknown exception in Python code.");
        }
      }

      void default_write(const T& conf) {
        TRACE("WriterPyWrapper::default_write")
                throw PYTHON_EXCEPTION("WriterPy.write() method not implemented");
      }

    };

  }// namespace pyexfel
}// namespace exfel

#endif

