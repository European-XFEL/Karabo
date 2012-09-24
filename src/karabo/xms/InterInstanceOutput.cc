/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "InterInstanceOutput.hh"

namespace exfel {
    namespace xms {

        EXFEL_REGISTER_FACTORY_2_CC(AbstractOutput, Output<exfel::util::Hash >, InterInstanceOutput<exfel::util::Hash>)
        EXFEL_REGISTER_FACTORY_CC(Output<exfel::util::Hash >, InterInstanceOutput<exfel::util::Hash>)
        
    }
}
