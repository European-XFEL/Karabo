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

#include "RawImageFileWriter.hh"

namespace karabo {
    namespace xip {
        
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<RawImageData >, RawImageFileWriter)

    }
}
