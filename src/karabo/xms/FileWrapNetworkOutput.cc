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

namespace karabo {
    namespace xms {

        KARABO_REGISTER_FACTORY_2_CC(AbstractOutput, Output<std::string >, FileWrapNetworkOutput)
        KARABO_REGISTER_FACTORY_CC(Output<std::string >, FileWrapNetworkOutput)



    }
}
