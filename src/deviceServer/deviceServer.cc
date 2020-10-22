/**
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "karabo/util/Exception.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/core/Runner.hh"
#include "karabo/log/Logger.hh"

#include <iostream>
#include <future>

using namespace karabo::util;
using namespace karabo::log;
using namespace karabo::net;
using namespace karabo::core;


int main(int argc, const char** argv) {

    try {

        // We always change the directory to $KARABO/var/data
        boost::filesystem::current_path(boost::filesystem::path(Version::getPathToKaraboInstallation() + "/var/data"));

        // It is important to load plugins even before having a device server
        // instance, as this allows the help function to correctly show available
        // devices and enabling the server to autoStart them if needed.
        // Loading plugins means one needs to know the plugin directory, so we
        // parse the command line already here (but silently).
        Hash runnerConfig;
        Runner::parseCommandLine(argc, argv, runnerConfig, true);
        const Hash& pluginConfig = (runnerConfig.has("pluginDirectory")
                                    ? Hash("pluginDirectory", runnerConfig.get<std::string>("pluginDirectory"))
                                    : Hash());
        PluginLoader::create("PluginLoader", pluginConfig)->update();

        // Instantiate device server
        DeviceServer::Pointer deviceServer = Runner::instantiate(argc, argv);
        
        // Empty pointer will be returned in case of "-h" or "--help"
        if (deviceServer) {

            // Handle signals using the event-loop
            EventLoop::setSignalHandler([&deviceServer](int signo) {
                deviceServer.reset(); // triggers the destructor
            });

            // Prepare start of the device server on the event loop
            std::promise<void> prom;
            std::future<void> serverInitialized = prom.get_future();
            // Bind weak_ptr: shared_ptr would keep the server alive when event loop stopped before initializer started.
            DeviceServer::WeakPointer weakServer(deviceServer);
            auto initializer = [weakServer, &prom]() {
                try {
                    DeviceServer::Pointer server(weakServer); // throws if not available
                    server->finalizeInternalInitialization(); // throws if e.g. invalid serverId
                    prom.set_value();
                } catch (std::exception&) {
                    // Stop event loop, otherwise EventLoop::work() blocks and serverInitialised.get() is not reached.
                    EventLoop::stop();
                    prom.set_exception(std::current_exception());
                }
            };

            EventLoop::getIOService().post(initializer);

            // Start central event loop  and block until event loop stopped, usually by a signal
            EventLoop::work();
            serverInitialized.get(); // throws if initializer failed
        }

        Logger::logInfo() << argv[0] << " has exited!\n";
        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        std::string msg(argv[0]);
        (msg += " has exited after catching an exception: ") += e.what();
        Logger::logError() << msg << "\n";
        std::cerr << msg << std::endl; // in case logger could not be established
    }
    // else: Don't care about insane exceptions.

    return EXIT_FAILURE;
}
