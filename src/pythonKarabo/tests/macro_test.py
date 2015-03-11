import karabo
karabo.api_version = 2

from asyncio import (async, coroutine, gather, set_event_loop, sleep,
                     TimeoutError)
from unittest import TestCase, main
from functools import wraps
import time

from karabo.eventloop import EventLoop
from karabo.macro import Macro
from karabo.python_device import Device
from karabo import Slot, Integer

def async_tst(f):
    @wraps(f)
    def wrapper(self):
        loop.run_until_complete(loop.run_in_executor(None, f, self))
    return wrapper


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
    def doit(self):
        self.done = True

    @Slot()
    def changeit(self):
        self.value -= 4

    @Slot()
    def count(self):
        for i in range(30):
            self.counter = i
            yield from sleep(0.1)

    generic = Superslot()


class Local(Macro):
    @Slot()
    def letitdo(self):
        with self.getDevice("remote") as d:
            d.doit()

    @Slot()
    def letitchange(self):
        with self.getDevice("remote") as d:
            d.changeit()

    @Slot()
    def disconnect(self):
        d = self.getDevice("remote")
        d.count()
        time.sleep(0.3)
        self.f1 = d.counter
        time.sleep(0.3)
        with self.updateDevice(d):
            self.f2 = d.counter
        time.sleep(1)
        self.f3 = d.counter
        with d:
            time.sleep(2)
            self.f4 = d.counter

    @Slot()
    def letset(self):
        with self.getDevice("remote") as d:
            self.f1 = d.value
            d.value = 10
            time.sleep(0.1)
            self.f2 = d.value
            d.changeit()
            time.sleep(0.1)
            self.f3 = d.value

    @Slot()
    def dogeneric(self):
        d = self.getDevice("remote")
        d.generic()


    @Slot()
    def other(self):
        with self.getDevice("remote") as d:
            d.other = 102

    @Slot()
    def selfcall(self):
        self.other()

    @Slot()
    def setwait(self):
        d = self.getDevice("remote")
        self.set(d, value=200, counter=300)

    @Slot()
    def setnowait(self):
        d = self.getDevice("remote")
        self.setNoWait(d, value=200, counter=300)

    @Slot()
    def waituntil(self):
        with self.getDevice("remote") as d:
            d.counter = 0
            self.f1 = d.counter
            self.executeNoWait(d, "count")
            self.waitUntil(lambda: d.counter > 10)
            self.f2 = d.counter
            try:
                self.waitUntil(lambda: d.counter > 40, timeout=3)
                self.timeout = False
            except TimeoutError:
                self.timeout = True

    @Slot()
    def waituntilnew(self):
        with self.getDevice("remote") as d:
            d.counter = 0
            sleep(0.1)
            self.executeNoWait(d, "count")
            for i in range(30):
                j = self.waitUntilNew(d).counter
                if i != j:
                    self.max = i
                    break
            else:
                self.max = 30


class Tests(TestCase):
    @async_tst
    def test_execute(self):
        remote.done = False
        local.letitdo()
        time.sleep(0.1)
        self.assertTrue(remote.done)

    @async_tst
    def test_change(self):
        remote.value = 7
        local.letitchange()
        time.sleep(0.1)
        self.assertEqual(remote.value, 3)

    @async_tst
    def test_disconnect(self):
        local.disconnect()
        self.assertEqual(local.f1, -1)
        self.assertNotEqual(local.f2, -1)
        self.assertEqual(local.f2, local.f3)
        self.assertEqual(local.f4, 29)


    @async_tst
    def test_set(self):
        remote.value = 7
        local.letset()
        self.assertEqual(local.f1, 7)
        self.assertEqual(local.f2, 10)
        self.assertEqual(local.f3, 6)


    @async_tst
    def test_generic(self):
        remote.value = 7
        local.dogeneric()
        time.sleep(0.1)
        self.assertEqual(remote.value, 22)

    @async_tst
    def test_other(self):
        remote.value = 7
        local.other()
        time.sleep(0.1)
        self.assertEqual(remote.value, 102)

    @async_tst
    def test_selfcall(self):
        remote.value = 7
        local.selfcall()
        time.sleep(0.1)
        self.assertEqual(remote.value, 102)

    @async_tst
    def test_setwait(self):
        local.setwait()
        self.assertEqual(remote.value, 200)
        self.assertEqual(remote.counter, 300)

    @async_tst
    def test_setnowait(self):
        remote.value = 0
        remote.counter = 0
        local.setnowait()
        self.assertEqual(remote.value, 0)
        self.assertEqual(remote.counter, 0)
        time.sleep(0.1)
        self.assertEqual(remote.value, 200)
        self.assertEqual(remote.counter, 300)

    @async_tst
    def test_waituntil(self):
        local.waituntil()
        self.assertEqual(local.f1, 0)
        self.assertEqual(local.f2, 11)
        self.assertTrue(local.timeout)

    @async_tst
    def test_waituntilnew(self):
        local.waituntilnew()
        self.assertEqual(local.max, 30)


def setUpModule():
    global loop, remote, local
    loop = EventLoop()
    set_event_loop(loop)

    local = Local(_deviceId_="local", project="test", module="test")
    remote = Remote(dict(_deviceId_="remote"))
    loop.run_until_complete(gather(local.run_async(), remote.run_async()))


def tearDownModule():
    loop.run_until_complete(gather(
        local.slotKillDevice(), remote.slotKillDevice()))
    loop.close()


if __name__ == "__main__":
    main()
