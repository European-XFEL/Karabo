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


        void getLeaves(const util::Hash& configuration, const util::Schema& schema, std::vector<std::string>& result, const char separator) {
            if (configuration.empty() || schema.empty()) return;
            getLeaves_r(configuration, schema, result, "", separator, false);            
        }


        void getLeaves_r(const util::Hash& hash, const util::Schema& schema, std::vector<std::string>& result,
                         std::string prefix, const char separator, const bool fullPaths) {
            if (hash.empty()) {
                return;
            }

            for (util::Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                std::string currentKey = it->getKey();

                if (!prefix.empty()) {
                    char separators[] = {separator, 0};
                    currentKey = prefix + separators + currentKey;
                }
                if (it->is<util::Hash > () && (fullPaths || !it->hasAttribute(KARABO_HASH_CLASS_ID))) { // Recursion, but no hash sub classes
                    getLeaves_r(it->getValue<util::Hash > (), schema, result, currentKey, separator, fullPaths);
                } else if (it->is<std::vector<util::Hash> > ()) { // Recursion for vector
                    //if this is a LEAF then don't go to recurse further ... leaf!
                    if (schema.has(currentKey) && schema.isLeaf(currentKey)) {
                        result.push_back(currentKey);
                    } else {
                        for (size_t i = 0; i < it->getValue<std::vector<util::Hash> > ().size(); ++i) {
                            std::ostringstream os;
                            os << currentKey << "[" << i << "]";
                            getLeaves_r(it->getValue<std::vector<util::Hash> > ().at(i), schema, result, os.str(), separator, fullPaths);
                        }
                    }
                } else {
                    result.push_back(currentKey);
                }
            }
        }

    } // end of karabo::util
}

