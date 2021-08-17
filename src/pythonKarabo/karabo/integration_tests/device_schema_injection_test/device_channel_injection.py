from karabo.bound import (
    KARABO_CLASSINFO, PythonDevice,
    INPUT_CHANNEL, INT32_ELEMENT, NODE_ELEMENT, OUTPUT_CHANNEL)

from karabo import __version__ as karaboVersion


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

            INT32_ELEMENT(expected).key("count")
            .readOnly().initialValue(0)
            .commit(),
        )

    def __init__(self, config):
        super(DeviceChannelInjection, self).__init__(config)
        # Define the slots
        self.KARABO_SLOT(self.slotAppendSchema)
        self.KARABO_SLOT(self.slotUpdateSchema)
        self.KARABO_SLOT(self.slotWriteOuput)

    def slotAppendSchema(self, schema):
        self.appendSchema(schema)
        if "injectedInput" in schema:
            # Asssign data handler for new input channel
            self.KARABO_ON_DATA("injectedInput", self.onData)

    def slotUpdateSchema(self, schema):
        self.updateSchema(schema)
        if "injectedInput" in schema:
            # Asssign data handler for new input channel
            self.KARABO_ON_DATA("injectedInput", self.onData)

    def slotWriteOuput(self, data):
        self.writeChannel("output", data)

    def onData(self, data, meta):
        if "schema" in data:
            schema = data["schema"]
            self.appendSchema(schema)
            # if "injectedInput" in schema: (and if needed in further tests)
            #    # re-register handler:
            #    self.KARABO_ON_DATA("injectedInput", self.onData)
        self["count"] = self["count"] + 1
