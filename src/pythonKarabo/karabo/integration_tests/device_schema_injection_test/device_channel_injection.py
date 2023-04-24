# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo import __version__ as karaboVersion
from karabo.bound import (
    INPUT_CHANNEL, INT32_ELEMENT, KARABO_CLASSINFO, NODE_ELEMENT,
    OUTPUT_CHANNEL, PythonDevice)


@KARABO_CLASSINFO("DeviceChannelInjection", karaboVersion)
class DeviceChannelInjection(PythonDevice):

    def expectedParameters(expected):
        (
            INPUT_CHANNEL(expected).key("input")
            .commit(),

            OUTPUT_CHANNEL(expected).key("output")
            .commit(),

            # Not channel related, but for test that it gets not erased
            NODE_ELEMENT(expected).key("node")
            .commit(),

            INT32_ELEMENT(expected).key("intInOnData")
            .readOnly().initialValue(0)
            .commit(),

            INT32_ELEMENT(expected).key("numCallsOnInput")
            .readOnly().initialValue(0)
            .commit()
        )

    def __init__(self, config):
        super(DeviceChannelInjection, self).__init__(config)
        # Define the slots
        self.KARABO_SLOT(self.slotAppendSchema)
        self.KARABO_SLOT(self.slotUpdateSchema)
        self.KARABO_SLOT(self.slotWriteOutput)
        self.KARABO_SLOT(self.slotSendEos)
        self.KARABO_SLOT(self.slotRegisterOnDataInputEos)

    def slotAppendSchema(self, schema):
        self.appendSchema(schema)

    def slotUpdateSchema(self, schema):
        self.updateSchema(schema)

    def slotWriteOutput(self, data):
        self.writeChannel("output", data)

    def slotSendEos(self, channelNames):
        for channelName in channelNames:
            self.signalEndOfStream(channelName)

    def slotRegisterOnDataInputEos(self, inputChannelName):
        self.KARABO_ON_DATA(inputChannelName, self.onData)
        self.KARABO_ON_INPUT(inputChannelName, self.onInput)
        self.KARABO_ON_EOS(inputChannelName, self.onEos)

    def onData(self, data, meta):
        if "schema" in data:
            schema = data["schema"]
            self.appendSchema(schema)
        received = -1
        if data.has("int"):
            received = data.get("int")
        self["intInOnData"] = received

    def onInput(self, input):
        soFar = self.get("numCallsOnInput")
        self["numCallsOnInput"] = soFar + 1

    def onEos(self, input):
        oldValue = self.get("intInOnData")
        self["intInOnData"] = -oldValue  # just flip sign
