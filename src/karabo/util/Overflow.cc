/*
 * File:   Overflow.cc
 * Author: gero.flucke@xfel.eu
 *
 * Created on February 23, 2018, 5:04 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 *
 */

#include "Overflow.hh"

namespace karabo {
    namespace util {

        unsigned long long safeAddToFirst(unsigned long long& first, unsigned long long second) {

            unsigned long long overflow = 0ull;

            const unsigned long long allBits = -1; // all bits set!
            if (first > allBits - second) {
                ++overflow; // overflow is only one bit
            }
            first += second; // if overflow, remaining bits are OK

            return overflow;
        }


        std::pair<unsigned long long, unsigned long long>
        safeMultiply(unsigned long long a, unsigned long long b) {
            // Inspired by Norman Ramsey's answer at
            // https://stackoverflow.com/questions/1815367/multiplication-of-large-numbers-how-to-catch-overflow

            const unsigned long long maskHigh = (1ull << 32ull) - 1ull; // only lower 32-bits are set

            const unsigned long long aHigh = a >> 32ull;
            const unsigned long long bHigh = b >> 32ull;
            const unsigned long long aLow = a & maskHigh;
            const unsigned long long bLow = b & maskHigh;

            // Now the result is 2**64 * aHigh * bHigh + 2**32 * (aHigh * bLow + aLow * bHigh) + aLow * bLow,
            // so calculate each term:
            unsigned long long low = aLow * bLow;
            const unsigned long long mid1 = aHigh * bLow;
            const unsigned long long mid2 = aLow * bHigh;
            unsigned long long high = aHigh * bHigh;

            // Add high parts of mid1 and mid2 to high:
            high += (mid1 >> 32ull); // Shifting by 32 bits divides by 2**32 while adding it to high implicitly...
            high += (mid2 >> 32ull); // ...multiplies by 2**64, so overall we multiply by 2**32 as desired.

            // Add low parts of mid1 to low, taking care of overflow:
            const unsigned long long mid1Low = mid1 << 32ull; // Shift 32 bits to multiply with 2**32 as desired.
            high += safeAddToFirst(low, mid1Low);

            // Add low parts of mid2 to low, taking care of overflow:
            const unsigned long long mid2Low = mid2 << 32ull; // Shift 32 bits to multiply with 2**32 as desired.
            high += safeAddToFirst(low, mid2Low);

            return std::make_pair(high, low);
        }
    }
}