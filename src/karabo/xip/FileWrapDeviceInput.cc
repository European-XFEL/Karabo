/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "CpuImage.hh"
#include "FileWrapDeviceInput.hh"

namespace exfel {
    namespace xip {
        
        EXFEL_REGISTER_FACTORY_2_CC(AbstractInput, Input<std::string >, FileWrapDeviceInput)
        EXFEL_REGISTER_FACTORY_CC(Input<std::string >, FileWrapDeviceInput)
                
    }
}
