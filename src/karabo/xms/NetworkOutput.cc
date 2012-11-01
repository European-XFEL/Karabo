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

namespace karabo {
    namespace xms {

        KARABO_REGISTER_FACTORY_2_CC(AbstractOutput, Output<karabo::util::Hash >, NetworkOutput<karabo::util::Hash>)
        KARABO_REGISTER_FACTORY_CC(Output<karabo::util::Hash >, NetworkOutput<karabo::util::Hash>)
        
    }
}
