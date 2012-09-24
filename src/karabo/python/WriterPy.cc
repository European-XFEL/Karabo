/* 
 * File:   WriterPy.cc
 * Author: irinak
 * 
 * Created on April 28, 2011, 11:42 AM
 */

#include "WriterPy.hh"

namespace exfel {
  namespace pyexfel {

    using namespace exfel::io;
    using namespace exfel::util; 
    EXFEL_REGISTER_FACTORY_CC(Writer<Hash>, WriterPy<Hash>)
    EXFEL_REGISTER_FACTORY_CC(Writer<Schema>, WriterPy<Schema>)

  } // namespace pyexfel
} // namespace exfel

