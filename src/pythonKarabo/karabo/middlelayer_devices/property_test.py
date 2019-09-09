from asyncio import sleep

from karabo.middlelayer import (
    AccessLevel, AccessMode, background, Bool, Configurable, DaqDataType,
    Device, Double, Float, Hash, InputChannel, Int32, Int64, NDArray, Node,
    OutputChannel, Overwrite, UInt32, UInt64, Unit, Slot, slot, State, String,
    VectorBool, VectorChar, VectorDouble, VectorFloat, VectorHash, VectorInt32,
    VectorInt64, VectorUInt32, VectorUInt64, VectorString)


class DataNode(Configurable):
    daqDataType = DaqDataType.TRAIN

    counter = UInt64(
        defaultValue=0,
        accessMode=AccessMode.READONLY)


class ChannelNode(Configurable):
    data = Node(DataNode)


class CPPNode(Configurable):
    int32 = Int32(accessMode=AccessMode.READONLY)
    string = String(accessMode=AccessMode.READONLY)
    vecInt64 = VectorInt64(accessMode=AccessMode.READONLY)
    ndarray = NDArray(accessMode=AccessMode.READONLY)


class CPPChannelNode(Configurable):
    node = Node(CPPNode)


class VectorNode(Configurable):
    boolProperty = VectorBool(
        displayedName="Boolean properties",
        description="Vector boolean values",
        defaultValue=[True, False, True, False, True, False],
        minSize=1,
        maxSize=10)

    charProperty = VectorChar(
        displayedName="Char Vector",
        description="Vector char values",
        minSize=1,
        maxSize=10,
        defaultValue="ABCDEF")

    int32Property = VectorInt32(
        displayedName="Int32 Vector (No Limits)",
        description="Vector int32 values",
        defaultValue=[-1, 2, 3])

    uint32Property = VectorUInt32(
        displayedName="UInt32 Vector",
        description="Vector uint32 values",
        defaultValue=[1, 2, 3],
        minSize=1,
        maxSize=10)

    int64Property = VectorInt64(
        displayedName="Int64 Vector (No Limits)",
        description="Vector int64 values",
        defaultValue=[1, 2],
        minSize=1,
        maxSize=10)

    uint64Property = VectorUInt64(
        displayedName="UInt64 Vector",
        description="Vector uint64 values",
        defaultValue=[-10, -10],
        minSize=1,
        maxSize=10)

    floatProperty = VectorFloat(
        displayedName="Float Vector (No Limits)",
        description="Vector float values",
        defaultValue=[7.1, 0.1, 5.3])

    doubleProperty = VectorDouble(
        displayedName="Double Vector",
        description="Vector double values",
        defaultValue=[1.23, 0.4, 1.0],
        minSize=1,
        maxSize=10)

    stringProperty = VectorString(
        displayedName="String Vector",
        description="Vector string values",
        minSize=1,
        maxSize=10,
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


class PropertyTestMDL(Device):
    __version__ = "2.3.0"

    allowedStates = [State.INIT, State.ON, State.ACQUIRING]

    state = Overwrite(
        defaultValue=State.INIT,
        options=allowedStates)

    visibility = Overwrite(
        defaultValue=AccessLevel.ADMIN,
        options=[AccessLevel.ADMIN])

    boolProperty = Bool(
        displayedName="Bool",
        description="a boolean value",
        defaultValue=False)

    boolPropertyReadonly = Bool(
        displayedName="Bool Readonly",
        description="A readonly boolean property",
        defaultValue=True,
        accessMode=AccessMode.READONLY)

    int32Property = Int32(
        displayedName="Int32",
        description="An integer property (32 Bit)",
        defaultValue=2,
        minInc=-20,
        maxInc=20)

    int32PropertyReadonly = Int32(
        displayedName="Int32 (RO)",
        description="A integer property",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    uint32Property = UInt32(
        displayedName="UInt32",
        description="An unsigned integer property (32 Bit)",
        defaultValue=2)

    uint32PropertyReadonly = UInt32(
        displayedName="UInt32 (RO)",
        description="A readonly integer property",
        defaultValue=30,
        accessMode=AccessMode.READONLY)

    int64Property = Int64(
        displayedName="Int64",
        description="An integer 64 Bit property",
        defaultValue=10)

    int64PropertyReadonly = Int64(
        displayedName="Int64 (RO)",
        description="A readonly integer property",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    uint64Property = UInt64(
        displayedName="UInt64",
        description="An unsigned integer property (64 Bit)",
        defaultValue=30)

    uint64PropertyReadonly = UInt64(
        displayedName="UInt64 (RO)",
        description="A readonly integer property (64 Bit)",
        defaultValue=47,
        accessMode=AccessMode.READONLY)

    floatProperty = Float(
        displayedName="Float (Min / Max)",
        description="A float property",
        defaultValue=20.0,
        minExc=-1000.0,
        maxExc=1000.0)

    floatPropertyReadonly = Float(
        displayedName="Float (RO)",
        description="A readonly float property",
        defaultValue=0.0,
        accessMode=AccessMode.READONLY)

    doubleProperty = Double(
        displayedName="Double",
        description="A double property",
        defaultValue=0.1)

    doublePropertyReadonly = Double(
        displayedName="Double (RO)",
        description="A readonly double property",
        defaultValue=3.1415,
        accessMode=AccessMode.READONLY)

    stringProperty = String(
        displayedName="String",
        description="A string property",
        defaultValue="XFEL")

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

    @InputChannel(displayedName="Input", raw=False)
    async def input(self, data, meta):
        print(data, "Data received")
        print(meta, "Meta Received")
        self.packetReceived = self.packetReceived.value + 1

    @input.endOfStream
    async def input(self, channel):
        print("End of Stream handler called by", channel)

    @input.close
    async def input(self, channel):
        print("Close handler called by", channel)

    output = OutputChannel(
        ChannelNode,
        displayedName="Output")

    outputForCPP = OutputChannel(
        CPPChannelNode,
        displayedName="Output for C++ input")

    frequency = Int32(
        displayedName="Frequency",
        unitSymbol=Unit.HERTZ,
        defaultValue=2)

    packetSend = Int32(
        displayedName="Packets Send",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    packetReceived = Int32(
        displayedName="Packets Received",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    async def onInitialization(self):
        self.state = State.ON
        self.packet_number = 0
        self.acquiring = False
        background(self._send_data_action())

    @Slot(displayedName="Start", allowedStates=[State.ON])
    async def start(self):
        self.state = State.ACQUIRING
        self.acquiring = True

    @Slot(displayedName="Stop", allowedStates=[State.ACQUIRING])
    async def stop(self):
        self.state = State.ON
        self.acquiring = False
        self.counter = 0

    @Slot(displayedName="Reset Counters", allowedStates=[State.ON])
    async def resetCounter(self):
        self.packet_number = 0
        self.packetSend = 0
        self.packetReceived = 0

    async def _send_data_action(self):
        while True:
            if self.acquiring:
                output = self.output.schema.data
                output.counter = self.packet_number + 1
                await self.output.writeData()
                output = self.outputForCPP.schema.node
                output.int32 = self.packet_number + 1
                output.string = f'BLA {output.int32}'
                output.vecInt64 = [output.int32] * 4
                await self.outputForCPP.writeData()
                self.packet_number += 1
                self.packetSend = self.packetSend.value + 1

            await sleep(1 / self.frequency.value)

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
