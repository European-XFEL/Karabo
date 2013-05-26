/*
 * $Id: VLArray.cc 9537 2013-04-26 07:36:59Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
 
 
#include "VLArray.hh"
using namespace karabo::io;
      
namespace karabo {  
  namespace io {
    namespace h5 {
             
   
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int8ArrayElement)
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int16ArrayElement)
    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int32VLArrayElement) 
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int64ArrayElement)
    //KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt8VLArrayElement)
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt16ArrayElement)
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt32ArrayElement)
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt64ArrayElement)
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, BoolArrayElement)
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, StringArrayElement)
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, FloatArrayElement)
//    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DoubleArrayElement)
                   
    }
  }
}
