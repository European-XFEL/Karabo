/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "ImageFileWriter.hh"

namespace karabo {
    namespace xip {

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<CpuImage<double> >, ImageFileWriter<double>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<CpuImage<float> >, ImageFileWriter<float>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<CpuImage<unsigned int> >, ImageFileWriter<unsigned int>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<CpuImage<unsigned short> >, ImageFileWriter<unsigned short>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<CpuImage<unsigned char> >, ImageFileWriter<unsigned char>)

    }
}
