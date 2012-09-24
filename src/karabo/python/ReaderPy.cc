/* 
 * File:   ReaderPy.cc
 * Author: irinak
 * 
 * Created on April 26, 2011, 4:06 PM
 */

#include "ReaderPy.hh"

namespace exfel {
  namespace pyexfel {

    using namespace exfel::io;
    using namespace exfel::util;
    EXFEL_REGISTER_FACTORY_CC(Reader<Hash>, ReaderPy<Hash>)
  
  } // namespace pyexfel
} // namespace exfel
