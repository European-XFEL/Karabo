/*
 * $Id$
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "NDArrayH5.hh"


namespace karabo {
    namespace io {
        namespace h5 {


            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, CharNDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int8NDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int16NDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int32NDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int64NDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt8NDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt16NDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt32NDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt64NDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, BoolNDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, StringNDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, FloatNDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DoubleNDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, ComplexFloatNDArrayH5Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, ComplexDoubleNDArrayH5Element)

        }
    }
}
