# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import sys
from os.path import basename


def runSingleDeviceServer(*pluginNames, serverId=""):
    """ Run an API 1 device server with a single device."""
    from karabo.bound import DeviceServer, Hash

    serverId = serverId or basename(sys.argv[0])
    configuration = Hash(
        "DeviceServer.serverId", serverId,
        "DeviceServer.Logger.priority", "INFO",
        "DeviceServer.pluginNames", ",".join(pluginNames),
    )

    try:
        server = DeviceServer.create(configuration)
        if server:
            server.run()
    except Exception as e:
        print("Exception caught: " + str(e))
