from karabo.bound import (
    KARABO_CLASSINFO, VECTOR_STRING_ELEMENT, PythonDevice, launchPythonDevice)
from karabo.common.states import State


@KARABO_CLASSINFO("InvalidImportDevice", "2.0")
class InvalidImportDevice(PythonDevice):

    def expectedParameters(expected):

        import xyz

        (
            VECTOR_STRING_ELEMENT(expected).key("availableScenes")
            .readOnly().initialValue([])
            .commit(),
        )

    def __init__(self, config):
        super(InvalidImportDevice, self).__init__(config)
        self.registerInitialFunction(self.initialize)

    def initialize(self):
        self.updateState(State.NORMAL)
