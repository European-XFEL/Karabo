/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "ScalarAttribute.hh"
using namespace karabo::io::h5;

namespace karabo {
    namespace io {
        namespace h5 {

            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, CharAttribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, Int8Attribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, Int16Attribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, Int32Attribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, Int64Attribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, UInt8Attribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, UInt16Attribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, UInt32Attribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, UInt64Attribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, BoolAttribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, StringAttribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, FloatAttribute)
            KARABO_REGISTER_FOR_CONFIGURATION(Attribute, DoubleAttribute)


        }
    }
}
