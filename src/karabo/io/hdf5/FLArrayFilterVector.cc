/*
 * $Id: FLArrayFilterVector.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FLArrayFilterVector.hh"


using namespace std;
using namespace exfel::util;

namespace exfel {
  namespace io {
    namespace hdf5 {


      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<signed char>, Int8VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<short>, Int16VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<int>, Int32VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<long long>, Int64VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned char>, UInt8VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned short>, UInt16VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned int>, UInt32VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<unsigned long long>, UInt64VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<float>, FloatVectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<double>, DoubleVectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(FLArrayFilter<std::string>, StringVectorFLArrayFilter)
      // std::vector<bool> is not supported


      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int8VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int16VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int32VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int64VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt8VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt16VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt32VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt64VectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, FloatVectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, DoubleVectorFLArrayFilter)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, StringVectorFLArrayFilter)

    }
  }
}
