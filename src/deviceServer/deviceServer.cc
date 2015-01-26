/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 * Modified by: <krzysztof.wrona@xfel.eu>
 * Modified by: <djelloul.boukhelef@xfel.eu> 
 * 2014-Apr-01: Add signal handler
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/core/DeviceServer.hh>
#include <karabo/core/Runner.hh>
#include <karabo/util/SignalHandler.hh>

using namespace karabo::core;
using namespace karabo::util;
using namespace karabo::log;


int main(int argc, char** argv) {
    srand(time(0) + getpid());

    try {
        DeviceServer::Pointer deviceServer;
        
        #ifdef __linux__
        pthread_t sig_thr_id; // Signal handler thread ID
        int ret = pthread_create(&sig_thr_id, NULL, GlobalExceptionHandler::SignalThread, NULL);

        usleep(100000);

        // Store this point in the stack so that, in case of any signal or serious exception, 
        // the program can resume its execution at this point, and perform any needed cleaning
        int signum = 0;
        if (signum = setjmp(context)) {
            sigset_t signal_mask;
            sigemptyset(&signal_mask);
            sigaddset(&signal_mask, signum);
            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
            // cout << "Trigger thread: " << pthread_self() << "\t\tsignum = " << signum << endl;

            std::cout << "Thread: " << pthread_self() << " -> performs final cleaning after receiving '"
                    << strsignal(signum) << " (" << signum << ")' signal ...\n\n";

            if (deviceServer) {
                // Warning: if the deviceServer instance is not in stable state, this may trigger segmentation violation!!!!
                std::cout << "Shutting down the device server \"" << deviceServer->getInstanceId() << "\" ...\n\n";
                deviceServer->call(deviceServer->getInstanceId(), "slotKillServer");

                
                int maxWait = 0;
                while (deviceServer->isRunning() && maxWait < 10) {
                    std::cout << "Waiting for device server to die..." << std::endl;
                    maxWait++;
                    usleep(1000000);
                }
                if (deviceServer->isRunning()) {
                    std::cout << "\n\n" << "Device server could not be shut down in time, still exiting now." << std::endl;
                    usleep(1000000);
                }
            }

            // Log the error in a temporary file
            /*            std::string tmpdir = boost::filesystem::temp_directory_path().string();
                        boost::filesystem::path temp = boost::filesystem::unique_path();
                        const std::string filename = tmpdir + "/" + string(program_invocation_short_name) + ".log"; // optional, temp.native()

                        ofstream file(filename.c_str(), std::ofstream::out | std::ofstream::app);

                        file << Epochstamp().toIso8601() << " \t " << program_invocation_short_name << "\t" << getpid() << "\t" << pthread_self() << endl;

                        file.close();
             */
            // Auto-restart application
            // system(program_invocation_name);

            std::cout << "Thread: " << pthread_self() << " -> invoke the default handler for \"" << strsignal(signum) << "\"\n\n";
            // Raise the signal again using default handler.
            // This trick will trigger the core dump
            signal(signum, SIG_DFL);
            kill(getpid(), signum);

            abort();

            return EXIT_FAILURE;
        }
        #endif

        deviceServer = Runner<DeviceServer>::instantiate(argc, argv);
        if (deviceServer) {
            
            // Register signal handlers
            static SignalHandler<SegmentationViolation> __SegmentationFaultHandler;
            static SignalHandler<GenericException> __GenericExceptionHandler;
            static SignalHandler<FloatingPointException> __FloatingPointExceptionHandler;
            static SignalHandler<QuitSignal> __QuitSignalHandler;
            static SignalHandler<TerminateSignal> __TerminateSignalHandler;
            static SignalHandler<HangupSignal> __HangupSignalHandler;

            // Global instance of ExceptionHandler
            static GlobalExceptionHandler __GlobalExceptionHandler;

            //std::cout << "Main thread: " << pthread_self() << endl;
            // Mask all signals except SIGSEGV, SIGFPE, SIGBUS
            // Every thread will be responsible for his own error of this types.
            // Other async. signals will be blocked for all worker threads, and caught/handled by one SignalThread
            sigset_t signal_mask;
            sigfillset(&signal_mask);
            sigdelset(&signal_mask, SIGSEGV);
            sigdelset(&signal_mask, SIGFPE);
            sigdelset(&signal_mask, SIGBUS);
            
            // In debug mode, SIGINT is also not blocked so that breakpoints can be set by debugger
            if (deviceServer->isDebugMode()) {
                sigdelset(&signal_mask, SIGINT);
            } else {
                static SignalHandler<InterruptSignal> __InterruptSignalHandler;
            }

            int ret = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

            deviceServer->run();
        }

        return EXIT_SUCCESS;

    } catch (const karabo::util::Exception& e) {
        std::cout << e;
    } catch (...) {
        KARABO_RETHROW;
    }
    return EXIT_FAILURE;
}
