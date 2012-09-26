/*
 * $Id: FLArrayFilterArrayView.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FLArrayFilterArrayView.hh"



namespace karabo {
  namespace io {
    namespace hdf5 {

      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<signed char>, Int8ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<short>, Int16ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<int>, Int32ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<long long>, Int64ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned char>, UInt8ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned short>, UInt16ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned int>, UInt32ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned long long>, UInt64ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<float>, FloatArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<double>, DoubleArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<std::string>, StringArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<bool>, BoolArrayViewFLArrayFilter)


      KARABO_REGISTER_FACTORY_CC(DataTypes, Int8ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int16ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int32ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int64ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt8ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt16ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt32ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt64ArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, FloatArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, DoubleArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, StringArrayViewFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, BoolArrayViewFLArrayFilter)


    }
  }
}
