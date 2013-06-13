/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "CpuEnvironment.hh"

namespace karabo {
    namespace xip {

        KARABO_REGISTER_IN_FACTORY(Environment<float>, CpuEnvironment<float>)
        KARABO_REGISTER_IN_FACTORY(Environment<double>, CpuEnvironment<double>)

    }
}

