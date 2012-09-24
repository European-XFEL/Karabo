/*
 * $Id: DataTypesScalar.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "DataTypesScalar.hh"


using namespace std;
using namespace exfel::util;

namespace exfel {
  namespace io {
    namespace hdf5 {



      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int8ScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int16ScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int32ScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, Int64ScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt8ScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt16ScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt32ScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, UInt64ScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, FloatScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, DoubleScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, StringScalarDataTypes)
      EXFEL_REGISTER_FACTORY_CC(DataTypes, BoolScalarDataTypes)










    }
  }
}
