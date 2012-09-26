/*
 * $Id: FLArrayFilterArrayViewBuffer.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FLArrayFilterArrayViewBuffer.hh"



namespace karabo {
  namespace io {
    namespace hdf5 {

      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<signed char>, Int8ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<short>, Int16ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<int>, Int32ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<long long>, Int64ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<unsigned char>, UInt8ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<unsigned short>, UInt16ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<unsigned int>, UInt32ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<unsigned long long>, UInt64ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<float>, FloatArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<double>, DoubleArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<std::string>, StringArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilterBuffer<bool>, BoolArrayViewFLArrayBufferFilter)


      KARABO_REGISTER_FACTORY_CC(DataTypes, Int8ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int16ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int32ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int64ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt8ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt16ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt32ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt64ArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, FloatArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, DoubleArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, StringArrayViewFLArrayBufferFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, BoolArrayViewFLArrayBufferFilter)


    }
  }
}
