import gc
import time
import weakref
from asyncio import (
    ensure_future, get_event_loop, set_event_loop, sleep, wait_for)
from contextlib import closing
from itertools import count
from unittest import TestCase, main, skipIf

from karabo.common.states import State
from karabo.middlelayer_api.broker import amqp, jms
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import (
    callNoWait, findDevices, getClients, getDevice, getDevices, shutdown)
from karabo.middlelayer_api.eventloop import NoEventLoop
from karabo.middlelayer_api.ikarabo import (
    DeviceClient, connectDevice, start_device_client)
from karabo.middlelayer_api.macro import EventThread, Macro, RemoteDevice
from karabo.middlelayer_api.tests.eventloop import setEventLoop
from karabo.native import Int32 as Int, KaraboError, Slot


class Remote(Macro):
    counter = Int(defaultValue=-1)

    # avoid that test_main gets stuck. Raise a timeout error instead.
    async def _run(self, *args, **kwargs):
        return (await wait_for(super()._run(*args, **kwargs), timeout=5))

    @Slot()
    def count(self):
        for i in range(30):
            self.counter = i
            time.sleep(0.1)

    def onDestruction(self):
        Remote.destructed = True


class NoRemote(Macro):
    rd = RemoteDevice("DoesNotExist")


class Other(Device):
    something = Int(defaultValue=333)
    counter = Int(defaultValue=-1)
    task = None

    async def do_count(self):
        try:
            for i in count():
                self.counter = i
                await sleep(0.1)
        finally:
            self.state = State.ON

    @Slot()
    async def count(self):
        self.state = State.PROCESSING
        self.task = ensure_future(self.do_count())

    @Slot()
    async def stopCount(self):
        if self.task is not None:
            self.task.cancel()

    async def onInitialization(self):
        self.myself = await getDevice("other")
        self.myself.something = 222
        self.state = State.ON

    async def onDestruction(self):
        with self.myself:
            self.myself.something = 111
            await sleep(0.02)


@skipIf(amqp, reason="not working on amqp")
class Tests(TestCase):
    def test_delete(self):
        thread = EventThread()
        thread.start()
        try:
            remote = Remote()
            remote.count()
            self.assertEqual(remote.counter, 29)
            r = weakref.ref(remote)
            Remote.destructed = False
            del remote
            if jms:
                time.sleep(0.02)
            else:
                time.sleep(0.2)
            gc.collect()
            if jms:
                time.sleep(0.02)
            else:
                time.sleep(0.2)
            self.assertIsNone(r())
            self.assertTrue(Remote.destructed)
        finally:
            thread.stop()
            thread.join(0.5)
            self.assertFalse(thread.is_alive())

    def test_main(self):
        Remote.main(["", "count", "counter=7"])

    def test_remote_timeout(self):
        """Test that remote devices can timeout and send logs"""
        thread = None
        try:
            # start a thread with event loop and then try
            # to run the device
            thread = start_device_client()
            with self.assertLogs("NoRemote"):
                NoRemote(_deviceId_="NoRemote")
        finally:
            if thread is not None:
                thread.stop()
                thread.join(0.5)
                self.assertFalse(thread.is_alive())

    async def init_other(self):
        self.other = Other(dict(_deviceId_="other", _serverId_="tserver"))
        await self.other.startInstance()

    def test_autodisconnect(self):
        """test the automatic disconnect after 15 s ATTENTION! LONG TEST!

        this tests that a device connected to by connectDevice automatically
        disconnects again after 15 s. Unfortunately this means the test
        needs to run very long..."""
        thread = EventThread()
        thread.start()
        try:
            devices = DeviceClient(_deviceId_="ikarabo-test")
            oel = get_event_loop()
            set_event_loop(NoEventLoop(devices))
            time.sleep(0.1)
            thread.loop.call_soon_threadsafe(ensure_future, self.init_other())
            time.sleep(4)

            # test whether proxy stays alive while used
            other = connectDevice("other")
            self.assertEqual(other.state, State.ON)
            other.count()
            lastcount = -2
            for i in range(20):
                self.assertEqual(other.state, State.PROCESSING)
                self.assertLess(lastcount, other.counter)
                lastcount = other.counter
                time.sleep(1)

            # test proxy disconnects when not used
            time.sleep(16)
            # but call to stop counting, we don't receive the state
            # update as well
            callNoWait("other", "stopCount")
            lastcount = other.__dict__["counter"]
            laststate = other.__dict__["state"]
            time.sleep(3)
            self.assertEqual(other.__dict__["counter"], lastcount)
            self.assertEqual(laststate, State.PROCESSING)
            # reconnect with fresh state and counter
            self.assertNotEqual(other.counter, lastcount)
            self.assertEqual(other.state, State.ON)

            set_event_loop(oel)
            del self.other
        finally:
            thread.stop()
            thread.join(5)
            self.assertFalse(thread.is_alive())
    test_autodisconnect.slow = 1

    async def init_topo(self, dc):
        other = Other(dict(_deviceId_="other", _serverId_="tserver"))
        self.assertNotIn("other", getDevices())
        self.assertNotIn("other", getClients())
        self.assertNotIn("other", getDevices("tserver"))
        await other.startInstance()
        await sleep(0.1)
        if amqp:
            await sleep(5.0)
        self.assertIn("other", getDevices())
        self.assertIn("other", getDevices("tserver"))
        self.assertNotIn("other", findDevices("beep"))
        if amqp:
            await sleep(2.0)
        self.assertIn("other", findDevices("other"))
        self.assertIn("other", findDevices("OTHER"))
        self.assertIn("other", findDevices("OT"))
        self.assertNotIn("other", getDevices("bserver"))

        double = Other(dict(_deviceId_="other", _serverId_="bserver"))
        with self.assertRaises(KaraboError):
            await double.startInstance()
        self.assertIn("other", getDevices())
        self.assertIn("other", getDevices("tserver"))
        self.assertNotIn("other", getDevices("bserver"))

        await shutdown("other")
        self.assertNotIn("other", getDevices())

    def test_topology(self):
        loop = setEventLoop()
        with closing(loop):
            dc = DeviceClient(dict(_deviceId_="ikarabo-test2"))
            loop.run_until_complete(dc.startInstance())
            task = loop.create_task(self.init_topo(dc), dc)
            loop.run_until_complete(task)
            loop.run_until_complete(dc.slotKillDevice())

    def test_ikarabo(self):
        thread = None
        try:
            with self.assertLogs(level="WARNING"):
                thread = start_device_client()
        except AssertionError:
            pass
        else:
            self.fail("no log should be generated!")
        finally:
            if thread is not None:
                thread.stop()
                thread.join(0.5)
                self.assertFalse(thread.is_alive())


if __name__ == "__main__":
    main()
