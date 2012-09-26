/*
 * $Id: FLArrayFilterRawPointer.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FLArrayFilterRawPointer.hh"



namespace karabo {
  namespace io {
    namespace hdf5 {

      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<signed char>, Int8RawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<short>, Int16RawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<int>, Int32RawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<long long>, Int64RawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned char>, UInt8RawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned short>, UInt16RawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned int>, UInt32RawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned long long>, UInt64RawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<float>, FloatRawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<double>, DoubleRawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<std::string>, StringRawPointerFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<bool>, BoolRawPointerFLArrayFilter)

    }
  }
}
