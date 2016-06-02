""" This is a test device, written in the bound API, for the
API cross test
"""

from karabo.bound import (
    AMPERE, Configurator, Hash, INT32_ELEMENT, MILLI, KARABO_CLASSINFO,
    PythonDevice, SLOT_ELEMENT
)


@KARABO_CLASSINFO("TestDevice", "1.5")
class TestDevice(PythonDevice):
    @staticmethod
    def expectedParameters(expected):
        (
            INT32_ELEMENT(expected).key("a")
            .unit(AMPERE)
            .metricPrefix(MILLI)
            .expertAccess()
            .assignmentOptional()
            .defaultValue(55)
            .commit(),

            SLOT_ELEMENT(expected).key("setA")
            .commit()
        )

    def __init__(self, configuration):
        super().__init__(configuration)
        self.registerInitialFunction(self.initialize)
        self.registerSlot(self.setA)

    def initialize(self):
        pass

    def setA(self):
        self.set("a", 33)


if __name__ == "__main__":
    # most of what's happening here is magic. Maybe some wizard
    # (SE?) should have a look at it and try to simplify it?
    config = Hash()
    config.set("appenders[0].Ostream.layout.Pattern.format", "%p %c: %m%n")
    config.set("appenders[1].RollingFile.layout.Pattern.format", "blua")
    device = Configurator(PythonDevice).create(
        "TestDevice", Hash("_deviceId_", "testDevice", "Logger", config))
    device.run()
