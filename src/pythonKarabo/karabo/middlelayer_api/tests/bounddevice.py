""" This is a test device, written in the bound API, for the
API cross test
"""

from karabo.bound import (
    AMPERE, Configurator, Hash, DOUBLE_ELEMENT, Epochstamp, KARABO_CLASSINFO,
    KILO, METER, MILLI, NODE_ELEMENT, PythonDevice, SLOT_ELEMENT, Timestamp,
    Trainstamp
)


@KARABO_CLASSINFO("TestDevice", "1.5")
class TestDevice(PythonDevice):
    @staticmethod
    def expectedParameters(expected):
        (
            DOUBLE_ELEMENT(expected).key("a")
            .minExc(22).maxExc(33).minInc(11).maxInc(23)
            .displayedName("parameter a")
            .alias("something")
            .description("a's description")
            .allowedStates("some thing")
            .unit(AMPERE)
            .metricPrefix(MILLI)
            .expertAccess()
            .assignmentOptional()
            .defaultValue(22.5)
            .commit(),

            NODE_ELEMENT(expected).key("node")
            .commit(),

            DOUBLE_ELEMENT(expected).key("node.b")
            .unit(METER)
            .metricPrefix(KILO)
            .assignmentOptional()
            .defaultValue(33)
            .commit(),

            SLOT_ELEMENT(expected).key("setA")
            .commit(),

            SLOT_ELEMENT(expected).key("backfire")
            .commit()
        )

    def __init__(self, configuration):
        super().__init__(configuration)
        self.registerInitialFunction(self.initialize)
        self.registerSlot(self.setA)
        self.registerSlot(self.backfire)

    def initialize(self):
        pass

    def setA(self):
        ts = Timestamp(Epochstamp("20090901T135522"), Trainstamp(0))
        self.set("a", 22.7, ts)
        ts = Timestamp(Epochstamp("20160617T135522"), Trainstamp(0))
        self.set("node.b", 100, ts)

    def backfire(self):
        remote = self.remote()
        remote.set("middlelayerDevice", "value", 99)
        remote.execute("middlelayerDevice", "slot")


if __name__ == "__main__":
    # most of what's happening here is magic. Maybe some wizard
    # (SE?) should have a look at it and try to simplify it?
    config = Hash()
    config.set("appenders[0].Ostream.layout.Pattern.format", "%p %c: %m%n")
    config.set("appenders[1].RollingFile.layout.Pattern.format", "blua")
    device = Configurator(PythonDevice).create(
        "TestDevice", Hash("_deviceId_", "boundDevice", "Logger", config))
    device.run()
