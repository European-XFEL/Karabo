/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Scalar.hh"
using namespace karabo::io::h5;

namespace karabo {
    namespace io {
        namespace h5 {

            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, CharElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int8Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int16Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int32Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, Int64Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt8Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt16Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt32Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, UInt64Element)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, BoolElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, StringElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, FloatElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DoubleElement)

            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, CharAttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, Int8AttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, Int16AttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, Int32AttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, Int64AttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, UInt8AttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, UInt16AttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, UInt32AttributeElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, UInt64AttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, BoolAttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, StringAttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, FloatAttributeElement)
            //            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DatasetAttribute, DoubleAttributeElement)


        }
    }
}
