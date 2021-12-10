
from karabo.bound import KARABO_CLASSINFO, PythonDevice


@KARABO_CLASSINFO("RaiseInitializationDevice", "2.0")
class RaiseInitializationDevice(PythonDevice):

    def __init__(self, configuration):
        super().__init__(configuration)
        self.registerInitialFunction(self.initialise)

    def initialise(self):
        raise RuntimeError("Stupidly failed in initialise")
