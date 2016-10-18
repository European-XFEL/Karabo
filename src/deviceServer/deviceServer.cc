/*
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "karabo/util/StackTrace.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/core/Runner.hh"


using namespace karabo::util;
using namespace karabo::log;
using namespace karabo::net;
using namespace karabo::core;


int main(int argc, char** argv) {

    try {


        // Instantiate device server
        DeviceServer::Pointer deviceServer = Runner<DeviceServer>::instantiate(argc, argv);
        if (deviceServer) {

            // TODO: Re-factor later to use central event-loop
            boost::thread t(boost::bind(&DeviceServer::run, deviceServer));

            // Register system signals we want to deal with
            boost::asio::signal_set signals(EventLoop::getIOService(), SIGINT, SIGTERM, SIGSEGV);

            // Handle signals using the event-loop
            signals.async_wait(
                               [deviceServer](boost::system::error_code /*ec*/, int signo) {
                                   deviceServer->call(deviceServer->getInstanceId(), "slotKillServer");
                               if (signo == SIGSEGV) {
                               std::cout << StackTrace() << std::endl;
                                   }
                               });

            EventLoop::work(); // Blocking central event loop
            t.join();
        }

        return EXIT_SUCCESS;

    } catch (const Exception& e) {
        cerr << e;
    } catch (...) {
        cerr << "Encountered unknown exception" << endl;
    }
    return EXIT_FAILURE;
}
