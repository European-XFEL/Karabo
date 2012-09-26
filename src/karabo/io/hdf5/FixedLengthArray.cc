/*
 * $Id: FixedLengthArray.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FixedLengthArray.hh"
using namespace karabo::io;

namespace karabo {
  namespace io {
    namespace hdf5 {

    KARABO_REGISTER_FACTORY_CC(RecordElement, Int8ArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, Int16ArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, Int32ArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, Int64ArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, UInt8ArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, UInt16ArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, UInt32ArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, UInt64ArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, BoolArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, StringArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, FloatArrayElement)
    KARABO_REGISTER_FACTORY_CC(RecordElement, DoubleArrayElement)

    }
  }
}
