/*
 * Author: boukhelef
 *
 * Created on March 31, 2014, 3:52 PM
 */

#ifndef KARABO_UTIL_STACKTRACE_HH
#define	KARABO_UTIL_STACKTRACE_HH

#include <string>
#include <iostream>

namespace karabo {
    namespace util {

        /*************************************************************
         *
         * StackTrace prints out the exception stack,
         * Symbols are, in the best case, C++ demangled.
         *
         *-----------------------------------------------------------*/

        class StackTrace {

        public:
            static std::string demangle(const char* symbol);
            static void print(std::ostream& os);
        };

        std::ostream& operator<<(std::ostream& os, const StackTrace& trace);
    }
}

#endif	/* SIGNALHANDLER_HH */

