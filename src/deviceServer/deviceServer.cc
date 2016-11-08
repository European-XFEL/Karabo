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

        boost::thread t(boost::bind(&EventLoop::work));

        // Instantiate device server
        DeviceServer::Pointer deviceServer = Runner<DeviceServer>::instantiate(argc, argv);

        // Handle signals using the event-loop
        EventLoop::setSignalHandler([&deviceServer](int signo) {
            deviceServer.reset(); // triggers the destructor
        });

        // Start the device server
        deviceServer->finalizeDeviceConstruction();

        t.join(); // Blocking central event loop

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
