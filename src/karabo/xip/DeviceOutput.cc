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
#include "DeviceOutput.hh"

namespace exfel {
    namespace xip {

        //EXFEL_REGISTER_FACTORY_2_CC(AbstractOutput, Output<CpuImage<float> >, DeviceOutput<CpuImage<float> >)
        //EXFEL_REGISTER_FACTORY_CC(Output<CpuImage<int> >, DeviceOutput<CpuImage<int> >)
        EXFEL_REGISTER_FACTORY_2_CC(AbstractOutput, Output<exfel::util::Hash >, DeviceOutput<exfel::util::Hash>)
        EXFEL_REGISTER_FACTORY_CC(Output<exfel::util::Hash >, DeviceOutput<exfel::util::Hash>)



    }
}
