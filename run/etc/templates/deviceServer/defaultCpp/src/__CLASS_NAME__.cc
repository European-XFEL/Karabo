/*
 * $Id$
 *
 * Author: <__EMAIL__>
 *
 * Created on __DATE__
 * 
 * Copyright (C) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/core/DeviceServer.hh>
#include <karabo/core/Runner.hh>

using namespace karabo::core;
using namespace karabo::util;
using namespace karabo::log;

int main(int argc, char** argv) {
  try {
      
      DeviceServer::Pointer deviceServer = Runner<DeviceServer>::instantiate(argc, argv);
      if (deviceServer) deviceServer->run();
      
  } catch (const karabo::util::TimeoutException& e) {
    std::cout << std::endl << "An error has occurred: Network response timed out." << std::endl;
    std::cout << "Make sure that a master-device-server is running under the configured broker/topic." << std::endl;
  } catch (const karabo::util::Exception& e) {
      std::cout << e;
  } catch (...) {
      RETHROW
  }
  return EXIT_SUCCESS;
}
