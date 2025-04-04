/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef KARABO_DATA_TYPES_BYTESWAP_HH
#define KARABO_DATA_TYPES_BYTESWAP_HH

#include <stdint.h>

#include <string>

// gcc only
#if defined __GNUC__ && __GNUC__ >= 2 && __linux__
#include <byteswap.h>
#endif

// TODO: Mac OS X and Windows optimization

namespace karabo {
    namespace data {

        // Byte swap utility
        uint16_t byteSwap16(uint16_t in);
        uint32_t byteSwap32(uint32_t in);
        uint64_t byteSwap64(uint64_t in);

    } // namespace data
} // namespace karabo

#endif // KARABO_DATA_TYPES_BYTESWAP_HH
