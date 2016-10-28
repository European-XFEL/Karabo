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

        // TODO: Re-factor later to use central event-loop
        boost::thread t(boost::bind(&DeviceServer::run, deviceServer));

        // Handle signals using the event-loop
        EventLoop::setSignalHandler([&deviceServer](int signo) {
            deviceServer->call(deviceServer->getInstanceId(), "slotKillServer");
        });

        EventLoop::work(); // Blocking central event loop
        t.join();

        return EXIT_SUCCESS;
    } catch (const Exception& e) {
        std::cerr << "Exception caught: " << e << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Standard exception caught: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Encountered unknown exception" << std::endl;
    }
    return EXIT_FAILURE;
}
