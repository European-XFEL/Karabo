/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "FileWrapNetworkOutput.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace xms {

        KARABO_REGISTER_FOR_CONFIGURATION(AbstractOutput, Output<std::string >, FileWrapNetworkOutput)
        KARABO_REGISTER_FOR_CONFIGURATION(Output<std::string >, FileWrapNetworkOutput)

    }
}
