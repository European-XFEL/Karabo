/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Device.hh"
#include "Runner.hh"

using namespace exfel::core;
using namespace exfel::util;
using namespace exfel::log;

int main(int argc, char** argv) {
  try {
      Logger::Pointer log = Logger::create("Logger");
      log->initialize();
      Device::Pointer device = Runner<Device>::instantiate(argc, argv);
      if (device) device->run();
      
  } catch (const exfel::util::Exception& e) {
    std::cout << e;
  } catch (...) {
    RETHROW
  }
  return EXIT_SUCCESS;
}
