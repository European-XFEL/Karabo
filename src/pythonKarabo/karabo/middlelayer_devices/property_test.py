from asyncio import CancelledError, shield

import numpy as np
import time

from karabo.middlelayer import (
    AccessLevel, AccessMode, AlarmCondition,
    background, Bool, Configurable, DaqDataType,
    Device, Double, Float, Hash, InputChannel, Int32, Int64, MetricPrefix,
    NDArray, Node, OutputChannel, Overwrite, RegexString, UInt32, UInt64,
    Unit, sleep, Slot, slot, State, String, VectorBool, VectorChar,
    VectorDouble, VectorFloat, VectorHash, VectorInt32, VectorInt64,
    VectorUInt32, VectorUInt64, VectorString)


VECTOR_MAX_SIZE = 10

NDARRAY_SHAPE = (100, 200)


class DataNode(Configurable):
    daqDataType = DaqDataType.TRAIN

    int32 = Int32(accessMode=AccessMode.READONLY)
    string = String(accessMode=AccessMode.READONLY)
    vecInt64 = VectorInt64(
        accessMode=AccessMode.READONLY,
        minSize=1,
        maxSize=VECTOR_MAX_SIZE)
    ndarray = NDArray(
        accessMode=AccessMode.READONLY,
        dtype=Float,
        shape=NDARRAY_SHAPE)


class ChannelNode(Configurable):
    node = Node(DataNode)


class VectorNode(Configurable):
    boolProperty = VectorBool(
        displayedName="Boolean properties",
        description="Vector boolean values",
        defaultValue=[True, False, True, False, True, False],
        minSize=1,
        maxSize=VECTOR_MAX_SIZE)

    charProperty = VectorChar(
        displayedName="Char Vector",
        description="Vector char values",
        minSize=1,
        maxSize=VECTOR_MAX_SIZE,
        defaultValue=b"ABCDEF")

    int32Property = VectorInt32(
        displayedName="Int32 Vector (No Limits)",
        description="Vector int32 values",
        defaultValue=[-1, 2, 3])

    uint32Property = VectorUInt32(
        displayedName="UInt32 Vector",
        description="Vector uint32 values",
        defaultValue=[1, 2, 3],
        minSize=1,
        maxSize=VECTOR_MAX_SIZE)

    int64Property = VectorInt64(
        displayedName="Int64 Vector (No Limits)",
        description="Vector int64 values",
        defaultValue=[1, 2],
        minSize=1,
        maxSize=VECTOR_MAX_SIZE)

    uint64Property = VectorUInt64(
        displayedName="UInt64 Vector",
        description="Vector uint64 values",
        defaultValue=[-10, -10],
        minSize=1,
        maxSize=VECTOR_MAX_SIZE)

    floatProperty = VectorFloat(
        displayedName="Float Vector (No Limits)",
        description="Vector float values",
        defaultValue=[7.1, 0.1, 5.3])

    doubleProperty = VectorDouble(
        displayedName="Double Vector",
        description="Vector double values",
        defaultValue=[1.23, 0.4, 1.0],
        minSize=1,
        maxSize=VECTOR_MAX_SIZE)

    stringProperty = VectorString(
        displayedName="String Vector",
        description="Vector string values",
        minSize=1,
        maxSize=VECTOR_MAX_SIZE,
        defaultValue=["A", "B", "C"])


class TableRow(Configurable):
    intProperty = UInt32(
        displayedName="Integer Property",
        description="An optional integer property",
        options=[1, 2, 3, 4],
        defaultValue=2)

    stringProperty = String(
        displayedName="String Property",
        description="An option property for strings",
        options=["spam", "eggs"],
        defaultValue="spam")

    boolInit = Bool(
        displayedName="Bool (INIT)",
        defaultValue=True,
        accessMode=AccessMode.INITONLY)

    bool = Bool(
        displayedName="Bool",
        defaultValue=False,
        accessMode=AccessMode.RECONFIGURABLE)

    boolReadOnly = Bool(
        displayedName="Bool (RO)",
        defaultValue=False,
        accessMode=AccessMode.READONLY)

    floatProperty = Float(
        displayedName="Float Property",
        description="A float property",
        defaultValue=-3.0)

    doubleProperty = Double(
        displayedName="Double Property",
        description="A double property",
        defaultValue=14.0)


class CounterNode(Configurable):

    @Slot(displayedName="Increment",
          description="Increment 'Counter read-only'")
    def increment(self):
        self.counterReadOnly = self.counterReadOnly + 1

    @Slot(displayedName="Reset",
          description="Reset 'Counter read-only'")
    def reset(self):
        self.counterReadOnly = 0

    @UInt32(displayedName="Counter",
            description="Values will be transferred to 'Counter read-only'",
            defaultValue=0)
    def counter(self, newValue):
        self.counter = newValue
        self.counterReadOnly = newValue

    counterReadOnly = UInt32(
        displayedName="Counter read-only",
        accessMode=AccessMode.READONLY,
        defaultValue=0,

        warnHigh=1_000_000,
        alarmInfo_warnHigh="Rather high",
        alarmNeedsAck_warnHigh=True,
        alarmHigh=100_000_000,
        alarmInfo_alarmHigh="Too high",
        alarmNeedsAck_alarmHigh=False)  # False for tests


class PropertyTestMDL(Device):

    # As long as part of Karabo framework, just inherit __version__ from Device

    allowedStates = [
        State.INIT, State.STARTED, State.NORMAL, State.STARTING,
        State.STOPPING]

    state = Overwrite(
        defaultValue=State.INIT,
        options=allowedStates)

    visibility = Overwrite(
        defaultValue=AccessLevel.ADMIN,
        options=[AccessLevel.ADMIN])

    @Bool(displayedName="Bool",
          description="a boolean value",
          defaultValue=False)
    def boolProperty(self, newValue):
        self.boolProperty = newValue
        self.boolPropertyReadOnly = newValue

    boolPropertyReadOnly = Bool(
        displayedName="Bool Readonly",
        description="A readonly boolean property",
        defaultValue=True,
        accessMode=AccessMode.READONLY)

    @Int32(displayedName="Int32",
           description="An integer property (32 Bit)",
           defaultValue=2,
           minInc=-20,
           maxInc=20)
    def int32Property(self, newValue):
        self.int32Property = newValue
        self.int32PropertyReadOnly = newValue

    int32PropertyReadOnly = Int32(
        displayedName="Int32 (RO)",
        description="An integer property",
        defaultValue=0,
        accessMode=AccessMode.READONLY,
        alarmLow=-32_000_000,
        alarmInfo_alarmLow="Too low",
        alarmNeedsAck_alarmLow=True,
        warnLow=-10,
        alarmInfo_warnLow="Rather low",
        alarmNeedsAck_warnLow=False)

    @UInt32(displayedName="UInt32",
            description="An unsigned integer property (32 Bit)",
            defaultValue=2)
    def uint32Property(self, newValue):
        self.uint32Property = newValue
        self.uint32PropertyReadOnly = newValue

    uint32PropertyReadOnly = UInt32(
        displayedName="UInt32 (RO)",
        description="A readonly integer property",
        defaultValue=30,
        accessMode=AccessMode.READONLY)

    @Int64(displayedName="Int64",
           description="An integer 64 Bit property",
           defaultValue=10)
    def int64Property(self, newValue):
        self.int64Property = newValue
        self.int64PropertyReadOnly = newValue

    int64PropertyReadOnly = Int64(
        displayedName="Int64 (RO)",
        description="A readonly integer property",
        defaultValue=0,
        accessMode=AccessMode.READONLY,
        # FIXME: values outside int32 (!) range lead to failures!
        alarmLow=-2**31,  # -3_200_000_000,
        alarmInfo_alarmLow="Too low",
        alarmNeedsAck_alarmLow=True,
        warnLow=-3200,
        alarmInfo_warnLow="Rather low",
        alarmNeedsAck_warnLow=False)

    @UInt64(displayedName="UInt64",
            description="An unsigned integer property (64 Bit)",
            defaultValue=30)
    def uint64Property(self, newValue):
        self.uint64Property = newValue
        self.uint64PropertyReadOnly = newValue

    uint64PropertyReadOnly = UInt64(
        displayedName="UInt64 (RO)",
        description="A readonly integer property (64 Bit)",
        defaultValue=47,
        accessMode=AccessMode.READONLY)

    @Float(displayedName="Float (Min / Max)",
           description="A float property",
           defaultValue=20.0,
           minExc=-1000.0,
           maxExc=1000.0)
    def floatProperty(self, newValue):
        self.floatProperty = newValue
        self.floatPropertyReadOnly = newValue

    floatPropertyReadOnly = Float(
        displayedName="Float (RO)",
        description="A readonly float property",
        defaultValue=0.0,
        accessMode=AccessMode.READONLY,
        alarmLow=-1000.0,
        alarmInfo_alarmLow="Too low",
        alarmNeedsAck_alarmLow=True,
        warnLow=-100.0,
        alarmInfo_warnLow="Rather low",
        alarmNeedsAck_warnLow=False,
        warnHigh=100.0,
        alarmInfo_warnHigh="Rather high",
        alarmNeedsAck_warnHigh=False,
        alarmHigh=1000.0,
        alarmInfo_alarmHigh="Too high",
        alarmNeedsAck_alarmHigh=True
    )

    @Double(displayedName="Double",
            description="A double property",
            defaultValue=0.1)
    def doubleProperty(self, newValue):
        self.doubleProperty = newValue
        self.doublePropertyReadOnly = newValue

    doublePropertyReadOnly = Double(
        displayedName="Double (RO)",
        description="A readonly double property for testing alarms",
        defaultValue=3.1415,
        accessMode=AccessMode.READONLY,
        alarmLow=-100.0,
        alarmInfo_alarmLow="Too low",
        alarmNeedsAck_alarmLow=True,
        warnLow=-10.0,
        alarmInfo_warnLow="Rather low",
        alarmNeedsAck_warnLow=False,
        warnHigh=10.0,
        alarmInfo_warnHigh="Rather high",
        alarmNeedsAck_warnHigh=False,
        alarmHigh=100.0,
        alarmInfo_alarmHigh="Too high",
        alarmNeedsAck_alarmHigh=True)

    stringProperty = String(
        displayedName="String",
        description="A string property",
        defaultValue="XFEL")

    regexProperty = RegexString(
        displayedName="Regex String",
        regex="^[A-Za-z0-9_-]{1,20}$",
        description="A regex string property",
        defaultValue="RegexKarabo")

    @Slot(displayedName="Set Alarm",
          description="Set alarm to value of String property - if convertable")
    def setAlarm(self):
        level = AlarmCondition(self.stringProperty)
        self.globalAlarmCondition = level

    @Slot(displayedName="Set Alarm (no ackn.)",
          description="Foreseen for settting an alarm that does not require "
                      "acknowledgment - but not supported!")
    def setNoAckAlarm(self):
        raise NotImplementedError("In middlelayer, global alarm always "
                                  "require acknowledgment.")

    vectors = Node(
        VectorNode,
        displayedName="Vectors",
        description="A node containing vector properties")

    table = VectorHash(
        TableRow,
        displayedName="Table property",
        description="Table containing one node",
        accessMode=AccessMode.RECONFIGURABLE,
        defaultValue=[
            Hash('intProperty', 1, 'stringProperty', 'spam', 'boolInit', True,
                 'bool', False, 'boolReadOnly', False, 'floatProperty', 5.0,
                 'doubleProperty', 27.1),
            Hash('intProperty', 3, 'stringProperty', 'eggs', 'boolInit', False,
                 'bool', True, 'boolReadOnly', True, 'floatProperty', 1.3,
                 'doubleProperty', 22.5)])

    # TODO: Add tableReadOnly = VectorHash(...)
    #       and make table a function changing it

    @InputChannel(displayedName="Input", raw=False)
    async def input(self, data, meta):
        procTimeSecs = self.processingTime.value/1000.
        await sleep(procTimeSecs)

        self.inputCounter = self.inputCounter.value + 1
        self.currentInputId = data.node.int32

        await self._send_data_action()

    @input.endOfStream
    async def input(self, channel):
        self.inputCounterAtEos = self.inputCounter.value
        await self.output.writeEndOfStream()

    @input.close
    async def input(self, channel):
        self.logger.info(f"Close handler called by {channel}")

    processingTime = UInt32(
        displayedName="Processing Time",
        description="Processing time of input channel data handler",
        defaultValue=0,
        unitSymbol=Unit.SECOND,
        metricPrefixSymbol=MetricPrefix.MILLI,
        accessMode=AccessMode.RECONFIGURABLE,
        )
    output = OutputChannel(
        ChannelNode,
        displayedName="Output")

    outputFrequency = Float(
        displayedName="Output frequency",
        description="The target frequency for continously writing to 'Output'",
        unitSymbol=Unit.HERTZ,
        accessMode=AccessMode.RECONFIGURABLE,
        maxInc=1000.,
        minExc=0.0,
        defaultValue=1.)

    outputCounter = Int32(
        displayedName="Output counter",
        description="Last value sent as 'int32' via output channel 'Output'",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    currentInputId = Int32(
        displayedName="Current Input Id",
        description="Last value received as 'int32' on input channel",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    inputCounter = Int32(
        displayedName="Input counter",
        description="Value of 'Input Counter' when endOfStream was received",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    inputCounterAtEos = Int32(
        displayedName="Input counter @ EOS",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    async def onInitialization(self):
        self.state = State.NORMAL
        self.acquiring_task = None

    @Slot(displayedName="Start Writing",
          description="Start writing continously to output channel "
                      "'Output'",
          allowedStates=[State.NORMAL])
    async def startWritingOutput(self):
        self.state = State.STARTING
        self.acquiring_task = background(self._send_data_task)

    @Slot(displayedName="Stop Writing",
          description="Stop writing continously to output channel "
                      "'Output'",
          allowedStates=[State.STARTED])
    async def stopWritingOutput(self):
        self.state = State.STOPPING
        if self.acquiring_task:
            self.acquiring_task.cancel()
        else:
            self.state = State.NORMAL

    @Slot(displayedName="EOS to Output",
          description="Write end-of-stream to output channel 'Output'",
          allowedStates=[State.NORMAL])
    async def eosOutput(self):
        await self.output.writeEndOfStream()

    @Slot(displayedName="Write to Output",
          description="Write once to output channel 'Output'",
          allowedStates=[State.NORMAL])
    async def writeOutput(self):
        await self._send_data_action()

    @Slot(displayedName="Reset Channels",
          description="Reset counters involved in input/output channel "
                      "data flow",
          allowedStates=[State.NORMAL])
    async def resetChannelCounters(self):
        self.inputCounter = 0
        self.inputCounterAtEos = 0
        self.outputCounter = 0
        self.currentInputId = 0

    async def _send_data_action(self):
        outputCounter = self.outputCounter.value + 1
        output = self.output.schema.node
        output.int32 = outputCounter
        output.string = f'{outputCounter}'
        output.vecInt64 = [outputCounter] * VECTOR_MAX_SIZE
        output.ndarray = np.full(NDARRAY_SHAPE,
                                 outputCounter, dtype=np.float32)
        # XXX: implement image data

        async def write_and_count():
            await self.output.writeData()
            self.outputCounter = outputCounter

        # Prohibit task cancelation in between writing to output and
        # counter increase to keep them in sync.
        await shield(write_and_count())

    async def _send_data_task(self):
        self.state = State.STARTED
        while True:
            try:
                before = time.clock()
                await self._send_data_action()
                delay = 1 / self.outputFrequency.value
                # Adjust the delay by what it took to send data
                delay = max(0., delay - (time.clock() - before))
                await sleep(delay)
            except CancelledError:
                self.state = State.NORMAL
                self.acquiring_task = None
                return

    node = Node(
        CounterNode,
        displayedName="Node")

    _available_macros = ['default', 'another', 'the_third']

    availableMacros = VectorString(
        displayedName="Available Macros",
        description="Provides macros from the device",
        accessMode=AccessMode.READONLY,
        defaultValue=_available_macros)

    def generate_macro(self, name):
        if name in self._available_macros[:2]:
            code = "from karabo.middlelayer import Macro, Slot, String\n\n" \
                "class helloWorld(Macro):\n" \
                f"    name = String(defaultValue='{name}')\n" \
                "    @Slot()\n" \
                "    def sayHello(self):\n" \
                "        print(f'Hello, {self.name} macro!')\n"
        else:
            code = "import this\n"
        return code

    @slot
    def requestMacro(self, params):
        payload = Hash('success', True)

        name = params.get('name', default='')
        payload.set('data', self.generate_macro(name))
        return Hash('type', 'deviceMacro', 'origin', self.deviceId, 'payload',
                    payload)

    @String(displayedName="Faulty String",
            description="A string property that could not be set from "
            "the system",
            defaultValue="Karabo")
    def faultyString(self, value):
        if value != "Karabo":
            raise RuntimeError(f"Only 'Karabo' is allowed here not '{value}'")
        else:
            self.faultyString = value
