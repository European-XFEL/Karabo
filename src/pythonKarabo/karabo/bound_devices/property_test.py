#!/usr/bin/env python

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

__author__ = "gero.flucke@xfel.eu"
__date__ = "September, 2017, 13:45 PM"

import threading
import time

import numpy as np

from karabo import __version__ as karaboVersion
from karabo.bound import (
    BOOL_ELEMENT, DOUBLE_ELEMENT, FLOAT_ELEMENT, IMAGEDATA_ELEMENT,
    INPUT_CHANNEL, INT32_ELEMENT, INT64_ELEMENT, KARABO_CLASSINFO,
    NDARRAY_ELEMENT, NODE_ELEMENT, OUTPUT_CHANNEL, OVERWRITE_ELEMENT,
    SLOT_ELEMENT, STRING_ELEMENT, TABLE_ELEMENT, UINT32_ELEMENT,
    UINT64_ELEMENT, VECTOR_BOOL_ELEMENT, VECTOR_CHAR_ELEMENT,
    VECTOR_DOUBLE_ELEMENT, VECTOR_FLOAT_ELEMENT, VECTOR_INT32_ELEMENT,
    VECTOR_INT64_ELEMENT, VECTOR_STRING_ELEMENT, VECTOR_UINT32_ELEMENT,
    VECTOR_UINT64_ELEMENT, AlarmCondition, DaqDataType, Encoding, Hash,
    ImageData, MetricPrefix, PythonDevice, Schema, State, Types, Unit)


@KARABO_CLASSINFO("PropertyTest", karaboVersion)
class PropertyTest(PythonDevice):

    defVectorMaxSize = 10  # maximum size of pipeline vector

    def __init__(self, configuration):
        # always call PythonDevice constructor first!
        super().__init__(configuration)

        # Define the slots
        self.KARABO_SLOT(self.setAlarm)
        self.KARABO_SLOT(self.setNoAckAlarm)
        self.KARABO_SLOT(self.writeOutput)
        self.KARABO_SLOT(self.startWritingOutput)
        self.KARABO_SLOT(self.stopWritingOutput)
        self.KARABO_SLOT(self.resetChannelCounters)
        self.KARABO_SLOT(self.eosOutput)
        self.KARABO_SLOT(self.node_increment)
        self.KARABO_SLOT(self.node_reset)

        self._writingOutput = False
        self._writingMutex = threading.Lock()
        self._writingWorker = None

        # Define first function to be called after the constructor has finished
        self.registerInitialFunction(self.initialization)

    @staticmethod
    def expectedParameters(expected):
        '''Description of device parameters statically known'''
        (
            OVERWRITE_ELEMENT(expected).key("state")
            .setNewOptions(State.INIT, State.NORMAL, State.ERROR,
                           State.STARTED, State.STOPPING)
            .setNewDefaultValue(State.INIT)
            .commit(),

            BOOL_ELEMENT(expected).key("boolProperty")
            .displayedName("Bool")
            .description("A bool property")
            .reconfigurable()
            .assignmentOptional().defaultValue(False)
            .commit(),

            BOOL_ELEMENT(expected).key("boolPropertyReadOnly")
            .displayedName("Readonly Bool")
            .description("A bool property for testing alarms")
            .readOnly().initialValue(True)
            .commit(),

            # CHAR_ELEMENT, (U)INT8_ELEMENT, (U)INT16_ELEMENT do not exists

            INT32_ELEMENT(expected).key("int32Property")
            .displayedName("Int32")
            .description("An int32 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(-32_000_000)
            .commit(),

            INT32_ELEMENT(expected).key("int32PropertyReadOnly")
            .displayedName("Readonly Int32")
            .description("An int32 property for testing")
            .readOnly().initialValue(32_000_000)
            .commit(),

            UINT32_ELEMENT(expected).key("uint32Property")
            .displayedName("UInt32")
            .description("A uint32 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(32_000_000)
            .commit(),

            UINT32_ELEMENT(expected).key("uint32PropertyReadOnly")
            .displayedName("Readonly UInt32")
            .description("A uint32 property for testing")
            .readOnly().initialValue(32_000_000)
            .commit(),

            INT64_ELEMENT(expected).key("int64Property")
            .displayedName("Int64")
            .description("An int64 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(3_200_000_000)
            .commit(),

            INT64_ELEMENT(expected).key("int64PropertyReadOnly")
            .displayedName("Readonly Int64")
            .description("An int64 property for testing")
            .readOnly().initialValue(3_200_000_000)
            .commit(),

            UINT64_ELEMENT(expected).key("uint64Property")
            .displayedName("UInt64 property")
            .description("A uint64 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(3_200_000_000)
            .commit(),

            UINT64_ELEMENT(expected).key("uint64PropertyReadOnly")
            .displayedName("Readonly UInt64")
            .description("A uint64 property for testing")
            .readOnly().initialValue(3_200_000_000)
            .commit(),

            FLOAT_ELEMENT(expected).key("floatProperty")
            .displayedName("Float property")
            .description("A float property")
            .reconfigurable()
            .assignmentOptional().defaultValue(3.141596)
            .commit(),

            FLOAT_ELEMENT(expected).key("floatPropertyReadOnly")
            .displayedName("Readonly Float")
            .description("A float property for testing")
            .readOnly().initialValue(3.141596)
            .commit(),

            DOUBLE_ELEMENT(expected).key("doubleProperty")
            .displayedName("Double")
            .description("A double property")
            .reconfigurable()
            .assignmentOptional().defaultValue(0.)
            .commit(),

            DOUBLE_ELEMENT(expected).key("doublePropertyReadOnly")
            .displayedName("Readonly Double")
            .description("A double property for testing")
            .readOnly().initialValue(0.)
            .commit(),

            STRING_ELEMENT(expected).key("stringProperty")
            .displayedName("String")
            .description("A string property")
            .reconfigurable()
            .assignmentOptional().defaultValue("Some arbitrary text.")
            .commit(),

            SLOT_ELEMENT(expected).key("setAlarm")
            .displayedName("Set Alarm")
            .description("Set an acknowledgment requiring alarm to value of "
                         "String property - if convertable")
            .commit(),

            SLOT_ELEMENT(expected).key("setNoAckAlarm")
            .displayedName("Set Alarm (no ackn.)")
            .description("Set an alarm (that does not require acknowledgment) "
                         "to value of String property - if convertable")
            .commit(),

            NODE_ELEMENT(expected).key("vectors")
            .displayedName("Vectors")
            .description("A node containing vector properties")
            .commit(),

            VECTOR_BOOL_ELEMENT(expected).key("vectors.boolProperty")
            .displayedName("Bool property")
            .description("A vector boolean property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([True, False, True,
                                                False, True, False])
            .commit(),

            VECTOR_CHAR_ELEMENT(expected).key("vectors.charProperty")
            .displayedName("Char property")
            .description("A vector character property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue(['A', 'B', 'C', 'D', 'E', 'F'])
            .commit(),

            VECTOR_INT32_ELEMENT(expected).key("vectors.int32Property")
            .displayedName("Int32 property")
            .description("A vector int32 property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([20000041, 20000042, 20000043,
                                                20000044, 20000045, 20000046])
            .commit(),

            VECTOR_UINT32_ELEMENT(expected).key("vectors.uint32Property")
            .displayedName("UInt32 property")
            .description("A vector uint32 property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([90000041, 90000042, 90000043,
                                                90000044, 90000045, 90000046])
            .commit(),

            VECTOR_INT64_ELEMENT(expected).key("vectors.int64Property")
            .displayedName("Int64 property")
            .description("A vector int64 property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([20000000041, 20000000042,
                                                20000000043, 20000000044,
                                                20000000045, 20000000046])
            .commit(),

            VECTOR_UINT64_ELEMENT(expected).key("vectors.uint64Property")
            .displayedName("UInt64 property")
            .description("A vector uint64 property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([90000000041, 90000000042,
                                                90000000043, 90000000044,
                                                90000000045, 90000000046])
            .commit(),

            VECTOR_FLOAT_ELEMENT(expected).key("vectors.floatProperty")
            .displayedName("Float property")
            .description("A vector float property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([1.23456, 2.34567, 3.45678,
                                                4.56789, 5.67891, 6.78912])
            .commit(),

            VECTOR_DOUBLE_ELEMENT(expected).key("vectors.doubleProperty")
            .displayedName("Double property")
            .description("A vector double property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([1.234567891, 2.345678912,
                                                3.456789123, 4.567891234,
                                                5.678901234, 6.123456789])
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("vectors.stringProperty")
            .displayedName("String property")
            .description("A vector string property")
            .reconfigurable()
            .minSize(1).maxSize(10)
            .assignmentOptional().defaultValue(["AAAAA", "BBBBB", "CCCCC",
                                                "XXXXX", "YYYYY", "ZZZZZ"])
            .commit(),
        )

        # Now prepare table
        columns = Schema()
        (
            STRING_ELEMENT(columns).key("e1")
            .displayedName("E1")
            .description("E1 property")
            .assignmentOptional().defaultValue("E1")
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(columns).key("e2")
            .displayedName("E2")
            .description("E2 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(False)
            .commit(),

            INT32_ELEMENT(columns).key("e3")
            .displayedName("E3")
            .description("E3 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(42)
            .commit(),

            FLOAT_ELEMENT(columns).key("e4")
            .displayedName("E4")
            .description("E4 property")
            .assignmentOptional().defaultValue(-3.14)
            .reconfigurable()
            .commit(),

            DOUBLE_ELEMENT(columns).key("e5")
            .displayedName("E5")
            .description("E5 property")
            .assignmentOptional().defaultValue(3.14)
            .reconfigurable()
            .commit(),
        )

        (

            TABLE_ELEMENT(expected).key("table")
            .displayedName("Table property")
            .description("Table containing one node.")
            .setColumns(columns)
            .assignmentOptional().defaultValue([Hash("e1", "abc", "e2", True,
                                                     "e3", 12, "e4", 0.9837,
                                                     "e5", 1.2345),
                                                Hash("e1", "xyz", "e2", False,
                                                     "e3", 42, "e4", 2.33333,
                                                     "e5", 7.77777)])
            .reconfigurable()
            .commit(),
        )
        # Transform access mode of columns for readOnly table
        for key in columns.getKeys():
            OVERWRITE_ELEMENT(columns).key(key).setNowReadOnly().commit()
        (

            TABLE_ELEMENT(expected).key("tableReadOnly")
            .displayedName("Read-only table property")
            .description("Read-only table with two values.")
            .setColumns(columns)
            .readOnly().initialValue([Hash("e1", "abc", "e2", True,
                                           "e3", 12, "e4", 0.9837,
                                           "e5", 1.2345),
                                      Hash("e1", "xyz", "e2", False,
                                           "e3", 42, "e4", 2.33333,
                                           "e5", 7.77777)])
            .commit(),
        )

        pipeData = Schema()
        (
            NODE_ELEMENT(pipeData).key("node")
            .displayedName("Node for DAQ")
            .description("An intermediate node needed by DAQ")
            .setDaqDataType(DaqDataType.TRAIN)
            .commit(),

            INT32_ELEMENT(pipeData).key("node.int32")
            .description("A signed 32-bit integer sent via the pipeline")
            .readOnly()
            .commit(),

            STRING_ELEMENT(pipeData).key("node.string")
            .description("A string send via the pipeline")
            .readOnly()
            .commit(),

            VECTOR_INT64_ELEMENT(pipeData).key("node.vecInt64")
            .description("A vector of signed 64-bit integers sent via the "
                         "pipeline")
            .maxSize(PropertyTest.defVectorMaxSize)  # DAQ needs that
            .readOnly()
            .commit(),

            NDARRAY_ELEMENT(pipeData).key("node.ndarray")
            .description("A multi dimensional array of floats sent via the "
                         "pipeline")
            .dtype(Types.FLOAT)
            .shape("100,200")
            .commit(),

            IMAGEDATA_ELEMENT(pipeData).key("node.image")
            .description("An image with pixels as 16-bit unsigned integers "
                         "sent via the pipeline")
            .setDimensions("400,500")
            .setEncoding(Encoding.GRAY)
            .setType(Types.UINT16)
            .commit(),
        )

        (
            OUTPUT_CHANNEL(expected).key("output")
            .displayedName("Output")
            .dataSchema(pipeData)
            .commit(),

            SLOT_ELEMENT(expected).key("writeOutput")
            .displayedName("Write to Output")
            .description("Write once to output channel 'Output'")
            .allowedStates(State.NORMAL)
            .commit(),

            FLOAT_ELEMENT(expected).key("outputFrequency")
            .displayedName("Output frequency")
            .description("The target frequency for continously writing to "
                         "'Output'")
            .unit(Unit.HERTZ)
            .maxInc(1000)
            .minExc(0.0)
            .assignmentOptional().defaultValue(1.0)
            .expertAccess()
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("outputCounter")
            .displayedName("Output Counter")
            .description("Last value sent as 'int32' via output channel "
                         "'Output'")
            .readOnly()
            .initialValue(0)
            .commit(),

            SLOT_ELEMENT(expected).key("startWritingOutput")
            .displayedName("Start Writing")
            .description("Start writing continously to output channel "
                         "'Output'")
            .allowedStates(State.NORMAL)
            .commit(),

            SLOT_ELEMENT(expected).key("stopWritingOutput")
            .displayedName("Stop Writing")
            .description("Stop writing continously to output channel "
                         "'Output'")
            .allowedStates(State.STARTED)
            .commit(),

            SLOT_ELEMENT(expected).key("eosOutput")
            .displayedName("EOS to Output")
            .description("Write end-of-stream to output channel 'Output'")
            .allowedStates(State.NORMAL)
            .commit(),

            INPUT_CHANNEL(expected).key("input")
            .displayedName("Input")
            .dataSchema(pipeData)  # re-use what the output channel sends
            .commit(),

            UINT32_ELEMENT(expected).key("processingTime")
            .displayedName("Processing Time")
            .description("Processing time of input channel data handler")
            .assignmentOptional()
            .defaultValue(0)
            .reconfigurable()
            .unit(Unit.SECOND)
            .metricPrefix(MetricPrefix.MILLI)
            .expertAccess()
            .commit(),

            INT32_ELEMENT(expected).key("currentInputId")
            .displayedName("Current Input Id")
            .description("Last value received as 'int32' on input channel "
                         "(default: 0)")
            .readOnly().initialValue(0)
            .commit(),

            UINT32_ELEMENT(expected).key("inputCounter")
            .displayedName("Input Counter")
            .description("Number of data items received on input channel")
            .readOnly().initialValue(0)
            .commit(),

            UINT32_ELEMENT(expected).key("inputCounterAtEos")
            .displayedName("Input Counter @ EOS")
            .description("Value of 'Input Counter' when endOfStream was "
                         "received")
            .readOnly().initialValue(0)
            .commit(),

            SLOT_ELEMENT(expected).key("resetChannelCounters")
            .displayedName("Reset Channels")
            .description("Reset counters involved in input/output channel "
                         "data flow")
            .allowedStates(State.NORMAL)
            .commit(),


            NODE_ELEMENT(expected).key("node")
            .displayedName("Node")
            .commit(),

            SLOT_ELEMENT(expected).key("node.increment")
            .displayedName("Increment")
            .description("Increment 'Counter read-only'")
            .commit(),

            SLOT_ELEMENT(expected).key("node.reset")
            .displayedName("Reset")
            .description("Reset 'Counter read-only'")
            .commit(),

            UINT32_ELEMENT(expected).key("node.counterReadOnly")
            .displayedName("Counter read-only")
            .readOnly()
            .initialValue(0)
            .commit(),

            UINT32_ELEMENT(expected).key("node.counter")
            .displayedName("Counter")
            .description("Values will be transferred to 'Counter read-only' "
                         "under same node")
            .reconfigurable()
            .assignmentOptional()
            .defaultValue(0)
            .commit(),
        )

        (
            STRING_ELEMENT(expected)
            .key("stringInternal")
            .displayedName("Internal String")
            .description("A string property with assignment internal")
            .reconfigurable()
            .assignmentInternal()
            .defaultValue("Internal Only")
            .commit(),

            STRING_ELEMENT(expected)
            .key("stringInitInternal")
            .displayedName("Internal Init String")
            .description("A string property with assignment internal "
                         "and init only")
            .init()
            .assignmentInternal()
            .defaultValue("Init Internal Only")
            .commit(),
        )

    def initialization(self):
        self.updateState(State.NORMAL)
        self.KARABO_ON_DATA("input", self.onData)
        self.KARABO_ON_EOS("input", self.onEndOfStream)

    def preReconfigure(self, incomingCfg):
        props = ["boolProperty", "int32Property", "uint32Property",
                 "int64Property", "uint64Property",
                 "floatProperty", "doubleProperty",
                 "table", "node.counter"]

        bulkSets = Hash()
        for prop in props:
            if prop in incomingCfg:
                readOnlyProp = prop + "ReadOnly"
                bulkSets[readOnlyProp] = incomingCfg[prop]
        if bulkSets:
            self.set(bulkSets)

    def setAlarm(self):
        alarm = AlarmCondition.fromString(self["stringProperty"])
        self.setAlarmCondition(alarm, needsAcknowledging=True,
                               description="Acknowledgment requiring alarm")

    def setNoAckAlarm(self):
        alarm = AlarmCondition.fromString(self["stringProperty"])
        self.setAlarmCondition(alarm, needsAcknowledging=False,
                               description="No acknowledgment requiring alarm")

    def writeOutput(self):
        outputCounter = self["outputCounter"]
        outputCounter += 1
        self.set("outputCounter", outputCounter)

        # Set all numbers inside to outputCounter:
        data = Hash("node", Hash())
        node = data["node"]

        # setAs needed if counter exceeds INT32 range...
        node.setAs("int32", outputCounter, Types.INT32)
        node.set("string", str(outputCounter))
        # Using plain Hash.set(..), type of vector is determined from 1st elem:
        node.setAs("vecInt64", outputCounter * self.defVectorMaxSize,
                   Types.VECTOR_INT64)
        arr = np.full((100, 200), outputCounter, dtype=np.float32)
        node.set("ndarray", arr)
        imArr = np.full((400, 500), outputCounter, dtype=np.uint16)
        node.set("image", ImageData(imArr, encoding=Encoding.GRAY))

        self.writeChannel("output", data)

    def eosOutput(self):
        self.signalEndOfStream("output")

    def startWritingOutput(self):
        self._writingOutput = True
        self.updateState(State.STARTED)
        self._writingWorker = threading.Thread(target=self.writeLoop)
        self._writingWorker.start()

    def stopWritingOutput(self):
        self.updateState(State.STOPPING)
        with self._writingMutex:
            self._writingOutput = False
        self._writingWorker.join()
        self._writingWorker = None

    def resetChannelCounters(self):
        self.set(Hash("inputCounter", 0,
                      "inputCounterAtEos", 0,
                      "outputCounter", 0,
                      "currentInputId", 0))

    def writeLoop(self):
        # Always run at least once: write should start immediately
        shouldWrite = True
        while shouldWrite:
            before = time.time()
            self.writeOutput()

            with self._writingMutex:
                shouldWrite = self._writingOutput

            if shouldWrite:
                # Waits for an interval as close as possible to the interval
                # defined by the nominal outputFrequency.
                delay = 1.0 / self.get("outputFrequency")
                delay = max(0., delay - (time.time() - before))
                time.sleep(delay)

                # "Refreshes" the shouldWrite flag before entering a new loop
                # interaction During the sleep an stop command might have been
                # issued and we want to avoid the extra writeOutput call.
                with self._writingMutex:
                    shouldWrite = self._writingOutput

        self.updateState(State.NORMAL)

    def onData(self, data, meta):
        # Sleeps to simulate heavy work.
        procTimeSecs = self["processingTime"]/1000.0
        time.sleep(procTimeSecs)

        currentInputId = data.get("node.int32")
        inputCounter = self["inputCounter"]

        self.set(Hash("currentInputId", currentInputId,
                      "inputCounter", inputCounter+1))

        # Writes data received to output channel to allow Property_Test to
        # build pipelines of chained devices.
        self.writeOutput()

    def onEndOfStream(self, inputChannel):
        inputCounter = self["inputCounter"]
        self.set("inputCounterAtEos", inputCounter)
        # Forward endOfStream as well
        self.signalEndOfStream("output")

    def node_increment(self):
        counter = self.get("node.counterReadOnly")
        self.set("node.counterReadOnly", counter + 1)

    def node_reset(self):
        self.set("node.counterReadOnly", 0)
