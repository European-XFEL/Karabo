/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 25, 2011, 8:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "CpuImage.hh"
#include <karabo/xms/NetworkInput.hh>
#include <karabo/xms/NetworkOutput.hh>


namespace exfel {
    namespace xip {

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::AbstractOutput, karabo::io::Output<karabo::xip::CpuImage<float> >, karabo::xms::NetworkOutput<karabo::xip::CpuImage<float> >)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<karabo::xip::CpuImage<float> >, karabo::xms::NetworkOutput<karabo::xip::CpuImage<float> >)
                
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::AbstractOutput, karabo::io::Output<karabo::xip::CpuImage<double> >, karabo::xms::NetworkOutput<karabo::xip::CpuImage<double> >)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<karabo::xip::CpuImage<double> >, karabo::xms::NetworkOutput<karabo::xip::CpuImage<double> >)

    }
}
