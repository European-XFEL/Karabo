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
from asyncio import TimeoutError, get_event_loop, sleep

import numpy as np
import pytest
import pytest_asyncio

from karabo import __version__ as karaboVersion
from karabo.common.states import State
from karabo.middlelayer import (
    AccessMode, KaraboError, background, getDevice, getOutputChannelInfo,
    setWait, sleep as mdlsleep, waitUntil, waitWhile)
from karabo.middlelayer.device import Device
from karabo.middlelayer.device_client import (
    call, get_instance, getProperties, getSchema)
from karabo.middlelayer.pipeline import InputChannel, OutputChannel
from karabo.middlelayer.signalslot import slot
from karabo.middlelayer.testing import AsyncDeviceContext, run_test, sleepUntil
from karabo.middlelayer.utils import AsyncTimer, get_property, set_property
from karabo.native import (
    Bool, Configurable, Float, Hash, Int32, Node, Slot, Timestamp, VectorHash)


class RowSchema(Configurable):
    x = Float(defaultValue=1.0)
    y = Float(defaultValue=1.0)


class ChannelData(Configurable):
    """The properties for the pipeline channels"""
    floatProperty = Float(defaultValue=10.0,
                          displayedName="Float")


class PropertyData(Configurable):
    floatProperty = Float(defaultValue=10.0,
                          displayedName="Float",
                          allowedStates=[State.ON])
    intProperty = Int32(defaultValue=7,
                        displayedName="Integer",
                        allowedStates=[State.ERROR])


class ChannelNode(Configurable):
    """A node with an output channel"""
    output = OutputChannel()


class NodeWithSlot(Configurable):
    """A slot in a node"""
    @Slot()
    async def pressMe(self):
        return 6


class MyDevice(Device):
    __version__ = "1.2.3"

    preInitError = Bool(
        defaultValue=False,
        description="Decide if this device's preInitialization should raise")

    initError = Bool(
        defaultValue=False,
        description="Decide if this device should throw an error in "
                    "onInitialization")

    isDown = Bool(
        defaultValue=False,
        description="Indicator if this device went down. This is set"
                    "on slotKillDevice")

    integer = Int32(
        defaultValue=0,
        minInc=0,
        maxInc=10)

    counter = Int32(
        defaultValue=0,
        minInc=0,
        maxInc=2000)

    # an output channel without schema
    output = OutputChannel()
    # output channel with schema
    dataOutput = OutputChannel(ChannelData)
    # output channel in a node
    nodeOutput = Node(ChannelNode)

    noded = Node(PropertyData)

    @InputChannel()
    async def input(self, data, meta):
        """The input channel with data and meta
        """

    table = VectorHash(
        rows=RowSchema,
        displayedName="Table",
        defaultValue=[Hash("x", 2.0, "y", 5.6),
                      Hash("x", 1.0, "y", 1.6)],
        accessMode=AccessMode.RECONFIGURABLE)

    @Slot(displayedName="Increase", allowedStates=[State.ON])
    async def increaseCounter(self):
        self.state = State.ACQUIRING
        self.counter = self.counter.value + 1
        background(self._state_sleep)

    async def _state_sleep(self):
        await sleep(0.2)
        self.state = State.ON

    async def set_state(self, state):
        self.state = state

    @Slot(displayedName="start")
    async def start(self):
        return 5

    async def preInitialization(self):
        if self.preInitError:
            raise RuntimeError("I am going down")

    async def onInitialization(self):
        self.state = State.ON
        if self.initError:
            raise RuntimeError("Status must not be empty")

    async def slotKillDevice(self):
        self.isDown = True
        return await super().slotKillDevice()

    async def setState(self, state):
        self.state = state

    nodeWithSlot = Node(NodeWithSlot)


@pytest_asyncio.fixture(scope="module")
@pytest.mark.asyncio
async def deviceTest():
    myDevice = MyDevice(dict(_deviceId_="MyDevice"))
    async with AsyncDeviceContext(myDevice=myDevice) as ctx:
        yield ctx


@pytest.mark.timeout(30)
@run_test
def test_device_version(deviceTest):
    myDevice = deviceTest["myDevice"]
    expected = "karabo-1.2.3"
    assert myDevice.classVersion == expected
    assert myDevice.karaboVersion == karaboVersion


@pytest.mark.timeout(30)
@run_test
def test_output_names(deviceTest):
    myDevice = deviceTest["myDevice"]
    names = myDevice.slotGetOutputChannelNames()
    expected = ["dataOutput", "nodeOutput.output", "output"]
    assert names == expected


@pytest.mark.timeout(30)
@run_test
def test_displayType_state(deviceTest):
    myDevice = deviceTest["myDevice"]
    assert myDevice.state.descriptor.displayType == "State"


@pytest.mark.timeout(30)
@run_test
def test_displayType_output(deviceTest):
    myDevice = deviceTest["myDevice"]
    assert myDevice.output.displayType == "OutputChannel"
    assert myDevice.dataOutput.displayType == "OutputChannel"


@pytest.mark.timeout(30)
@run_test
def test_displayType_input(deviceTest):
    myDevice = deviceTest["myDevice"]
    assert myDevice.input.displayType, "InputChannel"


@pytest.mark.timeout(30)
@run_test
def test_displayType_lockedBy(deviceTest):
    myDevice = deviceTest["myDevice"]
    assert myDevice.lockedBy.descriptor.displayType == "lockedBy"


@pytest.mark.timeout(30)
@run_test
def test_classId_output(deviceTest):
    myDevice = deviceTest["myDevice"]
    assert myDevice.output.classId == "OutputChannel"
    assert myDevice.dataOutput.classId == "OutputChannel"


@pytest.mark.timeout(30)
@run_test
def test_classId_input(deviceTest):
    myDevice = deviceTest["myDevice"]
    assert myDevice.input.classId == "InputChannel"


@pytest.mark.timeout(30)
@run_test
async def test_state_dependent_schema(deviceTest):
    myDevice = deviceTest["myDevice"]
    schema_before = await getSchema("MyDevice", onlyCurrentState=False)
    # The notation `before` refers to the device in state ON.
    with (await getDevice("MyDevice")) as d:
        # Set a transient state
        await myDevice.set_state(State.ACQUIRING)
        proxy_before_schema = await getSchema(d, onlyCurrentState=False)
        assert schema_before.hash.paths() == proxy_before_schema.hash.paths()
        schema_after = await getSchema("MyDevice", onlyCurrentState=True)
        p_after_schema = await getSchema(d, onlyCurrentState=True)
        # Back to State.ON
        await myDevice.set_state(State.ON)
        assert proxy_before_schema.hash.paths() != p_after_schema.hash.paths()
        assert schema_before.hash.paths() != schema_after.hash.paths()
        # Check slot in schemas before (ON) and after (ACQUIRING)
        assert "increaseCounter" in schema_before.hash
        assert "increaseCounter" not in schema_after.hash
        assert "increaseCounter" in proxy_before_schema.hash
        assert "increaseCounter" not in p_after_schema.hash


@pytest.mark.timeout(30)
@run_test
async def test_classId_slot(deviceTest):
    s = await getSchema("MyDevice")
    assert s.hash.hasAttribute("start", "classId")
    assert s.hash.getAttribute("start", "classId") == "Slot"


@pytest.mark.timeout(30)
@run_test
async def test_classId_node(deviceTest):
    s = await getSchema("MyDevice")
    assert not s.hash.hasAttribute("nodeOutput", "classId")


@pytest.mark.timeout(30)
@run_test
async def test_send_raw(deviceTest):
    myDevice = deviceTest["myDevice"]
    hsh = Hash("Itchy", 10)
    assert myDevice.output.schema is None
    await myDevice.output.writeRawData(hsh)

    # provoke attribute error because we don"t have a schema
    with pytest.raises(AttributeError):
        await myDevice.output.writeData(hsh)


@pytest.mark.timeout(30)
@run_test
def test_send_raw_no_wait(deviceTest):
    myDevice = deviceTest["myDevice"]
    hsh = Hash("Scratchy", 20)
    assert myDevice.output.schema is None
    myDevice.output.writeRawDataNoWait(hsh)

    # provoke attribute error because we don"t have a schema
    with pytest.raises(AttributeError):
        myDevice.output.writeDataNoWait(hsh)


@pytest.mark.timeout(30)
@run_test
async def test_lastCommand(deviceTest):
    myDevice = deviceTest["myDevice"]
    assert myDevice.lastCommand == ""
    leadId = get_event_loop().instance().deviceId
    d = await getDevice("MyDevice")
    with d:
        await d.nodeWithSlot.pressMe()
        assert myDevice.lastCommand == f"nodeWithSlot.pressMe <- {leadId}"
        await d.start()
    assert myDevice.lastCommand == f"start <- {leadId}"
    await getSchema("MyDevice")
    assert myDevice.lastCommand == f"start <- {leadId}"
    with d:
        current = d.integer
        await setWait(d, integer=2)
        assert d.lastCommand == f"slotReconfigure <- {leadId}"
        await setWait(d, integer=current)


@pytest.mark.timeout(30)
@run_test
async def test_two_calls_concurrent(deviceTest):
    myDevice = deviceTest["myDevice"]
    assert myDevice.counter == 0
    with (await getDevice("MyDevice")) as d:
        await d.increaseCounter()
        await waitUntil(lambda: d.state != State.ON)
        await waitWhile(lambda: d.state == State.ACQUIRING)
        assert myDevice.counter == 1
        # Concurrent slot calls, one will return due to state block
        myDevice.callNoWait("MyDevice", "increaseCounter")
        myDevice.callNoWait("MyDevice", "increaseCounter")
        await waitUntil(lambda: d.state != State.ON)
        await waitWhile(lambda: d.state == State.ACQUIRING)
        assert myDevice.counter == 2


@pytest.mark.timeout(30)
@run_test
async def test_clear_table_external(deviceTest):
    with (await getDevice("MyDevice")) as d:
        dtype = d.table.descriptor.dtype
        current_value = np.array((2.0, 5.6), dtype=dtype)
        assert d.table[0].value == current_value

        # pop the first item and compare
        check_value = d.table.pop(0)
        assert check_value.value == current_value

        # The former second is now first
        current_value = np.array((1.0, 1.6), dtype=dtype)
        assert d.table[0].value == current_value
        assert len(d.table.value) == 1

        # clear the value on the device side. The values are popped.
        d.table.clear()
        empty_table = np.array([], dtype=dtype)
        success = np.array_equal(d.table.value, empty_table)
        assert success


@pytest.mark.timeout(30)
@run_test
async def test_allowed_state_reconfigure_nodes(deviceTest):
    with (await getDevice("MyDevice")) as d:
        assert d.noded.floatProperty == 10.0
        await setWait(d, "noded.floatProperty", 27.0)
        assert d.noded.floatProperty == 27.0
        await setWait(d, "noded.floatProperty", 10.0)
        assert d.noded.floatProperty == 10.0
        with pytest.raises(KaraboError):
            await setWait(d, "noded.intProperty", 22)
        assert d.noded.intProperty == 7


@pytest.mark.timeout(30)
@run_test
def test_slot_verification(deviceTest):
    myDevice = deviceTest["myDevice"]
    assert myDevice.slotHasSlot("increaseCounter") is True
    assert myDevice.slotHasSlot("output") is False
    assert myDevice.slotHasSlot("doesNotExist") is False


@pytest.mark.timeout(30)
@run_test
async def test_instance_info(deviceTest):
    device = deviceTest["myDevice"]
    assert device._sigslot.info["status"] == "ok"
    await device.setState(State.ERROR)
    await sleep(0)
    assert device._sigslot.info["status"] == "error"
    await device.setState(State.UNKNOWN)
    await sleep(0)
    assert device._sigslot.info["status"] == "unknown"
    await device.setState(State.ON)
    await sleep(0)
    assert device._sigslot.info["status"] == "ok"


@pytest.mark.timeout(30)
@run_test
async def test_output_information(deviceTest):
    device = deviceTest["myDevice"]
    # Second argument processId is not used in MDL
    h = device.slotGetOutputChannelInformation(
        Hash("channelId", "output"))
    assert h["success"]
    data = h["info"]
    assert data["connectionType"] == "tcp"
    assert data["memoryLocation"] == "remote"
    assert isinstance(data["port"], np.uint32)

    info = await getOutputChannelInfo(device.deviceId, "output")
    info = info["info"]
    assert info["connectionType"] == "tcp"
    assert info["memoryLocation"] == "remote"
    assert isinstance(info["port"], np.uint32)
    assert info["port"] == data["port"]

    h = device.slotGetOutputChannelInformation(
        Hash("channelId", "doesNotExist"))
    assert not h["success"]
    assert h["info"] == Hash()


@pytest.mark.timeout(30)
@run_test
async def test_get_property_set_property(deviceTest):
    myDevice = deviceTest["myDevice"]

    prop = get_property(myDevice, "integer")
    set_property(myDevice, "integer", 0)
    assert prop == 0
    set_property(myDevice, "noded.floatProperty", 10)
    prop = get_property(myDevice, "noded.floatProperty")
    assert prop == 10
    set_property(myDevice, "noded.floatProperty", 15)
    assert prop is not myDevice.noded.floatProperty
    prop = get_property(myDevice, "noded.floatProperty")
    assert prop == 15
    with pytest.raises(AttributeError):
        get_property(myDevice, "noded.notthere")
    with pytest.raises(AttributeError):
        get_property(myDevice, "notthere")
    with pytest.raises(AttributeError):
        set_property(myDevice, "notthere", 25)
    with pytest.raises(AttributeError):
        set_property(myDevice, "noded.notthere", 2)


@pytest.mark.timeout(30)
@run_test
async def test_get_properties_hash(deviceTest):
    myDevice = deviceTest["myDevice"]
    myDevice.integer = 0

    # Remote slot call via slotGetConfigurationSlice
    deviceId = myDevice.deviceId
    h = await getProperties(deviceId, "integer")
    assert isinstance(h, Hash)
    assert h["integer"] == 0
    assert len(h.paths(intermediate=False)) == 1

    slot = "slotGetConfigurationSlice"
    r = await get_instance().call(
        deviceId, slot, Hash("paths", ["integer"]))
    assert r.fullyEqual(h)

    # And we have the timestamp
    attrs = h["integer", ...]
    assert len(attrs) == 3
    assert "tid" in attrs
    assert "frac" in attrs
    assert "sec" in attrs

    set_property(myDevice, "noded.floatProperty", 15)
    h = await getProperties(deviceId,
                            ["integer", "noded.floatProperty"])
    assert h["integer"] == 0
    assert h["noded.floatProperty"] == 15
    assert len(h.paths(intermediate=True)) == 3
    assert len(h.paths(intermediate=False)) == 2

    # exception for failing properties
    with pytest.raises(KaraboError):
        h = await getProperties(deviceId, ["integer", "noded.notthere"])

    # KaraboError for failing garbage
    for fail in (
        [False, True],
        [Hash()],
        [1, 2],
        [1.78],
    ):
        with pytest.raises(KaraboError):
            h = await getProperties(deviceId, fail)

    # Working with proxy
    async with getDevice(myDevice.deviceId) as proxy:
        h = await getProperties(proxy, ["integer", "noded.floatProperty"])
        assert h["integer"] == 0
        assert h["noded.floatProperty"] == 15
        assert len(h.paths(intermediate=False)) == 2


@pytest.mark.timeout(30)
@run_test
async def test_slot_time(deviceTest):
    h = await call("MyDevice", "slotGetTime")
    assert h is not None
    assert h["time"]
    assert h.getAttributes("time")["sec"] > 0
    assert h.getAttributes("time")["frac"] > 0
    assert h.getAttributes("time")["tid"] == 0
    timestamp_first = Timestamp.fromHashAttributes(h["time", ...])
    h = await call("MyDevice", "slotGetTime")
    timestamp_second = Timestamp.fromHashAttributes(h["time", ...])
    assert timestamp_second > timestamp_first


@pytest.mark.timeout(30)
@run_test
async def test_initialization(deviceTest):
    # this will not throw in the preInitialization
    bob = MyDevice({"_deviceId_": "bob", "preInitError": True})
    # fails in preInitialization -> instantiation will throw
    with pytest.raises(RuntimeError):
        await bob.startInstance()
    assert bob.isDown
    assert not bob.is_initialized

    # this will not throw in the onInitialization
    alice = MyDevice({"_deviceId_": "alice", "initError": True})
    await alice.startInstance()
    # for backward compatibility, we kill the device on initError.
    assert alice.isDown
    assert not alice.is_initialized

    # this will succeed
    charlie = MyDevice({"_deviceId_": "charlie"})
    await charlie.startInstance()
    assert not charlie.isDown
    assert charlie.is_initialized
    await charlie.slotKillDevice()


@pytest.mark.timeout(30)
@run_test
async def test_preinitialization(deviceTest):
    """Test the preinitialization taking too long"""
    class ASlowInitDevice(Device):

        async def preInitialization(self):
            await sleep(10)

        async def slotKillDevice(self):
            self.isDown = True
            await super().slotKillDevice()

    charlie = ASlowInitDevice({"_deviceId_": "charlie"})
    with pytest.raises(TimeoutError):
        await charlie.startInstance()
    assert charlie.isDown
    assert not charlie.is_initialized

    class SlowInitDevice(Device):

        def preInitialization(self):
            mdlsleep(10)

        async def slotKillDevice(self):
            self.isDown = True
            await super().slotKillDevice()

    echo = SlowInitDevice({"_deviceId_": "echo"})
    with pytest.raises(TimeoutError):
        await echo.startInstance()
    assert echo.isDown
    assert not echo.is_initialized


@pytest.mark.timeout(30)
@run_test
async def test_atimer_destruct(deviceTest):

    global counter
    counter = 0

    class TimerDevice(Device):
        __version__ = "1.2.3"
        killed = False

        async def onInitialization(self):
            self.timer = AsyncTimer(self.timer_callback, 0.1)
            self.timer.start()
            get_event_loop().something_changed()

        async def timer_callback(self):
            global counter
            counter += 1

        async def slotKillDevice(self, message=None):
            ret = await super().slotKillDevice(message)
            self.killed = True
            return ret

        slotKillDevice = slot(slotKillDevice, passMessage=True)

    device = TimerDevice({"_deviceId_": "timerDeviceTest"})
    await device.startInstance()
    assert counter == 0
    await waitUntil(lambda: device.is_initialized)
    await sleep(0.2)
    assert counter > 0
    await device.slotKillDevice()
    old_counter = counter
    await sleep(0.2)
    assert counter == old_counter

    counter = 0
    device = TimerDevice({"_deviceId_": "timerDeviceTest"})
    await device.startInstance()
    await waitUntil(lambda: device.is_initialized)
    await sleep(0.2)
    assert counter > 0
    device.__del__()
    await sleepUntil(lambda: device.killed, 5)
    old_counter = counter
    await sleep(0.2)
    assert counter == old_counter

    counter = 0
    device = TimerDevice({"_deviceId_": "timerDeviceTest"})
    await device.startInstance()
    await waitUntil(lambda: device.is_initialized)
    await device.slotKillDevice()
    # Between start and shutdown there might have been a tick,
    # but the timer gracefully stopped
    assert counter <= 1
    assert not device.timer.is_running()
