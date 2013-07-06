/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "ImageFileReader.hh"

namespace karabo {
    namespace xip {
        
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::AbstractInput, karabo::io::Input<CpuImage<char> >, ImageFileReader<char>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<CpuImage<char> >, ImageFileReader<char>)

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::AbstractInput, karabo::io::Input<CpuImage<float> >, ImageFileReader<float>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<CpuImage<float> >, ImageFileReader<float>)
                
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::AbstractInput, karabo::io::Input<CpuImage<double> >, ImageFileReader<double>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<CpuImage<double> >, ImageFileReader<double>)


    }
}
