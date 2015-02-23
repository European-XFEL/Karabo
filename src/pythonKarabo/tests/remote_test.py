import karabo
karabo.api_version = 2

from asyncio import (async, coroutine, gather, set_event_loop, sleep, wait_for,
                     TimeoutError)
from unittest import TestCase, main
from functools import wraps

from karabo.eventloop import EventLoop
from karabo.python_device import Device
from karabo import Slot, Integer

def async_tst(f):
    @wraps(f)
    def wrapper(self, *args, **kwargs):
        coro = coroutine(f)
        return loop.run_until_complete(coro(self, *args, **kwargs))
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

    def __init__(self, configuration):
        super().__init__(configuration)
        self.done = False

    @Slot()
    def count(self):
        for i in range(30):
            self.counter = i
            yield from sleep(0.1)

    generic = Superslot()


class Local(Device):
    @Slot()
    def letitdo(self):
        with (yield from self.getDevice("remote")) as d:
            d.doit()

    @Slot()
    def letitchange(self):
        with (yield from self.getDevice("remote")) as d:
            d.changeit()

    @Slot()
    def disconnect(self):
        d = yield from self.getDevice("remote")
        d.count()
        yield from sleep(0.3)
        self.f1 = d.counter
        yield from sleep(0.3)
        with (yield from d):
            self.f2 = d.counter
        yield from sleep(1)
        self.f3 = d.counter
        with d:
            yield from sleep(2)
            self.f4 = d.counter

    @Slot()
    def letset(self):
        with (yield from self.getDevice("remote")) as d:
            self.f1 = d.value
            d.value = 10
            yield from sleep(0.1)
            self.f2 = d.value
            d.changeit()
            yield from sleep(0.1)
            self.f3 = d.value

    @Slot()
    def dogeneric(self):
        d = yield from self.getDevice("remote")
        d.generic()

    @Slot()
    def other(self):
        with (yield from self.getDevice("remote")) as d:
            d.other = 102

    @Slot()
    def setwait(self):
        d = yield from self.getDevice("remote")
        yield from self.set(d, value=200, counter=300)

    @Slot()
    def setnowait(self):
        d = yield from self.getDevice("remote")
        self.setNoWait(d, value=200, counter=300)

    @Slot()
    def waituntil(self):
        with (yield from self.getDevice("remote")) as d:
            d.counter = 0
            yield from self.waitUntil(lambda: d.counter == 0)
            self.f1 = d.counter
            d.count()
            yield from self.waitUntil(lambda: d.counter > 10)
            self.f2 = d.counter
            try:
                yield from wait_for(self.waitUntil(lambda: d.counter > 40),
                                    timeout=3)
                self.timeout = False
            except TimeoutError:
                self.timeout = True


class Tests(TestCase):
    @async_tst
    def test_execute(self):
        self.assertFalse(remote.done)
        yield from local.letitdo()
        yield from sleep(0.1)
        self.assertTrue(remote.done)

    @async_tst
    def test_change(self):
        remote.value = 7
        yield from local.letitchange()
        yield from sleep(0.1)
        self.assertEqual(remote.value, 3)

    @async_tst
    def test_disconnect(self):
        yield from local.disconnect()
        self.assertEqual(local.f1, -1)
        self.assertNotEqual(local.f2, -1)
        self.assertEqual(local.f2, local.f3)
        self.assertEqual(local.f4, 29)


    @async_tst
    def test_set(self):
        remote.value = 7
        yield from local.letset()
        self.assertEqual(local.f1, 7)
        self.assertEqual(local.f2, 10)
        self.assertEqual(local.f3, 6)


    @async_tst
    def test_generic(self):
        remote.value = 7
        yield from local.dogeneric()
        yield from sleep(0.1)
        self.assertEqual(remote.value, 22)

    @async_tst
    def test_other(self):
        yield from local.other()
        yield from sleep(0.1)
        self.assertEqual(remote.value, 102)

    @async_tst
    def test_setwait(self):
        yield from local.setwait()
        self.assertEqual(remote.value, 200)
        self.assertEqual(remote.counter, 300)

    @async_tst
    def test_setnowait(self):
        remote.value = 0
        remote.counter = 0
        yield from local.setnowait()
        self.assertEqual(remote.value, 0)
        self.assertEqual(remote.counter, 0)
        yield from sleep(0.1)
        self.assertEqual(remote.value, 200)
        self.assertEqual(remote.counter, 300)


    @async_tst
    def test_waituntil(self):
        yield from local.waituntil()
        self.assertEqual(local.f1, 0)
        self.assertEqual(local.f2, 11)
        self.assertTrue(local.timeout)


def setUpModule():
    global loop, remote, local
    loop = EventLoop()
    set_event_loop(loop)

    local = Local({"_deviceId_": "local"})
    remote = Remote({"_deviceId_": "remote"})
    loop.run_until_complete(gather(local.run_async(), remote.run_async()))


def tearDownModule():
    loop.run_until_complete(gather(
        local.slotKillDevice(), remote.slotKillDevice()))
    loop.close()


if __name__ == "__main__":
    main()
