/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *   
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <exfel/core/Device.hh>
#include <exfel/core/Runner.hh>


using namespace exfel::core;


int main(int argc, char** argv) {
    try {

        Device::Pointer device = Runner<Device>::instantiate(argc, argv);
        if (device) device->run();

    } catch (const exfel::util::Exception& e) {
        std::cout << e;
    } catch (...) {
        RETHROW
    }
}
