# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.bound import KARABO_CLASSINFO, PythonDevice, launchPythonDevice
from karabo.common.states import State


@KARABO_CLASSINFO("NonSceneProvidingDevice", "2.0")
class NonSceneProvidingDevice(PythonDevice):

    def __init__(self, config):
        super(NonSceneProvidingDevice, self).__init__(config)
        self.registerInitialFunction(self.initialize)

    def initialize(self):
        self.updateState(State.NORMAL)

# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()
