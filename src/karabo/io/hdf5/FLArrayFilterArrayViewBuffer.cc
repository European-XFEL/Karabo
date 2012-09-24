/*
 * $Id: FLArrayFilterArrayViewBuffer.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FLArrayFilterArrayViewBuffer.hh"



namespace exfel {
  namespace io {
    namespace hdf5 {

      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<signed char>, Int8ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<short>, Int16ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<int>, Int32ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<long long>, Int64ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<unsigned char>, UInt8ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<unsigned short>, UInt16ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<unsigned int>, UInt32ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<unsigned long long>, UInt64ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<float>, FloatArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<double>, DoubleArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<std::string>, StringArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilterBuffer<bool>, BoolArrayViewFLArrayBufferFilter)


      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int8ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int16ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int32ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int64ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt8ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt16ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt32ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt64ArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, FloatArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, DoubleArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, StringArrayViewFLArrayBufferFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, BoolArrayViewFLArrayBufferFilter)


    }
  }
}
