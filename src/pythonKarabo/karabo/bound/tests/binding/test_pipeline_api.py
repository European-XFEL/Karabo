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

import copy
import socket
import threading
import time
import uuid

import pytest

# Enable logs for debugging - does not matter which bindings are used:
from karabo.bound import (
    STRING_ELEMENT, VECTOR_INT32_ELEMENT, ChannelMetaData, ConnectionStatus,
    EventLoop, Hash, InputChannel, Logger, OutputChannel, Schema,
    SignalSlotable, Timestamp, Types, VectorHash)

config = Hash("level", "FATAL")
# config = Hash("level", "DEBUG")
Logger.configure(config)
Logger.useConsole()


def createConnectedInput(InputClass,
                         inputInstanceId, inputChannelId, inputCfg,
                         outputChannel):
    """
    Creates an input channel of class InputChannel with given instance/channel
    ids and config.
    Then synchronously connects to outputChannel (with assert on success).
    """
    outputInfo = outputChannel.getInformation()
    outputInfo["outputChannelString"] = outputChannel.getInstanceIdName()
    outputInfo["memoryLocation"] = "local"

    inputCfg["connectedOutputChannels"] = outputInfo["outputChannelString"]
    inputChannel = InputClass.create(inputInstanceId, inputChannelId,
                                     inputCfg)
    assert "" == inputChannel.connectSync(outputInfo)

    inputId = inputInstanceId + ":" + inputChannelId
    trials = 1000
    while trials > 0:
        trials -= 1
        # Check whether InputChannel is registered to receive all data
        done = (outputChannel.hasRegisteredCopyInputChannel(inputId)
                or outputChannel.hasRegisteredSharedInputChannel(inputId))
        if done:
            break
        time.sleep(0.001)

    assert done

    return inputChannel


class Writer:
    def __init__(self, chan, Hash, num, meta):
        self.out = chan
        self.Hash = Hash
        self.numData = num
        self.meta = meta

    def __call__(self):
        for i in range(self.numData):
            self.out.write(self.Hash('uint', i), self.meta)
            self.out.update()
        self.out.signalEndOfStream()


def test_pipeline_many_to_one():
    t = threading.Thread(target=EventLoop.work)
    t.start()
    assert t.is_alive() is True

    n_threads = 6
    EventLoop.addThread(n_threads)

    outputInstanceIdPre = "outputChannel_"
    oconf = Hash()
    outputs = []
    outputIds = []

    for i in range(n_threads):
        chanid = f'output{i}'
        out = OutputChannel.create(outputInstanceIdPre + str(i), chanid, oconf)
        outputs.append(out)
        outputIds.append(out.getInstanceId() + ':' + chanid)

    # Set up input channel
    inputInstanceId = "inputChannel"
    iconf = Hash("connectedOutputChannels", outputIds,
                 "onSlowness", "wait")
    nReceivedEos = 0
    lockEos = threading.Lock()
    connectionStatus = {}
    receivedData = dict()
    for oid in outputIds:
        receivedData[oid] = None

    # DataHandler
    def iData(data, meta):
        sourceName = meta['source']
        value = receivedData[sourceName]
        if value is None:
            value = [data['uint']]
        else:
            value.append(data['uint'])
        receivedData[sourceName] = value

    # EndOfStreamEventHandler
    def iEos(chan):
        nonlocal nReceivedEos
        nonlocal lockEos
        lockEos.acquire()
        nReceivedEos += 1
        lockEos.release()

    # ConnectionTracker
    def tracker(outId, status):
        nonlocal connectionStatus
        connectionStatus[outId] = status

    # Use factory function to create InputChannel instance ...
    inputChannel = InputChannel.create(inputInstanceId, "input0", iconf)
    # Here dataHandler, but None as inputhandler (see test_pipeline_handlers)
    inputChannel.registerDataHandler(iData)
    inputChannel.registerInputHandler(None)
    inputChannel.registerEndOfStreamEventHandler(iEos)
    inputChannel.registerConnectionTracker(tracker)

    # Store 'outputInfo' here
    infos = Hash()
    for i in range(len(outputs)):
        oid = outputIds[i]
        out = outputs[i]
        outputInfo = out.getInformation()
        assert outputInfo['port'] > 0
        outputInfo["outputChannelString"] = oid
        outputInfo["memoryLocation"] = "local" if i % 2 == 0 else "remote"

        assert "" == inputChannel.connectSync(outputInfo)
        infos[oid] = outputInfo

    # Make sure that OutputChannel objects have registered our input channel
    inputId = inputChannel.getInstanceId()
    for i in range(len(outputs)):
        # get instanceId of output channel
        oid = outputIds[i]
        # get OutputChannel object
        out = outputs[i]
        trials = 1000
        while trials > 0:
            trials -= 1
            # Check whether InputChannel is registered to receive all data
            # from current OutputChannel with "dataDistribution = copy"
            done = out.hasRegisteredCopyInputChannel(inputId)
            if done:
                break
            time.sleep(0.001)
        assert done is True

    numData = 200
    # Start sending data in parallel via outputs ...
    for i in range(len(outputs)):
        src = outputIds[i]
        meta = ChannelMetaData(src, Timestamp())
        out = outputs[i]
        writer = Writer(out, Hash, numData, meta)
        # On EventLoop the 'writer' object will be called as 'writer()'
        EventLoop.post(writer)

    # Wait for endOfStream arrival
    trials = 3000
    while trials >= 0:
        trials -= 1
        time.sleep(0.003)
        if nReceivedEos > 0:
            break

    # endOfStream received once
    # We give some time for more to arrive - but there should only be one,
    # although each output sent it!
    time.sleep(0.2)
    assert nReceivedEos == 1, "Data received:\n" + str(receivedData)

    # Proper number and order of data received from each output
    for oid in outputIds:
        data = receivedData[oid]
        assert numData == len(data), \
            oid + " lacks data, all received:\n" + str(receivedData)
        for i in range(len(data)):
            assert i == data[i], oid + ", data " + str(i)

    # Disconnect InputChannel from OutputChannels
    for oid in outputIds:
        inputChannel.disconnect(infos[oid])

    EventLoop.stop()
    t.join()


def test_pipeline_connect_disconnect():
    t = threading.Thread(target=EventLoop.work)
    t.start()
    assert t.is_alive() is True

    # Set up OutputChannel
    sso = SignalSlotable("outputChannel" + str(uuid.uuid4()))
    sso.start()

    out = sso.createOutputChannel("output", Hash("output"))
    table = []

    # ConnectionsHandler
    def connsHandler(connections):
        nonlocal table
        table = connections

    # Set up InputChannel
    ssi = SignalSlotable("inputChannel" + str(uuid.uuid4()))
    ssi.start()
    iconf = Hash("inputChannel.connectedOutputChannels",
                 ["outputChannel:output"], "inputChannel.onSlowness", "wait")
    calls = 0

    def inputDataHandler(data, meta):
        nonlocal calls
        calls += 1

    # EndOfStreamEventHandler
    def endOfStream(chan):
        pass

    trackedStatus = []

    def tracker(outId, status):
        if outId == "outputChannel:output":
            trackedStatus.append(status)

    inputChannel = ssi.createInputChannel("inputChannel", iconf,
                                          inputDataHandler, None, endOfStream,
                                          tracker)

    # Write first data - nobody connected yet.
    meta = ChannelMetaData("outputChannel:output", Timestamp())
    assert meta.getSource() == meta['source']
    assert meta.getTimestamp() == meta['timestamp']

    out.write(Hash("key", 42), meta)
    out.update()

    time.sleep(0.02)
    assert calls == 0
    assert len(table) == 0

    outputInfo = out.getInformation()
    assert outputInfo['port'] > 0
    outputInfo["outputChannelString"] = "outputChannel:output"
    outputInfo["memoryLocation"] = "local"

    n = 5
    for i in range(n):
        trackedStatus.clear()
        calls = 0
        stat = inputChannel.getConnectionStatus()
        assert len(stat) == 1
        assert stat["outputChannel:output"] == ConnectionStatus.DISCONNECTED
        # try to connect synchronously
        assert "" == inputChannel.connectSync(outputInfo)
        # ... but we need to wait for registration (getting 'hello') complete
        trials = 1000
        registered = False
        while trials > 0:
            trials -= 1
            registered = out.hasRegisteredSharedInputChannel(
                inputChannel.getInstanceId())
            if registered:
                break
            time.sleep(0.001)
        stat = inputChannel.getConnectionStatus()
        assert len(stat) == 1
        assert stat["outputChannel:output"] == ConnectionStatus.CONNECTED
        assert len(trackedStatus) > 1
        out.write(Hash("key", 43), meta)
        out.write(Hash("key", -43), meta)
        out.update()

        trials = 200
        while trials > 0:
            if calls == 2:
                break
            time.sleep(0.005)
            trials -= 1
        assert calls == 2

        inputChannel.disconnect(outputInfo)
        stat = inputChannel.getConnectionStatus()
        assert len(stat) == 1
        assert stat["outputChannel:output"] == ConnectionStatus.DISCONNECTED

    # Now test connection attempts that fail
    badOutputInfos = []
    # --------------------------- code 93, "Protocol not supported"
    oInfo = copy.copy(outputInfo)
    badOutputInfos.append(oInfo)
    badOutputInfos[-1]["connectionType"] = "udp"
    # --------------------------- code 111, "Connection refused"
    oInfo = copy.copy(outputInfo)
    badOutputInfos.append(oInfo)
    badOutputInfos[-1].setAs("port", 0, Types.UINT32)
    # --------------------------- code 1, "Host not found (authoritative)"
    oInfo = copy.copy(outputInfo)
    badOutputInfos.append(oInfo)
    badOutputInfos[-1]["hostname"] = "exflblablupp-not-there.desy.de"
    # --------------------------- code 33, "Numerical argument out of domain"
    oInfo = copy.copy(outputInfo)
    badOutputInfos.append(oInfo)
    del badOutputInfos[-1]["memoryLocation"]

    count = 0
    for info in badOutputInfos:
        # check sync connect ...
        msg = inputChannel.connectSync(info)
        assert len(msg) > 0
        count += 1

    calls = 0
    # Connect handler

    def handler(msg):
        nonlocal calls
        assert len(msg) > 0
        calls += 1

    for info in badOutputInfos:
        # check async connect with handler
        inputChannel.connect(info, handler)

    trials = 1000
    while trials > 0:
        if calls == count:
            break
        trials -= 1
        time.sleep(0.002)

    EventLoop.stop()
    time.sleep(.2)
    t.join()

    assert calls == count


def test_pipeline_one_to_shared():
    # Test "shared selector"
    prefix = Hash.__module__  # distinguished names for each run
    t = threading.Thread(target=EventLoop.work)
    t.start()

    outputCh = OutputChannel.create(prefix + "output", "out",
                                    Hash("noInputShared", "wait"))

    # Create 2 input channels with configuration...
    cfg = Hash("connectedOutputChannels", [prefix + "output:out"],
               "dataDistribution", "shared")
    inputCh1 = InputChannel.create(prefix + "input", "in1", cfg)
    inputCh2 = InputChannel.create(prefix + "input", "in2", cfg)

    # Prepare handlers for both input channels
    counter1 = 0
    counter2 = 0
    meta1 = None
    meta2 = None

    # ... data handler for input channel 1 ...
    def dataHandler(data, meta):
        nonlocal meta1
        nonlocal counter1
        meta1 = meta
        counter1 += 1

    # ... and input handler for input channel 2 ...
    def inputHandler(channel):
        nonlocal meta2
        nonlocal counter2
        meta2 = channel.getMetaData()
        counter2 += 1

    def eosHandler1(channel):
        nonlocal counter1
        counter1 = 0

    def eosHandler2(channel):
        nonlocal counter2
        counter2 = 0

    # Register handlers correspondingly ...
    inputCh1.registerDataHandler(dataHandler)
    inputCh2.registerInputHandler(inputHandler)
    inputCh1.registerEndOfStreamEventHandler(eosHandler1)
    inputCh2.registerEndOfStreamEventHandler(eosHandler2)

    outputInfo = outputCh.getInformation()
    assert outputInfo["port"] > 0
    outputInfo["outputChannelString"] = prefix + "output:out"
    outputInfo["memoryLocation"] = "local"

    # Connect both input channels synchronously
    assert "" == inputCh1.connectSync(outputInfo)
    assert "" == inputCh2.connectSync(outputInfo)
    # Before we send data, wait until output channel took note (got 'hello')
    trials = 1000
    while trials > 0:
        trials -= 1
        id2 = inputCh2.getInstanceId()
        if (outputCh.hasRegisteredSharedInputChannel(inputCh1.getInstanceId())
                and outputCh.hasRegisteredSharedInputChannel(id2)):
            break
        time.sleep(0.001)

    assert trials > 0

    # Now connected - we prepare and register the shared input selector
    numOfInput = 1  # which of the two inputs should get the data

    def selector(inputs):
        return prefix + "input:in" + str(numOfInput)

    outputCh.registerSharedInputSelector(selector)

    # Send first data - to input 1
    meta = ChannelMetaData(outputInfo["outputChannelString"], Timestamp())
    outputCh.write(Hash('int', 1), meta)
    outputCh.update()

    def waitForTotalNumData(total):
        trials = 1000
        while trials > 0:
            if counter1 + counter2 == total:
                return
            trials -= 1
            time.sleep(0.001)
        assert trials > 0

    def checkMetaDataInfo(m):
        assert m is not None
        assert isinstance(m, VectorHash) or isinstance(m, Hash)

        def checkEntry(entry):
            assert isinstance(entry, Hash)
            if not entry.empty():
                assert isinstance(entry['source'], str)
                assert 'output:out' in entry['source']
                assert entry['timestamp'] is True
                ts = Timestamp.fromHashAttributes(
                    entry.getAttributes('timestamp'))
                assert isinstance(ts, Timestamp)

        if isinstance(m, VectorHash):
            for h in m:
                checkEntry(h)
        else:
            checkEntry(m)

    waitForTotalNumData(1)
    assert counter1 + counter2 == 1
    assert counter1 == 1
    assert counter2 == 0
    checkMetaDataInfo(meta1)
    assert meta2 is None

    # Another data to input 1
    outputCh.write(Hash('int', 2), meta)
    outputCh.update()
    waitForTotalNumData(2)
    assert counter1 == 2
    assert counter2 == 0
    checkMetaDataInfo(meta1)
    assert meta2 is None

    # Another data, now to input 2 via asyncUpdate() with defaults
    meta1 = None
    numOfInput = 2
    outputCh.write(Hash('int', 3), meta)
    outputCh.asyncUpdate()
    waitForTotalNumData(3)
    assert counter1 == 2
    assert counter2 == 1
    assert meta1 is None
    checkMetaDataInfo(meta2)

    # Another data to input 2 via asyncUpdate(..) with arguments
    asyncUpdateDone = False

    def updateDoneHandler():
        nonlocal asyncUpdateDone
        asyncUpdateDone = True

    outputCh.write(Hash('int', 4), meta)
    outputCh.asyncUpdate(False, updateDoneHandler)
    waitForTotalNumData(4)
    assert counter1 == 2
    assert counter2 == 2
    assert meta1 is None
    checkMetaDataInfo(meta2)

    # Also check that handler is called
    trials = 1000
    while trials > 0:
        if asyncUpdateDone:
            break
        trials -= 1
        time.sleep(0.001)

    assert trials > 0
    assert asyncUpdateDone

    # Now test signalEndOfStream which travels to all inputs and resets counter
    outputCh.signalEndOfStream()
    waitForTotalNumData(0)
    assert counter1 == 0
    assert counter2 == 0

    # Send again to input 1 & 2 to clear it below with asyncSignalEndOfStream
    numOfInput = 1
    outputCh.write(Hash('int', 5), meta)
    outputCh.update()
    numOfInput = 2
    outputCh.write(Hash('int', 6), meta)
    outputCh.update()
    waitForTotalNumData(2)
    assert counter1 == 1
    assert counter2 == 1

    # Now test asyncSignalEndOfStream with default argument
    outputCh.asyncSignalEndOfStream()
    waitForTotalNumData(0)
    assert counter1 == 0
    assert counter2 == 0

    meta1 = None
    meta2 = None

    # Test that we can reset the selector.
    # (But we loose control who receives, so can hardly test.)
    outputCh.registerSharedInputSelector(None)

    outputCh.write(Hash('int', 7), meta)
    outputCh.update()
    outputCh.write(Hash('int', 8), meta)
    outputCh.update()
    waitForTotalNumData(2)

    assert counter1 + counter2 == 2
    checkMetaDataInfo(meta1)
    checkMetaDataInfo(meta2)

    # Finally test asyncSignalEndOfStream with handler argument
    asyncEosDone = False

    def eosSent():
        nonlocal asyncEosDone
        asyncEosDone = True

    outputCh.asyncSignalEndOfStream(eosSent)
    waitForTotalNumData(0)
    assert counter1 == 0
    assert counter2 == 0
    # Also check that handler is called
    trials = 1000
    while trials > 0:
        if asyncEosDone:
            break
        trials -= 1
        time.sleep(0.001)

    assert trials > 0
    assert asyncEosDone

    EventLoop.stop()
    time.sleep(.2)
    t.join(timeout=10)
    assert not t.is_alive()


def test_pipeline_input_channel():
    t = threading.Thread(target=EventLoop.work)
    t.start()
    sso = SignalSlotable("outputChannel" + str(uuid.uuid4()))
    sso.start()

    out = sso.createOutputChannel("output", Hash("output"))

    # Set up InputChannel
    ssi = SignalSlotable("inputChannel" + str(uuid.uuid4()))
    ssi.start()
    iconf = Hash("inputChannel.connectedOutputChannels",
                 ["outputChannel:output"], "inputChannel.onSlowness", "wait")
    calls = 0

    def inputDataHandler(data, meta):
        nonlocal calls
        calls += 1

    inputChannel = ssi.createInputChannel("inputChannel", iconf,
                                          inputDataHandler, None, None, None)

    # Write first data - nobody connected yet.
    meta = ChannelMetaData("outputChannel:output", Timestamp())

    out.write(Hash("key", 42), meta)
    out.update()

    time.sleep(0.02)
    assert calls == 0

    outputInfo = out.getInformation()
    assert outputInfo['port'] > 0
    outputInfo["outputChannelString"] = "outputChannel:output"
    outputInfo["memoryLocation"] = "local"

    calls = 0
    stat = inputChannel.getConnectionStatus()
    assert len(stat) == 1
    assert stat["outputChannel:output"] == ConnectionStatus.DISCONNECTED
    # try to connect synchronously
    assert "" == inputChannel.connectSync(outputInfo)
    # ... but we need to wait for registration (getting 'hello') complete
    trials = 1000
    while trials > 0:
        trials -= 1
        registered = out.hasRegisteredCopyInputChannel(
            inputChannel.getInstanceId())
        if registered:
            break
        time.sleep(0.001)

    assert registered is True

    stat = inputChannel.getConnectionStatus()
    assert len(stat) == 1
    assert stat["outputChannel:output"] == ConnectionStatus.CONNECTED

    out.write(Hash("key", 43), meta)
    out.write(Hash("key", -43), meta)
    out.update()

    trials = 200
    while trials > 0:
        if calls == 2:
            break
        time.sleep(0.005)
        trials -= 1
    assert calls == 2

    inputChannel.disconnect(outputInfo)
    stat = inputChannel.getConnectionStatus()
    assert len(stat) == 1
    assert stat["outputChannel:output"] == ConnectionStatus.DISCONNECTED

    EventLoop.stop()
    time.sleep(.2)
    t.join()


def test_pipeline_handlers():
    t = threading.Thread(target=EventLoop.work)
    t.start()

    # Test handlers that can be registered to input and output channels and
    # are not treated in other tests

    # 1) Create output and input channels
    output = OutputChannel.create("output", "out", Hash())
    initCfg = output.getInitialConfiguration()
    # Since we have "default" as "hostname" property, channel address is
    # resolved to hostname
    assert initCfg["address"] == socket.gethostname()

    # Need connected input channel to get IO event to be handled
    inputChannel = createConnectedInput(InputChannel, "input", "in",
                                        Hash("onSlowness", "wait"), output)

    # 2)
    # Specify input handler, but 'None' as data and eos handlers
    # (vice versa tested in test_many_to_one)
    inputHandlerCalled = False

    def inputHandler(chan):
        nonlocal inputHandlerCalled
        inputHandlerCalled = True

    inputChannel.registerDataHandler(None)
    inputChannel.registerInputHandler(inputHandler)
    inputChannel.registerEndOfStreamEventHandler(None)

    output.write(Hash("data", "more"), ChannelMetaData("source", Timestamp()))
    output.update()

    timeout = 5.
    while timeout > 0.:
        if inputHandlerCalled:
            break
        time.sleep(0.01)
        timeout -= 0.01

    assert inputHandlerCalled

    EventLoop.stop()
    time.sleep(.2)
    t.join()


def test_schema_validation():

    # A schema to validate against
    schema = Schema()
    VECTOR_INT32_ELEMENT(schema).key("v_int32").maxSize(10).readOnly().commit()
    STRING_ELEMENT(schema).key("str").readOnly().commit()

    # Validate only defaults to test binding, the rest is tested in C++
    output = OutputChannel.create("output", "out", Hash(), schema)
    meta = ChannelMetaData(output.getInstanceIdName(), Timestamp())
    vec = [1, 3, 4, 5]

    # Test extra key
    with pytest.raises(RuntimeError):
        output.write(Hash("v_int32", vec, "str", "some", "tooMuch", 0), meta)

    # Matching data
    output.write(Hash("v_int32", vec, "str", "some"), meta)

    ##########################################################################
    # Test creation of channel from SignalSlotable (which needs event loop)
    t = threading.Thread(target=EventLoop.work)
    t.start()
    assert t.is_alive() is True

    sigSlot = SignalSlotable("ForOutputChannelSchemaVal")
    sigSlot.start()
    cfg = Hash("output.validateSchema", "always")
    output = sigSlot.createOutputChannel("output", cfg, schema)

    with pytest.raises(RuntimeError):
        output.write(Hash("v_int32", vec, "str", "some", "tooMuch", 0), meta)

    # Matching data
    output.write(Hash("v_int32", vec, "str", "some"), meta)

    # Always validate, also after one matching data item
    with pytest.raises(RuntimeError):
        output.write(Hash("v_int32", vec, "str", "some", "tooMuch", 0), meta)

    EventLoop.stop()
    # time.sleep(.2)
    t.join()
