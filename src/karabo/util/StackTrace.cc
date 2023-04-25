/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * Author: boukhelef
 *
 * Created on March 31, 2014, 3:52 PM
 */

#include "StackTrace.hh"

#ifdef __linux__

#if defined(__GNUC__)
#if defined(__GLIBC__)
#ifndef __UCLIBC__
#define _HAVE_EXECINFO_H_ 1
#endif
#endif
#endif

#ifdef _HAVE_EXECINFO_H_
#include <cxxabi.h>
#include <execinfo.h>
#endif

#endif

using namespace std;

namespace karabo {
    namespace util {

#ifdef __linux__

        std::string StackTrace::demangle(const char* symbol) {
            size_t size;
            int status;
            char temp[256];
            char* demangled;
            // Try to demangle a c++ name
            if (1 == sscanf(symbol, "%*[^(]%*[^_]%127[^)+]", temp)) {
                if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
                    std::string result(demangled);
                    free(demangled);
                    return result;
                }
            }
            // if that fails, try to get a regular c symbol
            if (sscanf(symbol, "%250s", temp) == 1) {
                return temp;
            }

            // if all doesn't work, just return the symbol
            return symbol;
        }


        void StackTrace::print(std::ostream& os) {
            const size_t TRACE_SIZE = 128;

            void* array[TRACE_SIZE];
            int trace_size = backtrace(array, (sizeof(array) / sizeof(array[0])));
            char** symbols = backtrace_symbols(array, trace_size);
            for (int i = 0; i < trace_size; ++i) {
                // TODO: demangle symbols from other libraries
                os << demangle(symbols[i]) << endl;
            }

            free(symbols);
        }

#else
        void StackTrace::print(std::ostream& os) {
            os << "Not supported" << endl;
        }

#endif
        std::ostream& operator<<(std::ostream& os, const StackTrace& trace) {
            trace.print(os);
            return os;
        }
    } // namespace util
} // namespace karabo
