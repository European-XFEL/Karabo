/* 
 * File:   Wrapper.hh
 * Author: irinak
 *
 * Created on April 18, 2011, 12:23 PM
 */

#ifndef EXFEL_PYEXFEL_WRAPPER_HH
#define	EXFEL_PYEXFEL_WRAPPER_HH

#include "Python.h"

namespace exfel {
  namespace pyexfel {

    template <class T>
    class Wrapper : public T {
    public:

      Wrapper(PyObject* self_) : self(self_) {
      }

      Wrapper(PyObject*& self_, const T& copy) : T(copy), self(self_) {
      }

      virtual ~Wrapper() {
      }
      PyObject* self;
    };
  }
}

#endif	/* EXFEL_PYEXFEL_WRAPPER_HH */

