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
from sys import getsizeof
from time import sleep

import numpy as np

from karabo.bound import (
    BOOL_ELEMENT, FLOAT_ELEMENT, INPUT_CHANNEL, INT32_ELEMENT,
    KARABO_CLASSINFO, NODE_ELEMENT, UINT32_ELEMENT, VECTOR_BOOL_ELEMENT,
    VECTOR_STRING_ELEMENT, VECTOR_UINT32_ELEMENT, MetricPrefix, PythonDevice,
    Schema, State, Unit)


@KARABO_CLASSINFO("PPReceiverDevice", "2.2.4")
class PPReceiverDevice(PythonDevice):

    def expectedParameters(expected):

        data = Schema()
        (

            INT32_ELEMENT(data).key("dataId")
            .readOnly()
            .commit(),
            INPUT_CHANNEL(expected).key("input")
            .displayedName("Input")
            .description("Input channel: client")
            .dataSchema(data)
            .commit(),
            INPUT_CHANNEL(expected).key("input2")
            .displayedName("Input2")
            .description("Input channel: client")
            .commit(),
            # One channel under a node
            NODE_ELEMENT(expected).key("node")
            .commit(),

            INPUT_CHANNEL(expected).key("node.input3")
            .displayedName("Input3")
            .description("Input channel: client")
            .commit(),
            BOOL_ELEMENT(expected).key("onData")
            .displayedName("Use callback interface onData")
            .description("If false, use callback per InputChannel, "
                         "not per Data")
            .assignmentOptional().defaultValue(False)
            .commit(),
            UINT32_ELEMENT(expected).key("processingTime")
            .displayedName("Processing Time")
            .description("Simulated processing time")
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
            UINT32_ELEMENT(expected).key("dataItemSize")
            .displayedName("Data element size")
            .description("Data element size in bytes.")
            .readOnly()
            .commit(),
            UINT32_ELEMENT(expected).key("nTotalData")
            .displayedName("Total number of data tokens")
            .description("The total number of data received within "
                         "one stream")
            .readOnly()
            .commit(),
            UINT32_ELEMENT(expected).key("nTotalOnEos")
            .displayedName("Total on Eos ")
            .description("The total number of data received when "
                         "End of Stream was received")
            .readOnly()
            .commit(),
            VECTOR_STRING_ELEMENT(expected).key("dataSources")
            .displayedName("Data sources on input")
            .readOnly()
            .commit(),
            VECTOR_STRING_ELEMENT(expected).key("dataSourcesFromIndex")
            .displayedName("Data sources on input from index resolve")
            .readOnly()
            .commit(),
            FLOAT_ELEMENT(expected).key("averageTransferTime")
            .readOnly()
            .commit(),
            VECTOR_UINT32_ELEMENT(expected).key("numSources")
            .readOnly()
            .commit(),
            VECTOR_BOOL_ELEMENT(expected).key("sourcesCorrect")
            .readOnly()
            .commit(),
            UINT32_ELEMENT(expected).key("numSourceLength")
            .readOnly()
            .commit()
        )

    def __init__(self, config):
        super().__init__(config)
        self.KARABO_SLOT(self.reset)
        self.registerInitialFunction(self.initialize)
        self.transferTimes = []

    def initialize(self):
        if self.get("onData"):
            self.KARABO_ON_DATA("input", self.onData)
        else:
            self.KARABO_ON_INPUT("input", self.onInput)
        self.KARABO_ON_INPUT("input2", self.onInputProfile)
        self.KARABO_ON_INPUT("node.input3", self.onInputMultiSource)
        self.KARABO_ON_EOS("input", self.onEndOfStream)
        self.KARABO_ON_EOS("input2", self.onEndOfStreamProfile)
        self.KARABO_ON_EOS("node.input3", self.onEndOfStreamMultiSource)
        self.updateState(State.NORMAL)

    def onInput(self, input):
        allMeta = input.getMetaData()

        sources = []

        for i in range(input.size()):
            data = input.read(i)
            sources.append(allMeta[i]["source"])
            self.onData(data, allMeta[i])
        self.set("dataSourcesFromIndex", sources)

    def onData(self, data, metaData):

        self.set("dataSources", [metaData["source"], ])
        self.set("currentDataId", data.get("dataId"))
        v = data.get("data")
        self.set("dataItemSize", len(v)*getsizeof(v[0]))
        self.set("nTotalData", self.get("nTotalData") + 1)
        processingTime = self.get("processingTime")
        if processingTime > 0:
            sleep(processingTime/1000)

    def onEndOfStream(self, channel):
        self.set("nTotalOnEos", self.get("nTotalData"))

    def onInputProfile(self, input):
        for i in range(input.size()):
            data = input.read(i)
            transferTime = datetime.now().timestamp() - data.get("inTime")
            self.transferTimes.append(transferTime)
            self.set("nTotalData", self.get("nTotalData") + 1)
            arr = data.get("array")
            self.log.DEBUG(f"Array size: {arr.size}"*getsizeof(arr[0]))

    def onEndOfStreamProfile(self, channel):
        self.set("averageTransferTime", np.mean(self.transferTimes))

    def onInputMultiSource(self, input):
        numSources = self.get("numSources")
        numSources.append(input.size())
        self.set("numSources", numSources)

        sourcesCorrect = self.get("sourcesCorrect")
        allMeta = input.getMetaData()

        for i in range(input.size()):
            data = input.read(i)
            meta = allMeta[i]
            arr = data.get("array")
            if data.get("from") == "firstWrite":
                test = (meta["source"] == "source1") and (arr[-1] == 99)
                sourcesCorrect.append(bool(test))
            elif data.get("from") == "secondWrite":
                test = (meta["source"] == "source2") and (arr[-1] == 199)
                sourcesCorrect.append(bool(test))
        self.set("sourcesCorrect", sourcesCorrect)

    def onEndOfStreamMultiSource(self, channel):
        self.set("numSourceLength", len(self.get("numSources")))

    def reset(self):
        self.transferTimes = []
