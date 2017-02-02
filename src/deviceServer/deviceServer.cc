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

        // We always change the directory to $KARABO/var/data
        boost::filesystem::current_path(boost::filesystem::path(Version::getPathToKaraboInstallation() + "/var/data"));

        // Let the factory now about all available plugins.
        // It is important to load plugins even before having a device server
        // instance, as this allows the help function to correctly show available
        // devices and enabling the server to autoStart them if needed.
        Hash pluginConfig("pluginDirectory", Version::getPathToKaraboInstallation() + "/plugins");
        PluginLoader::create("PluginLoader", pluginConfig)->update();

        // Instantiate device server
        DeviceServer::Pointer deviceServer = Runner::instantiate(argc, argv);

        if (deviceServer) {

            boost::thread t(boost::bind(&EventLoop::work));

            // Handle signals using the event-loop
            EventLoop::setSignalHandler([&deviceServer](int signo) {
                deviceServer.reset(); // triggers the destructor
            });

            // Start the device server
            deviceServer->finalizeInternalInitialization();

            t.join(); // Blocking central event loop
        }

        std::cout << "\n" << argv[0] << " is done!\n" << std::endl;
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
