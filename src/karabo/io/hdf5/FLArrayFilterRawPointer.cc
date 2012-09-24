/*
 * $Id: FLArrayFilterRawPointer.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FLArrayFilterRawPointer.hh"



namespace exfel {
  namespace io {
    namespace hdf5 {

      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<signed char>, Int8RawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<short>, Int16RawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<int>, Int32RawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<long long>, Int64RawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned char>, UInt8RawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned short>, UInt16RawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned int>, UInt32RawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned long long>, UInt64RawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<float>, FloatRawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<double>, DoubleRawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<std::string>, StringRawPointerFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<bool>, BoolRawPointerFLArrayFilter)

    }
  }
}
