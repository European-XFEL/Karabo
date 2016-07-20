/*
 * $Id: FixedLengthArrayComplex.cc 9537 2013-04-26 07:36:59Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FixedLengthArrayComplex.hh"
using namespace karabo::io;

namespace karabo {
    namespace io {
        namespace h5 {

            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, FloatArrayComplexElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DoubleArrayComplexElement)

        }
    }
}
