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
""" This is a test device, written in the bound API, for the
API cross test
"""

import numpy

from karabo.bound import (
    BOOL_ELEMENT, DOUBLE_ELEMENT, IMAGEDATA_ELEMENT, INPUT_CHANNEL,
    INT32_ELEMENT, KARABO_CLASSINFO, NDARRAY_ELEMENT, NODE_ELEMENT,
    OUTPUT_CHANNEL, SLOT_ELEMENT, STRING_ELEMENT, TABLE_ELEMENT,
    VECTOR_STRING_ELEMENT, AlarmCondition, ChannelMetaData, Encoding,
    Epochstamp, Hash, ImageData, MetricPrefix, PythonDevice, Schema, State,
    TimeId, Timestamp, Types, Unit)


@KARABO_CLASSINFO("TestDevice", "1.5")
class TestDevice(PythonDevice):
    @staticmethod
    def expectedParameters(expected):
        tableSchema = Schema()
        (
            DOUBLE_ELEMENT(tableSchema).key("e")
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.KILO)
            .assignmentOptional()
            .defaultValue(0.0)
            .commit(),

            STRING_ELEMENT(tableSchema).key("s")
            .assignmentOptional()
            .defaultValue("")
            .commit()
        )
        pipeSchema = Schema()
        (
            # the default type of a NDARRAY elements and of IMAGEDATA
            # are now UNKNOWN. Here are left unspecified to test that
            # the integration tests do not require this definition.
            NDARRAY_ELEMENT(pipeSchema).key("ndarray")
            .dtype(Types.FLOAT)
            .shape("10")
            .commit(),

            IMAGEDATA_ELEMENT(pipeSchema).key("image")
            .setDimensions("50,50")
            .setType(Types.UINT16)
            .setEncoding(Encoding.GRAY)
            .commit(),

            # note, this will never be set by `send`
            # it is to prove that a raw channel input can manage
            # data mismatching the schema and an normal channel
            # does not fail in case of mismatching schema.
            DOUBLE_ELEMENT(pipeSchema).key("e")
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.KILO)
            .assignmentOptional()
            .noDefaultValue()
            .commit(),

            STRING_ELEMENT(pipeSchema).key("s")
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
            .unit(Unit.AMPERE)
            .metricPrefix(MetricPrefix.MILLI)
            .expertAccess()
            .assignmentOptional()
            .defaultValue(22.5)
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(expected).key("eosReceived")
            .displayedName("EOS Received")
            .assignmentOptional()
            .defaultValue(False)
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
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.KILO)
            .options("33,44,55,100")
            .assignmentOptional()
            .defaultValue(33)
            .commit(),

            STRING_ELEMENT(expected).key("middlelayerDevice")
            .assignmentMandatory()
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("maxSizeSchema")
            .readOnly()
            .initialValue(0)
            .commit(),

            SLOT_ELEMENT(expected).key("setA")
            .commit(),

            SLOT_ELEMENT(expected).key("backfire")
            .commit(),

            SLOT_ELEMENT(expected).key("injectSchema")
            .commit(),

            SLOT_ELEMENT(expected).key("send")
            .commit(),

            SLOT_ELEMENT(expected).key("sendMultipleHashes")
            .commit(),

            SLOT_ELEMENT(expected).key("end")
            .commit(),

            SLOT_ELEMENT(expected).key("compareSchema")
            .commit(),

            TABLE_ELEMENT(expected).key("table")
            .displayedName("bla")
            .setNodeSchema(tableSchema)
            .assignmentOptional()
            .defaultValue([Hash("e", 5, "s", "hallo")])
            .reconfigurable()
            .commit(),

            NDARRAY_ELEMENT(expected).key("ndarray")
            .shape("3,2")
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.KILO)
            .dtype("FLOAT")
            .commit(),

            OUTPUT_CHANNEL(expected).key("output1")
            .dataSchema(tableSchema)
            .commit(),

            OUTPUT_CHANNEL(expected).key("output2")
            .dataSchema(pipeSchema)
            .commit(),

            INPUT_CHANNEL(expected).key("input")
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("interfaces")
            .displayedName("Interfaces")
            .assignmentOptional().defaultValue(
                ["Motor", "Camera", "Processor", "DeviceInstantiator"])
            .commit(),

            INPUT_CHANNEL(expected).key("imageInput")
            .commit(),

            STRING_ELEMENT(expected).key("imagePath")
            .displayedName("Image Path")
            .assignmentOptional().defaultValue("data.image")
            .expertAccess()
            .init()
            .commit(),

            STRING_ELEMENT(expected).key("ndarrayPath")
            .displayedName("NDArray Path")
            .assignmentOptional().defaultValue("data.array")
            .expertAccess()
            .init()
            .commit(),

            INT32_ELEMENT(expected).key("imagesReceived")
            .readOnly()
            .initialValue(0)
            .commit(),

            INT32_ELEMENT(expected).key("ndarraysReceived")
            .readOnly()
            .initialValue(0)
            .commit(),

        )

    def __init__(self, configuration):
        super().__init__(configuration)
        self.registerInitialFunction(self.initialize)
        self.registerSlot(self.setA)
        self.registerSlot(self.backfire)
        self.registerSlot(self.injectSchema)
        self.registerSlot(self.send)
        self.registerSlot(self.sendMultipleHashes)
        self.registerSlot(self.end)
        self.registerSlot(self.compareSchema)
        self.KARABO_ON_DATA("input", self.onData)
        self.KARABO_ON_EOS("input", self.onEndOfStream)
        self.KARABO_ON_DATA("imageInput", self.onImageData)
        self.word_no = 1

    def initialize(self):
        # HACK/FIXME:
        # We trigger a temporary (15 s) connection to our counterpart and fill
        # the remote's cache already here in initialisation.
        # Otherwise this will block in the slot calls where it is used - and
        # block forever since one cannot synchronously call back to a slot
        # caller due to thte way ordering per sender is implemented.
        mdlDevice = self.get("middlelayerDevice")
        self.remote().getDeviceSchema(mdlDevice)
        self.remote().get(mdlDevice)

    def setA(self):
        ts = Timestamp(Epochstamp("20090901T135522"), TimeId(0))
        self.set("a", 22.7, ts)
        ts = Timestamp(Epochstamp("20160617T135522"), TimeId(0))
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
            STRING_ELEMENT(schema).key(f"word{self.word_no}")
            .displayedName(f"Word #{self.word_no}")
            .description("The word")
            .assignmentOptional().defaultValue("Hello")
            .reconfigurable()
            .commit(),

            NODE_ELEMENT(schema).key("injectedNode")
            .commit(),

            DOUBLE_ELEMENT(schema)
            .key(f"injectedNode.number{self.word_no}")
            .assignmentOptional()
            .noDefaultValue()
            .commit()
        )
        self.updateSchema(schema)
        self.set(f"injectedNode.number{self.word_no}", self.word_no)
        self.word_no += 1

    def send(self):
        self.writeChannel("output1", Hash("e", 5., "s", "hallo"))
        node = Hash("e", 5., "s", "hallo")
        arr = numpy.full((10), 42., dtype=numpy.float32)
        node.set("ndarray", arr)
        imArr = numpy.full((50, 50), 42, dtype=numpy.uint16)
        node.set("image", ImageData(imArr, encoding=Encoding.GRAY))
        self.writeChannel("output2", node)

    def sendMultipleHashes(self):
        channel = self._sigslot.getOutputChannel("output1")

        for i in range(10):
            sourceName = f"{self.getInstanceId()}:output{i}"
            timestamp = self.getActualTimestamp()
            meta = ChannelMetaData(sourceName, timestamp)
            channel.write(Hash("e", float(i), "s", "hallo"), meta)
        channel.update()

    def end(self):
        self.signalEndOfStream("output1")
        self.signalEndOfStream("output2")

    def onData(self, data, metaData):
        self.set("a", data.get("number"))

    def onEndOfStream(self, channel):
        self.set("eosReceived", True)

    def checkData(self, data, update, dataPrefix, klass):
        prop_path = self[f"{dataPrefix}Path"]
        if data.has(prop_path) and isinstance(data[prop_path], klass):
            count = self[f"{dataPrefix}sReceived"] + 1
            update.set(f"{dataPrefix}sReceived", count)
        else:
            update.set("status", "unexpected data type"
                       f" '{type(data[prop_path])}'"
                       f" instead of '{str(klass)}'"
                       f" for path '{prop_path}'")

    def onImageData(self, data, metaData):
        h = Hash()
        self.checkData(data, h, "image", ImageData)
        self.checkData(data, h, "ndarray", numpy.ndarray)
        self.set(h)

    def compareSchema(self):
        """This function tries to get the maxSize of a middlelayer device
        """
        instance_id = self.get("middlelayerDevice")
        remote = self.remote()
        schema = remote.getDeviceSchema(instance_id)
        max_size = schema.getMaxSize("vectorMaxSize")
        self.set("maxSizeSchema", max_size)
        self.reply(max_size)
