#############################################################################
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
#############################################################################

from karabo.bound import (
    INT32_ELEMENT, KARABO_CLASSINFO, NODE_ELEMENT, OVERWRITE_ELEMENT,
    SLOT_ELEMENT, STRING_ELEMENT, UINT32_ELEMENT, VECTOR_INT32_ELEMENT,
    EventLoop, Hash, PythonDevice, State)


@KARABO_CLASSINFO("BoundOrderTestDevice", "2.0")
class BoundOrderTestDevice(PythonDevice):

    @staticmethod
    def expectedParameters(expected):
        (
            OVERWRITE_ELEMENT(expected).key("state")
            .setNewOptions(State.NORMAL, State.STARTING, State.STARTED,
                           State.ERROR)
            .setNewDefaultValue(State.NORMAL)
            .commit(),

            # int32Property needs to match name in C++ PropertyTest
            INT32_ELEMENT(expected).key("int32Property")
            .displayedName("Int32")
            .description("Number of messages of order test")
            .reconfigurable()
            .allowedStates(State.NORMAL)
            .assignmentOptional().defaultValue(10_000)
            .commit(),

            STRING_ELEMENT(expected).key("stringProperty")
            .displayedName("String")
            .description("Id of other device in order test")
            .reconfigurable()
            .allowedStates(State.NORMAL)
            .assignmentOptional().defaultValue("")
            .commit(),

            NODE_ELEMENT(expected).key("orderTest")
            .displayedName("Order Test")
            .commit(),

            SLOT_ELEMENT(expected).key("orderTest.slotStart")
            .displayedName("Start")
            .description(
                "Start test that signals and direct slot calls from another "
                "instance are received in order. 'Other' BoundOrderTestDevice "
                "instance is defined via 'stringProperty' and number of "
                "messages via 'int32Property'. "
                "Results are stored in properties below.")
            .allowedStates(State.NORMAL)
            .commit(),

            VECTOR_INT32_ELEMENT(expected)
            .key("orderTest.nonConsecutiveCounts")
            .displayedName("Non Consecutive Counts")
            # If all fine for N received counts: 0
            # If 3 and 4 where switched: 0, 4, 3, 5
            # If 3 is missing: 0, 4,
            .description("All received counts N whose predecessor was not N-1")
            .maxSize(1000)
            .readOnly()
            .initialValue([])
            .commit(),

            UINT32_ELEMENT(expected).key("orderTest.receivedCounts")
            .displayedName("Received Counts")
            .description("Number of counts received in test cycle")
            .readOnly()
            .initialValue(0)
            .commit(),
        )

    def __init__(self, configuration):
        # always call PythonDevice constructor first!
        super().__init__(configuration)

        # self.registerInitialFunction(self.initialization)

        self.signalSlotable.registerSignal("signalCount", int)  # 32 bit??

        self.KARABO_SLOT(self.orderTest_slotStart)
        self.KARABO_SLOT(self.slotCount)  # not in schema
        self.KARABO_SLOT(self.slotStartCount)  # not in schema

        self._counts = []

    def orderTest_slotStart(self):
        self.updateState(State.STARTING)

        def startOrderTest():
            other = self["stringProperty"]
            updates = Hash("stringProperty", self.deviceid,
                           "int32Property", self["int32Property"])
            try:
                req = self.request(other, "slotReconfigure", updates)
                req.waitForReply(5000)
                if not self.connect(other, "signalCount", "slotCount"):
                    raise RuntimeError("Failed to connect to 'signalCount' of "
                                       f"'{other}'")
            except Exception as e:  # noqa  # otherwise complains about unused e
                self.log.ERROR("Failed starting order test: {repr(e)}")
                self.updateState(State.ERROR)
                self.set("status", "Failed starting order test")
            else:
                self.updateState(State.STARTED)
                self.set(Hash("orderTest", Hash("nonConsecutiveCounts", [],
                                                "receivedCounts", 0)))
                self.call(other, "slotStartCount")

        EventLoop.post(startOrderTest)

    def slotCount(self, count):
        nCounts = len(self._counts)
        if count >= 0:
            self._counts.append(count)
            if nCounts % 1000 == 0:
                self.set("orderTest.receivedCounts", nCounts)
                self.log.INFO(f"slotCount received {nCounts} counts so far.")
        else:
            # Loop of calls and signals is done - publish result
            updates = Hash("orderTest.receivedCounts", nCounts)

            nonConsecutiveCounts = []
            for i in range(nCounts):
                if i == 0 or self._counts[i] != self._counts[i - 1] + 1:
                    if len(nonConsecutiveCounts) < 1000:
                        nonConsecutiveCounts.append(self._counts[i])
                    else:
                        break  # limit output size as defined in schema
            updates.set("orderTest.nonConsecutiveCounts", nonConsecutiveCounts)
            self._counts = []  # clear for next round

            log = ("Count summary: At least "
                   f"{len(nonConsecutiveCounts) - 1} "  # -1 for first count
                   f"out of {nCounts} messages out of order!")
            self.log.INFO(log)
            # publish state and other updates
            # (not yet possible in a single message as in C++)
            self.set(updates)
            self.updateState(State.NORMAL)

    def slotStartCount(self):
        self.updateState(State.STARTING)
        other = self["stringProperty"]
        numCounts = self["int32Property"]

        self.log.INFO(f"Start alternatingly calling '{other}' and emitting "
                      f"'signalCount' {numCounts // 2} times each")

        def doCount():
            self.updateState(State.STARTED)

            for i in range(0, numCounts, 2):
                self.call(other, "slotCount", i)
                self.emit("signalCount", i + 1)

            self.log.INFO(f"Done messaging {other}")
            self.call(other, "slotCount", -1)  # Tell that loop is done

            self.updateState(State.NORMAL)

        EventLoop.post(doCount)
