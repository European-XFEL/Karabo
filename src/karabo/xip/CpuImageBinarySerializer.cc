/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 10, 2012, 6:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "CpuImageBinarySerializer.hh"

namespace karabo {
    namespace xip {
        
        KARABO_REGISTER_FACTORY_CC(karabo::io::BinarySerializer<CpuImage<float> >, CpuImageBinarySerializer<float> )
                


    }
}
