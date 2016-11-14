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
    }
}

#endif	/* SIGNALHANDLER_HH */

