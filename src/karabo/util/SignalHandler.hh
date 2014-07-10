/* 
 * File:   SignalHandler.hh
 * Author: boukhelef
 *
 * Created on March 31, 2014, 3:52 PM
 */

#ifndef SIGNALHANDLER_HH
#define	SIGNALHANDLER_HH


#ifdef __linux__
#include <limits.h>
#include <setjmp.h>
#include <pthread.h>
#include <signal.h>

#if defined(__GNUC__)
#if defined(__GLIBC__)
#ifndef __UCLIBC__
#define _HAVE_EXECINFO_H_ 1
#endif
#endif
#endif

#ifdef _HAVE_EXECINFO_H_
#include <execinfo.h>
#include <cxxabi.h>
#endif

#endif

#include <exception>
#include <boost/thread/pthread/mutex.hpp>

namespace karabo {
    namespace util {

        #ifdef __linux__

        jmp_buf context;

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

        std::ostream& operator <<(std::ostream& os, const StackTrace& trace) {
            trace.print(os);
            return os;
        }

        /*************************************************************
         * 
         * SignalHandler class install signal handler provided at the template argument
         * 
         *-----------------------------------------------------------*/


        template <class SignalExceptionClass> class SignalHandler {

        private:
            class Singleton {

            public:

                Singleton() {
                    // TODO: save/restore previous signal handler
                    struct sigaction action, old_action;
                    memset(&action, 0, sizeof (struct sigaction));
                    sigemptyset(&action.sa_mask);
                    action.sa_sigaction = HandleSignal;
                    action.sa_flags = SA_SIGINFO | SA_ONSTACK; // | SA_RESTART | SA_RESETHAND

                    sigaction(SignalExceptionClass::GetSignalNumber(), &action, &old_action);
                }

                static void HandleSignal(int signum, siginfo_t *siginfo, void *ctx) {

                    switch (SignalExceptionClass::PreProcessing()) {
                        case 0: throw SignalExceptionClass();
                            break;
                        case 1:
                            // Jump to the cleaning section
                            longjmp(context, signum);
                            break;
                        default:;
                    }
                }
            };

        public:

            SignalHandler() {
                static Singleton __SignalHandler;
            }
        };

        // Handler for SIGSEGV

        class SegmentationViolation : public std::exception {

        public:

            static int GetSignalNumber() {
                return SIGSEGV;
            }

            static int PreProcessing() {
                return 0;
            }
        };

        // Handler for generic exception

        class GenericException : public std::exception {

        public:

            static int GetSignalNumber() {
                return _NSIG;
            }

            static int PreProcessing() {
                return 0;
            }
        };

        // Handler for SIGFPE

        class FloatingPointException : public std::exception {

        public:

            static int GetSignalNumber() {
                return SIGFPE;
            }

            static int PreProcessing() {
                return 0;
            }
        };

        // Handler for SIGINT

        class InterruptSignal : public std::exception {

        public:

            static int GetSignalNumber() {
                return SIGINT;
            }

            static int PreProcessing() {
                return 1;
            }
        };

        // Handler for SIGQUIT

        class QuitSignal : public std::exception {

        public:

            static int GetSignalNumber() {
                return SIGQUIT;
            }

            static int PreProcessing();
        };

        // Handler for SIGTERM

        class TerminateSignal : public std::exception {

        public:

            static int GetSignalNumber() {
                return SIGTERM;
            }

            static int PreProcessing() {
                return 1;
            }
        };

        // Handler for SIGHUP

        class HangupSignal : public std::exception {

        public:

            static int GetSignalNumber() {
                return SIGHUP;
            }

            static int PreProcessing() {
                return 1;
            }
        };

        /*************************************************************
         * 
         * Generic exceptions and signals handler
         * This class handles signals using two methods:
         * TerminateCallback: handles signals (eg. SIGSEGV,...) that were caught by other thread 
         * and were translated into exceptions using the SignalHandler translator
         * SignalThread: Handles other signals (eg. SIGINT, SIGTERM, ...)
         * This trick improves the robustness of the application by delegating one thread to handle all async. signals.
         * In the best case, the other worker threads continue running without being disturbed.
         * 
         *-----------------------------------------------------------*/

        class GlobalExceptionHandler {

        private:

            class Singleton {

                static boost::mutex m_mutex;
            public:

                Singleton() {
                    std::set_terminate(TerminateCallback);
                }

                static void TerminateCallback();
            };

        public:
            static void *SignalThread(void *arg);

            GlobalExceptionHandler() {
                static Singleton __GlobalExceptionHandler;
            }
        };

        #endif

    }
}

#endif	/* SIGNALHANDLER_HH */

