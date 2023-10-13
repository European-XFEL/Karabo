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
from time import sleep

from karabo.bound import KARABO_CLASSINFO, PythonDevice


@KARABO_CLASSINFO("RaiseInitializationDevice", "2.0")
class RaiseInitializationDevice(PythonDevice):

    def __init__(self, configuration):
        super().__init__(configuration)
        self.registerInitialFunction(self.initialise)

    def initialise(self):
        # Postpone the exception that will trigger the device to shutdown
        # itself to allow detection of the device in the topology
        sleep(1)
        raise RuntimeError("Stupidly failed in initialise")


@KARABO_CLASSINFO("RaiseOnDunderInitDevice", "2.0")
class RaiseOnDunderInitDevice(PythonDevice):

    def __init__(self, configuration):
        super().__init__(configuration)
        self.registerInitialFunction(self.initialise)
        raise RuntimeError("This device raises on __init__")

    def initialise(self):
        """nothing happens here"""
