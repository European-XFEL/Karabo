/*
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

#include "BaseElement.hh"

#include <string>

#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Hash.hh"

void karabo::data::checkPropertyPath(const std::string& name, bool strict) {
    if (name.empty() || name.back() == Hash::k_defaultSep) {
        throw KARABO_PARAMETER_EXCEPTION(("Bad (sub-)key '" + name) += "': empty");
    }
    // '/' is special: only allowed for backward compatibility in metro devices of DA group
    constexpr char allowedCharacters[] =
          ".0123456789_/" // In 3.1.X move '/' to tolerated characters?
          "abcdefghijklmnopqrstuvwxyz"
          "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    assert(allowedCharacters[0] == karabo::data::Hash::k_defaultSep);

    if (strict) {
        const size_t pos = name.find_first_not_of(allowedCharacters);
        if (pos != std::string::npos) {
            std::string msg(("Bad (sub-)key '" + name) += "': illegal character at position ");
            throw KARABO_PARAMETER_EXCEPTION(msg += toString(pos));
        }
    } else {
        // If requested to be not strict, we tolerate some characters.
        // Note we must not tolerate ',', '=', or space for sake of the influxDB line protocol!
        const std::string toleratedCharacters("@-");
        const std::string allAllowedCharacters(allowedCharacters + toleratedCharacters);
        const size_t pos = name.find_first_not_of(allAllowedCharacters);
        if (pos != std::string::npos) {
            std::string msg(("Bad (sub-)key '" + name) += "': not tolerated character at position ");
            throw KARABO_PARAMETER_EXCEPTION(msg += toString(pos));
        }
    }
    // Now check that leading character of (last part of) name is not a digit
    // (non-last parts are checked when their nodes were added)
    constexpr char digits[] = "0123456789/"; // '/' not as first character as well
    const size_t pos2 = name.rfind(karabo::data::Hash::k_defaultSep);
    // pos2 + 1 is a safe index since above we checked that name.back() != Hash::k_defaultSep
    const char characterToCheck = (pos2 == std::string::npos ? name[0] : name[pos2 + 1]);
    for (size_t i = 0ul; i < sizeof(digits) / sizeof(digits[0]); ++i) {
        if (characterToCheck == digits[i]) {
            throw KARABO_PARAMETER_EXCEPTION(("Bad (sub-)key '" + name) += "': Starts with a digit or '/'");
        }
    }
}
