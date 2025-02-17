# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import time
from asyncio import CancelledError, shield

import numpy as np

from karabo.common.scenemodel.api import (
    BoxLayoutModel, CheckBoxModel, DisplayCommandModel, DisplayLabelModel,
    DisplayListModel, DisplayStateColorModel, DoubleLineEditModel,
    EditableListModel, EditableRegexModel, IntLineEditModel, LabelModel,
    LineEditModel, LineModel, NDArrayGraphModel, SceneModel, TableElementModel,
    VectorGraphModel, write_scene)
from karabo.middlelayer import (
    AccessLevel, AccessMode, AlarmCondition, Bool, Configurable, DaqDataType,
    Device, Double, Float, Hash, InputChannel, Int32, Int64, KaraboError,
    MetricPrefix, NDArray, Node, OutputChannel, Overwrite, RegexString, Slot,
    State, String, UInt32, UInt64, Unit, VectorBool, VectorChar, VectorDouble,
    VectorFloat, VectorHash, VectorInt32, VectorInt64, VectorRegexString,
    VectorString, VectorUInt32, VectorUInt64, background, sleep, slot)

VECTOR_MAX_SIZE = 10

NDARRAY_SHAPE = (100, 200)

DEFAULT_OPT = ["a", "b", "c"]


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
        description="Vector Uint64 values",
        defaultValue=[10, 10],
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
           defaultValue=32_000_000)
    def int32Property(self, newValue):
        self.int32Property = newValue
        self.int32PropertyReadOnly = newValue

    int32PropertyReadOnly = Int32(
        displayedName="Int32 (RO)",
        description="An integer property",
        defaultValue=32_000_000,
        accessMode=AccessMode.READONLY,
        alarmLow=-32_000_000,
        alarmInfo_alarmLow="Too low",
        alarmNeedsAck_alarmLow=True,
        warnLow=-10,
        alarmInfo_warnLow="Rather low",
        alarmNeedsAck_warnLow=False)

    @UInt32(displayedName="UInt32",
            description="An unsigned integer property (32 Bit)",
            defaultValue=32_000_000)
    def uint32Property(self, newValue):
        self.uint32Property = newValue
        self.uint32PropertyReadOnly = newValue

    uint32PropertyReadOnly = UInt32(
        displayedName="UInt32 (RO)",
        description="A readonly integer property",
        defaultValue=32_000_000,
        warnHigh=32_000_001,
        alarmNeedsAck_warnHigh=False,
        alarmInfo_warnHigh="Rather high",
        alarmHigh=64_000_000,
        alarmNeedsAck_alarmHigh=True,
        alarmInfo_alarmHigh="Too high",
        accessMode=AccessMode.READONLY)

    @Int64(displayedName="Int64",
           description="An integer 64 Bit property",
           defaultValue=3_200_000_000)
    def int64Property(self, newValue):
        self.int64Property = newValue
        self.int64PropertyReadOnly = newValue

    int64PropertyReadOnly = Int64(
        displayedName="Int64 (RO)",
        description="A readonly integer property",
        defaultValue=3_200_000_000,
        accessMode=AccessMode.READONLY,
        alarmLow=-3_200_000_000,
        alarmInfo_alarmLow="Too low",
        alarmNeedsAck_alarmLow=True,
        warnLow=-3200,
        alarmInfo_warnLow="Rather low",
        alarmNeedsAck_warnLow=False)

    @UInt64(displayedName="UInt64",
            description="An unsigned integer property (64 Bit)",
            defaultValue=3_200_000_000)
    def uint64Property(self, newValue):
        self.uint64Property = newValue
        self.uint64PropertyReadOnly = newValue

    uint64PropertyReadOnly = UInt64(
        displayedName="UInt64 (RO)",
        description="A readonly integer property (64 Bit)",
        defaultValue=3_200_000_000,
        accessMode=AccessMode.READONLY,
        warnHigh=3_200_000_001,
        alarmInfo_warnHigh="Rather High",
        alarmNeedsAck_warnHigh=False,
        alarmHigh=6_400_000_000,
        alarmInfo_alarmHigh="Too high",
        alarmNeedsAck_alarmHigh=True)

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
        regex=r"^[A-Za-z0-9_\-]{1,20}$",
        description="A regex string property",
        defaultValue="RegexKarabo")

    vectorRegexProperty = VectorRegexString(
        displayedName="Vector Regex String",
        regex=r"^[A-Za-z0-9_\-]{1,20}$",
        description="A vector regex string property",
        defaultValue=["VectorRegexKarabo"])

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
            Hash('intProperty', 1, 'stringProperty', 'spam',
                 'bool', False, 'boolReadOnly', False, 'floatProperty', 5.0,
                 'doubleProperty', 27.1),
            Hash('intProperty', 3, 'stringProperty', 'eggs',
                 'bool', True, 'boolReadOnly', True, 'floatProperty', 1.3,
                 'doubleProperty', 22.5)])

    # TODO: Add tableReadOnly = VectorHash(...)
    #       and make table a function changing it

    @InputChannel(displayedName="Input", raw=False)
    async def input(self, data, meta):
        procTimeSecs = self.processingTime.value / 1000.
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
        requiredAccessLevel=AccessLevel.ADMIN,
    )
    output = OutputChannel(
        ChannelNode,
        displayedName="Output")

    outputFrequency = Float(
        displayedName="Output frequency",
        description="The target frequency for continously writing to 'Output'",
        unitSymbol=Unit.HERTZ,
        accessMode=AccessMode.RECONFIGURABLE,
        requiredAccessLevel=AccessLevel.ADMIN,
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

    availableScenes = VectorString(
        displayedName="Available Scenes",
        displayType="Scenes",
        description="Provides a scene for the Configuration Manager.",
        accessMode=AccessMode.READONLY,
        defaultValue=['scene'])

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
                before = time.time()
                await self._send_data_action()
                delay = 1 / self.outputFrequency.value
                # Adjust the delay by what it took to send data
                delay = max(0., delay - (time.time() - before))
                # Cast since with numpy 2.0.1 delay is np.float32 and that
                # causes extra delay (with numpy 2.0.1).
                await sleep(float(delay))
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
    def requestScene(self, params):
        """Fulfill a scene request from another device.

        :param params: A `Hash` containing the method parameters
        """
        payload = Hash('success', False)
        name = params.get('name', default='scene')
        if name == 'scene':
            payload.set('success', True)
            payload.set('name', name)
            payload.set('data', get_scene(self.deviceId))

        return Hash('type', 'deviceScene',
                    'origin', self.deviceId,
                    'payload', payload)

    @slot
    def requestMacro(self, params):
        payload = Hash('success', True)

        name = params.get('name', default='')
        payload.set('data', self.generate_macro(name))
        return Hash('type', 'deviceMacro', 'origin', self.deviceId, 'payload',
                    payload)

    faultyError = String(
        displayedName="Faulty Error",
        description="Configure the type of exception to be thrown",
        defaultValue="KaraboError",
        options=["KaraboError", "RuntimeError"]
    )

    @String(displayedName="Faulty String",
            description="A string property that could not be set from "
                        "the system",
            defaultValue="Karabo")
    def faultyString(self, value):
        if value != "Karabo":
            if self.faultyError == "RuntimeError":
                exception = RuntimeError
            else:
                exception = KaraboError
            raise exception(f"Only 'Karabo' is allowed here not '{value}'")
        else:
            self.faultyString = value

    @Slot(displayedName="Faulty Slot",
          description="A slot that throws an error",
          defaultValue="Karabo")
    def faultySlot(self):
        if self.faultyError == "RuntimeError":
            exception = RuntimeError
        else:
            exception = KaraboError
        raise exception("Faulty Slot cannot be executed")

    @VectorString(displayedName="Options for optString",
                  minSize=1,
                  defaultValue=DEFAULT_OPT)
    async def optionsForOptString(self, value):
        self.optionsForOptString = value
        background(self._injectStringOpt())

    async def _injectStringOpt(self):
        self.__class__.optString = Overwrite(
            options=self.optionsForOptString.value,
            defaultValue=self.optionsForOptString.value[0]
        )
        await self.publishInjectedParameters()

    optString = String(displayedName="String with Options",
                       defaultValue=DEFAULT_OPT[0],
                       options=DEFAULT_OPT)


def get_scene(deviceId):
    scene0 = DisplayStateColorModel(
        height=21.0, keys=[f'{deviceId}.state'],
        parent_component='DisplayComponent', show_string=True, width=231.0,
        x=70.0, y=40.0)
    scene10 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=21.0, parent_component='DisplayComponent', text='Host',
        width=35.0, x=310.0, y=40.0)
    scene11 = DisplayLabelModel(
        font_size=10, height=21.0, keys=[f'{deviceId}.hostName'],
        parent_component='DisplayComponent', width=166.0, x=345.0, y=40.0)
    scene1 = BoxLayoutModel(
        height=21.0, width=201.0, x=310.0, y=40.0, children=[scene10, scene11])
    scene2 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=21.0, parent_component='DisplayComponent', text='DeviceID',
        width=57.0, x=10.0, y=10.0)
    scene3 = DisplayLabelModel(
        font_size=10, height=21.0, keys=[f'{deviceId}.deviceId'],
        parent_component='DisplayComponent', width=441.0, x=70.0, y=10.0)
    scene400 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='Bool Readonly',
        width=88.0, x=20.0, y=90.0)
    scene401 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='Int32 (RO)',
        width=88.0, x=20.0, y=130.0)
    scene402 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='UInt32 (RO)',
        width=88.0, x=20.0, y=170.0)
    scene403 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=41.0, parent_component='DisplayComponent', text='Int64 (RO)',
        width=88.0, x=20.0, y=210.0)
    scene404 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='Float (RO)',
        width=88.0, x=20.0, y=251.0)
    scene405 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='Double (RO)',
        width=88.0, x=20.0, y=291.0)
    scene40 = BoxLayoutModel(
        direction=2, height=241.0, width=88.0, x=20.0, y=90.0,
        children=[scene400, scene401, scene402, scene403, scene404, scene405])
    scene410 = CheckBoxModel(
        height=40.0, keys=[f'{deviceId}.boolPropertyReadOnly'],
        parent_component='DisplayComponent', width=158.0, x=108.0, y=90.0)
    scene411 = DisplayLabelModel(
        font_size=10, height=40.0, keys=[f'{deviceId}.int32PropertyReadOnly'],
        parent_component='DisplayComponent', width=158.0, x=108.0, y=130.0)
    scene412 = DisplayLabelModel(
        font_size=10, height=40.0, keys=[f'{deviceId}.uint32PropertyReadOnly'],
        parent_component='DisplayComponent', width=158.0, x=108.0, y=170.0)
    scene413 = DisplayLabelModel(
        font_size=10, height=41.0, keys=[f'{deviceId}.int64PropertyReadOnly'],
        parent_component='DisplayComponent', width=158.0, x=108.0, y=210.0)
    scene414 = DisplayLabelModel(
        font_size=10, height=40.0, keys=[f'{deviceId}.floatPropertyReadOnly'],
        parent_component='DisplayComponent', width=158.0, x=108.0, y=251.0)
    scene415 = DisplayLabelModel(
        font_size=10, height=40.0, keys=[f'{deviceId}.doublePropertyReadOnly'],
        parent_component='DisplayComponent', width=158.0, x=108.0, y=291.0)
    scene41 = BoxLayoutModel(
        direction=2, height=241.0, width=158.0, x=108.0, y=90.0,
        children=[scene410, scene411, scene412, scene413, scene414, scene415])
    scene420 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='Bool',
        width=99.0, x=266.0, y=90.0)
    scene421 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='Int32',
        width=99.0, x=266.0, y=130.0)
    scene422 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='UInt32',
        width=99.0, x=266.0, y=170.0)
    scene423 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=41.0, parent_component='DisplayComponent', text='Int64',
        width=99.0, x=266.0, y=210.0)
    scene424 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent',
        text='Float (Min / Max)', width=99.0, x=266.0, y=251.0)
    scene425 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='Double',
        width=99.0, x=266.0, y=291.0)
    scene42 = BoxLayoutModel(
        direction=2, height=241.0, width=99.0, x=266.0, y=90.0,
        children=[scene420, scene421, scene422, scene423, scene424, scene425])
    scene430 = CheckBoxModel(
        height=40.0, keys=[f'{deviceId}.boolProperty'],
        parent_component='DisplayComponent', width=158.0, x=365.0, y=90.0)
    scene431 = DisplayLabelModel(
        font_size=10, height=40.0, keys=[f'{deviceId}.int32Property'],
        parent_component='DisplayComponent', width=158.0, x=365.0, y=130.0)
    scene432 = DisplayLabelModel(
        font_size=10, height=40.0, keys=[f'{deviceId}.uint32Property'],
        parent_component='DisplayComponent', width=158.0, x=365.0, y=170.0)
    scene433 = DisplayLabelModel(
        font_size=10, height=41.0, keys=[f'{deviceId}.int64Property'],
        parent_component='DisplayComponent', width=158.0, x=365.0, y=210.0)
    scene434 = DisplayLabelModel(
        font_size=10, height=40.0, keys=[f'{deviceId}.floatProperty'],
        parent_component='DisplayComponent', width=158.0, x=365.0, y=251.0)
    scene435 = DisplayLabelModel(
        font_size=10, height=40.0, keys=[f'{deviceId}.doubleProperty'],
        parent_component='DisplayComponent', width=158.0, x=365.0, y=291.0)
    scene43 = BoxLayoutModel(
        direction=2, height=241.0, width=158.0, x=365.0, y=90.0,
        children=[scene430, scene431, scene432, scene433, scene434, scene435])
    scene440 = CheckBoxModel(
        height=40.0, keys=[f'{deviceId}.boolProperty'],
        klass='EditableCheckBox',
        parent_component='EditableApplyLaterComponent', width=158.0, x=523.0,
        y=90.0)
    scene441 = IntLineEditModel(
        height=40.0, keys=[f'{deviceId}.int32Property'],
        parent_component='EditableApplyLaterComponent', width=158.0, x=523.0,
        y=130.0)
    scene442 = IntLineEditModel(
        height=40.0, keys=[f'{deviceId}.uint32Property'],
        parent_component='EditableApplyLaterComponent', width=158.0, x=523.0,
        y=170.0)
    scene443 = IntLineEditModel(
        height=41.0, keys=[f'{deviceId}.int64Property'],
        parent_component='EditableApplyLaterComponent', width=158.0, x=523.0,
        y=210.0)
    scene444 = DoubleLineEditModel(
        height=40.0, keys=[f'{deviceId}.floatProperty'],
        parent_component='EditableApplyLaterComponent', width=158.0, x=523.0,
        y=251.0)
    scene445 = DoubleLineEditModel(
        height=40.0, keys=[f'{deviceId}.doubleProperty'],
        parent_component='EditableApplyLaterComponent', width=158.0, x=523.0,
        y=291.0)
    scene44 = BoxLayoutModel(
        direction=2, height=241.0, width=158.0, x=523.0, y=90.0,
        children=[scene440, scene441, scene442, scene443, scene444, scene445])
    scene4 = BoxLayoutModel(
        height=241.0, width=661.0, x=20.0, y=90.0,
        children=[scene40, scene41, scene42, scene43, scene44])
    scene500 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent', text='String',
        width=114.0, x=10.0, y=360.0)
    scene501 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=41.0, parent_component='DisplayComponent', text='Regex String',
        width=114.0, x=10.0, y=400.0)
    scene502 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=40.0, parent_component='DisplayComponent',
        text='Vector Regex String', width=114.0, x=10.0, y=441.0)
    scene50 = BoxLayoutModel(
        direction=2, height=121.0, width=114.0, x=10.0, y=360.0,
        children=[scene500, scene501, scene502])
    scene510 = DisplayLabelModel(
        font_size=10, height=40.0, keys=[f'{deviceId}.stringProperty'],
        parent_component='DisplayComponent', width=274.0, x=124.0, y=360.0)
    scene511 = DisplayLabelModel(
        font_size=10, height=41.0, keys=[f'{deviceId}.regexProperty'],
        parent_component='DisplayComponent', width=274.0, x=124.0, y=400.0)
    scene512 = DisplayListModel(
        height=40.0, keys=[f'{deviceId}.vectorRegexProperty'],
        parent_component='DisplayComponent', width=274.0, x=124.0, y=441.0)
    scene51 = BoxLayoutModel(
        direction=2, height=121.0, width=274.0, x=124.0, y=360.0,
        children=[scene510, scene511, scene512])
    scene520 = LineEditModel(
        height=40.0, keys=[f'{deviceId}.stringProperty'],
        klass='EditableLineEdit',
        parent_component='EditableApplyLaterComponent', width=273.0, x=398.0,
        y=360.0)
    scene521 = EditableRegexModel(
        height=41.0, keys=[f'{deviceId}.regexProperty'],
        parent_component='EditableApplyLaterComponent', width=273.0, x=398.0,
        y=400.0)
    scene522 = EditableListModel(
        height=40.0, keys=[f'{deviceId}.vectorRegexProperty'],
        parent_component='EditableApplyLaterComponent', width=273.0, x=398.0,
        y=441.0)
    scene52 = BoxLayoutModel(
        direction=2, height=121.0, width=273.0, x=398.0, y=360.0,
        children=[scene520, scene521, scene522])
    scene5 = BoxLayoutModel(
        height=121.0, width=661.0, x=10.0, y=360.0,
        children=[scene50, scene51, scene52])
    scene60 = TableElementModel(
        height=194.0, keys=[f'{deviceId}.table'],
        parent_component='DisplayComponent', width=366.0, x=20.0, y=510.0)
    scene61 = TableElementModel(
        height=194.0, keys=[f'{deviceId}.table'], klass='EditableTableElement',
        parent_component='EditableApplyLaterComponent', width=365.0, x=386.0,
        y=510.0)
    scene6 = BoxLayoutModel(
        height=194.0, width=731.0, x=20.0, y=510.0,
        children=[scene60, scene61])
    scene7 = LineModel(
        stroke='#000000', x1=10.0, x2=760.0, y1=490.0, y2=490.0)
    scene8 = LineModel(
        stroke='#000000', x1=10.0, x2=760.0, y1=80.0, y2=80.0)
    scene9 = LineModel(
        stroke='#000000', x1=10.0, x2=770.0, y1=350.0, y2=350.0)
    scene10 = LineModel(
        stroke='#000000', x1=800.0, x2=800.0, y1=40.0, y2=690.0)
    scene11 = DoubleLineEditModel(
        height=27.0,
        keys=[f'{deviceId}.outputFrequency'],
        parent_component='EditableApplyLaterComponent',
        width=121.0, x=1050.0, y=90.0)
    scene12 = LabelModel(
        font='Source Sans Pro,12,-1,5,50,0,1,0,0,0,Normal', height=20.0,
        parent_component='DisplayComponent', text='Output Channel',
        width=220.0, x=820.0, y=40.0)
    scene13 = LineModel(
        stroke='#000000', x1=810.0, x2=1330.0, y1=80.0, y2=80.0)
    scene140 = DisplayLabelModel(
        font_size=10, height=27.0, keys=[f'{deviceId}.outputFrequency'],
        parent_component='DisplayComponent', width=99.0, x=942.0, y=90.0)
    scene141 = DisplayLabelModel(
        font_size=10, height=27.0, keys=[f'{deviceId}.outputCounter'],
        parent_component='DisplayComponent', width=99.0, x=942.0, y=117.0)
    scene142 = DisplayLabelModel(
        font_size=10, height=27.0, keys=[f'{deviceId}.currentInputId'],
        parent_component='DisplayComponent', width=99.0, x=942.0, y=144.0)
    scene143 = DisplayLabelModel(
        font_size=10, height=27.0, keys=[f'{deviceId}.inputCounter'],
        parent_component='DisplayComponent', width=99.0, x=942.0, y=171.0)
    scene144 = DisplayLabelModel(
        font_size=10, height=27.0, keys=[f'{deviceId}.inputCounterAtEos'],
        parent_component='DisplayComponent', width=99.0, x=942.0, y=198.0)
    scene14 = BoxLayoutModel(
        direction=2, height=135.0, width=99.0, x=942.0, y=90.0,
        children=[scene140, scene141, scene142, scene143, scene144])
    scene150 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=27.0, parent_component='DisplayComponent',
        text='Output frequency', width=122.0, x=820.0, y=90.0)
    scene151 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=27.0, parent_component='DisplayComponent',
        text='Output counter', width=122.0, x=820.0, y=117.0)
    scene152 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=27.0, parent_component='DisplayComponent',
        text='Current Input Id', width=122.0, x=820.0, y=144.0)
    scene153 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=27.0, parent_component='DisplayComponent', text='Input counter',
        width=122.0, x=820.0, y=171.0)
    scene154 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=27.0, parent_component='DisplayComponent',
        text='Input counter @ EOS', width=122.0, x=820.0, y=198.0)
    scene15 = BoxLayoutModel(
        direction=2, height=135.0, width=122.0, x=820.0, y=90.0,
        children=[scene150, scene151, scene152, scene153, scene154])
    scene16 = NDArrayGraphModel(
        height=191.0, keys=[f'{deviceId}.output.schema.node.ndarray'],
        parent_component='DisplayComponent', width=521.0, x=820.0, y=560.0)
    scene17 = VectorGraphModel(
        height=191.0, keys=[f'{deviceId}.output.schema.node.vecInt64'],
        parent_component='DisplayComponent', width=531.0, x=810.0, y=340.0)
    scene18 = DisplayCommandModel(
        height=31.0, keys=[f'{deviceId}.startWritingOutput'],
        parent_component='DisplayComponent', width=111.0, x=1060.0, y=140.0)
    scene19 = DisplayCommandModel(
        height=34.0, keys=[f'{deviceId}.stopWritingOutput'],
        parent_component='DisplayComponent', width=111.0, x=1180.0, y=140.0)
    scene20 = DisplayCommandModel(
        height=31.0, keys=[f'{deviceId}.eosOutput'],
        parent_component='DisplayComponent', width=111.0, x=1060.0, y=180.0)
    scene21 = DisplayCommandModel(
        height=31.0, keys=[f'{deviceId}.writeOutput'],
        parent_component='DisplayComponent', width=111.0, x=1180.0, y=180.0)
    scene22 = DisplayCommandModel(
        height=31.0, keys=[f'{deviceId}.resetChannelCounters'],
        parent_component='DisplayComponent', width=111.0, x=1060.0, y=220.0)
    scene2300 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=21.0, parent_component='DisplayComponent', text='int32',
        width=41.0, x=820.0, y=260.0)
    scene2301 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0', foreground='#000000',
        height=21.0, parent_component='DisplayComponent', text='string',
        width=41.0, x=820.0, y=281.0)
    scene230 = BoxLayoutModel(
        direction=2, height=42.0, width=41.0, x=820.0, y=260.0,
        children=[scene2300, scene2301])
    scene2310 = DisplayLabelModel(
        font_size=10, height=21.0,
        keys=[f'{deviceId}.output.schema.node.int32'],
        parent_component='DisplayComponent', width=170.0, x=861.0, y=260.0)
    scene2311 = DisplayLabelModel(
        font_size=10, height=21.0,
        keys=[f'{deviceId}.output.schema.node.string'],
        parent_component='DisplayComponent', width=170.0, x=861.0, y=281.0)
    scene231 = BoxLayoutModel(
        direction=2, height=42.0, width=170.0, x=861.0, y=260.0,
        children=[scene2310, scene2311])
    scene23 = BoxLayoutModel(
        height=42.0, width=211.0, x=820.0, y=260.0,
        children=[scene230, scene231])
    scene24 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0,Normal', height=18.0,
        parent_component='DisplayComponent', text='NDArray', width=111.0,
        x=810.0, y=540.0)
    scene25 = LabelModel(
        font='Source Sans Pro,10,-1,5,50,0,0,0,0,0,Normal', height=18.0,
        parent_component='DisplayComponent', text='VectorInt64', width=111.0,
        x=810.0, y=310.0)
    scene = SceneModel(
        height=766.0, width=1355.0,
        children=[scene0, scene1, scene2, scene3, scene4, scene5, scene6,
                  scene7, scene8, scene9, scene10, scene11, scene12, scene13,
                  scene14, scene15, scene16, scene17, scene18, scene19,
                  scene20, scene21, scene22, scene23, scene24, scene25])
    return write_scene(scene)
