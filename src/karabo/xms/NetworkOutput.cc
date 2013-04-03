/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "NetworkOutput.hh"

using namespace io;

namespace karabo {
    namespace xms {

        KARABO_REGISTER_FOR_CONFIGURATION(AbstractOutput, Output<karabo::util::Hash >, NetworkOutput<karabo::util::Hash>)
        KARABO_REGISTER_FOR_CONFIGURATION(Output<karabo::util::Hash >, NetworkOutput<karabo::util::Hash>)
        
    }
}
