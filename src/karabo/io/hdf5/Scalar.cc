/*
 * $Id: Scalar.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Scalar.hh"
using namespace karabo::io;

namespace karabo {
  namespace io {
    namespace hdf5 {

      KARABO_REGISTER_FACTORY_CC(RecordElement, Int8Element)
      KARABO_REGISTER_FACTORY_CC(RecordElement, Int16Element)
      KARABO_REGISTER_FACTORY_CC(RecordElement, Int32Element)
      KARABO_REGISTER_FACTORY_CC(RecordElement, Int64Element)
      KARABO_REGISTER_FACTORY_CC(RecordElement, UInt8Element)
      KARABO_REGISTER_FACTORY_CC(RecordElement, UInt16Element)
      KARABO_REGISTER_FACTORY_CC(RecordElement, UInt32Element)
      KARABO_REGISTER_FACTORY_CC(RecordElement, UInt64Element)
      KARABO_REGISTER_FACTORY_CC(RecordElement, BoolElement)
      KARABO_REGISTER_FACTORY_CC(RecordElement, StringElement)
      KARABO_REGISTER_FACTORY_CC(RecordElement, FloatElement)
      KARABO_REGISTER_FACTORY_CC(RecordElement, DoubleElement)
    }
  }
}
