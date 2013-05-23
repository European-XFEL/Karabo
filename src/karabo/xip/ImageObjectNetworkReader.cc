/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "ImageObjectNetworkReader.hh"

namespace exfel {
    namespace xip {

        EXFEL_REGISTER_FACTORY_CC(Input<CpuImgD>, ImageObjectNetworkReader<double>)

    }
}

