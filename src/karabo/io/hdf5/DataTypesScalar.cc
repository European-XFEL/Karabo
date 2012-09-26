/*
 * $Id: DataTypesScalar.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "DataTypesScalar.hh"


using namespace std;
using namespace karabo::util;

namespace karabo {
  namespace io {
    namespace hdf5 {



      KARABO_REGISTER_FACTORY_CC(DataTypes, Int8ScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int16ScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int32ScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, Int64ScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt8ScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt16ScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt32ScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, UInt64ScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, FloatScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, DoubleScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, StringScalarDataTypes)
      KARABO_REGISTER_FACTORY_CC(DataTypes, BoolScalarDataTypes)










    }
  }
}
