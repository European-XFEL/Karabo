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

#include "WaitUtils.hh"

#include <chrono>
#include <cmath>
#include <thread>

namespace karabo::tests {
    bool waitForCondition(std::function<bool()> checker, unsigned int timeoutMillis) {
        constexpr unsigned int sleepIntervalMillis = 2;
        unsigned int numOfWaits = 0;
        const unsigned int maxNumOfWaits = static_cast<unsigned int>(std::ceil(timeoutMillis / sleepIntervalMillis));
        while (numOfWaits < maxNumOfWaits && !checker()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepIntervalMillis));
            numOfWaits++;
        }
        return (numOfWaits < maxNumOfWaits);
    }
} // namespace karabo::tests
