/*
 * $Id: Complex.cc 9537 2013-04-26 07:36:59Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Complex.hh"
using namespace karabo::io;

namespace karabo {
    namespace io {
        namespace h5 {

            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, FloatComplexElement)
            KARABO_REGISTER_FOR_CONFIGURATION(Element, Dataset, DoubleComplexElement)

        }
    }
}
