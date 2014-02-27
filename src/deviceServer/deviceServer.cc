/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 * Modified by: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/core/DeviceServer.hh>
#include <karabo/core/Runner.hh>

using namespace karabo::core;
using namespace karabo::util;
using namespace karabo::log; 


int main(int argc, char** argv) {
    try {  
        

        vector<char*> newArgvVec(argc + 1);
        char** newArgv = &newArgvVec[0];
        for (int i = 0; i < argc; ++i) {
            newArgv[i] = argv[i]; 
        }

        string serverIdFileName("serverId.xml");
        if (boost::filesystem::exists(serverIdFileName)) {
            newArgv[argc] = (char*) "serverId.xml";
            argc++;
        }


        DeviceServer::Pointer deviceServer = Runner<DeviceServer>::instantiate(argc, newArgv);
        if (deviceServer) deviceServer->run();

    } catch (const karabo::util::TimeoutException& e) {
        std::cout << std::endl << "An error has occurred: Network response timed out." << std::endl;
        std::cout << "Make sure that a master-device-server is running under the configured broker/topic." << std::endl;
    } catch (const karabo::util::Exception& e) {
        std::cout << e;
    } catch (...) {
        KARABO_RETHROW;
    }
    return EXIT_SUCCESS;
}
