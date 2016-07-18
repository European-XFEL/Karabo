/*
 * $Id$
 *
 * Author: <gero.flucke@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "DataLogUtils.hh"

#include "karabo/util/StringTools.hh"

namespace karabo {
    namespace core {


        util::Epochstamp stringDoubleToEpochstamp(const std::string& timestampAsDouble) {
            std::vector<std::string> tparts;
            boost::split(tparts, timestampAsDouble, boost::is_any_of("."));
            const unsigned long long seconds = util::fromString<unsigned long long>(tparts[0]);
            // If by chance we hit a full second without fractions, we have no ".":
            const unsigned long long fractions = (tparts.size() >= 2 ?
                                                  util::fromString<unsigned long long>(tparts[1]) * 1000000000000ULL : 0ULL); // ATTOSEC

            return util::Epochstamp(seconds, fractions);
        }

    } // end of karabo::core
}

