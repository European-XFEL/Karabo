/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "InterInstanceInput.hh"

namespace karabo {
    namespace xms {
        
        KARABO_REGISTER_FACTORY_2_CC(AbstractInput, Input<karabo::util::Hash >, InterInstanceInput<karabo::util::Hash>)
        KARABO_REGISTER_FACTORY_CC(Input<karabo::util::Hash >, InterInstanceInput<karabo::util::Hash>)
                
    }
}
