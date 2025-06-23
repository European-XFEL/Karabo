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

    def __init__(self, klass):
        """
        :param klass a python class decorated with KARABO_CLASSINFO
        """
        self.klass = klass

    def instantiate(self, argv):
        """
        Parse the given arguments as configuration and create an object
        of the class passed to the constructor
        """
        ok, config = self.parseCommandLine(argv)
        if ok:
            classId = self.klass.__classid__
            return self.klass.create(classId, config)

        # Not ok!
        return None

    def parseCommandLine(self, argv):
        need_help = False
        if "--help" in argv:
            need_help = True
            argv.remove("--help")
        if "-h" in argv:
            need_help = True
            argv.remove("-h")

        if need_help:
            if len(argv) > 1:
                self.print_usage(argv[0], argv[1:])
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

        return True, configuration

    def print_usage(self, name, keys=[]):
        classId = self.klass.__classid__  # as specified by @KARABO_CLASSINFO
        print("\n ############################################################"
              "####")
        print(f" #                   Karabo {classId}")
        print(" #")
        print(cr_cmt)
        print(" ############################################################"
              "####\n")
        if not keys:
            print("Usage: {} <configuration>\n".format(name.split('/')[-1]))
            print("Positional arguments:\n")
            print("<configuration> A set of (hierarchical) <key>=<value> "
                  "pairs")
            print("                Use: --help [key1 [key2 [...]]] to list "
                  "available\n"
                  "                     keys or sub-keys, respectively")
            self.klass.getSchema(classId).help()
        else:
            for key in keys:
                self.klass.getSchema(classId).help(key)
        print("")
