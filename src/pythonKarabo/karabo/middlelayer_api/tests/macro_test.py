import sys
import time
import weakref
from asyncio import Future, TimeoutError, ensure_future
from contextlib import contextmanager
from unittest import main

from flaky import flaky

from karabo.common.states import State
from karabo.middlelayer_api.broker import amqp, jms
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import (
    Queue, call, callNoWait, connectDevice, executeNoWait, getConfiguration,
    getDevice, getInstanceInfo, getProperties, getSchema, lock, setNoWait,
    setWait, updateDevice, waitUntil, waitUntilNew, waitWhile)
from karabo.middlelayer_api.device_server import KaraboStream
from karabo.middlelayer_api.macro import Macro, MacroSlot
from karabo.middlelayer_api.pipeline import OutputChannel, PipelineContext
from karabo.middlelayer_api.signalslot import slot
from karabo.middlelayer_api.synchronization import background, sleep
from karabo.middlelayer_api.tests.eventloop import (
    DeviceTest, async_tst, sleepUntil, sync_tst)
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

    generic = Superslot()


class Local(Macro):
    @Slot()
    def remotecalls(self):
        self.nowslot = self.currentSlot
        self.nowstate = self.state
        print("superpuper", end="hero")

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


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.local = Local(_deviceId_="local", project="test", module="test",
                          may_start_thread=False)
        cls.remote = Remote(dict(_deviceId_="remote"))
        cls.pipe_remote = RemotePipeline(dict(_deviceId_="piperemote"))
        cls.localMacro = LocalMacroSlot(_deviceId_="localMacroSlot",
                                        project="no", module="test")
        with cls.deviceManager(cls.remote, cls.localMacro, cls.pipe_remote,
                               lead=cls.local):
            yield

    @async_tst
    async def tearDown(self):
        with (await getDevice("remote")) as d:
            await waitUntil(lambda: d.state == State.UNKNOWN)

    @async_tst
    async def test_macro_slotter_custom(self):
        """Test that we can customize the state machine"""
        localMacro = LocalMacroSlot(_deviceId_="localCustomSlot",
                                    project="no", module="test")
        await localMacro.startInstance()
        await waitUntil(lambda: localMacro.is_initialized)
        try:
            with await getDevice("localCustomSlot") as d:
                await sleep(1)
                await localMacro.set_state(State.ON)
                if not jms:
                    await updateDevice(d)
                for _ in range(4):
                    await waitUntil(lambda: d.state == State.ON)
                    self.assertEqual(d.state, State.ON)
                    await d.startCustom()
                    self.assertEqual(d.state, State.MOVING)
        finally:
            await localMacro.set_state(State.PASSIVE)
            await localMacro.slotKillDevice()

    @sync_tst
    def test_get_properties_hash(self):
        """Test that we can get properties via slotGetConfigurationSlice"""
        h = getProperties("remote", ["state"])
        self.assertEqual(h["state"], "UNKNOWN")
        attrs = h["state", ...]
        self.assertIn("sec", attrs)
        self.assertIn("frac", attrs)
        self.assertIn("tid", attrs)

    @sync_tst
    def test_macro_pipeline_context(self):
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
            self.assertIsNotNone(data)
            self.assertTrue(channel.is_alive())
            proxy.stopSending()
        self.assertFalse(channel.is_alive())

    @sync_tst
    @flaky(max_runs=FLAKY_MAX_RUNS, min_passes=FLAKY_MIN_PASSES)
    def test_macro_slotter_sync(self):
        """Test the execution of MacroSlots in sync case"""
        with getDevice("localMacroSlot") as d:
            # 1.1
            self.assertEqual(d.state, State.PASSIVE)
            d.startSync()
            self.assertEqual(d.state, State.ACTIVE)
            waitUntil(lambda: d.state == State.PASSIVE)
            self.assertEqual(d.state, State.PASSIVE)

            # 1.2 Cancel the sync macro slot
            d.startSync()
            self.assertEqual(d.state, State.ACTIVE)
            d.cancel()
            self.assertIs(self.localMacro.cancelled_slot,
                          LocalMacroSlot.startSync)
            waitUntil(lambda: d.state == State.PASSIVE)
            self.assertEqual(d.state, State.PASSIVE)

            self.localMacro.cancelled_slot = None

            # 2.1 Nodes
            self.assertEqual(d.state, State.PASSIVE)
            d.node.startSync()
            self.assertEqual(d.state, State.ACTIVE)
            waitUntil(lambda: d.state == State.PASSIVE)
            self.assertEqual(d.state, State.PASSIVE)

            # 2.2 Cancel the sync macro slot
            d.node.startSync()
            self.assertEqual(d.state, State.ACTIVE)
            d.cancel()
            self.assertIs(self.localMacro.cancelled_slot,
                          LocalMacroSlot.node.cls.startSync)
            waitUntil(lambda: d.state == State.PASSIVE)
            self.assertEqual(d.state, State.PASSIVE)

            self.localMacro.cancelled_slot = None

    @async_tst
    async def test_macro_slotter_async(self):
        """Test the execution of MacroSlots in async case"""
        with await getDevice("localMacroSlot") as d:
            # Using async with works out of the box ...
            # 1.1
            self.assertEqual(d.state, State.PASSIVE)
            await d.startASync()
            self.assertEqual(d.state, State.ACTIVE)
            await waitUntil(lambda: d.state == State.PASSIVE)
            self.assertEqual(d.state, State.PASSIVE)

            # 1.2 Cancel the async macro slot
            await d.startASync()
            self.assertEqual(d.state, State.ACTIVE)
            await d.cancel()
            self.assertIs(self.localMacro.cancelled_slot,
                          LocalMacroSlot.startASync)
            await waitUntil(lambda: d.state == State.PASSIVE)
            self.assertEqual(d.state, State.PASSIVE)

            self.localMacro.cancelled_slot = None

            # 2.1 Nodes
            self.assertEqual(d.state, State.PASSIVE)
            await d.node.startASync()
            self.assertEqual(d.state, State.ACTIVE)
            await waitUntil(lambda: d.state == State.PASSIVE)
            self.assertEqual(d.state, State.PASSIVE)

            # 2.2 Cancel the sync macro slot
            await d.node.startASync()
            self.assertEqual(d.state, State.ACTIVE)
            await d.cancel()
            self.assertIs(self.localMacro.cancelled_slot,
                          LocalMacroSlot.node.cls.startASync)
            await waitUntil(lambda: d.state == State.PASSIVE)
            self.assertEqual(d.state, State.PASSIVE)

            self.localMacro.cancelled_slot = None

    @async_tst
    async def test_macro_slotter_error(self):
        """test that errors are properly logged and error functions called"""
        with self.assertLogs(logger="localMacroSlot", level="ERROR"), \
                (await getDevice("localMacroSlot")) as d:
            await d.error()
            await sleep(TIMEOUT_LOGS)

        self.assertIs(self.localMacro.exc_slot, LocalMacroSlot.error)
        self.assertIsInstance(self.localMacro.exception, RuntimeError)
        self.assertIsNotNone(self.localMacro.traceback)
        self.localMacro.exc_slot = None
        self.localMacro.exception = None
        self.localMacro.traceback = None

    @async_tst
    async def test_macro_slotter_error_in_error(self):
        """test errors in error handlers are logged in a macro slot"""
        with self.assertLogs(logger="localMacroSlot", level="ERROR") as logs, \
                (await getDevice("localMacroSlot")) as d:
            await d.error_in_error()
            await sleep(TIMEOUT_LOGS)
        self.assertEqual(logs.records[-1].msg, "error in error handler")
        self.assertIs(self.localMacro.exc_slot, LocalMacroSlot.error_in_error)
        self.assertIsInstance(self.localMacro.exception, RuntimeError)
        self.assertIsNotNone(self.localMacro.traceback)
        self.localMacro.exc_slot = None
        self.localMacro.exception = None
        self.localMacro.traceback = None

    @sync_tst
    def test_execute(self):
        """test the execution of remote commands"""
        self.remote.done = False
        with getDevice("remote") as d:
            d.doit()
        self.assertTrue(self.remote.done)

    @async_tst
    async def test_destruct_macro_timer(self):
        macro = Local(_deviceId_="macrodestruct", project="timer",
                      module="timer")
        await macro.startInstance()
        await sleepUntil(lambda: macro.is_initialized is True)
        self.assertTrue(macro.stackTimer.is_running())
        await macro.slotKillDevice()
        self.assertFalse(macro.stackTimer.is_running())
        self.assertIsNone(macro.stackTimer.loop)

    @sync_tst
    def test_archive(self):
        """test the archive setting of a macro"""
        with getDevice("remote") as d:
            with self.assertRaises(AttributeError):
                _ = d.archive
        info = getInstanceInfo("local")
        self.assertFalse(info["archive"])

    @sync_tst
    def test_change(self):
        """test that changes on a remote device reach the macro"""
        self.remote.value = 7
        with getDevice("remote") as d:
            d.changeit()
        self.assertEqual(self.remote.value, 3)

    @sync_tst
    def test_disconnect(self):
        """check that we don't get updates when we're not connected"""
        d = getDevice("remote")    # proxy is not connected after 5 seconds
        sleep(5)
        executeNoWait(d, "count")
        if not amqp:
            time.sleep(0.1)
        self.assertEqual(d.counter, -1)
        with updateDevice(d):
            self.assertNotEqual(d.counter, -1)
            time.sleep(0.1)
            self.assertNotEqual(d.counter, 29)
        time.sleep(0.1)
        last = d.counter
        time.sleep(0.1)
        self.assertEqual(last, d.counter)
        self.assertNotEqual(d.counter, 29)
        with d:
            updateDevice(d)
            waitUntil(lambda: d.counter == 29)
        self.assertEqual(d.counter, 29)

    @sync_tst
    def test_set(self):
        """test setting of parameters on a remote device"""
        self.remote.value = 7
        with getDevice("remote") as d:
            self.assertEqual(d.value, 7)
            d.value = 10
            time.sleep(0.1)
            self.assertEqual(d.value, 10)
            d.changeit()
            self.assertEqual(d.value, 6)

    @sync_tst
    def test_generic(self):
        """call a generic slot"""
        self.remote.value = 7
        d = getDevice("remote")
        d.generic()
        self.assertEqual(self.remote.value, 22)

    @sync_tst
    def test_other(self):
        """test properties with special setters"""
        self.remote.value = 7
        with getDevice("remote") as d:
            d.other = 102
        time.sleep(0.1)
        self.assertEqual(self.remote.value, 102)

    @sync_tst
    def test_selfcall(self):
        """test that slots can be called like normal methods"""
        self.local.selfcall()
        self.assertEqual(self.local.marker, 77)

    @sync_tst
    def test_setwait(self):
        """test the setWait function"""
        d = getDevice("remote")
        setWait(d, value=200, counter=300)

        self.assertEqual(self.remote.value, 200)
        self.assertEqual(self.remote.counter, 300)

        # test the setWait function with args
        setWait(d, "value", 400, "counter", 400)
        self.assertEqual(self.remote.value, 400)
        self.assertEqual(self.remote.counter, 400)
        with self.assertRaises(RuntimeError):
            setWait(d, "value", 1, "counter")

        setWait(d, "deep.value", 400, "deep.counter", 400)
        self.assertEqual(self.remote.deep.counter, 400)
        self.assertEqual(self.remote.deep.value, 400)

    @sync_tst
    def test_setnowait(self):
        """test the setNoWait function"""
        self.remote.value = 0
        self.remote.counter = 0
        self.remote.deep.value = 0
        self.remote.deep.counter = 0

        d = getDevice("remote")
        setNoWait(d, value=200, counter=300)
        self.assertEqual(self.remote.value, 0)
        self.assertEqual(self.remote.counter, 0)
        if not jms:
            updateDevice(d)
        else:
            time.sleep(0.1)
        self.assertEqual(self.remote.value, 200)
        self.assertEqual(self.remote.counter, 300)

        # test the setNoWait function with args
        setNoWait(d, "value", 0, "counter", 0)
        self.assertEqual(self.remote.value, 200)
        self.assertEqual(self.remote.counter, 300)
        if not jms:
            updateDevice(d)
        else:
            time.sleep(0.1)
        self.assertEqual(self.remote.value, 0)
        self.assertEqual(self.remote.counter, 0)
        with self.assertRaises(RuntimeError):
            setNoWait(d, "value", 0, 100)

        setNoWait(d, "deep.value", 400, "deep.counter", 400)
        self.assertEqual(self.remote.deep.counter, 0)
        self.assertEqual(self.remote.deep.value, 0)
        if not jms:
            updateDevice(d)
        else:
            time.sleep(0.1)
        self.assertEqual(self.remote.deep.counter, 400)
        self.assertEqual(self.remote.deep.value, 400)

    @sync_tst
    def test_waituntil(self):
        """test the waitUntil function"""
        with getDevice("remote") as d:
            d.counter = 0
            self.assertEqual(d.counter, 0)
            executeNoWait(d, "count")
            waitUntil(lambda: d.counter > 10)
            self.assertEqual(d.counter, 11)
            with self.assertRaises(TimeoutError):
                waitUntil(lambda: d.counter > 40, timeout=0.1)

    @sync_tst
    def test_waitWhile(self):
        """test the waitWhile function"""
        with getDevice("remote") as d:
            d.counter = 0
            if not jms:
                updateDevice(d)
            self.assertEqual(d.counter, 0)
            executeNoWait(d, "count")
            waitWhile(lambda: d.counter <= 10)
            self.assertGreaterEqual(d.counter, 10)

    @sync_tst
    def test_waituntilnew(self):
        """test the waitUntilNew function"""
        with getDevice("remote") as d:
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
                self.assertGreaterEqual(d.counter, i)
                i = d.counter + 1   # next (expected) remote value

            self.assertEqual(29, d.counter)

    @async_tst
    async def test_print(self):
        """test that macros can print via expected parameters"""
        sys.stdout = KaraboStream(sys.stdout)
        try:
            self.assertEqual(self.local.currentSlot, "")
            self.assertEqual(self.local.state, State.PASSIVE)
            await self.remote.call_local()
            self.assertEqual(self.local.nowslot, "remotecalls")
            self.assertEqual(self.local.nowstate, State.ACTIVE)
            self.assertEqual(self.local.currentSlot, "")
            self.assertEqual(self.local.state, State.PASSIVE)
            # Must wait at least 0.5 seconds. The print is bulked
            await sleep(1)
            self.assertGreater(self.local.doNotCompressEvents, 0)
            self.assertEqual(self.local.print, "superpuper\nhero")
        finally:
            sys.stdout = sys.stdout.base

    @sync_tst
    def test_call(self):
        """test calling a slot"""
        with getDevice("remote") as d:
            d.value = 0
            if not jms:
                updateDevice(d)
            self.assertEqual(d.value, 0)
            call("remote", "changeit")
            self.assertEqual(d.value, -4)

    @sync_tst
    def test_getSchema(self):
        """test calling get Schema"""
        schema_deviceId = getSchema("remote")
        with getDevice("remote") as d:
            schema_proxy = getSchema(d)

        hash_deviceId = schema_deviceId.hash
        hash_proxy = schema_proxy.hash

        self.assertTrue(hash_deviceId.fullyEqual(hash_proxy))
        self.assertEqual(hash_deviceId.paths(),
                         hash_proxy.paths())
        self.assertEqual(hash_proxy["value", "accessMode"],
                         AccessMode.RECONFIGURABLE.value)
        self.assertEqual(hash_deviceId["value", "accessMode"],
                         AccessMode.RECONFIGURABLE.value)

        self.assertIn("counter", hash_deviceId)
        self.assertIn("counter", hash_proxy)
        self.assertEqual(hash_deviceId["counter", "defaultValue"], -1)
        self.assertEqual(hash_proxy["counter", "defaultValue"], -1)
        self.assertEqual(schema_deviceId.name, schema_proxy.name)

    @sync_tst
    def test_getConfiguration(self):
        """test calling get getConfiguration"""
        conf_deviceId = getConfiguration("remote")
        with getDevice("remote") as d:
            conf_proxy = getConfiguration(d)

        self.assertTrue(conf_deviceId.fullyEqual(conf_proxy))
        self.assertEqual(conf_deviceId.paths(), conf_proxy.paths())

    @sync_tst
    def test_call_param(self):
        """test calling a slot with a parameter"""
        h = call("remote", "setStatus", "Token")
        self.assertEqual(self.remote.status, "Token")
        self.assertEqual(h, "Token")

    @async_tst
    async def test_call_param_nowait(self):
        """test no wait calling a slot with a parameter"""
        fut = self.remote.get_future()
        callNoWait("remote", "setStatus", "NewToken")
        await fut
        self.assertEqual(fut.result(), "NewToken")

    @sync_tst
    def test_queue(self):
        """test change queues of properties"""
        with getDevice("remote") as d:
            if not jms:
                updateDevice(d)
            executeNoWait(d, "count")
            waitUntil(lambda: d.counter == 0)
            q = Queue(d.counter)
            for i in range(1, 30):
                j = q.get()
                self.assertEqual(i, j)
                time.sleep(i * 0.002)

    @async_tst
    async def test_error(self):
        """test that errors are properly logged and error functions called"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR"), \
                (await getDevice("local")) as d, \
                self.assertRaises(KaraboError):
            await d.error()
        self.assertTrue(self.remote.done)
        self.assertIs(self.local.exc_slot, Local.error)
        self.assertIsInstance(self.local.exception, RuntimeError)
        self.local.traceback.tb_lasti  # check whether that is a traceback
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback

    @async_tst
    async def test_error_in_error(self):
        """test that errors in error handlers are properly logged"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR") as logs, \
                (await getDevice("local")) as d, \
                self.assertRaises(KaraboError):
            await d.error_in_error()
        self.assertFalse(self.remote.done)
        self.assertEqual(logs.records[-1].msg, "error in error handler")
        self.assertIs(self.local.exc_slot, Local.error_in_error)
        self.assertIsInstance(self.local.exception, RuntimeError)
        self.local.traceback.tb_lasti  # check whether that is a traceback
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback

    @async_tst
    async def test_cancel(self):
        """test proper cancellation of slots

        when a slot is cancelled, test that
          * the onCancelled callback is called (which sets cancelled_slot)
          * the slot got cancelled in the right place (slept has right value)
          * the task gets properly marked done
        Test that for both if the slot is stuck in a karabo function, or not.
        """
        # Rename sleep to make it clean which sleep is being used
        karabo_sleep = sleep
        d = await getDevice("local")
        with d:
            await updateDevice(d)
            # cancel during time.sleep
            self.local.cancelled_slot = None
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
            counter = 100
            while counter > 0:
                await karabo_sleep(0.01)
                if self.local.slept_count == 1:
                    break
                counter -= 1
            counter = 100
            while counter > 0:
                await karabo_sleep(0.01)
                if task.done():
                    break
                counter -= 1
            self.assertEqual(self.local.cancelled_slot, Local.sleepalot)
            self.assertEqual(self.local.slept_count, 1)
            assert task.done()

            # cancel during karabo.sleep
            self.local.cancelled_slot = None
            task = ensure_future(d.sleepalot())
            # Sleep for long enough for the macro to end up in the really long
            # sleep at the end
            await karabo_sleep(0.13)
            # Then cancel, while the macro is in that interruptable sleep
            await d.cancel()
            # Sleep a little while, so the task can finish. With a static
            # karabo_sleep of 0.06 this failed e.g. in
            # https://git.xfel.eu/Karabo/Framework/-/jobs/141622
            counter = 100
            while counter > 0:
                await karabo_sleep(0.01)
                if self.local.slept_count == 2:
                    break
                counter -= 1
            self.assertEqual(self.local.slept_count, 2)
            self.assertEqual(self.local.cancelled_slot, Local.sleepalot)
            if jms:
                assert task.done()

    @sync_tst
    def test_connectdevice(self):
        self.remote.value = 123
        d = connectDevice("remote")
        try:
            self.assertEqual(d.value, 123)
            self.remote.value = 456
            if jms:
                waitUntil(lambda: d.value == 456, timeout=0.1)  # noqa
            else:
                waitUntil(lambda: d.value == 456, timeout=1.0)  # noga
        finally:
            # check that the proxy gets collected when not used anymore
            weak = weakref.ref(d)
            del d
            self.assertIsNone(weak())

    @sync_tst
    def test_proxy_dead(self):

        async def starter():
            a = Remote({"_deviceId_": "moriturus"})
            await a.startInstance()
            proxy = await getDevice("moriturus")
            await a.slotKillDevice()
            return proxy

        proxy = background(starter()).wait()
        with self.assertRaisesRegex(KaraboError, "died"):
            proxy.count()
        with self.assertRaisesRegex(KaraboError, "died"):
            proxy.value = 5

    @sync_tst
    def test_locked(self):
        with getDevice("remote") as d:
            d.value = 22
            d.lockedBy = "whoever"
            try:
                with self.assertRaisesRegex(KaraboError, "lock"):
                    d.value = 7
                self.assertEqual(d.value, 22)
            finally:
                self.remote.lockedBy = ""
            d.value = 3
            self.assertEqual(d.value, 3)

    @sync_tst
    def test_lock(self):
        with getDevice("remote") as d:
            with lock(d):
                self.assertEqual(d.lockedBy, "local")
                with lock(d):
                    self.assertEqual(d.lockedBy, "local")
                    d.value = 33
                    self.assertEqual(d.value, 33)
                self.assertEqual(d.lockedBy, "local")
            self.assertEqual(d.lockedBy, "")

    @sync_tst
    @flaky(max_runs=FLAKY_MAX_RUNS, min_passes=FLAKY_MIN_PASSES)
    def test_lock_nowait(self):
        with getDevice("remote") as d:
            with lock(d, wait_for_release=False):
                self.assertEqual(d.lockedBy, "local")
                d.value = 33
                self.assertEqual(d.value, 33)
            self.assertEqual(d.lockedBy, "local")
            waitUntil(lambda: d.lockedBy == "")

    @async_tst
    async def test_async_slot_macro(self):
        d = await getDevice("local")
        with d:
            self.assertEqual(d.state, State.PASSIVE)
            await d.asyncSleep()
            # Macro slot is blocking with a sleep
            self.assertEqual(d.state, State.PASSIVE)

            self.local.cancelled_slot = None
            # Normal round trip non blocking
            ensure_future(self.local.asyncSleep())
            await waitUntil(lambda: d.state == State.ACTIVE)
            self.assertEqual(d.state, State.ACTIVE)
            await waitUntil(lambda: d.state == State.PASSIVE)
            self.assertEqual(d.state, State.PASSIVE)

            # We can cancel our tasks
            ensure_future(self.local.asyncSleep())
            await d.cancel()
            self.assertEqual(self.local.cancelled_slot, Local.asyncSleep)
            self.assertEqual(d.state, State.PASSIVE)

    @async_tst
    async def test_request_macro(self):
        """test that the macro can provide its own code"""
        # at first the code is missing in the tests.
        self.assertEqual(self.local.code, "")
        request = Hash("name", "macro")

        # No code attached
        result = self.local.requestMacro(request)
        self.assertTrue(result["payload.success"])
        self.assertEqual(result["payload.data"], "")

        # check that we can handle an existing macro correctly
        example_code = "The macro's own code."
        self.local.code = example_code
        self.assertIn("macro", self.local.availableMacros.value)
        self.assertEqual(self.local.code, example_code)

        # Check for both correct and wrong name
        result = self.local.requestMacro(request)
        self.assertTrue(result["payload.success"])
        self.assertEqual(result["payload.data"], example_code)

        bad_request = Hash("name", "nomacro")
        result = self.local.requestMacro(bad_request)
        self.assertFalse(result["payload.success"])
        self.assertEqual(result["payload.data"], example_code)

    @sync_tst
    def test_macro_klass_inheritance(self):
        """Test that we always have the correct macro instance"""
        class B(Macro):
            """Lower level macro base class"""

        class A(B):
            """Top level macro class"""

        macro = A()
        self.assertTrue(issubclass(macro.klass, A))
        self.assertTrue(issubclass(macro.klass, B))


class AbstractMacroTest(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.local = LocalAbstract(_deviceId_="local_abstract",
                                  project="test", module="test")
        with cls.deviceManager(lead=cls.local):
            yield

    @async_tst
    async def test_state_machine(self):
        """test the execution of abstract macro with new state machine"""
        with await getDevice("local_abstract") as d:
            self.assertEqual(d.state, State.ON)
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
            self.assertTrue(seen)


if __name__ == "__main__":
    main()
