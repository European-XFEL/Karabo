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

__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$May 24, 2013 11:36:55 AM$"


from karabind import Hash

from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS

cr_cmt = " # Copyright (C) European XFEL GmbH Schenefeld. All rights reserved."


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Runner", "1.0")
class Runner:

    def __init__(self, deviceServer):
        self.deviceServer = deviceServer

    def instantiate(self, argv):
        classId = 'DeviceServer'
        ok, config = self.parseCommandLine(argv)
        if ok:
            if not config.empty():
                if config.has(classId):
                    return self.deviceServer.create(config)
                else:
                    return self.deviceServer.create(classId, config)
            else:
                return self.deviceServer.create(classId, config)

        # Not ok!
        return None

    def parseCommandLine(self, argv):
        if "--help" in argv or "-h" in argv:
            if len(argv) > 2:
                self.print_usage(argv[1], argv[2])
            else:
                self.print_usage(argv[0])
            return False, Hash()

        try:
            params = {k: v for k, v in (a.split("=", 1) for a in argv[1:])}
        except ValueError:
            print("Invalid input format: expected key-value pairs in "
                  "the form 'key=value'.")
            return False, Hash()
        configuration = Hash()
        for k, v in params.items():
            configuration.set(k, v)

        assert "autostart" not in configuration
        return True, configuration

    def print_usage(self, name, key=""):
        print("\n ############################################################"
              "####")
        print(" #                   Karabo Device Server")
        print(" #")
        print(cr_cmt)
        print(" ############################################################"
              "####\n")
        if not key:
            print("Usage: {} <configuration>\n".format(name.split('/')[-1]))
            print("Positional arguments:\n")
            print("<configuration> A set of (hierarchical) <key>=<value> "
                  "pairs")
            print("                Use: --help [key] to list available keys "
                  "or sub-keys, respectively")
            self.deviceServer.getSchema('DeviceServer').help()
        else:
            self.deviceServer.getSchema('DeviceServer').help(key)
        print("")
