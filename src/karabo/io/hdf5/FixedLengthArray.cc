/*
 * $Id: FixedLengthArray.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FixedLengthArray.hh"
using namespace exfel::io;

namespace exfel {
  namespace io {
    namespace hdf5 {

    EXFEL_REGISTER_FACTORY_CC(RecordElement, Int8ArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, Int16ArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, Int32ArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, Int64ArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, UInt8ArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, UInt16ArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, UInt32ArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, UInt64ArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, BoolArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, StringArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, FloatArrayElement)
    EXFEL_REGISTER_FACTORY_CC(RecordElement, DoubleArrayElement)

    }
  }
}
