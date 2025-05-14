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
import sys
import time
import weakref
from asyncio import (
    Future, TimeoutError, ensure_future, get_running_loop, wait_for)

import pytest
import pytest_asyncio

from karabo.common.states import State
from karabo.middlelayer.device import Device
from karabo.middlelayer.device_client import (
    Queue, call, callNoWait, connectDevice, executeNoWait, getConfiguration,
    getDevice, getInstanceInfo, getProperties, getSchema, lock, setNoWait,
    setWait, updateDevice, waitUntil, waitUntilNew, waitWhile)
from karabo.middlelayer.macro import Macro, MacroSlot
from karabo.middlelayer.output import KaraboStream
from karabo.middlelayer.pipeline import OutputChannel, PipelineContext
from karabo.middlelayer.signalslot import slot
from karabo.middlelayer.synchronization import background, sleep
from karabo.middlelayer.testing import AsyncDeviceContext, run_test, sleepUntil
from karabo.native import (
    AccessMode, Configurable, Hash, Int32 as Int, KaraboError, Node, Slot)

FLAKY_MAX_RUNS = 5
FLAKY_MIN_PASSES = 3
TIMEOUT_LOGS = 3


class Superslot(Slot):
    async def method(self, device):
        device.value = 22


class MyNode(Configurable):
    value = Int(defaultValue=7)
    counter = Int(defaultValue=-1)


def get_channel_node(displayed_name=""):
    class ChannelNode(Configurable):
        data = Int(
            displayedName=displayed_name,
            defaultValue=0)

    return ChannelNode


class RemotePipeline(Device):
    running = False

    output = OutputChannel(
        get_channel_node(),
        accessMode=AccessMode.READONLY)

    @Slot()
    async def startSending(self):
        background(self._keep_sending())

    @Slot()
    async def stopSending(self):
        self.running = False

    async def _keep_sending(self):
        self.running = True
        while self.running:
            await self.sendData()
            await sleep(0.1)

    @Slot()
    async def sendData(self):
        self.output.schema.data = self.output.schema.data.value + 1
        await self.output.writeData()


class Remote(Device):
    status_fut = []

    value = Int(
        defaultValue=7,
        accessMode=AccessMode.RECONFIGURABLE)

    counter = Int(defaultValue=-1)
    deep = Node(MyNode)

    def get_future(self):
        fut = Future()
        self.status_fut.append(fut)
        return fut

    @Int()
    def other(self, value):
        self.value = value

    @Slot()
    async def doit(self):
        self.done = True

    @Slot()
    async def changeit(self):
        self.value -= 4

    @Slot()
    async def start(self):
        self.status = "Started"

    @Slot()
    async def stop(self):
        self.status = "Stopped"

    @slot
    def setStatus(self, token):
        self.status = token
        for future in self.status_fut:
            future.set_result(token)
        self.status_fut = []
        return token

    @Slot()
    async def count(self):
        self.state = State.INCREASING
        for i in range(30):
            self.counter = i
            await sleep(0.05)
        self.state = State.UNKNOWN

    @Slot()
    async def call_local(self):
        with (await getDevice("local")) as leet:
            await leet.remotecalls()

    @Slot()
    async def call_local_another(self):
        with (await getDevice("local")) as leet:
            await leet.remotecallsanother()

    generic = Superslot()


class Local(Macro):
    @Slot()
    def remotecalls(self):
        self.nowslot = self.currentSlot
        self.nowstate = self.state
        print("superpuper")
        print("hero")

    @Slot()
    def remotecallsanother(self):
        self.nowslot = self.currentSlot
        self.nowstate = self.state
        print("superpuper", end="nohero")

    @Slot()
    def selfcall(self):
        self.marker = 77

    @Slot()
    def error(self):
        raise RuntimeError

    @Slot()
    def error_in_error(self):
        raise RuntimeError

    def onException(self, slot, exc, tb):
        self.exc_slot = slot
        self.exception = exc
        self.traceback = tb
        if self.exc_slot is Local.error_in_error:
            raise RuntimeError
        else:
            with getDevice("remote") as d:
                d.doit()

    def onCancelled(self, slot):
        self.cancelled_slot = slot

    @Slot()
    def sleepalot(self):
        non_karabo_sleep = time.sleep
        karabo_sleep = sleep
        self.slept_count = 0

        non_karabo_sleep(0.1)
        self.slept_count = 1
        karabo_sleep(0.01)
        self.slept_count = 2
        karabo_sleep(10)
        self.slept_count = 3

    @Slot()
    async def asyncSleep(self):
        """This is an async Slot. We will test that macros can have async
        slots and that they can be cancelled"""
        await sleep(1)


class LocalAbstract(Macro):
    abstractPassiveState = State.ON
    abstractActiveState = State.MOVING

    @Slot()
    def start(self):
        """Just sleep a bit for state change"""
        sleep(0.1)


class NodeSlow(Configurable):

    @MacroSlot()
    async def startASync(self):
        """Just sleep a bit for state change"""
        await sleep(0.1)

    @MacroSlot()
    def startSync(self):
        """Just sleep a bit for state change"""
        sleep(0.1)


class LocalMacroSlot(Macro):
    exc_slot = None
    exception = None
    traceback = None
    cancelled_slot = None

    node = Node(NodeSlow)

    async def set_state(self, state):
        self.state = state

    @MacroSlot(activeState=State.MOVING, passiveState=State.ON)
    async def startCustom(self):
        """Just sleep a bit for state change"""
        await sleep(0.1)

    @MacroSlot()
    def startSync(self):
        """Just sleep a bit for state change"""
        sleep(0.1)

    @MacroSlot()
    async def startASync(self):
        """Just sleep a bit for state change"""
        await sleep(0.1)

    @MacroSlot()
    def error(self):
        raise RuntimeError

    @MacroSlot()
    def error_in_error(self):
        raise RuntimeError

    def onException(self, slot, exc, tb):
        self.exc_slot = slot
        self.exception = exc
        self.traceback = tb
        if self.exc_slot is LocalMacroSlot.error_in_error:
            raise RuntimeError

    def onCancelled(self, slot):
        self.cancelled_slot = slot


@pytest_asyncio.fixture(scope="module")
@pytest.mark.asyncio
async def deviceTest():
    local = Local(_deviceId_="local", project="test", module="test",
                  may_start_thread=False)
    remote = Remote(dict(_deviceId_="remote"))
    remotePipeline = RemotePipeline(dict(_deviceId_="piperemote"))
    localMacro = LocalMacroSlot(_deviceId_="localMacroSlot",
                                project="no", module="test")
    waitUntilRemote = Remote(dict(_deviceId_="remotewaituntil"))
    get_running_loop().lead = local
    async with AsyncDeviceContext(
            remote=remote, local=local, waitUntilRemote=waitUntilRemote,
            localMacro=localMacro, remotePipeline=remotePipeline) as ctx:
        yield ctx


@pytest.mark.timeout(30)
@run_test
async def test_macro_instance_info(deviceTest):
    info = await getInstanceInfo("local")
    assert info["status"] == "ok"
    assert info["project"] == "test"
    assert info["type"] == "macro"
    assert deviceTest["local"].state == State.PASSIVE


@pytest.mark.timeout(30)
@run_test
async def test_macro_slotter_custom(deviceTest):
    """Test that we can customize the state machine"""
    localMacro = LocalMacroSlot(_deviceId_="localCustomSlot",
                                project="no", module="test")
    await localMacro.startInstance()
    await waitUntil(lambda: localMacro.is_initialized)
    try:
        with await getDevice("localCustomSlot") as d:
            await sleep(1)
            await localMacro.set_state(State.ON)
            for _ in range(4):
                await waitUntil(lambda: d.state == State.ON)
                assert d.state == State.ON
                await d.startCustom()
                assert d.state == State.MOVING
    finally:
        await localMacro.set_state(State.PASSIVE)
        await localMacro.slotKillDevice()


@pytest.mark.timeout(30)
@run_test
def test_get_properties_hash(deviceTest):
    """Test that we can get properties via slotGetConfigurationSlice"""
    h = getProperties("remote", ["state"])
    assert h["state"] == "UNKNOWN"
    attrs = h["state", ...]
    assert "sec" in attrs
    assert "frac" in attrs
    assert "tid" in attrs


@pytest.mark.timeout(30)
@run_test
def test_macro_pipeline_context(deviceTest):
    """Test the sync operation of a pipeline context channel

    Note: The reconnection of the context is tested in the remote
    pipeline test.
    """
    proxy = getDevice("piperemote")
    proxy.startSending()
    channel = PipelineContext("piperemote:output")
    data = None
    with channel:
        channel.wait_connected()
        data = channel.get_data()
        assert data is not None
        assert channel.is_alive()
        proxy.stopSending()
    assert not channel.is_alive()


@pytest.mark.timeout(30)
@run_test
def test_macro_slotter_sync(deviceTest):
    """Test the execution of MacroSlots in sync case"""
    localMacro = deviceTest["localMacro"]
    with getDevice("localMacroSlot") as d:
        # 1.1
        assert d.state == State.PASSIVE
        d.startSync()
        assert d.state == State.ACTIVE
        waitUntil(lambda: d.state == State.PASSIVE)
        assert d.state == State.PASSIVE

        # 1.2 Cancel the sync macro slot
        d.startSync()
        assert d.state == State.ACTIVE
        d.cancel()
        waitUntil(lambda: d.state == State.PASSIVE)
        assert d.state == State.PASSIVE
        slot = LocalMacroSlot.startSync
        sleepUntil(lambda: localMacro.cancelled_slot is slot)
        assert localMacro.cancelled_slot is slot

        localMacro.cancelled_slot = None

        # 2.1 Nodes
        assert d.state == State.PASSIVE
        d.node.startSync()
        assert d.state == State.ACTIVE
        waitUntil(lambda: d.state == State.PASSIVE)
        assert d.state == State.PASSIVE

        # 2.2 Cancel the sync macro slot
        d.node.startSync()
        assert d.state == State.ACTIVE
        d.cancel()
        waitUntil(lambda: d.state == State.PASSIVE)
        assert d.state == State.PASSIVE
        slot = LocalMacroSlot.node.cls.startSync
        sleepUntil(lambda: localMacro.cancelled_slot is slot)
        assert localMacro.cancelled_slot is slot

        localMacro.cancelled_slot = None


@pytest.mark.timeout(30)
@run_test
async def test_macro_slotter_async(deviceTest):
    """Test the execution of MacroSlots in async case"""
    localMacro = deviceTest["localMacro"]
    with await getDevice("localMacroSlot") as d:
        # 1.1
        assert d.state == State.PASSIVE
        await d.startASync()
        assert d.state == State.ACTIVE
        await waitUntil(lambda: d.state == State.PASSIVE)
        assert d.state == State.PASSIVE

        # 1.2 Cancel the async macro slot
        await d.startASync()
        assert d.state == State.ACTIVE
        await d.cancel()
        assert localMacro.cancelled_slot is LocalMacroSlot.startASync
        await waitUntil(lambda: d.state == State.PASSIVE)
        assert d.state == State.PASSIVE

        localMacro.cancelled_slot = None

        # 2.1 Nodes
        assert d.state == State.PASSIVE
        await d.node.startASync()
        assert d.state == State.ACTIVE
        await waitUntil(lambda: d.state == State.PASSIVE)
        assert d.state == State.PASSIVE

        # 2.2 Cancel the sync macro slot
        await d.node.startASync()
        assert d.state == State.ACTIVE
        await d.cancel()
        assert localMacro.cancelled_slot is LocalMacroSlot.node.cls.startASync
        await waitUntil(lambda: d.state == State.PASSIVE)
        assert d.state == State.PASSIVE
        localMacro.cancelled_slot = None


@pytest.mark.timeout(30)
@run_test
async def test_macro_slotter_error(deviceTest, caplog):
    """test that errors are properly logged and error functions called"""
    localMacro = deviceTest["localMacro"]
    with caplog.at_level("ERROR", logger="localMacroSlot"), \
            (await getDevice("localMacroSlot")) as d:
        await d.error()
        await sleep(TIMEOUT_LOGS)
        assert len(caplog.records)

    assert localMacro.exc_slot is LocalMacroSlot.error
    assert isinstance(localMacro.exception, RuntimeError)
    assert localMacro.traceback is not None
    localMacro.exc_slot = None
    localMacro.exception = None
    localMacro.traceback = None


@pytest.mark.timeout(30)
@run_test
async def test_macro_slotter_error_in_error(deviceTest, caplog):
    """test errors in error handlers are logged in a macro slot"""
    localMacro = deviceTest["localMacro"]
    with caplog.at_level("ERROR", logger="localMacroSlot"), \
            (await getDevice("localMacroSlot")) as d:
        await d.error_in_error()
        await sleep(TIMEOUT_LOGS)
    assert caplog.records[-1].msg == "error in error handler"
    assert localMacro.exc_slot == LocalMacroSlot.error_in_error
    assert isinstance(localMacro.exception, RuntimeError)
    assert localMacro.traceback is not None
    localMacro.exc_slot = None
    localMacro.exception = None
    localMacro.traceback = None


@pytest.mark.timeout(30)
@run_test
def test_execute(deviceTest):
    """test the execution of remote commands"""
    remote = deviceTest["remote"]
    remote.done = False
    with getDevice("remote") as d:
        d.doit()
    assert remote.done


@pytest.mark.timeout(30)
@run_test
async def test_destruct_macro_timer():
    macro = Local(_deviceId_="macrodestruct", project="timer",
                  module="timer")
    await macro.startInstance()
    await sleepUntil(lambda: macro.is_initialized is True)
    assert macro.stackTimer.is_running()
    await macro.slotKillDevice()
    assert not macro.stackTimer.is_running()
    assert macro.stackTimer.loop is None


@pytest.mark.timeout(30)
@run_test
def test_change(deviceTest):
    """test that changes on a remote device reach the macro"""
    remote = deviceTest["remote"]
    remote.value = 7
    with getDevice("remote") as d:
        d.changeit()
    assert remote.value == 3


@pytest.mark.timeout(30)
@run_test
def test_disconnect(deviceTest):
    """check that we don't get updates when we're not connected"""
    d = getDevice("remote")  # proxy is not connected after 5 seconds
    sleep(5)
    executeNoWait(d, "count")
    # Total count is 30 times for 0.05 seconds each -> 1.5 seconds
    time.sleep(0.1)
    assert d.counter == -1
    with updateDevice(d):
        # We are connected now and should receive updates
        waitUntil(lambda: d.counter != -1)
        assert d.counter != -1
        time.sleep(0.1)
        assert d.counter != 29
    # Sleep after exit to let signals propagate the disconnect
    time.sleep(0.2)
    last = d.counter
    time.sleep(0.1)
    assert last == d.counter
    assert d.counter != 29
    with d:
        updateDevice(d)
        waitUntil(lambda: d.counter == 29)
    assert d.counter == 29


@pytest.mark.timeout(30)
@run_test
def test_set(deviceTest):
    """test setting of parameters on a remote device"""
    remote = deviceTest["remote"]
    remote.value = 7
    with getDevice("remote") as d:
        assert d.value == 7
        d.value = 10
        time.sleep(0.1)
        assert d.value == 10
        d.changeit()
        assert d.value == 6


@pytest.mark.timeout(30)
@run_test
def test_generic(deviceTest):
    """call a generic slot"""
    remote = deviceTest["remote"]
    remote.value = 7
    d = getDevice("remote")
    d.generic()
    assert remote.value == 22


@pytest.mark.timeout(30)
@run_test
def test_other(deviceTest):
    """test properties with special setters"""
    remote = deviceTest["remote"]
    remote.value = 7
    with getDevice("remote") as d:
        d.other = 102
    time.sleep(0.1)
    assert remote.value == 102


@pytest.mark.timeout(30)
@run_test
def test_selfcall(deviceTest):
    """test that slots can be called like normal methods"""
    local = deviceTest["local"]
    local.selfcall()
    assert local.marker == 77


@pytest.mark.timeout(30)
@run_test
def test_setwait(deviceTest):
    """test the setWait function"""
    remote = deviceTest["remote"]

    d = getDevice("remote")
    setWait(d, value=200, counter=300)

    assert remote.value == 200
    assert remote.counter == 300

    # test the setWait function with args
    setWait(d, "value", 400, "counter", 400)
    assert remote.value == 400
    assert remote.counter == 400
    with pytest.raises(RuntimeError):
        setWait(d, "value", 1, "counter")

    setWait(d, "deep.value", 400, "deep.counter", 400)
    assert remote.deep.counter == 400
    assert remote.deep.value == 400


@pytest.mark.timeout(30)
@run_test
def test_setnowait(deviceTest):
    """test the setNoWait function"""
    remote = deviceTest["remote"]

    remote.value = 0
    remote.counter = 0
    remote.deep.value = 0
    remote.deep.counter = 0

    d = getDevice("remote")
    setNoWait(d, value=200, counter=300)
    assert remote.value == 0
    assert remote.counter == 0
    time.sleep(0.1)
    assert remote.value == 200
    assert remote.counter == 300

    # test the setNoWait function with args
    setNoWait(d, "value", 0, "counter", 0)
    assert remote.value == 200
    assert remote.counter == 300
    time.sleep(0.1)
    assert remote.value == 0
    assert remote.counter == 0
    with pytest.raises(RuntimeError):
        setNoWait(d, "value", 0, 100)

    setNoWait(d, "deep.value", 400, "deep.counter", 400)
    assert remote.deep.counter == 0
    assert remote.deep.value == 0
    time.sleep(0.1)
    assert remote.deep.counter == 400
    assert remote.deep.value == 400


@pytest.mark.timeout(30)
@run_test
def test_waituntil(deviceTest):
    """test the waitUntil function"""
    with getDevice("remote") as d:
        d.counter = 0
        assert d.counter == 0
        executeNoWait(d, "count")
        waitUntil(lambda: d.counter > 15)
        assert d.counter == 16
        # If the above fails again with d.counter > 16 it may be due to threads
        # used in this synchronous test. Maybe better these asserts:
        # assert d.counter > 15
        # assert d.counter < 30  # count stops at 29
        with pytest.raises(TimeoutError):
            waitUntil(lambda: d.counter > 40, timeout=0.1)
        waitUntil(lambda: d.state == State.UNKNOWN)


@pytest.mark.timeout(30)
@pytest.mark.asyncio
@run_test
async def test_waituntil_async(deviceTest):
    """test the waitUntil function in coroutine"""
    with (await getDevice("remote")) as d:
        await setWait(d, counter=0)
        assert d.counter == 0
        executeNoWait(d, "count")
        await waitUntil(lambda: d.counter > 15)
        assert d.counter == 16
        with pytest.raises(TimeoutError):
            await wait_for(waitUntil(lambda: d.counter > 40), timeout=0.1)
        await waitUntil(lambda: d.state == State.UNKNOWN)


@pytest.mark.timeout(30)
@run_test
def test_waitWhile(deviceTest):
    """test the waitWhile function"""
    with getDevice("remote") as d:
        d.counter = 0
        assert d.counter == 0
        executeNoWait(d, "count")
        waitWhile(lambda: d.counter <= 10)
        assert d.counter >= 10
        waitUntil(lambda: d.state == State.UNKNOWN)


@pytest.mark.timeout(30)
@run_test
def test_waituntilnew(deviceTest):
    """test the waitUntilNew function"""
    with getDevice("remotewaituntil") as d:
        d.counter = 0
        executeNoWait(d, "count")
        waitUntil(lambda: d.counter >= 0)
        i = 0
        while i < 30:
            waitUntilNew(d.counter)
            # ATTENTION:  test changed!!!
            # Here we can guarantee only that i <= d.counter
            # we cannot guarantee strict '==' if ...
            # ... the network (I think!) or something is getting slow
            assert d.counter >= i
            i = d.counter + 1  # next (expected) remote value

        assert 29 == d.counter
        waitUntil(lambda: d.state == State.UNKNOWN)


@pytest.mark.timeout(30)
@run_test
async def test_print(deviceTest):
    """test that macros can print via expected parameters"""
    sys.stdout = KaraboStream(sys.stdout)
    remote = deviceTest["remote"]
    local = deviceTest["local"]
    try:
        assert local.currentSlot == ""
        assert local.state == State.PASSIVE
        await remote.call_local()
        assert local.nowslot == "remotecalls"
        assert local.nowstate == State.ACTIVE
        assert local.currentSlot == ""
        assert local.state == State.PASSIVE
        # Must wait at least 0.5 seconds. Two prints might
        # get bulked
        await sleep(0.5)
        assert local.doNotCompressEvents >= 1
        if local.doNotCompressEvents == 1:
            assert local.print == "superpuper\nhero"
        else:
            assert "superpuper" not in local.print
            assert "hero" in local.print
            assert local.doNotCompressEvents == 2
        await remote.call_local_another()
        # Wait for bulk, but local_another has a print `end` which
        # we can only safely assert
        await sleep(0.5)
        assert local.doNotCompressEvents >= 2
        assert "nohero" in local.print
    finally:
        sys.stdout = sys.stdout.base


@pytest.mark.timeout(30)
@run_test
def test_call(deviceTest):
    """test calling a slot"""
    with getDevice("remote") as d:
        d.value = 0
        assert d.value == 0
        call("remote", "changeit")
        assert d.value == -4


@pytest.mark.timeout(30)
@run_test
def test_getSchema(deviceTest):
    """test calling get Schema"""
    schema_deviceId = getSchema("remote")
    with getDevice("remote") as d:
        schema_proxy = getSchema(d)

    hash_deviceId = schema_deviceId.hash
    hash_proxy = schema_proxy.hash

    assert hash_deviceId.fullyEqual(hash_proxy)
    assert hash_deviceId.paths() == hash_proxy.paths()
    access = AccessMode.RECONFIGURABLE.value
    assert hash_proxy["value", "accessMode"] == access
    assert hash_deviceId["value", "accessMode"] == access

    assert "counter" in hash_deviceId
    assert "counter" in hash_proxy
    assert hash_deviceId["counter", "defaultValue"] == -1
    assert hash_proxy["counter", "defaultValue"] == -1
    assert schema_deviceId.name == schema_proxy.name


@pytest.mark.timeout(30)
@run_test
def test_getConfiguration(deviceTest):
    """test calling get getConfiguration"""
    conf_deviceId = getConfiguration("remote")
    with getDevice("remote") as d:
        conf_proxy = getConfiguration(d)

    assert conf_deviceId.fullyEqual(conf_proxy)
    assert conf_deviceId.paths() == conf_proxy.paths()


@pytest.mark.timeout(30)
@run_test
def test_call_param(deviceTest):
    """test calling a slot with a parameter"""
    remote = deviceTest["remote"]
    h = call("remote", "setStatus", "Token")
    assert remote.status == "Token"
    assert h == "Token"


@pytest.mark.timeout(30)
@run_test
async def test_call_param_nowait(deviceTest):
    """test no wait calling a slot with a parameter"""
    remote = deviceTest["remote"]
    fut = remote.get_future()
    callNoWait("remote", "setStatus", "NewToken")
    await fut
    assert fut.result() == "NewToken"


@pytest.mark.timeout(30)
@run_test
def test_queue(deviceTest):
    """test change queues of properties"""
    with getDevice("remote") as d:
        executeNoWait(d, "count")
        waitUntil(lambda: d.counter == 0)
        q = Queue(d.counter)
        for i in range(1, 30):
            j = q.get()
            assert i == j
            time.sleep(i * 0.002)


@pytest.mark.timeout(30)
@run_test
async def test_error(deviceTest, caplog):
    """test that errors are properly logged and error functions called"""
    local = deviceTest["local"]
    remote = deviceTest["remote"]
    remote.done = False

    with caplog.at_level("ERROR", logger="local"), \
            (await getDevice("local")) as d, \
            pytest.raises(KaraboError):
        await d.error()
    assert remote.done
    assert local.exc_slot is Local.error
    assert isinstance(local.exception, RuntimeError)
    local.traceback.tb_lasti  # check whether that is a traceback
    del local.exc_slot
    del local.exception
    del local.traceback


@pytest.mark.timeout(30)
@run_test
async def test_error_in_error(deviceTest, caplog):
    """test that errors in error handlers are properly logged"""
    local = deviceTest["local"]
    remote = deviceTest["remote"]

    remote.done = False
    with caplog.at_level("ERROR", logger="local"), \
            (await getDevice("local")) as d, \
            pytest.raises(KaraboError):
        await d.error_in_error()
    assert not remote.done
    assert caplog.records[-1].msg == "error in error handler"
    assert local.exc_slot is Local.error_in_error
    assert isinstance(local.exception, RuntimeError)
    local.traceback.tb_lasti  # check whether that is a traceback
    del local.exc_slot
    del local.exception
    del local.traceback


@pytest.mark.timeout(30)
@run_test
async def test_cancel(deviceTest):
    """test proper cancellation of slots

    when a slot is cancelled, test that
        * the onCancelled callback is called (which sets cancelled_slot)
        * the slot got cancelled in the right place (slept has right value)
        * the task gets properly marked done
    Test that for both if the slot is stuck in a karabo function, or not.
    """
    # Rename sleep to make it clean which sleep is being used
    local = deviceTest["local"]
    karabo_sleep = sleep
    d = await getDevice("local")
    with d:
        await updateDevice(d)
        # cancel during time.sleep
        local.cancelled_slot = None
        task = ensure_future(d.sleepalot())
        # wait for a short time so that the macro gets started
        await waitUntil(lambda: d.state == State.ACTIVE)
        # Cancel the macro, which is in a non-interruptable non-karabo
        # sleep.
        # (which is just a place-holder for any other type of
        #  non-interruptable code)
        await d.cancel()
        # Finally, sleep for long enough that the macro runs in to the
        # karabo sleep which CAN be interrupted.
        await sleepUntil(lambda: local.slept_count == 1, 1)
        assert local.slept_count == 1
        await sleepUntil(lambda: task.done(), 0.1)
        assert local.cancelled_slot == Local.sleepalot

        # cancel during karabo.sleep
        local.cancelled_slot = None
        task = ensure_future(d.sleepalot())
        # Sleep for long enough for the macro to end up in the really long
        # sleep at the end
        await karabo_sleep(0.13)
        # Then cancel, while the macro is in that interruptable sleep
        await d.cancel()
        await sleepUntil(lambda: local.slept_count == 2, 1)
        assert local.slept_count == 2
        # Sleep, but only short to show that the task was cancelled
        await sleepUntil(lambda: task.done(), 0.1)
        assert task.done()
        assert local.cancelled_slot == Local.sleepalot


@pytest.mark.timeout(30)
@run_test
def test_connectdevice(deviceTest):
    remote = deviceTest["remote"]
    remote.value = 123
    d = connectDevice("remote")
    try:
        assert d.value == 123
        remote.value = 456
        waitUntil(lambda: d.value == 456, timeout=0.5)  # noqa
    finally:
        # check that the proxy gets collected when not used anymore
        weak = weakref.ref(d)
        del d
        assert weak() is None


@pytest.mark.timeout(30)
@run_test
def test_proxy_dead():
    async def starter():
        a = Remote({"_deviceId_": "moriturus"})
        await a.startInstance()
        proxy = await getDevice("moriturus")
        await a.slotKillDevice()

        return proxy

    proxy = background(starter()).wait()
    with pytest.raises(KaraboError):
        proxy.count()
    with pytest.raises(KaraboError):
        proxy.value = 5


@pytest.mark.timeout(30)
@run_test
def test_locked(deviceTest):
    remote = deviceTest["remote"]
    with getDevice("remote") as d:
        d.value = 22
        d.lockedBy = "whoever"
        try:
            # XXX: exception raise
            with pytest.raises(KaraboError):
                d.value = 7
            assert d.value == 22
        finally:
            remote.lockedBy = ""
        d.value = 3
        assert d.value == 3


@pytest.mark.timeout(30)
@run_test
def test_lock(deviceTest):
    with getDevice("remote") as d:
        with lock(d):
            assert d.lockedBy == "local"
            with lock(d):
                assert d.lockedBy == "local"
                d.value = 33
                assert d.value == 33
            assert d.lockedBy == "local"
        assert d.lockedBy == ""


@pytest.mark.flaky(max_runs=FLAKY_MAX_RUNS, min_passes=FLAKY_MIN_PASSES)
@pytest.mark.timeout(30)
@run_test
def test_lock_nowait(deviceTest):
    with getDevice("remote") as d:
        with lock(d, wait_for_release=False):
            assert d.lockedBy == "local"
            d.value = 33
            assert d.value == 33
        assert d.lockedBy == "local"
        waitUntil(lambda: d.lockedBy == "")


@pytest.mark.timeout(30)
@run_test
async def test_async_slot_macro(deviceTest):
    local = deviceTest["local"]
    d = await getDevice("local")
    with d:
        assert d.state == State.PASSIVE
        await d.asyncSleep()
        # Macro slot is blocking with a sleep
        assert d.state == State.PASSIVE

        local.cancelled_slot = None
        # Normal round trip non blocking
        ensure_future(local.asyncSleep())
        await waitUntil(lambda: d.state == State.ACTIVE)
        assert d.state == State.ACTIVE
        await waitUntil(lambda: d.state == State.PASSIVE)
        assert d.state == State.PASSIVE

        # We can cancel our tasks
        ensure_future(local.asyncSleep())
        await d.cancel()
        # A cancellation creates another task for the a cancel action
        # we wait and assert here, this should be very quick
        await sleepUntil(lambda: local.cancelled_slot is not None, 0.2)
        assert local.cancelled_slot == Local.asyncSleep
        assert d.state == State.PASSIVE


@pytest.mark.timeout(30)
@run_test
async def test_request_macro(deviceTest):
    """test that the macro can provide its own code"""
    # at first the code is missing in the tests.
    local = deviceTest["local"]
    assert local.code == ""
    request = Hash("name", "macro")

    # No code attached
    result = local.requestMacro(request)
    assert result["payload.success"]
    assert result["payload.data"] == ""

    # check that we can handle an existing macro correctly
    example_code = "The macro's own code."
    local.code = example_code
    assert "macro" in local.availableMacros.value
    assert local.code == example_code

    # Check for both correct and wrong name
    result = local.requestMacro(request)
    assert result["payload.success"]
    assert result["payload.data"] == example_code

    bad_request = Hash("name", "nomacro")
    result = local.requestMacro(bad_request)
    assert not result["payload.success"]
    assert result["payload.data"] == example_code


@pytest.mark.timeout(30)
def test_macro_klass_inheritance():
    """Test that we always have the correct macro instance"""

    class B(Macro):
        """Lower level macro base class"""

    class A(B):
        """Top level macro class"""

    macro = A()
    assert issubclass(macro.klass, A)
    assert issubclass(macro.klass, B)


@pytest.mark.timeout(30)
@run_test
async def test_state_machine():
    """test the execution of abstract macro with new state machine"""
    local = LocalAbstract(_deviceId_="local_abstract",
                          project="test", module="test")
    async with AsyncDeviceContext(local=local):
        with await getDevice("local_abstract") as d:
            assert d.state == State.ON
            seen = False

            async def show_active():
                nonlocal d, seen
                await waitUntil(lambda: d.state == State.MOVING)
                seen = True

            task = background(show_active())
            # Give background a chance to post on loop!
            await sleep(0.05)
            await d.start()
            await task
            assert seen
