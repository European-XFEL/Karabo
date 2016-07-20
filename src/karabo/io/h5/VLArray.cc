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


            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int8VLArrayElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int16VLArrayElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int32VLArrayElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int64VLArrayElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt8VLArrayElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt16VLArrayElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt32VLArrayElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt64VLArrayElement)
            //    KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, BoolArrayElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, FloatVLArrayElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DoubleVLArrayElement)

        }
    }
}
