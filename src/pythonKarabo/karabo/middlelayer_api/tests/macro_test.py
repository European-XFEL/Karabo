from asyncio import async, coroutine, TimeoutError
from contextlib import contextmanager
import sys
import time
from unittest import main
import weakref

from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import (
    waitUntilNew, waitUntil, setWait, setNoWait, getDevice, executeNoWait,
    updateDevice, Queue, connectDevice, sleep)
from karabo.middlelayer_api.device_server import KaraboStream
from karabo.middlelayer_api.hash import Int32 as Int, Slot
from karabo.middlelayer_api.macro import Macro

from .eventloop import DeviceTest, sync_tst, async_tst


class Superslot(Slot):
    @coroutine
    def method(self, device):
        device.value = 22


class Remote(Device):
    value = Int(defaultValue=7)
    counter = Int(defaultValue=-1)

    @Int()
    def other(self, value):
        self.value = value

    @Slot()
    @coroutine
    def doit(self):
        self.done = True

    @Slot()
    @coroutine
    def changeit(self):
        self.value -= 4

    @Slot()
    @coroutine
    def count(self):
        for i in range(30):
            self.counter = i
            yield from sleep(0.02)

    @Slot()
    @coroutine
    def call_local(self):
        with (yield from getDevice("local")) as l:
            yield from l.remotecalls()

    generic = Superslot()


class Local(Macro):
    @Slot()
    def remotecalls(self):
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


class Tests(DeviceTest):
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.local = Local(_deviceId_="local", project="test", module="test",
                          may_start_thread=False)
        cls.remote = Remote(dict(_deviceId_="remote"))
        with cls.deviceManager(cls.local, cls.remote):
            yield

    @sync_tst
    def test_execute(self):
        """test the execution of remote commands"""
        self.remote.done = False
        with getDevice("remote") as d:
            d.doit()
        time.sleep(0.01)
        self.assertTrue(self.remote.done)

    @sync_tst
    def test_change(self):
        """test that changes on a remote device reach the macro"""
        self.remote.value = 7
        with getDevice("remote") as d:
            d.changeit()
        time.sleep(0.01)
        self.assertEqual(self.remote.value, 3)

    @sync_tst
    def test_disconnect(self):
        """check that we don't get updates when we're not connected"""
        d = getDevice("remote")
        executeNoWait(d, "count")
        time.sleep(0.1)
        self.assertEqual(d.counter, -1)
        time.sleep(0.1)
        with updateDevice(d):
            self.assertNotEqual(d.counter, -1)
            self.assertNotEqual(d.counter, 29)
        time.sleep(0.1)
        last = d.counter
        time.sleep(0.1)
        self.assertEqual(last, d.counter)
        with d:
            time.sleep(0.4)
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
            time.sleep(0.1)
            self.assertEqual(d.value, 6)

    @sync_tst
    def test_generic(self):
        """call a generic slot"""
        self.remote.value = 7
        d = getDevice("remote")
        d.generic()
        time.sleep(0.1)
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

    @sync_tst
    def test_setnowait(self):
        """test the setNoWait function"""
        self.remote.value = 0
        self.remote.counter = 0
        d = getDevice("remote")
        setNoWait(d, value=200, counter=300)
        self.assertEqual(self.remote.value, 0)
        self.assertEqual(self.remote.counter, 0)
        time.sleep(0.1)
        self.assertEqual(self.remote.value, 200)
        self.assertEqual(self.remote.counter, 300)

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
                waitUntil(lambda: d.counter > 40, timeout=1)

    @sync_tst
    def test_waituntilnew(self):
        """test the waitUntilNew function"""
        with getDevice("remote") as d:
            d.counter = 0
            sleep(0.01)
            executeNoWait(d, "count")
            for i in range(30):
                j = waitUntilNew(d).counter
                self.assertEqual(i, j)

    @sync_tst
    def test_sleep(self):
        t = time.time()
        sleep(6)
        self.assertGreater(time.time() - t, 6)
    test_sleep.slow = 1

    @async_tst
    def test_print(self):
        """test that macros can print via expected parameters"""
        sys.stdout = KaraboStream(sys.stdout)
        try:
            self.assertEqual(self.local.state, "Idle...")
            yield from self.remote.call_local()
            self.assertEqual(self.local.nowstate, "remotecalls")
            self.assertEqual(self.local.state, "Idle...")
            self.assertEqual(self.local.print, "hero")
            self.assertEqual(self.local.printno, 2)
        finally:
            sys.stdout = sys.stdout.base

    @sync_tst
    def test_queue(self):
        """test change queues of properties"""
        with getDevice("remote") as d:
            executeNoWait(d, "count")
            waitUntil(lambda: d.counter == 0)
            q = Queue(d).counter
            for i in range(1, 30):
                j = q.get()
                self.assertEqual(i, j)
                time.sleep(i * 0.002)

    @async_tst
    def test_error(self):
        """test that errors are properly logged and error functions called"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR"), \
                (yield from getDevice("local")) as d:
            yield from d.error()
            yield from sleep(0.1)
        self.assertTrue(self.remote.done)
        self.assertIs(self.local.exc_slot, Local.error)
        self.assertIsInstance(self.local.exception, RuntimeError)
        self.local.traceback.tb_lasti  # check whether that is a traceback
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback

    @async_tst
    def test_error_in_error(self):
        """test that errors in error handlers are properly logged"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR") as logs, \
                (yield from getDevice("local")) as d:
            yield from d.error_in_error()
            yield from sleep(0.1)
        self.assertFalse(self.remote.done)
        self.assertEqual(logs.records[-1].msg, "error in error handler")
        self.assertIs(self.local.exc_slot, Local.error_in_error)
        self.assertIsInstance(self.local.exception, RuntimeError)
        self.local.traceback.tb_lasti  # check whether that is a traceback
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback

    @async_tst
    def test_cancel(self):
        """test proper cancellation of slots

        when a slot is cancelled, test that
          * the onCancelled callback is called (which sets cancelled_slot)
          * the slot got cancelled in the right place (slept has right value)
          * the task gets properly marked done
        Test that for both if the slot is stuck in a karabo function, or not.
        """
        # Rename sleep to make it clean which sleep is being used
        karabo_sleep = sleep
        with (yield from getDevice("local")) as d:
            # cancel during time.sleep
            self.local.cancelled_slot = None
            task = async(d.sleepalot())
            # Sleep for a short time so that the macro gets started
            yield from karabo_sleep(0.01)
            # Cancel the macro, which is in a non-interruptable non-karabo
            # sleep.
            # (which is just a place-holder for any other type of
            #  non-interruptable code)
            yield from d.cancel()
            # Finally, sleep for long enough that the macro runs in to the
            # karabo sleep which CAN be interrupted.
            yield from karabo_sleep(0.13)
            self.assertEqual(self.local.cancelled_slot, Local.sleepalot)
            self.assertEqual(self.local.slept_count, 1)
            assert task.done()

            # cancel during karabo.sleep
            self.local.cancelled_slot = None
            task = async(d.sleepalot())
            # Sleep for long enough for the macro to end up in the really long
            # sleep at the end
            yield from karabo_sleep(0.13)
            # Then cancel, while the macro is in that interruptable sleep
            yield from d.cancel()
            # Sleep a little while, so the task can finish
            yield from karabo_sleep(0.03)
            self.assertEqual(self.local.slept_count, 2)
            self.assertEqual(self.local.cancelled_slot, Local.sleepalot)
            assert task.done()

    @sync_tst
    def test_connectdevice(self):
        self.remote.value = 123
        d = connectDevice("remote")
        try:
            self.assertEqual(d.value, 123)
            self.remote.value = 456
            sleep(0.02)
            self.assertEqual(d.value, 456)
        finally:
            # check that the proxy gets collected when not used anymore
            weak = weakref.ref(d)
            del d
            self.assertIsNone(weak())


if __name__ == "__main__":
    main()
