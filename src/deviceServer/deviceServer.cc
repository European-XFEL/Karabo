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
        // Register system signals we want to deal with
        boost::asio::signal_set signals(EventLoop::getIOService(), SIGINT, SIGTERM, SIGSEGV);

        // Instantiate device server
        DeviceServer::Pointer deviceServer = Runner<DeviceServer>::instantiate(argc, argv);
        if (deviceServer) {

            boost::thread t(boost::bind(&DeviceServer::run, deviceServer));
            boost::asio::io_service::work work(EventLoop::getIOService());

            // Handle signals using the event-loop
            signals.async_wait(
                               [deviceServer](boost::system::error_code /*ec*/, int signo) {
                                   deviceServer->call(deviceServer->getInstanceId(), "slotKillServer");
                                   if (signo == SIGSEGV) {
                                       std::cout << StackTrace() << std::endl;
                                   }
                               });

            EventLoop::run(); // Block central event loop
            t.join();
        }

        return EXIT_SUCCESS;

    } catch (const Exception& e) {
        cout << e;
    } catch (...) {
        cout << "Encountered unknown exception" << endl;
    }
    return EXIT_FAILURE;
}
