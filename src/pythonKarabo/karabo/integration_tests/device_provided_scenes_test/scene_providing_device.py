# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.bound import (
    KARABO_CLASSINFO, VECTOR_STRING_ELEMENT, PythonDevice, launchPythonDevice)
from karabo.common.states import State


@KARABO_CLASSINFO("SceneProvidingDevice", "2.0")
class SceneProvidingDevice(PythonDevice):

    def expectedParameters(expected):

        (
            VECTOR_STRING_ELEMENT(expected).key("availableScenes")
                .readOnly().initialValue([])
                .commit()
            ,
        )

    def __init__(self, config):
        super().__init__(config)
        self.registerInitialFunction(self.initialize)

    def initialize(self):
        self.updateState(State.NORMAL)

# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()
