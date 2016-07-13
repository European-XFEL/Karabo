/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on May 6, 2013, 10:18 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Hdf5FileOutput.hh"

using namespace karabo::util;

namespace karabo {
    namespace io {

        KARABO_REGISTER_FOR_CONFIGURATION(Output<Hash>, Hdf5FileOutput<Hash>)
    }
}