/*
 * $Id$
 *
 * Author: <__EMAIL__>
 * 
 * Created on __DATE__
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/karabo.hpp>

using namespace std;
using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::log;
using namespace karabo::core;


int main(int argc, char** argv) {

    // Activate the Logger (the threshold priority can be changed to INFO, WARN or ERROR)
    Logger::configure(Hash("priority", "DEBUG"));

    // Create an instance of the device
    BaseDevice::Pointer d = BaseDevice::create("__CLASS_NAME__", Hash("deviceId", "Test___CLASS_NAME___0"));

    // Run the device
    d->run(); // Will block

    return (EXIT_SUCCESS);
}
