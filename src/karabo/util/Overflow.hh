/* 
 * File:   Overflow.hh
 * Author: gero.flucke@xfel.eu
 *
 * Created on February 23, 2018, 5:04 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 *
 */

#ifndef KARABO_UTIL_OVERFLOW_HH
#define	KARABO_UTIL_OVERFLOW_HH

#include <utility>

namespace karabo {
    namespace util {

        /// Add 'second' to 'first' and return overflow bits shifted by 64 bit to the right
        unsigned long long
        safeAddToFirst(unsigned long long& first, unsigned long long second);

        /// Multiply two 64-bit unsigned long long numbers and return a pair to be interpreted as a single
        /// 128-bit value where 'first' is the higher (left) 64 bits and 'second' the lower ones, respectively.
        std::pair<unsigned long long, unsigned long long>
        safeMultiply(unsigned long long a, unsigned long long b);
    }
}

#endif	/* KARABO_UTIL_OVERFLOW_HH */

