import karabo

from asyncio import (async, coroutine, gather, set_event_loop, sleep, wait_for,
                     TimeoutError)
from unittest import TestCase, main

from karabo.api import Slot, Int
from karabo.python_device import Device
from karabo.device_client import (waitUntilNew, getDevice, waitUntil, setWait,
                                  setNoWait, Queue)
from karabo.schema import Configurable, Node

from .eventloop import startDevices, stopDevices, async_tst


class Superslot(Slot):
    @coroutine
    def method(self, device):
        device.value = 22


class SuperInteger(Int):
    def setter(self, device, value):
        device.value = 2 * value


class Nested(Configurable):
    val = Int()


class Remote(Device):
    value = Int(defaultValue=7)
    counter = Int(defaultValue=-1)

    nested = Node(Nested)

    @Int()
    def other(self, value):
        self.value = value

    @Int()
    def once(self, value):
        if self.once_value is None:
            self.once_value = value

    @Int(allowedStates=["Other state"])
    def disallowed_int(self, value):
        self.value = value

    @Slot(allowedStates=["uninitialized"])
    def disallow(self):
        self.state = "Other state"
        self.value = 777

    @Slot()
    @coroutine
    def doit(self):
        self.done = True

    @Slot()
    @coroutine
    def changeit(self):
        self.value -= 4

    def __init__(self, configuration):
        super().__init__(configuration)
        self.done = False
        self.once_value = None

    @Slot()
    @coroutine
    def count(self):
        for i in range(30):
            self.counter = i
            yield from sleep(0.1)

    generic = Superslot()
    generic_int = SuperInteger()


class Local(Device):
    @Slot()
    @coroutine
    def letitdo(self):
        with (yield from getDevice("remote")) as d:
            yield from d.doit()

    @Slot()
    @coroutine
    def letitchange(self):
        with (yield from getDevice("remote")) as d:
            yield from d.changeit()

    @Slot()
    @coroutine
    def disconnect(self):
        d = yield from getDevice("remote")
        task = async(d.count())
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
        yield from task

    @Slot()
    @coroutine
    def letset(self):
        with (yield from getDevice("remote")) as d:
            self.f1 = d.value
            d.value = 10
            yield from sleep(0.1)
            self.f2 = d.value
            yield from d.changeit()
            yield from sleep(0.1)
            self.f3 = d.value

    @Slot()
    @coroutine
    def dogeneric(self):
        d = yield from getDevice("remote")
        yield from d.generic()

    @Slot()
    @coroutine
    def dogenericint(self):
        d = yield from getDevice("remote")
        d.generic_int = 33
        yield from d

    @Slot()
    @coroutine
    def other(self):
        with (yield from getDevice("remote")) as d:
            d.other = 102

    @Slot()
    @coroutine
    def setwait(self):
        d = yield from getDevice("remote")
        yield from setWait(d, value=200, counter=300)

    @Slot()
    @coroutine
    def setnowait(self):
        d = yield from getDevice("remote")
        setNoWait(d, value=200, counter=300)

    @Slot()
    @coroutine
    def waituntil(self):
        with (yield from getDevice("remote")) as d:
            d.counter = 0
            yield from waitUntil(lambda: d.counter == 0)
            self.f1 = d.counter
            task = async(d.count())
            try:
                yield from waitUntil(lambda: d.counter > 10)
                self.f2 = d.counter
                try:
                    yield from wait_for(waitUntil(lambda: d.counter > 40),
                                        timeout=3)
                    self.timeout = False
                except TimeoutError:
                    self.timeout = True
            finally:
                yield from task

    @Slot()
    @coroutine
    def waituntilnew(self):
        with (yield from getDevice("remote")) as d:
            d.counter = 0
            yield from sleep(0.1)
            task = async(d.count())
            try:
                for i in range(30):
                    j = yield from waitUntilNew(d).counter
                    if i != j:
                        self.max = i
                        break
                else:
                    self.max = 30
            finally:
                yield from task

    @Slot()
    @coroutine
    def waituntildevice(self):
        with (yield from getDevice("remote")) as d:
            d.counter = 0
            yield from sleep(0.1)
            task = async(d.count())
            try:
                for i in range(30):
                    h = yield from waitUntilNew(d)
                    if i != h["counter"]:
                        self.max = i
                        break
                else:
                    self.max = 30
            finally:
                yield from task

    @Slot()
    @coroutine
    def collect_set(self):
        with (yield from getDevice("remote")) as d:
            d.once = 3
            d.once = 7
            d.once = 10

    @Slot()
    @coroutine
    def disallowed(self):
        with (yield from getDevice("remote")) as d:
            d.disallowed_int = 333
            yield from sleep(0.1)
            self.f1 = d.value
            yield from d.disallow()
            yield from sleep(0.1)
            self.f2 = d.value
            d.disallowed_int = 444
            yield from sleep(0.1)
            self.f3 = d.value
            yield from d.disallow()
            self.f4 = d.value

    @Slot()
    @coroutine
    def queue(self):
        with (yield from getDevice("remote")) as d:
            self.good = 0
            task = async(d.count())
            yield from waitUntil(lambda: d.counter == 0)
            try:
                q = Queue(d).counter
                for i in range(1, 30):
                    j = yield from q.get()
                    if i == j:
                        self.good += 1
                    yield from sleep(i * 0.01)
            finally:
                yield from task

    @coroutine
    def nested(self):
        with (yield from getDevice("remote")) as d:
            self.value = d.nested.val
            d.nested.val = 4


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
        self.assertTrue(local.f2 < 7 and (0 <= local.f3 - local.f2 < 2))
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
    def test_generic_int(self):
        remote.value = 7
        yield from local.dogenericint()
        yield from sleep(0.1)
        self.assertEqual(remote.value, 66)

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


    @async_tst
    def test_waituntilnew(self):
        yield from local.waituntilnew()
        self.assertEqual(local.max, 30)

    @async_tst
    def test_waituntildevice(self):
        yield from local.waituntildevice()
        self.assertEqual(local.max, 30)

    @async_tst
    def test_collect(self):
        yield from local.collect_set()
        yield from sleep(0.1)
        self.assertEqual(remote.once_value, 10)

    @async_tst
    def test_disallow(self):
        with self.assertLogs(logger="remote", level="ERROR"):
            yield from local.disallowed()
        self.assertEqual(local.f1, 333)
        self.assertEqual(local.f2, 777)
        self.assertEqual(local.f3, 777)
        self.assertEqual(local.f4, 777)

    @async_tst
    def test_queue(self):
        yield from local.queue()
        self.assertEqual(local.good, 29)

    @async_tst
    def test_nested(self):
        remote.nested.val = 3
        yield from local.nested()
        yield from sleep(1)
        self.assertEqual(local.value, 3)
        self.assertEqual(remote.nested.val, 4)


def setUpModule():
    global remote, local, loop
    local = Local({"_deviceId_": "local"})
    remote = Remote({"_deviceId_": "remote"})
    loop = startDevices(local, remote)
    Tests.instance = local


def tearDownModule():
    stopDevices(local, remote)


if __name__ == "__main__":
    main()
