import karabo

from asyncio import (async, coroutine, gather, set_event_loop, sleep,
                     TimeoutError)
from unittest import TestCase, main
from functools import wraps
import sys
import time

from karabo.eventloop import EventLoop
from karabo.macro import Macro
from karabo.python_device import Device
from karabo.python_server import KaraboStream
from karabo.device_client import (waitUntilNew, waitUntil, set, setNoWait,
                                  getDevice, executeNoWait, updateDevice,
                                  Queue)
from karabo import Slot, Integer

from .eventloop import startDevices, stopDevices, sync_tst, async_tst


class Superslot(Slot):
    @coroutine
    def method(self, device):
        device.value = 22


class Remote(Device):
    value = Integer(defaultValue=7)
    counter = Integer(defaultValue=-1)

    @Integer()
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

    generic = Superslot()


class Local(Macro):
    @Slot()
    def remotecalls(self):
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
        set(d, value=200, counter=300)

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
            yield from remote.call_local()
            self.assertEqual(local.print, "hero")
            self.assertEqual(local.printno, 2)
        finally:
            sys.stdout = sys.stdout.base

    @sync_tst
    def test_queue(self):
        local.queue()
        self.assertEqual(local.good, 29)


def setUpModule():
    global remote, local, loop
    local = Local(_deviceId_="local", project="test", module="test",
                  may_start_thread=False)
    remote = Remote(dict(_deviceId_="remote"))
    loop = startDevices(remote, local)
    Tests.instance = local


def tearDownModule():
    stopDevices(remote, local)


if __name__ == "__main__":
    main()
