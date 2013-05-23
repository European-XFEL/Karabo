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
#include "DeviceInput.hh"

namespace exfel {
    namespace xip {

        EXFEL_REGISTER_FACTORY_2_CC(AbstractInput, Input<CpuImage<float> >, DeviceInput<CpuImage<float> >)
        EXFEL_REGISTER_FACTORY_CC(Input<CpuImage<float> >, DeviceInput<CpuImage<float> >)

        EXFEL_REGISTER_FACTORY_2_CC(AbstractInput, Input<exfel::util::Hash >, DeviceInput<exfel::util::Hash>)
        EXFEL_REGISTER_FACTORY_CC(Input<exfel::util::Hash >, DeviceInput<exfel::util::Hash>)

        //EXFEL_REGISTER_FACTORY_2_CC(AbstractInput, Input<std::vector<char> >, DeviceInput<std::vector<char> >)
        //EXFEL_REGISTER_FACTORY_CC(Input<std::vector<char> >, DeviceInput<std::vector<char> >)

    }
}
