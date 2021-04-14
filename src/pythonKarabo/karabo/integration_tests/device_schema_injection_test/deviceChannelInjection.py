from karabo.bound import (
    KARABO_CLASSINFO, PythonDevice,
    INPUT_CHANNEL, NODE_ELEMENT, OUTPUT_CHANNEL)

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
        )

    def __init__(self, config):
        super(DeviceChannelInjection, self).__init__(config)
        # Define the slots
        self.KARABO_SLOT(self.slotAppendSchema)
        self.KARABO_SLOT(self.slotUpdateSchema)

    def slotAppendSchema(self, schema):
        self.appendSchema(schema)

    def slotUpdateSchema(self, schema):
        self.updateSchema(schema)
