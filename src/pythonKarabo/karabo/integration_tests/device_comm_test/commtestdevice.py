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
from karabo.bound import (
    INT32_ELEMENT, KARABO_CLASSINFO, SLOT_ELEMENT, STRING_ELEMENT,
    VECTOR_INT32_ELEMENT, Epochstamp, Hash, PythonDevice, State, Timestamp,
    launchPythonDevice)


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
            .assignmentOptional().defaultValue("")
            .reconfigurable()
            .commit(),
            INT32_ELEMENT(expected).key("nonReconfigurableProp")
            .assignmentOptional().defaultValue(0)
            .commit(),
            VECTOR_INT32_ELEMENT(expected).key("vectorInt32")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([1, 2, 3])
            .commit(),
        )

    def __init__(self, config):
        super().__init__(config)
        self.KARABO_SLOT(self.slotRequestArgs)
        self.KARABO_SLOT(self.slotRequestArgsAsync)
        self.KARABO_SLOT(self.slotRequestStateUpdate)
        self.KARABO_SLOT(self.slotRequestStateUpdatePlus)

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

    def slotRequestStateUpdatePlus(self, state, other):
        timestamp = None
        if "timestamp" in other:
            attrs = other.getAttributes("timestamp")
            print("Hash received:\n", other)
            timestamp = Timestamp.fromHashAttributes(attrs)
            del other["timestamp"]
        self.updateState(State(state),
                         propertyUpdates=other,
                         timestamp=timestamp)

    def slotWithoutArguments(self):
        self.set("someString", "slotWithoutArguments was called")

    def slotWithArguments(self, a1, a2):
        self.set("someString", a1)
        self.reply(a2, a1)

    def slotRequestArgs(self):
        ret = self.request(self.get("remote"), "slotWithArguments", "one",
                           Hash("a", 1)).waitForReply(5000)
        self.set("someString", ret[1])

    def slotRequestArgsAsync(self):
        # Same as slotRequestArgs, but using AsyncReply instead of
        # blocking by use of synchronous request
        aReply = self._sigslot.createAsyncReply()

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
