/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_BYTESWAP_HH
#define KARABO_UTIL_BYTESWAP_HH

#include <string>

#include <stdint.h>

// gcc only
#if defined __GNUC__ && __GNUC__ >= 2 && __linux__
#include <byteswap.h>
#endif

// TODO: Mac OS X and Windows optimization 

namespace karabo {
    namespace util {
        
        // Byte swap utility
        uint16_t byteSwap16(uint16_t in);
        uint32_t byteSwap32(uint32_t in);
        uint64_t byteSwap64(uint64_t in);
        
    } // util
} // karabo

#endif // KARABO_UTIL_BYTESWAP_HH
