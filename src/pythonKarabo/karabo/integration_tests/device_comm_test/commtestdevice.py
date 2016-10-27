from karabo.bound import (PythonDevice, Hash, SLOT_ELEMENT, STRING_ELEMENT,
                          KARABO_CLASSINFO, launchPythonDevice)
from karabo.common.states import State


@KARABO_CLASSINFO("CommTestDevice", "2.0")
class CommTestDevice(PythonDevice):

    def expectedParameters(expected):

        (
            SLOT_ELEMENT(expected).key("slotWithoutArguments")
                .commit()
            ,
            SLOT_ELEMENT(expected).key("slotRequestArgs")
                .commit()
            ,
            STRING_ELEMENT(expected).key("remote")
                .assignmentMandatory()
                .commit()
            ,
            STRING_ELEMENT(expected).key("someString")
                .readOnly()
                .commit()
            ,
        )

    def __init__(self, config):
        super(CommTestDevice,self).__init__(config)
        self.KARABO_SLOT(self.slotRequestArgs)
        self.registerSlot(self.slotWithoutArguments)
        self.registerSlot(self.slotWithArguments)
        self.registerSlot(self.slotEmitToSlotWithoutArgs)
        self.registerSlot(self.slotEmitToSlotWithArgs)
        self.registerSlot(self.slotCallSomething)
        self.registerSignal("callSlotWithArgs", str, Hash)
        self.connect("callSlotWithArgs", "slotWithArguments")
        self.registerSignal("callSlotWithoutArgs")
        self.connect("callSlotWithoutArgs", "slotWithoutArguments")
        self.registerInitialFunction(self.initialize)

    def initialize(self):
        self.updateState(State.NORMAL)

    def slotWithoutArguments(self):
        self.set("someString", "slotWithoutArguments was called")

    def slotWithArguments(self, a1, a2):
        self.set("someString", a1)
        self.reply(a2, a1)

    def slotRequestArgs(self):
        ret = self.request(self.get("remote"), "slotWithArguments", "one",
                           Hash("a", 1)).waitForReply(2000)
        self.set("someString", ret[1])

    def slotEmitToSlotWithoutArgs(self):
        self.emit("callSlotWithoutArgs")

    def slotEmitToSlotWithArgs(self):
        self.emit("callSlotWithArgs", "foo", "hoo")

    def slotCallSomething(self):
        self.call(self.get("remote"), "slotWithoutArguments")

# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()