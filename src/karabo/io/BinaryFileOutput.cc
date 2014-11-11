/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 07, 2013, 10:18 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "BinaryFileOutput.hh"

using namespace karabo::util;

namespace karabo {
    namespace io {

        KARABO_REGISTER_FOR_CONFIGURATION(Output<Hash>, BinaryFileOutput<Hash>)
        KARABO_REGISTER_FOR_CONFIGURATION(Output<Schema>, BinaryFileOutput<Schema>)
        KARABO_REGISTER_FOR_CONFIGURATION(Output<std::vector<char> >, BinaryFileOutput<std::vector<char> >)
        
    }
}
