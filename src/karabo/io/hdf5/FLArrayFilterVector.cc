/*
 * $Id: FLArrayFilterVector.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FLArrayFilterVector.hh"


using namespace std;
using namespace karabo::util;

namespace karabo {
  namespace io {
    namespace hdf5 {


      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<signed char>, Int8VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<short>, Int16VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<int>, Int32VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<long long>, Int64VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned char>, UInt8VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned short>, UInt16VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned int>, UInt32VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<unsigned long long>, UInt64VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<float>, FloatVectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<double>, DoubleVectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(FLArrayFilter<std::string>, StringVectorFLArrayFilter)
      // std::vector<bool> is not supported


      KARABO_REGISTER_FACTORY_CC(DataTypes, Int8VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int16VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int32VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int64VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt8VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt16VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt32VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt64VectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, FloatVectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, DoubleVectorFLArrayFilter)
      KARABO_REGISTER_FACTORY_CC(DataTypes, StringVectorFLArrayFilter)

    }
  }
}
