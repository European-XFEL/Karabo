""" This is a test device, written in the bound API, for the
API cross test
"""

import numpy

from karabo.bound import (
    AMPERE, AlarmCondition, Hash, DOUBLE_ELEMENT, Epochstamp, INPUT_CHANNEL,
    INT32_ELEMENT, VECTOR_STRING_ELEMENT, KARABO_CLASSINFO, KILO, METER,
    MILLI, NDARRAY_ELEMENT, NODE_ELEMENT, OUTPUT_CHANNEL, PythonDevice, Schema,
    SLOT_ELEMENT, State, STRING_ELEMENT, TABLE_ELEMENT, Timestamp, Trainstamp)


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
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("readonly")
            .displayedName("Readonly")
            .description("A readonly parameter")
            .readOnly()
            .initialValue(2)
            .commit(),

            NODE_ELEMENT(expected).key("node")
            .commit(),

            DOUBLE_ELEMENT(expected).key("node.b")
            .unit(METER)
            .metricPrefix(KILO)
            .options("33,44,55,100")
            .assignmentOptional()
            .defaultValue(33)
            .commit(),

            STRING_ELEMENT(expected).key("middlelayerDevice")
            .assignmentMandatory()
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("maxSizeSchema")
            .assignmentOptional()
            .defaultValue(0)
            .commit(),

            SLOT_ELEMENT(expected).key("setA")
            .commit(),

            SLOT_ELEMENT(expected).key("backfire")
            .commit(),

            SLOT_ELEMENT(expected).key("injectSchema")
            .commit(),

            SLOT_ELEMENT(expected).key("send")
            .commit(),

            SLOT_ELEMENT(expected).key("end")
            .commit(),

            SLOT_ELEMENT(expected).key("compareSchema")
            .commit(),

            TABLE_ELEMENT(expected).key("table")
            .displayedName("bla")
            .setNodeSchema(tableSchema)
            .assignmentOptional()
            .defaultValue([Hash("d", 5, "s", "hallo")])
            .reconfigurable()
            .commit(),

            NDARRAY_ELEMENT(expected).key("ndarray")
            .shape("3,2")
            .unit(METER)
            .metricPrefix(KILO)
            .dtype("FLOAT")
            .commit(),

            OUTPUT_CHANNEL(expected).key("output1")
            .dataSchema(tableSchema)
            .commit(),

            OUTPUT_CHANNEL(expected).key("output2")
            .dataSchema(tableSchema)
            .commit(),

            INPUT_CHANNEL(expected).key("input")
            .dataSchema(Schema())
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("interfaces")
            .displayedName("Interfaces")
            .assignmentOptional().defaultValue(
                ["Motor", "Camera", "Processor"])
            .commit(),
        )

    def __init__(self, configuration):
        super().__init__(configuration)
        self.registerInitialFunction(self.initialize)
        self.registerSlot(self.setA)
        self.registerSlot(self.backfire)
        self.registerSlot(self.injectSchema)
        self.registerSlot(self.send)
        self.registerSlot(self.end)
        self.registerSlot(self.compareSchema)
        self.KARABO_ON_DATA("input", self.onData)
        self.KARABO_ON_EOS("input", self.onEndOfStream)
        self.word_no = 1

    def initialize(self):
        # HACK/FIXME:
        # We trigger a temporary (15 s) connection to our counterpart and fill
        # the remote's cache already here in initialisation.
        # Otherwise this will block in the slot calls where it is used - and
        # block forever since one cannot synchronously call back to a slot
        # caller due to thte way ordering per sender is implemented.
        mdlDevice = self.get("middlelayerDevice")
        self.remote().cacheAndGetDeviceSchema(mdlDevice)
        self.remote().cacheAndGetConfiguration(mdlDevice)

    def setA(self):
        ts = Timestamp(Epochstamp("20090901T135522"), Trainstamp(0))
        self.set("a", 22.7, ts)
        ts = Timestamp(Epochstamp("20160617T135522"), Trainstamp(0))
        self.set("node.b", 100, ts)
        self.set("ndarray", numpy.array([[1, 2, 3], [4, 5, 6]]))

    def backfire(self):
        remote = self.remote()
        instance_id = self.get("middlelayerDevice")

        state = remote.get(instance_id, "state")
        # 'isinstance' instead of 'is' allows inheritance in future
        if not isinstance(state, State):
            raise RuntimeError("Middle layer 'state' property is of type",
                               type(state).__name__)
        alarm = remote.get(instance_id, "alarmCondition")
        if not isinstance(alarm, AlarmCondition):
            raise RuntimeError("Middle layer 'alarmCondition' property "
                               "is of type", type(alarm).__name__)
        # FIXME:
        # In principle we should use set and execute here, i.e. no 'NoWait'.
        # But, within a slot call, it does not work to call back synchronously
        # to the caller of the slot: The reply we expect here will not be
        # processed because we block here in backfire and messages are executed
        # in order of arrival per sender...
        # Could be fixed by posting the synchronous calls out of this thread
        # and using an AsyncReply - which does not yet exist for bound Python.
        remote.setNoWait(instance_id, "value", 99)
        remote.executeNoWait(instance_id, "slot")

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

    def send(self):
        self.writeChannel("output1", Hash("e", 5, "s", "hallo"))
        self.writeChannel("output2", Hash("e", 5, "s", "hallo"))

    def end(self):
        self.signalEndOfStream("output1")
        self.signalEndOfStream("output2")

    def onData(self, data, metaData):
        self.set("a", data.get("number"))

    def onEndOfStream(self):
        self.set("a", 0)

    def compareSchema(self):
        """This function tries to get the maxSize of a middlelayer device
        """
        instance_id = self.get("middlelayerDevice")
        remote = self.remote()
        schema = remote.getDeviceSchema(instance_id)
        max_size = schema.getMaxSize("vectorMaxSize")
        self.set("maxSizeSchema", max_size)
