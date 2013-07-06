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

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<CpuImage<float> >, CpuImageBinarySerializer<float>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<CpuImage<double> >, CpuImageBinarySerializer<double>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<CpuImage<char> >, CpuImageBinarySerializer<char>)

    }
}
