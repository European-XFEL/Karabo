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

USING_KARABO_NAMESPACES

int main(int argc, char** argv) { 
       
    try {
        
        DeviceServer::Pointer deviceServer;

        // In case command-line arguments are provided take them for configuration
        if (argc > 1) {

            deviceServer = Runner<DeviceServer>::instantiate(argc, argv);
                    
        } else { // In case NO command-line arguments are provided behave as configured below
            
            Hash config;
            
            // This starts the GuiServer (will complain if already exists, don't bother)
            config.set("autoStart[0]", Hash("GuiServerDevice"));
            
            // Set the logger priority (other options INFO, WARN, ERROR)
            config.set("Logger.priority", "DEBUG");
            
            // Switch of plug-in scanning
            config.set("scanPlugins", false);
            
             // This starts the FileDataLogger
            //config.set("autoStart[1]", Hash("FileDataLogger"));
            
            // Configure the broker hostname
            // config.set("connection.Jms.hostname", "localhost");
            
            // Configure the broker port
            // config.set("connection.Jms.port", 7676);
            
            // Configure the broker destination name (topic name)
            // config.set("connection.Jms.destinationName", "myTestTopic");
            
            // This creates an instance of the device server
            deviceServer = DeviceServer::create("DeviceServer", config);
        }

        // Run the server, function blocks here
        if (deviceServer) deviceServer->run();

    } catch (const karabo::util::Exception& e) {
        std::cerr << e;
    } catch (...) {
        KARABO_RETHROW;
    }
    return (EXIT_SUCCESS);
}

