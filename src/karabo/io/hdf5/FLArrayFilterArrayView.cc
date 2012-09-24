/*
 * $Id: FLArrayFilterArrayView.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FLArrayFilterArrayView.hh"



namespace exfel {
  namespace io {
    namespace hdf5 {

      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<signed char>, Int8ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<short>, Int16ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<int>, Int32ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<long long>, Int64ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned char>, UInt8ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned short>, UInt16ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned int>, UInt32ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned long long>, UInt64ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<float>, FloatArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<double>, DoubleArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<std::string>, StringArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<bool>, BoolArrayViewFLArrayFilter)


      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int8ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int16ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int32ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int64ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt8ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt16ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt32ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt64ArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, FloatArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, DoubleArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, StringArrayViewFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, BoolArrayViewFLArrayFilter)


    }
  }
}
