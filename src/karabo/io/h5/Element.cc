/*
 * $Id: Format.cc 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Element.hh"




using namespace karabo::io::h5;


namespace karabo {
    namespace io {
        namespace h5 {

            KARABO_REGISTER_IN_FACTORY(Element, Scalar<int>);
            KARABO_REGISTER_IN_FACTORY(Element, Scalar<float>);



        }
    }
}

