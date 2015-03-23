/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "NoiseProcessor.hh"

namespace karabo {
    namespace xip {

        // Noise types
        KARABO_REGISTER_FACTORY_CC(NoiseType<CpuImgD>, PoissonNoise<double>)

        KARABO_REGISTER_FACTORY_CC(NoiseType<CpuImgD>, GaussianNoise<double>)

        // Noise processor
        KARABO_REGISTER_FACTORY_CC(SingleProcessor<CpuImgD>, NoiseProcessor<double>)


    }
}

