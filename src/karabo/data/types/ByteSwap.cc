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

#include "ByteSwap.hh"

namespace karabo {
    namespace data {


        uint16_t byteSwap16(uint16_t in) {
            uint16_t out = 0;

#ifdef _BYTESWAP_H
            // gcc optimized code
            out = bswap_16(in);

#else
            // Generic, non-optimized code
            out = (in << 8) | (in >> 8);
#endif

            return out;
        }


        uint32_t byteSwap32(uint32_t in) {
            uint32_t out = 0;

#ifdef _BYTESWAP_H
            // gcc optimized code
            out = bswap_32(in);

#else
            // Generic, non-optimized code
            out = ((in << 24) & 0xFF000000) | ((in << 8) & 0x00FF0000) | ((in >> 8) & 0x0000FF00) |
                  ((in >> 24) & 0x000000FF);
#endif
            return out;
        }


        uint64_t byteSwap64(uint64_t in) {
            uint64_t out = 0;

#ifdef _BYTESWAP_H
            // gcc optimized code
            out = bswap_64(in);

#else
            // Generic, non-optimized code
            out = ((in << 56) & 0xFF00000000000000) | ((in << 40) & 0x00FF000000000000) |
                  ((in << 24) & 0x0000FF0000000000) | ((in << 8) & 0x000000FF00000000) |
                  ((in >> 8) & 0x00000000FF000000) | ((in >> 24) & 0x0000000000FF0000) |
                  ((in >> 40) & 0x000000000000FF00) | ((in >> 56) & 0x00000000000000FF);
#endif
            return out;
        }

    } // namespace data
} // namespace karabo
