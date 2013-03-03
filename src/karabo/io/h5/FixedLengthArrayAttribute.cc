/*
 * $Id: FixedLengthArray.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FixedLengthArrayAttribute.hh"
using namespace karabo::io;
 
namespace karabo {  
  namespace io {
    namespace h5 {
       
          
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, Int8ArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, Int16ArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, Int32ArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, Int64ArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, UInt8ArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, UInt16ArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, UInt32ArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, UInt64ArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, BoolArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, StringArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, FloatArrayAttribute)
    KARABO_REGISTER_FOR_CONFIGURATION(Attribute, DoubleArrayAttribute)

    }
  }
}
