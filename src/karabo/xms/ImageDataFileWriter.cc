/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifdef __MACH__
#include "../util/fmemopen.h"
#endif

#include "ImageDataFileWriter.hh"

namespace karabo {
    namespace xms {
        
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<ImageData >, ImageDataFileWriter)

    }
}
