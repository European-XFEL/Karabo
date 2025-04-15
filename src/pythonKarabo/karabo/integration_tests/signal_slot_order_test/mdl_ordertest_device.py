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
from asyncio import wait_for

from karabo.middlelayer import (
    AccessMode, Configurable, Device, Hash, Int32, Node, Overwrite, Signal,
    Slot, State, String, UInt32, VectorInt32, background, slot)


class OrderTestNode(Configurable):

    @Slot(displayedName="Start",
          description="Start test that signals and direct slot calls from "
                      "another instance are received in order. 'Other' "
                      "MdlOrderTestDevice instance is defined via "
                      "'stringProperty' and number of messages via "
                      "'int32Property'. "
                      "Results are stored in properties below.",
          allowedStates=[State.NORMAL])
    async def slotStart(self):
        # Kept here to match C++ PropertyTest
        device = self.get_root()
        device.state = State.STARTING
        background(device.startOrderTest)

    nonConsecutiveCounts = VectorInt32(
        displayedName="Non Consecutive Counts",
        description="All received counts N whose predecessor was not N-1",
        accessMode=AccessMode.READONLY,
        maxSize=1000,
        defaultValue=[])

    receivedCounts = UInt32(
        displayedName="Received Counts",
        description="Number of counts received in test cycle",
        accessMode=AccessMode.READONLY,
        defaultValue=0)


class MdlOrderTestDevice(Device):
    # As long as part of Karabo framework, just inherit __version__ from Device

    def __init__(self, configuration={}):
        super().__init__(configuration)
        self._counts = []

    state = Overwrite(
        defaultValue=State.NORMAL,
        options=[State.STARTED, State.NORMAL, State.STARTING])

    # int32Property needs to match name in C++ PropertyTest
    int32Property = Int32(
        displayedName="Int32",
        description="Number of messages of order test",
        defaultValue=10_000)

    # stringProperty neeeds to match name in C++ PropertyTest
    stringProperty = String(
        displayedName="String",
        description="Id of other device in order test",
        defaultValue="")

    orderTest = Node(
        OrderTestNode,
        displayedName="Order Test")

    signalCount = Signal(Int32())  # for order test

    async def startOrderTest(self):
        other = self.stringProperty
        updates = Hash("stringProperty", self.deviceId,
                       "int32Property",  self.int32Property)
        try:
            await wait_for(
                self.call(other, "slotReconfigure", updates),
                timeout=2)
            await wait_for(
                self.signalSlotable.async_connect(
                    other, ["signalCount"], self.slotCount),
                timeout=2)
        except Exception as e:
            self.log.WARN(f"Failed to start order test: {repr(e)}")
            self.state = State.ERROR
            self.status = "Failed to start order test"

        else:
            if self.status:
                self.status = ""
            self.orderTest.nonConsecutiveCounts = []
            self.orderTest.receivedCounts = 0
            self.state = State.STARTED

            self.callNoWait(other, "slotStartCount")

    async def startCount(self, other, numCounts):
        self.state = State.STARTED

        for i in range(0, numCounts, 2):
            self.callNoWait(other, "slotCount", i)
            self.signalCount(i + 1)

        self.log.INFO(f"Done messaging {other}")
        self.callNoWait(other, "slotCount", -1)  # Tell that loop is done
        self.state = State.NORMAL

    @slot
    async def slotCount(self, count):
        nCounts = len(self._counts)
        if count >= 0:
            self._counts.append(count)
            if nCounts % 1000 == 0:
                # Let others follow our progress
                self.orderTest.receivedCounts = nCounts
        else:
            # Loop of calls and signals is done - publish result
            self.orderTest.receivedCounts = nCounts

            nonConsecutiveCounts = []
            for i in range(self.orderTest.receivedCounts):
                if i == 0 or self._counts[i] != self._counts[i - 1] + 1:
                    if len(nonConsecutiveCounts) < 1000:
                        nonConsecutiveCounts.append(self._counts[i])
                    else:
                        break  # limit output size as defined in schema
            self.orderTest.nonConsecutiveCounts = nonConsecutiveCounts
            self._counts = []  # clear for next round

            log = ("Count summary: At least "
                   f"{len(nonConsecutiveCounts) - 1} "  # -1 for first count
                   f"out of {nCounts} messages out of order!")
            self.log.INFO(log)
            self.state = State.NORMAL

    @slot  # No need to expose to schema
    async def slotStartCount(self):
        self.state = State.STARTING
        other = self.stringProperty
        numCounts = self.int32Property.value

        self.log.INFO(f"Start alternatingly calling '{other}' and emitting "
                      f"'signalCount' {numCounts // 2} times each")
        background(self.startCount, other, numCounts)
