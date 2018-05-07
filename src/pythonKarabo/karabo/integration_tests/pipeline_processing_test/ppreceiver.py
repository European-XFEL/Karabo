from time import sleep
from datetime import datetime
from sys import getsizeof

import numpy as np

from karabo.bound import (BOOL_ELEMENT, FLOAT_ELEMENT, INPUT_CHANNEL,
                          INT32_ELEMENT, KARABO_CLASSINFO, launchPythonDevice,
                          MetricPrefix, PythonDevice, Schema, State,
                          UINT32_ELEMENT, Unit, VECTOR_STRING_ELEMENT)


@KARABO_CLASSINFO("PPReceiverDevice", "2.4")
class PPReceiverDevice(PythonDevice):

    def expectedParameters(expected):

        data = Schema()
        (

            INT32_ELEMENT(data).key("dataId")
                .readOnly()
                .commit()
                ,
            INPUT_CHANNEL(expected).key("input")
                .displayedName("Input")
                .description("Input channel: client")
                .dataSchema(data)
                .commit()
                ,
            INPUT_CHANNEL(expected).key("input2")
                .displayedName("Input2")
                .description("Input channel: client")
                .commit()
                ,
            BOOL_ELEMENT(expected).key("onData")
                .displayedName("Use callback interface onData")
                .description("If false, use callback per InputChannel, "
                             "not per Data")
                .assignmentOptional().defaultValue(False)
                .commit()
                ,
            UINT32_ELEMENT(expected).key("processingTime")
                .displayedName("Processing Time")
                .description("Simulated processing time")
                .assignmentOptional().defaultValue(0)
                .unit(Unit.SECOND)
                .metricPrefix(MetricPrefix.MILLI)
                .reconfigurable()
                .commit()
                ,
            UINT32_ELEMENT(expected).key("currentDataId")
                .displayedName("Current Data ID")
                .description("Monitors the currently processed data token")
                .readOnly()
                .commit()
                ,
            UINT32_ELEMENT(expected).key("dataItemSize")
                .displayedName("Data element size")
                .description("Data element size in bytes.")
                .readOnly()
                .commit()
                ,
            UINT32_ELEMENT(expected).key("nTotalData")
                .displayedName("Total number of data tokens")
                .description("The total number of data received within "
                             "one stream")
                .readOnly()
                .commit()
                ,
            UINT32_ELEMENT(expected).key("nTotalOnEos")
                .displayedName("Total on Eos ")
                .description("The total number of data received when "
                             "End of Stream was received")
                .readOnly()
                .commit()
                ,
            VECTOR_STRING_ELEMENT(expected).key("dataSources")
                .displayedName("Data sources on input")
                .readOnly()
                .commit()
                ,
            VECTOR_STRING_ELEMENT(expected).key("dataSourcesFromIndex")
                .displayedName("Data sources on input from index resolve")
                .readOnly()
                .commit()
                ,
            FLOAT_ELEMENT(expected).key("averageTransferTime")
                .readOnly()
                .commit()
            ,
        )

    def __init__(self, config):
        super(PPReceiverDevice, self).__init__(config)
        self.KARABO_SLOT(self.reset)
        self.registerInitialFunction(self.initialize)
        self.transferTimes = []

    def initialize(self):
        if self.get("onData"):
            self.KARABO_ON_DATA("input", self.onData)
        else:
            self.KARABO_ON_INPUT("input", self.onInput)
        self.KARABO_ON_INPUT("input2", self.onInputProfile)
        self.KARABO_ON_EOS("input", self.onEndOfStream)
        self.KARABO_ON_EOS("input2", self.onEndOfStreamProfile)
        self.updateState(State.NORMAL)

    def onInput(self, input):
        allMeta = input.getMetaData()
        self.set("dataSources", allMeta[0].get("source"))
        sources = []

        for i in range(input.size()):
            data = input.read(i)
            sources.append(allMeta[i]["source"])
            self.onData(data, allMeta[i])
        self.set("dataSourcesFromIndex", sources)

    def onData(self, data, metaData):

        self.set("dataSources", [metaData["source"], ])
        self.set("currentDataId", data.get("dataId"))
        v = np.array(data.get("data"))
        self.set("dataItemSize", v.size*getsizeof(v[0]))
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
            self.log.DEBUG("Array size: {}".format(arr.size)*getsizeof(arr[0]))

    def onEndOfStreamProfile(self, channel):
        self.set("averageTransferTime", np.mean(self.transferTimes))

    def reset(self):
        self.transferTimes = []


# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()
