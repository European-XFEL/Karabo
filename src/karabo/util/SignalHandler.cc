/* 
 * File:   SignalHandler.cc
 * Author: boukhelef
 * 
 * Created on March 31, 2014, 3:52 PM
 */

#include "SignalHandler.hh"

#include <stdio.h>
#include <iostream>

using namespace std;

namespace karabo {
    namespace util {

        #ifdef __linux__

        jmp_buf context;


        std::string StackTrace::demangle(const char* symbol) {
            size_t size;
            int status;
            char temp[256];
            char* demangled;
            //Try to demangle a c++ name
            if (1 == sscanf(symbol, "%*[^(]%*[^_]%127[^)+]", temp)) {
                if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
                    std::string result(demangled);
                    free(demangled);
                    return result;
                }

            }
            //if that fails, try to get a regular c symbol
            if (sscanf(symbol, "%250s", temp) == 1) {
                return temp;
            }

            //if all doesn't work, just return the symbol
            return symbol;
        }


        void StackTrace::print(std::ostream& os) {
            const size_t TRACE_SIZE = 128;

            void * array[TRACE_SIZE];
            int trace_size = backtrace(array, (sizeof (array) / sizeof (array[0])));
            char ** symbols = backtrace_symbols(array, trace_size);
            for (int i = 0; i < trace_size; ++i) {
                //TODO: demangle symbols from other libraries
                os << demangle(symbols[i]) << '\n';
            }

            free(symbols);
        }


        std::ostream& operator <<(std::ostream& os, const StackTrace& trace) {
            trace.print(os);
            return os;
        }


        int QuitSignal::PreProcessing() {
            std::string answer;
            cout << "\nDo you really want to exit? ";
            cin >> answer;
            return (answer == "yes") ? 1 : 2;
        }


        void GlobalExceptionHandler::Singleton::TerminateCallback() {
            boost::mutex::scoped_lock(m_mutex);
            // Exception from construction/destruction of global variables

            ostringstream oss;

            int signum = _NSIG;
            try {
                // re-throw
                throw;
            } catch (SegmentationViolation &) {
                signum = SegmentationViolation::GetSignalNumber();
            } catch (FloatingPointException &) {
                signum = FloatingPointException::GetSignalNumber();
            } catch (InterruptSignal&) {
                signum = InterruptSignal::GetSignalNumber();
            } catch (HangupSignal&) {
                signum = HangupSignal::GetSignalNumber();
            } catch (QuitSignal&) {
                signum = QuitSignal::GetSignalNumber();
            } catch (TerminateSignal&) {
                signum = TerminateSignal::GetSignalNumber();
            } catch (std::exception& e) {
                oss << getpid() << ": " << "std::exception" << endl;
            } catch (...) {
                oss << getpid() << ": " << "Unknown exception" << endl;
            }

            if (signum > 0) {
                oss << getpid() << ": " << strsignal(signum) << endl;
            }

            oss << endl << StackTrace() << endl;

            cerr << oss.str() << endl;

            // Jump to the a safe point in the stack where we can resume execution
            // This avoid infinite loop, and allow to perform any neede  cleaning during the last breathes 
            longjmp(context, signum);

            return;
            // This will done during the cleaning operation, mainly at the end of the main function
            // Re-process the signal using default handler, this trick will trigger the core dump
            signal(signum, SIG_DFL);
            kill(getpid(), signum);


            abort(); // In case of thread performing some core activity

            // pthread_exit(0);// In case worker thread, ie. servicing requests
        }


        void* GlobalExceptionHandler::SignalThread(void *arg) {
            // cout << "Signal handler thread: " << pthread_self() << endl;

            static sigset_t signal_mask;
            sigfillset(&signal_mask); //Wait for all signals except:
            sigdelset(&signal_mask, SIGSEGV);
            sigdelset(&signal_mask, SIGFPE);
            sigdelset(&signal_mask, SIGBUS);
            sigdelset(&signal_mask, SIGUSR1);
            sigdelset(&signal_mask, SIGUSR2);
//            sigdelset(&signal_mask, SIGINT);
//            sigdelset(&signal_mask, SIGHUP);

            while (true) {
                int sig_num;
                int ret = sigwait(&signal_mask, &sig_num);
                if (ret != 0) {
                    // TODO: handle the error
                }
                //cerr << "\nSignal -> " << sig_caught << ": " << string(strsignal(sig_caught)) << endl;
                switch (sig_num) {
                    case SIGQUIT:
                    {
                        std::string answer;
                        cout << "\nDo you really want to exit? ";
                        cin >> answer;
                        if (answer != "yes") continue;
                    }
                    case SIGINT: /* process SIGINT  */
                    case SIGTERM: /* process SIGTERM */
                    case SIGHUP: /* process SIGHUP  */
                        cerr << "\nSignal -> " << sig_num << ": " << string(strsignal(sig_num)) << endl;
                        break;
                    default: /* should normally not happen */
                        // cerr << "\nUnexpected signal " << sig_num << ": " << string(strsignal(sig_num)) << endl;
                        continue;
                        break;
                }

                longjmp(context, sig_num);

                exit(sig_num);
            }
        }

        #endif

    }
}
