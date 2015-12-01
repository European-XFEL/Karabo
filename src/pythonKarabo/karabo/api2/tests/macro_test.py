from asyncio import (async, coroutine, gather, set_event_loop,
                     TimeoutError)
import sys
import time
from unittest import TestCase, main

from karabo.api import Slot, Int, sleep
from karabo.api2.device import Device
from karabo.api2.device_client import (
    waitUntilNew, waitUntil, setWait, setNoWait, getDevice, executeNoWait,
    updateDevice, Queue)
from karabo.api2.device_server import KaraboStream
from karabo.api2.macro import Macro

from .eventloop import startDevices, stopDevices, sync_tst, async_tst


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
            yield from sleep(0.1)

    @Slot()
    @coroutine
    def call_local(self):
        with (yield from getDevice("local")) as l:
            yield from l.remotecalls()

    @Slot()
    @coroutine
    def error(self):
        with (yield from getDevice("local")) as d:
            yield from d.error()

    generic = Superslot()


class Local(Macro):
    @Slot()
    def remotecalls(self):
        self.nowstate = self.state
        print("superpuper", end="hero")

    @Slot()
    def letitdo(self):
        with getDevice("remote") as d:
            d.doit()

    @Slot()
    def letitchange(self):
        with getDevice("remote") as d:
            d.changeit()

    @Slot()
    def disconnect(self):
        d = getDevice("remote")
        executeNoWait(d, "count")
        time.sleep(0.3)
        self.f1 = d.counter
        time.sleep(0.3)
        with updateDevice(d):
            self.f2 = d.counter
        time.sleep(1)
        self.f3 = d.counter
        with d:
            time.sleep(2)
            self.f4 = d.counter

    @Slot()
    def letset(self):
        with getDevice("remote") as d:
            self.f1 = d.value
            d.value = 10
            time.sleep(0.1)
            self.f2 = d.value
            d.changeit()
            time.sleep(0.1)
            self.f3 = d.value

    @Slot()
    def dogeneric(self):
        d = getDevice("remote")
        d.generic()


    @Slot()
    def other(self):
        with getDevice("remote") as d:
            d.other = 102

    @Slot()
    def selfcall(self):
        self.other()

    @Slot()
    def setwait(self):
        d = getDevice("remote")
        setWait(d, value=200, counter=300)

    @Slot()
    def setnowait(self):
        d = getDevice("remote")
        setNoWait(d, value=200, counter=300)

    @Slot()
    def waituntil(self):
        with getDevice("remote") as d:
            d.counter = 0
            self.f1 = d.counter
            executeNoWait(d, "count")
            waitUntil(lambda: d.counter > 10)
            self.f2 = d.counter
            try:
                waitUntil(lambda: d.counter > 40, timeout=3)
                self.timeout = False
            except TimeoutError:
                self.timeout = True

    @Slot()
    def waituntilnew(self):
        with getDevice("remote") as d:
            d.counter = 0
            sleep(0.1)
            executeNoWait(d, "count")
            for i in range(30):
                j = waitUntilNew(d).counter
                if i != j:
                    self.max = i
                    break
            else:
                self.max = 30

    @Slot()
    def queue(self):
        with getDevice("remote") as d:
            self.good = 0
            executeNoWait(d, "count")
            waitUntil(lambda: d.counter == 0)
            q = Queue(d).counter
            for i in range(1, 30):
                j = q.get()
                if i == j:
                    self.good += 1
                time.sleep(i * 0.01)

    @Slot()
    def error(self):
        raise RuntimeError

    def onException(self, slot, exc, tb):
        self.exc_slot = slot
        self.exception = exc
        self.traceback = tb
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


class Tests(TestCase):
    @sync_tst
    def test_execute(self):
        remote.done = False
        local.letitdo()
        time.sleep(0.1)
        self.assertTrue(remote.done)

    @sync_tst
    def test_change(self):
        remote.value = 7
        local.letitchange()
        time.sleep(0.1)
        self.assertEqual(remote.value, 3)

    @sync_tst
    def test_disconnect(self):
        local.disconnect()
        self.assertEqual(local.f1, -1)
        self.assertNotEqual(local.f2, -1)
        self.assertEqual(local.f2, local.f3)
        self.assertEqual(local.f4, 29)


    @sync_tst
    def test_set(self):
        remote.value = 7
        local.letset()
        self.assertEqual(local.f1, 7)
        self.assertEqual(local.f2, 10)
        self.assertEqual(local.f3, 6)


    @sync_tst
    def test_generic(self):
        remote.value = 7
        local.dogeneric()
        time.sleep(0.1)
        self.assertEqual(remote.value, 22)

    @sync_tst
    def test_other(self):
        remote.value = 7
        local.other()
        time.sleep(0.1)
        self.assertEqual(remote.value, 102)

    @sync_tst
    def test_selfcall(self):
        remote.value = 7
        local.selfcall()
        time.sleep(0.1)
        self.assertEqual(remote.value, 102)

    @sync_tst
    def test_setwait(self):
        local.setwait()
        self.assertEqual(remote.value, 200)
        self.assertEqual(remote.counter, 300)

    @sync_tst
    def test_setnowait(self):
        remote.value = 0
        remote.counter = 0
        local.setnowait()
        self.assertEqual(remote.value, 0)
        self.assertEqual(remote.counter, 0)
        time.sleep(0.1)
        self.assertEqual(remote.value, 200)
        self.assertEqual(remote.counter, 300)

    @sync_tst
    def test_waituntil(self):
        local.waituntil()
        self.assertEqual(local.f1, 0)
        self.assertEqual(local.f2, 11)
        self.assertTrue(local.timeout)

    @sync_tst
    def test_waituntilnew(self):
        local.waituntilnew()
        self.assertEqual(local.max, 30)

    @async_tst
    def test_remotecalls(self):
        sys.stdout = KaraboStream(sys.stdout)
        try:
            self.assertEqual(local.state, "Idle...")
            yield from remote.call_local()
            self.assertEqual(local.nowstate, "remotecalls")
            self.assertEqual(local.state, "Idle...")
            self.assertEqual(local.print, "hero")
            self.assertEqual(local.printno, 2)
        finally:
            sys.stdout = sys.stdout.base

    @sync_tst
    def test_queue(self):
        local.queue()
        self.assertEqual(local.good, 29)

    @async_tst
    def test_error(self):
        remote.done = False
        with self.assertLogs(logger="local", level="ERROR"):
            yield from remote.error()
            yield from sleep(0.1)
        self.assertTrue(remote.done)
        self.assertIs(local.exc_slot, Local.error)
        self.assertIsInstance(local.exception, RuntimeError)
        local.traceback.tb_lasti  # stupid check whether that is a traceback
        del local.exc_slot
        del local.exception
        del local.traceback

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
            # cancel during non-karabo sleep
            local.cancelled_slot = None
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
            yield from karabo_sleep(0.2)
            self.assertEqual(local.cancelled_slot, Local.sleepalot)
            self.assertEqual(local.slept_count, 1)
            assert task.done()

            # cancel during karabo sleep
            local.cancelled_slot = None
            task = async(d.sleepalot())
            # Sleep for long enough for the macro to end up in the really long
            # sleep at the end
            yield from karabo_sleep(0.13)
            # Then cancel, while the macro is in that interruptable sleep
            yield from d.cancel()
            # Sleep a little while, so the task can finish
            yield from karabo_sleep(0.01)
            self.assertEqual(local.slept_count, 2)
            self.assertEqual(local.cancelled_slot, Local.sleepalot)
            assert task.done()


def setUpModule():
    global remote, local, loop
    local = Local(_deviceId_="local", project="test", module="test",
                  may_start_thread=False)
    remote = Remote(dict(_deviceId_="remote"))
    loop = startDevices(remote, local)
    Tests.instance = local


def tearDownModule():
    global remote, local
    stopDevices(remote, local)
    Tests.instance = remote = local = None


if __name__ == "__main__":
    main()
