/* 
 * File:   ModulePy.hh
 * Author: irinak
 *
 * Created on February 15, 2011, 5:03 PM
 */

#ifndef EXFEL_PYEXFEL_MODULEPY_HH
#define	EXFEL_PYEXFEL_MODULEPY_HH

#include <boost/shared_ptr.hpp>

// This is some stuff special to python
// TODO Double-check whether this approach here is correct
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#include <Python.h>
#include <boost/python.hpp>

#include <iostream>
#include <string>
#include <exfel/util/Hash.hh>
#include <exfel/core/Module.hh>

#include "PythonLoader.hh"
#include "Wrapper.hh"

namespace exfel {

  namespace pyexfel {

    class ModulePy : public exfel::core::Module {
    public:

      EXFEL_CLASSINFO(ModulePy, "ModulePy", "1.0")

      ModulePy();
      virtual ~ModulePy();

      static void expectedParameters(exfel::util::Schema& expected);

      // configure must be declared as virtual function - otherwise infinitive loop. See implementation.
      virtual void configure(const exfel::util::Hash& conf);

      // pure virtual functions must be declared here and implemented in cc.
      virtual void compute();

      static boost::shared_ptr<ModulePy> create(const std::string& className);


    private:

      boost::shared_ptr<ModulePy> m_pymod;

      std::string getModuleName(const exfel::util::Hash& conf) const;

    protected:

    };
    
    typedef Wrapper<exfel::core::Module> ModuleWrapper;

    class ModulePyWrapper : public ModulePy, ModuleWrapper {
    public:

      ModulePyWrapper(PyObject * self_);

      ModulePyWrapper(PyObject*& self_, const ModulePy & a);

      ~ModulePyWrapper();

      void configure(const exfel::util::Hash & conf);

      void compute();
      
      void default_compute();
    };

  }
}

#endif

