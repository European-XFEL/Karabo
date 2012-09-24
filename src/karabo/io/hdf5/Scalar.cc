/*
 * $Id: Scalar.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Scalar.hh"
using namespace exfel::io;

namespace exfel {
  namespace io {
    namespace hdf5 {

      EXFEL_REGISTER_FACTORY_CC(RecordElement, Int8Element)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, Int16Element)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, Int32Element)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, Int64Element)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, UInt8Element)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, UInt16Element)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, UInt32Element)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, UInt64Element)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, BoolElement)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, StringElement)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, FloatElement)
      EXFEL_REGISTER_FACTORY_CC(RecordElement, DoubleElement)
    }
  }
}
