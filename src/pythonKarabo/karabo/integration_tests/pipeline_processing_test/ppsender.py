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
from datetime import datetime
from threading import Thread
from time import sleep

import numpy as np

from karabo.bound import (
    BOOL_ELEMENT, INT32_ELEMENT, KARABO_CLASSINFO, NDARRAY_ELEMENT,
    NODE_ELEMENT, OUTPUT_CHANNEL, OVERWRITE_ELEMENT, SLOT_ELEMENT,
    STRING_ELEMENT, UINT32_ELEMENT, UINT64_ELEMENT, VECTOR_INT64_ELEMENT,
    ChannelMetaData, Hash, MetricPrefix, PythonDevice, Schema, State,
    Timestamp, Types, Unit, launchPythonDevice)


@KARABO_CLASSINFO("PPSenderDevice", "2.2.4")
class PPSenderDevice(PythonDevice):
    TEST_VECTOR_SIZE = 1000000

    def expectedParameters(expected):

        (
            OVERWRITE_ELEMENT(expected).key("state")
            .setNewOptions(State.NORMAL, State.ACTIVE)
            .setNewDefaultValue(State.NORMAL)
            .commit(),
            SLOT_ELEMENT(expected).key("write")
            .displayedName("Write")
            .description("Write some data")
            .allowedStates(State.NORMAL)
            .commit(),
        )

        data = Schema()

        (
            INT32_ELEMENT(data).key("dataId")
            .readOnly()
            .commit(),
            STRING_ELEMENT(data).key("sha1")
            .readOnly()
            .commit(),
            STRING_ELEMENT(data).key("flow")
            .readOnly()
            .commit(),
            VECTOR_INT64_ELEMENT(data).key("data")
            .readOnly()
            .commit(),
            NDARRAY_ELEMENT(data).key("array")
            .dtype(Types.DOUBLE)
            .shape("100,200,0")
            .commit(),
            OUTPUT_CHANNEL(expected).key("output1")
            .displayedName("Output1")
            .dataSchema(data)
            .commit(),
        )

        data2 = Schema()

        (
            UINT64_ELEMENT(data2).key("inTime")
            .readOnly()
            .commit(),
            NDARRAY_ELEMENT(data2).key("array")
            .dtype(Types.DOUBLE)
            .shape("256,256,512")
            .commit(),
            OUTPUT_CHANNEL(expected).key("output2")
            .displayedName("Output2")
            .dataSchema(data2)
            .commit(),
            UINT32_ELEMENT(expected).key("nData")
            .displayedName("Number of data")
            .description("Number of data")
            .assignmentOptional().defaultValue(12)
            .reconfigurable()
            .commit(),
            UINT32_ELEMENT(expected).key("delay")
            .displayedName("Delay")
            .description("Delay between writes")
            .assignmentOptional().defaultValue(0)
            .unit(Unit.SECOND)
            .metricPrefix(MetricPrefix.MILLI)
            .reconfigurable()
            .commit(),
            UINT32_ELEMENT(expected).key("currentDataId")
            .displayedName("Current Data ID")
            .description("Monitors the currently processed data token")
            .readOnly()
            .commit(),
            STRING_ELEMENT(expected).key("scenario")
            .options("test,profile,multiSource")
            .assignmentOptional().defaultValue("test")
            .reconfigurable()
            .commit(),
            BOOL_ELEMENT(expected).key("copyAllData")
            .assignmentOptional().defaultValue(True)
            .reconfigurable()
            .commit(),
        )

        data3 = Schema()

        (
            INT32_ELEMENT(data3).key("dataId")
            .readOnly()
            .commit(),
            STRING_ELEMENT(data3).key("from")
            .readOnly()
            .commit(),
            NDARRAY_ELEMENT(data3).key("array")
            .dtype(Types.UINT32)
            .shape("100")
            .commit(),
            # Put one under a node
            NODE_ELEMENT(expected).key("node")
            .commit(),

            OUTPUT_CHANNEL(expected).key("node.output3")
            .displayedName("Output3")
            .dataSchema(data3)
            .commit(),
        )

    def __init__(self, config):
        super().__init__(config)
        self.KARABO_SLOT(self.write)
        self.writingWorker = None
        self.registerInitialFunction(self.initialize)

    def initialize(self):
        self.updateState(State.NORMAL)

    def write(self):
        if self.writingWorker:
            if self.writingWorker.is_alive():
                self.writingWorker.join()
        if self.get("scenario") == "test":
            self.writingWorker = Thread(target=self.writing)
        elif self.get("scenario") == "profile":
            self.writingWorker = Thread(target=self.writingProfile)
        elif self.get("scenario") == "multiSource":
            self.writingWorker = Thread(target=self.writingMultiSource)

        self.writingWorker.start()
        self.updateState(State.ACTIVE)

    def writing(self):
        try:
            nData = self.get("nData")
            delayInMs = self.get("delay")
            vec = np.arange(self.TEST_VECTOR_SIZE).tolist()
            data = Hash()

            msg = ("PPSenderDevice::writing : nData={}, delay in ms = {}, "
                   "vector size = {}")
            self.log.DEBUG(msg.format(nData, delayInMs, len(vec)))

            for iData in range(nData):
                data.set("dataId", iData)
                vec[0] = -iData
                data.set("data", vec)
                self.writeChannel("output1", data)

                msg = f"Written data # {iData}"
                self.set("currentDataId", iData)
                self.log.DEBUG(msg)
                if delayInMs > 0:
                    sleep(delayInMs/1000)

        except Exception as e:
            self.log.ERROR(f"Stop writing because: {e}")

        self.signalEndOfStream("output1")
        self.updateState(State.NORMAL)

    def writingProfile(self):
        try:
            nData = self.get("nData")
            delayInMs = self.get("delay")
            ndarr = np.zeros((256, 256, 512), np.double)
            data = Hash()
            copyAllData = self.get("copyAllData")
            channel = self._sigslot.getOutputChannel("output2")
            meta = ChannelMetaData("p2pTestSender:output1", Timestamp())

            for iData in range(nData):
                data.set("array", ndarr)
                data.set("inTime", datetime.now().timestamp())

                channel.write(data, meta=meta, copyAllData=copyAllData)
                channel.update()
                self.set("currentDataId", iData)

                msg = f"Written data # {iData}"
                self.log.DEBUG(msg)
                if delayInMs > 0:
                    sleep(delayInMs/1000)

        except Exception as e:
            self.log.ERROR(f"Stop writing because: {e}")

        self.signalEndOfStream("output2")
        self.updateState(State.NORMAL)

    def writingMultiSource(self):
        try:
            nData = self.get("nData")
            delayInMs = self.get("delay")
            channel = self._sigslot.getOutputChannel("node.output3")
            data = Hash()
            for i in range(nData):
                data.set("dataId", i)
                data.set("from", "firstWrite")
                data.set("array", np.arange(100))
                meta = ChannelMetaData("source1", Timestamp())
                channel.write(data, meta=meta)

                data.set("dataId", 1000+i)
                data.set("from", "secondWrite")
                data.set("array", np.arange(100, 200))
                meta = ChannelMetaData("source2", Timestamp())
                channel.write(data, meta=meta)

                channel.update()
                if delayInMs > 0:
                    sleep(delayInMs/1000)

        except Exception as e:
            self.log.ERROR(f"Stop writing because: {e}")

        self.signalEndOfStream("node.output3")
        self.updateState(State.NORMAL)


# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()
