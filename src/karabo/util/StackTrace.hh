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
/*
 * Author: boukhelef
 *
 * Created on March 31, 2014, 3:52 PM
 */

#ifndef KARABO_UTIL_STACKTRACE_HH
#define KARABO_UTIL_STACKTRACE_HH

#include <iostream>
#include <string>

namespace karabo {
    namespace util {


        /**
         * @class StackTrace
         * @brief StackTrace prints out the exception stack, symbols are, in the best case, C++ demangled.
         */
        class StackTrace {
           public:
            /**
             * Demangle symbol into a string
             * @param symbol
             * @return
             */
            static std::string demangle(const char* symbol);

            /**
             * Print trace to an output stream
             * @param os
             */
            static void print(std::ostream& os);
        };

        std::ostream& operator<<(std::ostream& os, const StackTrace& trace);
    } // namespace util
} // namespace karabo

#endif /* SIGNALHANDLER_HH */
