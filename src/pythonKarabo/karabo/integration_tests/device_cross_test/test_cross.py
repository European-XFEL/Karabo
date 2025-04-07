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
"""This tests the communication between bound API and middlelayer API"""
import json
import os
import shutil
from asyncio import TimeoutError, get_running_loop, wait_for
from datetime import datetime

import numpy as np
import pytest
import pytest_asyncio

from karabo.common.enums import Capabilities, Interfaces
from karabo.middlelayer import (
    AccessLevel, AlarmCondition, Assignment, Configurable, DaqDataType,
    DeviceClientBase, Hash, Image, InputChannel, Int32, KaraboError,
    MetricPrefix, NDArray, Node, OutputChannel, PipelineContext, Slot, State,
    String, UInt32, Unit, VectorDouble, background, call, getDevice,
    getHistory, isSet, setWait, shutdown, sleep, unit, updateDevice, waitUntil,
    waitUntilNew)
from karabo.middlelayer.testing import (  # noqa
    AsyncDeviceContext, AsyncServerContext, event_loop_policy, sleepUntil)


class Child(Configurable):
    number = Int32()


class DataNode(Configurable):
    daqDataType = DaqDataType.TRAIN

    image = Image(
        shape=(10, 10),
        dtype=UInt32,
        displayedName="Image")

    array = NDArray(
        displayedName="NDArray",
        dtype=UInt32,
        shape=(10, 10))


class ImageChannel(Configurable):
    data = Node(DataNode)


class MiddlelayerDevice(DeviceClientBase):
    """A simple device with topology info"""
    channelcount = 0
    rawchannelcount = 0

    value = Int32()

    channelclose = String(
        defaultValue="")

    boundDevice = String(
        defaultValue="")

    child = Node(Child)

    vectorMaxSize = VectorDouble(
        defaultValue=[2.0, 2.0],
        minSize=2, maxSize=4)

    array = NDArray(
        dtype=UInt32,
        shape=(2,))

    foundCapabilities = Int32(
        defaultValue=0)

    foundInterfaces = Int32(
        defaultValue=0)

    @Slot()
    async def slot(self):
        self.marker = True

    @InputChannel()
    async def channel(self, data, meta):
        self.channelcount += 1
        self.channeldata = data
        self.channelmeta = meta

    @channel.close
    async def channel(self, output):
        self.channelclose = output

    @InputChannel(raw=True)
    def rawchannel(self, data, meta):
        self.rawchannelcount += 1
        self.rawchanneldata = data
        self.rawchannelmeta = meta

    output = OutputChannel(Child)

    imageOutput = OutputChannel(ImageChannel)

    rawOutput = OutputChannel()

    @Slot()
    async def retrieveInterfaces(self):
        device_topology = self.systemTopology["device"]
        capa = device_topology[self.boundDevice, "capabilities"]
        interfaces = device_topology[self.boundDevice, "interfaces"]
        # Add to our device and immediately update!
        self.foundCapabilities = capa
        self.foundInterfaces = interfaces
        self.update()

    @Slot()
    async def sendOutputEOS(self):
        await self.output.writeEndOfStream()

    @Slot()
    async def sendImageOutput(self):
        array = np.random.randint(1000, size=(10, 10), dtype=np.uint32)
        self.imageOutput.schema.data.array = array
        self.imageOutput.schema.data.image = array
        await self.imageOutput.writeData()


LOGGER_MAP = "loggermap.xml"


@pytest_asyncio.fixture(scope="module")
async def deviceTest():
    instance = get_running_loop().instance()
    instance.loggerMap = LOGGER_MAP

    starting_dir = os.getcwd()
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    # The cpp server always changes into "var/data" and logger manager
    # places its file into current directory - but we do not want to
    # interfere our test with what a developer had there before.
    loggerMap = "{}/var/data/{}".format(os.environ["KARABO"],
                                        LOGGER_MAP)
    if os.path.exists(loggerMap):
        shutil.move(loggerMap, os.getcwd())

    config = {
        "_deviceId_": "middlelayerDevice",
        "rawchannel": {
            "connectedOutputChannels": ["boundDevice:output2"],
            "onSlowness": "drop"},
        "channel": {"connectedOutputChannels": ["boundDevice:output1"]},
    }
    device = MiddlelayerDevice(config)
    async with AsyncDeviceContext(device=device) as ctx:
        yield ctx

    # Place back logger map if copied
    if os.path.exists(LOGGER_MAP):
        karaboRunDir = "{}/var/data".format(os.environ["KARABO"])
        shutil.copy2(LOGGER_MAP, karaboRunDir)
        os.remove(LOGGER_MAP)

    os.chdir(starting_dir)


@pytest.mark.timeout(90)
@pytest.mark.asyncio(loop_scope="module")
async def test_cross(deviceTest):
    await getDevice("middlelayerDevice")
    serverId = "karabo_cross_bound"
    config = {
        "boundDevice": {
            "classId": "TestDevice", "Logger.priority": "FATAL",
            "middlelayerDevice": "middlelayerDevice",
            "input.connectedOutputChannels":
                ["middlelayerDevice:output", "middlelayerDevice:rawOutput"]
                }
        }
    init = json.dumps(config)
    server = AsyncServerContext(
        serverId, [f"init={init}", "pluginNamespace=karabo.bound_device_test"],
        api="python")

    async with server:
        # And get a new proxy!
        proxy = await getDevice("boundDevice")

        assert proxy.lockedBy.descriptor.displayType == "lockedBy"
        msg = "didn't receive inital value from bound device"
        assert proxy.a == 22.5 * unit.milliampere, msg
        msg = "didn't receive initial value from bound device"
        assert proxy.readonly == 2, msg

        # Basic Interfaces Check AFTER bound device is up
        # -----------------------------------------------
        mdl_proxy = await getDevice("middlelayerDevice")
        assert mdl_proxy.lockedBy.descriptor.displayType == "lockedBy"

        with mdl_proxy:
            await setWait(mdl_proxy, boundDevice="boundDevice")
            assert mdl_proxy.boundDevice == "boundDevice"
            await mdl_proxy.retrieveInterfaces()
            capas = mdl_proxy.foundCapabilities.value
            interfaces = mdl_proxy.foundInterfaces.value
            assert capas == Capabilities.PROVIDES_INTERFACES
            interface_detail = (Interfaces.Motor + Interfaces.Camera +
                                Interfaces.Processor +
                                Interfaces.DeviceInstantiator)
            assert interfaces == interface_detail

            def _test_mask(mask, bit):
                return (mask & bit) == bit

            has_interfaces = _test_mask(capas,
                                        Capabilities.PROVIDES_INTERFACES)
            assert has_interfaces

            has_motor = _test_mask(interfaces, Interfaces.Motor)
            assert has_motor
            has_camera = _test_mask(interfaces, Interfaces.Camera)
            assert has_camera
            has_processor = _test_mask(interfaces, Interfaces.Processor)
            assert has_processor
            has_instantiator = _test_mask(interfaces,
                                          Interfaces.DeviceInstantiator)
            assert has_instantiator

        a_desc = type(proxy).a
        assert a_desc.unitSymbol is Unit.AMPERE
        assert a_desc.metricPrefixSymbol is MetricPrefix.MILLI
        assert a_desc.requiredAccessLevel is AccessLevel.EXPERT
        assert a_desc.assignment is Assignment.OPTIONAL
        assert a_desc.displayedName == "parameter a"
        assert a_desc.description == "a's description"
        assert a_desc.defaultValue == 22.5
        assert a_desc.minExc == 22
        assert a_desc.maxExc == 33
        assert a_desc.minInc == 11
        assert a_desc.maxInc == 23
        assert a_desc.allowedStates == {State.INIT, State.UNKNOWN}
        assert a_desc.tags == {"bla", "blub"}

        assert len(proxy.table) == 1
        assert proxy.table[0]["d"] == 5 * unit.kilometer

        with proxy:
            assert proxy.maxSizeSchema == 0
            # Test the maxSize from a vector property!
            # and the reply of a slot.
            reply = await proxy.compareSchema()
            assert proxy.maxSizeSchema == 4
            assert reply == 4

            await proxy.setA()
            msg = "didn't receive change from bound device"
            assert proxy.a == 22.7 * unit.milliampere, msg
            assert repr(proxy.a.timestamp) == "2009-09-01T13:55:22 UTC"
            assert proxy.node.b == 100 * unit.kilometer
            assert repr(proxy.node.b.timestamp) == "2016-06-17T13:55:22 UTC"
            assert proxy.state == State.UNKNOWN
            assert proxy.alarmCondition == AlarmCondition.NONE

            assert proxy.ndarray[0, 1] == 2 * unit.kilometer
            assert proxy.ndarray.shape == (2, 3)
            assert proxy.ndarray.descriptor.unitSymbol is Unit.METER
            prefix = proxy.ndarray.descriptor.metricPrefixSymbol
            assert prefix is MetricPrefix.KILO
            assert proxy.ndarray.dtype.str == "<i8"

            with pytest.raises(ValueError):
                proxy.a = 77
            assert proxy.a == 22.7 * unit.milliampere

            # Following test does not yet work, but in fact the request to
            # change readonly goes over the wire (but shouldn't) and is refused
            # on the other end which sends back an error...
            # with assertRaises(ValueError):  # or KaraboError?
            #    proxy.readonly = 1  # not allowed!
            with pytest.raises(KaraboError):
                await setWait("boundDevice", readonly=1)  # not allowed
            assert proxy.readonly == 2  # unchanged

            def setter():
                proxy.a = 22.3 * unit.milliampere
                assert proxy.a == 22.3 * unit.milliampere

            await background(setter)

            proxy.a = 0.0228 * unit.ampere
            msg = "proxy should set device's, not own value"
            assert not proxy.a == 22.8 * unit.milliampere, msg

            await waitUntil(lambda: proxy.a == 22.8 * unit.milliampere)

            msg = "didn't receive change from bound device"
            assert proxy.a == 22.8 * unit.milliampere, msg

            proxy.table = [(3, "african"), (7, "european")]
            assert len(proxy.table) == 1
            await waitUntilNew(proxy.table)
            assert len(proxy.table) == 2
            assert proxy.table[1]["d"] == 7 * unit.kilometer
            assert proxy.table[0]["s"] == "african"

            await proxy.injectSchema()
            assert proxy.word1 == "Hello"
            assert proxy.word1.descriptor.description == "The word"
            assert proxy.injectedNode.number1 == 1

        await proxy.injectSchema()
        await updateDevice(proxy)
        assert proxy.word2 == "Hello"
        assert proxy.injectedNode.number2 == 2

        await proxy.backfire()
        await sleep(1)  # See FIXME in backfire slot of the bound device

        device = deviceTest["device"]
        assert device.value == 99
        assert device.marker

        ####################################
        # check slotGetTime of bound device
        ####################################

        hTime = await call("boundDevice", "slotGetTime", Hash())
        assert hTime is not None
        assert hTime["time"]
        sec = hTime.getAttributes("time")["sec"]
        frac = hTime.getAttributes("time")["frac"]
        t1 = sec + frac / 1.e18
        # non-zero time, but zero tid
        assert t1 > 0
        assert hTime.getAttributes("time")["tid"] == 0
        # Now send tid info to device (1559600000 is June 3rd, 2019)
        await call("boundDevice", "slotTimeTick",
                   # id sec       frac periodInMicroSec
                   100, 1559600000, 0, 100000)
        # ask again, now non-zero tid and a later point in time
        hTime = await call("boundDevice", "slotGetTime", Hash())
        sec = hTime.getAttributes("time")["sec"]
        frac = hTime.getAttributes("time")["frac"]
        t2 = sec + frac / 1.e18
        assert t2 > t1
        assert hTime.getAttributes("time")["tid"] >= 100

        ####################################
        # pipeline part
        ####################################

        await proxy.send()
        # XXX: This was working without any wait or sleep in jms
        await sleepUntil(lambda: device.channelcount == 1)
        await sleepUntil(lambda: device.rawchannelcount == 1)
        assert device.channelcount == 1
        assert device.rawchannelcount == 1
        assert not isSet(device.channeldata.d)
        assert device.channeldata.s == "hallo"
        assert device.channelmeta.source == "boundDevice:output1"
        assert device.channelmeta.timestamp

        assert device.rawchannelmeta.source == "boundDevice:output2"
        assert device.rawchanneldata["e"] == 5
        assert device.rawchanneldata["s"] == "hallo"
        ndarray_hsh = device.rawchanneldata["ndarray"]
        assert ndarray_hsh is not None
        # the incoming ndarray is of shape (10) and type np.float32
        assert len(ndarray_hsh['data']) == 10 * 4
        image_hsh = device.rawchanneldata["image"]
        assert image_hsh is not None
        # the incoming image is of shape (50, 50) and type np.uint16
        assert len(image_hsh['pixels.data']) == 50 * 50 * 2
        assert device.rawchannelmeta.timestamp

        await proxy.send()
        await sleepUntil(lambda: device.channelcount == 2)
        assert device.channelcount == 2

        await proxy.end()
        # The end of stream is no longer setting the data to None
        assert device.channeldata.s == "hallo"
        assert device.channelmeta.source == "boundDevice:output1"
        assert device.channelcount == 2

        await proxy.send()
        await sleepUntil(lambda: device.channelcount == 3)
        assert device.channelcount == 3
        assert device.channeldata.s == "hallo"
        assert device.channelmeta.source == "boundDevice:output1"

        proxy.output1.connect()
        task = background(waitUntilNew(proxy.output1.schema.s))
        while not task.done():
            # unfortunately, connecting takes time and nobody knows how much
            await proxy.send()
        await task
        assert proxy.output1.schema.s == "hallo"

        with proxy:
            # Reentering a proxy needs a fresh configuration
            await updateDevice(proxy)
            device.output.schema.number = 23
            assert proxy.a == 22.8 * unit.milliampere
            await device.output.writeData()
            await waitUntil(lambda: proxy.a == 23 * unit.mA)

        proxy.output1.connect()
        task = background(waitUntilNew(proxy.output1.schema.s))
        while not task.done():
            # TODO: wait for connection by listening to
            # `input.missingConnections`
            # unfortunately, connecting takes time and
            # nobody knows how much
            await proxy.send()
        await task
        assert proxy.output1.schema.s == "hallo"

        ####################################
        # clean up
        ####################################

        await shutdown(proxy)
        # it takes up to 5 s for the bound device to actually shut down
        await shutdown("boundDevice")
        # Wait a few seconds for the broker message
        await sleep(2)
        assert device.channelclose == "boundDevice:output1"


@pytest.mark.timeout(90)
@pytest.mark.asyncio(loop_scope="module")
async def test_cross_pipeline(deviceTest):

    serverId = "karabo_cross_bound_2"
    config = {
        "boundDevice2": {
            "classId": "TestDevice", "Logger.priority": "FATAL",
            "middlelayerDevice": "middlelayerDevice",
            "imageInput.connectedOutputChannels":
                ["middlelayerDevice:imageOutput"],
            "imageInput.onSlowness": "wait",
            "imagePath": "data.image",
            "ndarrayPath": "data.array",
            }
        }
    init = json.dumps(config)
    server = AsyncServerContext(
        serverId, [f"init={init}", "pluginNamespace=karabo.bound_device_test"],
        api="python")

    async with server:

        # it takes typically 2 s for the bound device to start
        bound_proxy = await getDevice("boundDevice2")
        mdl_proxy = await getDevice("middlelayerDevice")

        ###################################################
        # Send data from middlelayer and receive from bound
        ###################################################

        with bound_proxy:
            try:
                await wait_for(
                    waitUntil(
                        lambda: bound_proxy.imageInput.missingConnections == []), # noqa
                    timeout=15)
            except TimeoutError:
                assert False, "bound proxy did not connect in time"

            task = background(waitUntilNew(bound_proxy.imagesReceived))
            await mdl_proxy.sendImageOutput()
            try:
                await wait_for(task, timeout=15)
                assert task.done()
            except (AssertionError, TimeoutError):
                msg = ("bound proxy did not update in time. status: "
                       f"{bound_proxy.status}")
                assert False, msg

            assert bound_proxy.imagesReceived > 0
            assert bound_proxy.ndarraysReceived > 0

            channel = PipelineContext("boundDevice2:output1")
            assert channel.size() == 0
            async with channel:
                await wait_for(channel.wait_connected(), timeout=5)
                await bound_proxy.sendMultipleHashes()
                await sleepUntil(lambda: channel.size() == 10,
                                 timeout=30)
                assert channel.size() == 10

                timestamp = None
                for i in range(9):
                    data, meta = await channel.get_data()
                    assert data["e"] == i
                    assert data["s"] == "hallo"
                    assert meta.source == f"boundDevice2:output{i}"
                    # validate different timestamps
                    assert meta.timestamp.timestamp != timestamp
                    timestamp = meta.timestamp.timestamp
                assert channel.size() == 1

            # a single item left in queue got cleaned up
            assert channel.size() == 0

        await shutdown(bound_proxy)


@pytest.mark.timeout(40)
@pytest.mark.asyncio(loop_scope="module")
async def test_history(deviceTest):
    before = datetime.now()
    serverId = "karabo_dataLogger"
    config = {"Karabo_DataLoggerManager_0":  # id as required by `getHistory`
              {"classId": "DataLoggerManager", "flushInterval": 1,
               "fileDataLogger": {"directory": "karaboHistory"},
               "serverList": [serverId]}}
    init = json.dumps(config)
    # Start karabo-cppserver with 'serverId' and init=<json-string>
    server = AsyncServerContext(serverId, [f"init={init}"], api="cpp")
    async with server:    # Wherever we run this test (by hands or in CI)
        # Wait until logger is ready for logging our middlelayer device,
        # i.e. published non-empty timestamp of any logged parameter
        with (await getDevice(
                f"DataLogger-{serverId}")) as logger:
            await updateDevice(logger)
            while True:
                found = False
                for row in logger.lastUpdatesUtc:
                    if (row["deviceId"] == "middlelayerDevice"
                            and row["lastUpdateUtc"]):
                        found = True
                if found:
                    break
                await waitUntilNew(logger.lastUpdatesUtc)

        # Initiate indexing for selected parameters: "value" and "child.number"
        after = datetime.now()

        # The first history request ever fails - but it triggers the
        # creation of index files needed later for reading back.
        # Sometimes this test runs in an environment where these requests
        # are not the first ones, so cannot assertRaise.
        try:
            await getHistory("middlelayerDevice.value",
                             before.isoformat(), after.isoformat())
        except KaraboError as e:
            assert "first history request" in str(e)
        try:
            await getHistory("middlelayerDevice.child.number",
                             before.isoformat(), after.isoformat())
        except KaraboError as e:
            assert "first history request" in str(e)

        # Now write data that is logged (and indexed for reading back!)
        for i in range(5):
            await setWait("middlelayerDevice", "value", i, "child.number", -i)

        device = await getDevice("middlelayerDevice")
        assert device.value == 4

        # Make sure to flush the logger
        await logger.flush()

        after = datetime.now()
        str_history = await getHistory(
            "middlelayerDevice.value", before.isoformat(), after.isoformat())
        proxy_history = await getHistory(
            device.value, before.isoformat(), after.isoformat())

        for hist in str_history, proxy_history:
            # Sort according to timestamp - order is not guaranteed!
            # (E.g. if shortcut communication between logged device and
            #  logger is switched on...)
            hist.sort(key=lambda x: x[0])
            assert [v for _, _, v in hist[-5:]] == list(range(5))

        node_history = await getHistory(
            "middlelayerDevice.child.number", before.isoformat(),
            after.isoformat())
        node_proxy_history = await getHistory(
            device.child.number, before.isoformat(), after.isoformat())

        for hist in node_history, node_proxy_history:
            # Sort needed - see above.
            hist.sort(key=lambda x: x[0])
            assert [-v for _, _, v in hist[-5:]] == list(range(5))
