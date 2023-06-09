# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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
