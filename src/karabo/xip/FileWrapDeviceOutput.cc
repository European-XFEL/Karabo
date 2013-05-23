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
#include "FileWrapDeviceOutput.hh"

namespace exfel {
    namespace xip {

        EXFEL_REGISTER_FACTORY_2_CC(AbstractOutput, Output<std::string >, FileWrapDeviceOutput)
        EXFEL_REGISTER_FACTORY_CC(Output<std::string >, FileWrapDeviceOutput)



    }
}
