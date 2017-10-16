/*
 * $Id$
 *
 * Author: <gero.flucke@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "karabo/util/StringTools.hh"

#include "DataLogUtils.hh"

namespace karabo {
    namespace util {


        util::Epochstamp stringDoubleToEpochstamp(const std::string& timestampAsDouble) {
            std::vector<std::string> tparts;
            boost::split(tparts, timestampAsDouble, boost::is_any_of("."));
            const unsigned long long seconds = util::fromString<unsigned long long>(tparts[0]);
            unsigned long long fractions = 0ULL;
            // If by chance we hit a full second without fractions, we have no ".":
            if (tparts.size() >= 2) {
                std::string& fracString = tparts[1];
                // We read in all after the dot. If coming from the raw data logger files, we should have exactly
                // 6 digits (e.g. ms), even with trailing zeros, but one never knows.
                unsigned long long factorToAtto = 1000000000000ULL;
                const size_t nDigitMicrosec = 6;
                if (nDigitMicrosec > fracString.size()) {
                    for (size_t i = 0; i < nDigitMicrosec - fracString.size(); ++i) {
                        factorToAtto *= 10ULL;
                    }
                } else if (nDigitMicrosec < fracString.size()) {
                    for (size_t i = 0; i < fracString.size() - nDigitMicrosec; ++i) {
                        factorToAtto /= 10ULL;
                    }
                }
                const size_t firstNonZero = fracString.find_first_not_of('0');
                if (firstNonZero != 0 && firstNonZero != std::string::npos) {
                    // Get rid of leading 0 which triggers interpretation as octal number:
                    fracString.replace(0, firstNonZero, firstNonZero, ' '); // replace by space avoids any re-allocation
                }
                // Finally, multiply to convert to ATTOSEC:
                fractions = util::fromString<unsigned long long>(fracString) * factorToAtto;
            }

            return util::Epochstamp(seconds, fractions);
        }

    } // end of karabo::util
}

