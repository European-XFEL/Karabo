from karabo.bound import (
    KARABO_CLASSINFO, SLOT_ELEMENT, STRING_ELEMENT, VECTOR_INT32_ELEMENT,
    Epochstamp, Hash, PythonDevice, State, launchPythonDevice)


@KARABO_CLASSINFO("CommTestDevice", "2.0")
class CommTestDevice(PythonDevice):

    def expectedParameters(expected):

        (
            SLOT_ELEMENT(expected).key("slotWithoutArguments")
            .commit(),
            SLOT_ELEMENT(expected).key("slotRequestArgs")
            .commit(),
            SLOT_ELEMENT(expected).key("slotRequestArgsAsync")
            .commit(),
            STRING_ELEMENT(expected).key("remote")
            .assignmentMandatory()
            .commit(),
            STRING_ELEMENT(expected).key("someString")
            # As reconfigurable() it will be sent with priority 4
            # (like commands and replies) and thus order is guaranteed by JMS
            # (as readOnly() [that is sent in C++/bound with priority 3]
            #  it failed e.g. in
            #  https://git.xfel.eu/Karabo/Framework/-/jobs/183917)
            .assignmentOptional().defaultValue("")
            .reconfigurable()
            .commit(),
            VECTOR_INT32_ELEMENT(expected).key("vectorInt32")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([1, 2, 3])
            .commit(),
        )

    def __init__(self, config):
        super(CommTestDevice, self).__init__(config)
        self.KARABO_SLOT(self.slotRequestArgs)
        self.KARABO_SLOT(self.slotRequestArgsAsync)
        self.KARABO_SLOT(self.slotRequestStateUpdate)

        self.registerSlot(self.slotWithoutArguments)
        self.registerSlot(self.slotWithArguments)
        self.registerSlot(self.slotEmitToSlotWithoutArgs)
        self.registerSlot(self.slotEmitToSlotWithArgs)
        self.registerSlot(self.slotCallSomething)
        # not for a communication test, but...:
        self.registerSlot(self.slotIdOfEpochstamp)

        self.registerSignal("callSlotWithArgs", str, Hash)
        self.connect("", "callSlotWithArgs", "", "slotWithArguments")
        self.registerSignal("callSlotWithoutArgs")
        self.connect("", "callSlotWithoutArgs", "", "slotWithoutArguments")

        self.registerInitialFunction(self.initialize)

    def initialize(self):
        self.updateState(State.NORMAL)

    def slotRequestStateUpdate(self, state):
        # Note: `updateState` replies slot call with state
        self.updateState(State(state))

    def slotWithoutArguments(self):
        self.set("someString", "slotWithoutArguments was called")

    def slotWithArguments(self, a1, a2):
        self.set("someString", a1)
        self.reply(a2, a1)

    def slotRequestArgs(self):
        ret = self.request(self.get("remote"), "slotWithArguments", "one",
                           Hash("a", 1)).waitForReply(2000)
        self.set("someString", ret[1])

    def slotRequestArgsAsync(self):
        # Same as slotRequestArgs, but using AsyncReply instead of
        # blocking by use of synchronous request
        aReply = self._ss.createAsyncReply()
        def replyer(a1, a2):
            self.set("someString", a2)
            aReply()

        requestor = self.request(self.get("remote"), "slotWithArguments",
                                 "two", Hash("a", 2))
        requestor.receiveAsync2(replyer)

    def slotEmitToSlotWithoutArgs(self):
        self.emit("callSlotWithoutArgs")

    def slotEmitToSlotWithArgs(self):
        self.emit("callSlotWithArgs", "foo", "hoo")

    def slotCallSomething(self):
        self.call(self.get("remote"), "slotWithoutArguments")

    def slotIdOfEpochstamp(self, sec, frac):
        epoch = Epochstamp(sec, frac)
        stamp = self.getTimestamp(epoch)

        self.reply(stamp.getTrainId())


# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()
