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
import threading
import time
import uuid

import karabind
import pytest

import karathon


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


@pytest.mark.parametrize(
    "EventLoop, SignalSlotable, Hash, ChannelMetaData, Timestamp",
    [(karathon.EventLoop, karathon.SignalSlotable, karathon.Hash,
      karathon.ChannelMetaData, karathon.Timestamp),
     (karabind.EventLoop, karabind.SignalSlotable, karabind.Hash,
      karabind.ChannelMetaData, karabind.Timestamp)
     ])
def test_pipeline_many_to_one(EventLoop, SignalSlotable, Hash,
                              ChannelMetaData, Timestamp):
    t = threading.Thread(target=EventLoop.work)
    t.start()
    assert t.is_alive() is True

    n_threads = 6
    EventLoop.addThread(n_threads)
    # Create SignalSlotable object to use later for OutputChannel creation
    # by C++ code.
    sso = SignalSlotable("outputChannel" + str(uuid.uuid4()))
    sso.start()
    oconf = Hash()
    outputs = []
    outputIds = []

    def onOutputHandler(chan):
        pass

    for i in range(n_threads):
        chanid = f'output{i}'
        oconf[chanid] = Hash()
        # Creation of OutputChannel object by C++
        out = sso.createOutputChannel(chanid, oconf, onOutputHandler)
        outputs.append(out)
        outputIds.append(out.getInstanceId() + ':' + chanid)

    # Set up input channel
    ssi = SignalSlotable("inputChannel" + str(uuid.uuid4()))
    ssi.start()
    iconf = Hash("input0.connectedOutputChannels", outputIds,
                 "input0.onSlowness", "wait")
    nReceivedEos = 0
    lockEos = threading.Lock()
    connectionStatus = {}
    receivedData = dict()
    for oid in outputIds:
        receivedData[oid] = None

    # DataHandler
    def iData(data, meta):
        # TODO:
        # karathon calls meta as Hash
        # karabind calls meta as ChannelMetaData
        if isinstance(meta, Hash):
            sourceName = meta['source']
        else:
            sourceName = meta.getSource()
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

    inputChannel = ssi.createInputChannel("input0", iconf, iData, None, iEos,
                                          tracker)

    # Store 'outputInfo' here
    infos = Hash()
    for i in range(len(outputs)):
        oid = outputIds[i]
        out = outputs[i]
        outputInfo = out.getInformation()
        assert outputInfo['port'] > 0
        outputInfo["outputChannelString"] = oid
        outputInfo["memoryLocation"] = "local" if i % 2 == 0 else "remote"

        inputChannel.connectSync(outputInfo)
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
            # from current OutputChannel with "dataDostribution = copy"
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

    # time.sleep(0.1)
    EventLoop.stop()
    # 'removeThread' works unreliable ... sometimes hangs
    # EventLoop.removeThread(n_threads)
    t.join()


@pytest.mark.parametrize(
    "EventLoop, SignalSlotable, ConnectionStatus, ChannelMetaData, Timestamp,\
     Hash, Types",
    [(karabind.EventLoop, karabind.SignalSlotable, karabind.ConnectionStatus,
      karabind.ChannelMetaData, karabind.Timestamp, karabind.Hash,
      karabind.Types),
     (karathon.EventLoop, karathon.SignalSlotable, karathon.ConnectionStatus,
      karathon.ChannelMetaData, karathon.Timestamp, karathon.Hash,
      karathon.Types)
     ])
def test_pipeline_connect_disconnect(EventLoop, SignalSlotable,
                                     ConnectionStatus, ChannelMetaData,
                                     Timestamp, Hash, Types):
    t = threading.Thread(target=EventLoop.work)
    t.start()
    assert t.is_alive() is True

    # Set up OutputChannel
    sso = SignalSlotable("outputChannel" + str(uuid.uuid4()))
    sso.start()

    def onOutputHandler(chan):
        pass

    out = sso.createOutputChannel("output", Hash("output"), onOutputHandler)
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
        inputChannel.connectSync(outputInfo)
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
    # --------------------------- "" : C++ returns success
    # oInfo = copy.copy(outputInfo)
    # badOutputInfos.append(oInfo)
    # badOutputInfos[-1]["outputChannelString"] == "not configured"
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
        time.sleep(0.2)  # give time for handler to be called

    EventLoop.stop()
    time.sleep(.2)
    t.join()

    assert calls == count
