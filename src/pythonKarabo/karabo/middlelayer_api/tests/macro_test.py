from asyncio import ensure_future, Future, TimeoutError
from contextlib import contextmanager
import sys
import time
from unittest import main
import weakref

from karabo.common.states import State
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import (
    call, callNoWait, waitUntilNew, waitUntil, waitWhile, setWait, setNoWait,
    getDevice, getConfiguration, getSchema, executeNoWait, updateDevice,
    Queue, connectDevice, lock)
from karabo.middlelayer_api.device_server import KaraboStream
from karabo.native import (
    AccessMode, Configurable, KaraboError, Int32 as Int, Node, Slot)

from karabo.middlelayer_api.macro import Macro
from karabo.middlelayer_api.signalslot import slot
from karabo.middlelayer_api.synchronization import background, sleep

from karabo.middlelayer_api.tests.eventloop import (
    DeviceTest, sync_tst, async_tst)
from karabo.middlelayer_api.tests.compat import jms


class Superslot(Slot):
    async def method(self, device):
        device.value = 22


class MyNode(Configurable):
    value = Int(defaultValue=7)
    counter = Int(defaultValue=-1)


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


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.local = Local(_deviceId_="local", project="test", module="test",
                          may_start_thread=False)
        cls.remote = Remote(dict(_deviceId_="remote"))
        with cls.deviceManager(cls.remote, lead=cls.local):
            yield

    @async_tst
    async def tearDown(self):
        with (await getDevice("remote")) as d:
            await waitUntil(lambda: d.state == State.UNKNOWN)

    @sync_tst
    def test_execute(self):
        """test the execution of remote commands"""
        self.remote.done = False
        with getDevice("remote") as d:
            d.doit()
        self.assertTrue(self.remote.done)

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
        if not jms:
            self.remote.counter = -1
            sleep(1)
        d = getDevice("remote")
        executeNoWait(d, "count")
        if jms:
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
            if not jms:
                updateDevice(d)
            else:
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
        if not jms:
            updateDevice(d)
        else:
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
        if not jms:
            sleep(1)
        with getDevice("remote") as d:
            d.counter = 0
            if not jms:
                updateDevice(d)
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
        if not jms:
            sleep(1)
        with getDevice("remote") as d:
            if not jms:
                updateDevice(d)
            d.counter = 0
            executeNoWait(d, "count")
            waitUntil(lambda: d.counter == 0)
            for i in range(30):
                waitUntilNew(d.counter)
                self.assertEqual(i, d.counter)

    @async_tst
    async def test_print(self):
        """test that macros can print via expected parameters"""
        if not jms:
            self.local.doNotCompressEvents = 0
            await sleep(1)
        sys.stdout = KaraboStream(sys.stdout)
        try:
            self.assertEqual(self.local.currentSlot, "")
            self.assertEqual(self.local.state, State.PASSIVE)
            await self.remote.call_local()
            self.assertEqual(self.local.nowslot, "remotecalls")
            self.assertEqual(self.local.nowstate, State.ACTIVE)
            self.assertEqual(self.local.currentSlot, "")
            self.assertEqual(self.local.state, State.PASSIVE)
            self.assertEqual(self.local.print, "hero")
            self.assertEqual(self.local.doNotCompressEvents, 2)
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
        if not jms:
            await sleep(1)
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
            # https://git.xfel.eu/gitlab/Karabo/Framework/-/jobs/141622
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
        if not jms:
            sleep(1)
        self.remote.value = 123
        d = connectDevice("remote")
        if not jms:
            updateDevice(d)
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
            if not jms:
                updateDevice(d)
            try:
                with self.assertRaisesRegex(KaraboError, "lock"):
                    d.value = 7
                    if not jms:
                        updateDevice(d)
                self.assertEqual(d.value, 22)
            finally:
                self.remote.lockedBy = ""
            d.value = 3
            if not jms:
                updateDevice(d)
            self.assertEqual(d.value, 3)

    @sync_tst
    def test_lock(self):
        with getDevice("remote") as d:
            if not jms:
                updateDevice(d)
            with lock(d):
                self.assertEqual(d.lockedBy, "local")
                with lock(d):
                    self.assertEqual(d.lockedBy, "local")
                    d.value = 33
                    if not jms:
                        updateDevice(d)
                    self.assertEqual(d.value, 33)
                self.assertEqual(d.lockedBy, "local")
            if not jms:
                updateDevice(d)
            self.assertEqual(d.lockedBy, "")

    @sync_tst
    def test_lock_nowait(self):
        with getDevice("remote") as d:
            if not jms:
                updateDevice(d)
            with lock(d, wait_for_release=False):
                self.assertEqual(d.lockedBy, "local")
                d.value = 33
                if not jms:
                    updateDevice(d)
                self.assertEqual(d.value, 33)
            self.assertEqual(d.lockedBy, "local")
            waitUntil(lambda: d.lockedBy == "")

    @async_tst
    async def test_async_slot_macro(self):
        d = await getDevice("local")
        with d:
            await updateDevice(d)
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
