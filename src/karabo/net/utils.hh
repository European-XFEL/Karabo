/*
 * File:   utils.hh
 * Author: gero.flucke@xfel.eu
 *
 * Created on February 11, 2016, 10:30 AM
 *
 *  * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_NET_UTILS_HH
#define	KARABO_NET_UTILS_HH

#include <string>

namespace karabo {
    namespace net {
        /// Bare host name after stripping domain (exflxxx12345.desy.de => exflxxx12345)
        std::string bareHostName();
    }
}

#endif	/* KARABO_NET_UTILS_HH */

