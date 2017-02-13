""" This is a test device, written in the bound API, for the
API cross test
"""
import threading

from karabo.bound import (
    AMPERE, Configurator, Hash, DOUBLE_ELEMENT, Epochstamp, KARABO_CLASSINFO,
    KILO, METER, MILLI, NODE_ELEMENT, PythonDevice, Schema, SLOT_ELEMENT,
    State, STRING_ELEMENT, TABLE_ELEMENT, Timestamp, Trainstamp, EventLoop)


@KARABO_CLASSINFO("TestDevice", "1.5")
class TestDevice(PythonDevice):
    @staticmethod
    def expectedParameters(expected):
        tableSchema = Schema()
        (
            DOUBLE_ELEMENT(tableSchema).key("d")
            .unit(METER)
            .metricPrefix(KILO)
            .assignmentOptional()
            .noDefaultValue()
            .commit(),

            STRING_ELEMENT(tableSchema).key("s")
            .assignmentOptional()
            .noDefaultValue()
            .commit()
        )
        (
            DOUBLE_ELEMENT(expected).key("a")
            .minExc(22).maxExc(33).minInc(11).maxInc(23)
            .displayedName("parameter a")
            .alias("something")
            .tags("bla,blub")
            .description("a's description")
            .allowedStates(State.INIT, State.UNKNOWN)
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
            .commit(),

            SLOT_ELEMENT(expected).key("injectSchema")
            .commit(),

            TABLE_ELEMENT(expected).key("table")
            .displayedName("bla")
            .setNodeSchema(tableSchema)
            .assignmentOptional()
            .defaultValue([Hash("d", 5, "s", "hallo")])
            .commit()
        )

    def __init__(self, configuration):
        super().__init__(configuration)
        self.registerInitialFunction(self.initialize)
        self.registerSlot(self.setA)
        self.registerSlot(self.backfire)
        self.registerSlot(self.injectSchema)
        self.word_no = 1

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

    def injectSchema(self):
        schema = Schema()

        (
            STRING_ELEMENT(schema).key("word{}".format(self.word_no))
            .displayedName("Word #{}".format(self.word_no))
            .description("The word")
            .assignmentOptional().defaultValue("Hello")
            .reconfigurable()
            .commit(),

            NODE_ELEMENT(schema).key("injectedNode")
            .commit(),

            DOUBLE_ELEMENT(schema)
            .key("injectedNode.number{}".format(self.word_no))
            .assignmentOptional()
            .noDefaultValue()
            .commit()
        )
        self.updateSchema(schema)
        self.set("injectedNode.number{}".format(self.word_no), self.word_no)
        self.word_no += 1


if __name__ == "__main__":
    config = Hash("_deviceId_", "boundDevice")
    t = threading.Thread(target=EventLoop.work)
    t.start()
    device = Configurator(PythonDevice).create("TestDevice", config)
    device._finalizeInternalInitialization()
    t.join()
