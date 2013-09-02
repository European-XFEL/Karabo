/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "FileWrapNetworkInput.hh"

using namespace karabo::util;
using namespace karabo::io;

namespace karabo {
    namespace xms {

        KARABO_REGISTER_FOR_CONFIGURATION(AbstractInput, Input<std::string >, FileWrapNetworkInput)
        KARABO_REGISTER_FOR_CONFIGURATION(Input<std::string >, FileWrapNetworkInput)

    }
}
