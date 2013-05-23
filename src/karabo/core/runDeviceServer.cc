/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "DeviceServer.hh"
#include "Runner.hh"

using namespace exfel::core;
using namespace exfel::util;
using namespace exfel::log;


int main(int argc, char** argv) {
    try {

        DeviceServer::Pointer deviceServer = Runner<DeviceServer>::instantiate(argc, argv);
        if (deviceServer) deviceServer->run();

    } catch (const exfel::util::Exception& e) {
        std::cout << e;
    } catch (...) {
        RETHROW
    }
    return EXIT_SUCCESS;
}
