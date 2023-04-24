# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
