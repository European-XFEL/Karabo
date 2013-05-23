/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "NormalizationProcessor.hh"

namespace karabo {
    namespace xip {

        KARABO_REGISTER_FACTORY_CC(SingleProcessor<CpuImgD>, NormalizationProcessor<double>)

    }
}

