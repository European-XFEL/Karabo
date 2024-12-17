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


#include <functional>

namespace karabo::tests {

    /**
     * Waits, for a maximum amount of time, for a condition checked by a
     * given function.
     *
     * @param checker the function that will evaluate if the target condition has
     *        been reached.
     *
     * @param timeoutMillis the maximum amount of time to wait for the condition
     *        (in milliseconds).
     *
     * @return true if the condition has been reached; false if time expired before
     *         the condition could have been reached.
     */
    bool waitForCondition(std::function<bool()> checker, unsigned int timeoutMillis);

} // namespace karabo::tests
