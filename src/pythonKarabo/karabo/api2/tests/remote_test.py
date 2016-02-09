from asyncio import (async, coroutine, get_event_loop, sleep, wait_for,
                     TimeoutError)
from datetime import datetime
import gc
from unittest import TestCase, main, expectedFailure

from karabo.api import Slot, Int
from karabo.api2.device import Device
from karabo.api2.device_client import (
    waitUntilNew, getDevice, waitUntil, setWait, setNoWait, Queue)
from karabo.api2.hash import Hash, VectorChar
from karabo.api2 import openmq
from karabo.api2.schema import Configurable, Node

from .eventloop import startDevices, stopDevices, async_tst


class Superslot(Slot):
    @coroutine
    def method(self, device):
        device.value = 22


class SuperInteger(Int):
    def setter(self, device, value):
        device.value = 2 * value


class NestNest(Configurable):
    value = Int()


class Nested(Configurable):
    val = Int()
    nestnest = Node(NestNest)


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
            yield from sleep(0.02)

    generic = Superslot()
    generic_int = SuperInteger()
    logmessage = VectorChar()

    @Slot()
    @coroutine
    def read_log(self):
        consumer = openmq.Consumer(self._ss.session, self._ss.destination,
                                   "target = 'log'", False)
        message = yield from get_event_loop().run_in_executor(
            None, consumer.receiveMessage, 1000)
        self.logmessage = message.data


class Local(Device):
    @Slot()
    @coroutine
    def error(self):
        raise RuntimeError

    @Slot()
    @coroutine
    def task_error(self):
        async(self.error())

    @coroutine
    def onException(self, slot, exc, tb):
        self.exc_slot = slot
        self.exception = exc
        self.traceback = tb
        with (yield from getDevice("remote")) as d:
            yield from d.doit()


class Tests(TestCase):
    @async_tst
    def test_execute(self):
        """test the execution of remote slots"""
        self.assertFalse(remote.done)
        with (yield from getDevice("remote")) as d:
            yield from d.doit()
        yield from sleep(0.1)
        self.assertTrue(remote.done)

    @async_tst
    def test_change(self):
        """test changing a remote parameter"""
        remote.value = 7
        with (yield from getDevice("remote")) as d:
            yield from d.changeit()
        yield from sleep(0.1)
        self.assertEqual(remote.value, 3)

    @async_tst
    def test_disconnect(self):
        """test values are not updating when disconnected"""
        d = yield from getDevice("remote")
        task = async(d.count())
        yield from sleep(0.1)
        self.assertEqual(d.counter, -1)
        yield from sleep(0.1)
        with (yield from d):
            tmp = d.counter
            self.assertNotEqual(d.counter, -1)
        yield from sleep(0.1)
        self.assertLess(tmp, 12)
        self.assertLess(d.counter - tmp, 2)
        with d:
            yield from sleep(0.5)
            self.assertEqual(d.counter, 29)
        yield from task

    @async_tst
    def test_set(self):
        """test setting of remote values works"""
        remote.value = 7
        with (yield from getDevice("remote")) as d:
            self.assertEqual(d.value, 7)
            d.value = 10
            yield from sleep(0.1)
            self.assertEqual(d.value, 10)
            yield from d.changeit()
            yield from sleep(0.1)
            self.assertEqual(d.value, 6)

    @async_tst
    def test_generic(self):
        """test calling a generic slot"""
        remote.value = 7
        d = yield from getDevice("remote")
        yield from d.generic()
        yield from sleep(0.1)
        self.assertEqual(remote.value, 22)

    @async_tst
    def test_generic_int(self):
        """test setting a generic property"""
        remote.value = 7
        d = yield from getDevice("remote")
        d.generic_int = 33
        yield from d
        yield from sleep(0.1)
        self.assertEqual(remote.value, 66)

    @async_tst
    def test_setter(self):
        """test setting a property with a setter method"""
        with (yield from getDevice("remote")) as d:
            d.other = 102
        yield from sleep(0.1)
        self.assertEqual(remote.value, 102)

    @async_tst
    def test_setwait(self):
        """test the setWait coroutine"""
        d = yield from getDevice("remote")
        yield from setWait(d, value=200, counter=300)
        self.assertEqual(remote.value, 200)
        self.assertEqual(remote.counter, 300)

    @async_tst
    def test_setnowait(self):
        """test the setNoWait coroutine"""
        remote.value = 0
        remote.counter = 0
        d = yield from getDevice("remote")
        setNoWait(d, value=200, counter=300)
        self.assertEqual(remote.value, 0)
        self.assertEqual(remote.counter, 0)
        yield from sleep(0.1)
        self.assertEqual(remote.value, 200)
        self.assertEqual(remote.counter, 300)


    @async_tst
    def test_waituntil(self):
        """test the waitUntil coroutine"""
        with (yield from getDevice("remote")) as d:
            d.counter = 0
            yield from waitUntil(lambda: d.counter == 0)
            self.assertEqual(d.counter, 0)
            task = async(d.count())
            try:
                with self.assertRaises(TimeoutError):
                    yield from wait_for(waitUntil(lambda: d.counter > 10),
                                        timeout=0.1)
                yield from waitUntil(lambda: d.counter > 10)
                self.assertEqual(d.counter, 11)
                with self.assertRaises(TimeoutError):
                    yield from wait_for(waitUntil(lambda: d.counter > 40),
                                        timeout=1)
            finally:
                yield from task

    @async_tst
    def test_waituntilnew(self):
        """test the waitUntilNew coroutine for properties"""
        with (yield from getDevice("remote")) as d:
            d.counter = 0
            yield from sleep(0.1)
            task = async(d.count())
            try:
                for i in range(30):
                    j = yield from waitUntilNew(d).counter
                    self.assertEqual(i, j)
            finally:
                yield from task

    @async_tst
    def test_waituntildevice(self):
        """test the waitUntilNew coroutine for devices"""
        with (yield from getDevice("remote")) as d:
            d.counter = 0
            yield from sleep(0.1)
            task = async(d.count())
            try:
                for i in range(30):
                    h = yield from waitUntilNew(d)
                    self.assertEqual(i, h["counter"])
            finally:
                yield from task

    @async_tst
    def test_collect(self):
        """test that multiple settings are gathered into one"""
        with (yield from getDevice("remote")) as d:
            d.once = 3
            d.once = 7
            d.once = 10
        yield from sleep(0.1)
        self.assertEqual(remote.once_value, 10)

    @async_tst
    def test_disallow(self):
        """test that values cannot be set if in wrong state"""
        with self.assertLogs(logger="remote", level="ERROR"):
            with (yield from getDevice("remote")) as d:
                d.disallowed_int = 333
                yield from sleep(0.1)
                self.assertEqual(d.value, 333)
                yield from d.disallow()
                yield from sleep(0.1)
                self.assertEqual(d.value, 777)
                d.disallowed_int = 444
                yield from sleep(0.1)
                self.assertEqual(d.value, 777)
                yield from d.disallow()
                self.assertEqual(d.value, 777)

    @async_tst
    def test_log(self):
        """test the logging of warnings and exceptions"""
        with (yield from getDevice("remote")) as d:
            t = async(d.read_log())
            yield from sleep(0.1)
            local.logger.warning("this is an info")
            yield from t
        hash = Hash.decode(remote.logmessage, "Bin")
        hash = hash["messages"][0]
        self.assertEqual(hash["message"], "this is an info")
        self.assertEqual(hash["type"], "WARN")
        self.assertEqual(hash["category"], "local")
        self.assertEqual(hash["timestamp"][:18],
                         datetime.now().isoformat()[:18])
        with (yield from getDevice("remote")) as d:
            t = async(d.read_log())
            yield from sleep(0.1)
            try:
                raise RuntimeError
            except Exception:
                local.logger.exception("expected exception")
            yield from t
        hash = Hash.decode(remote.logmessage, "Bin")
        hash = hash["messages"][0]
        self.assertEqual(hash["message"], "expected exception")
        self.assertEqual(hash["type"], "ERROR")
        self.assertEqual(hash["category"], "local")
        self.assertEqual(hash["timestamp"][:18],
                         datetime.now().isoformat()[:18])
        self.assertEqual(hash["funcname"], "test_log")
        self.assertEqual(hash["module"], "remote_test")
        self.assertEqual(len(hash["traceback"]), 3)

    @async_tst
    def test_queue(self):
        """test queues of properties"""
        with (yield from getDevice("remote")) as d:
            task = async(d.count())
            yield from waitUntil(lambda: d.counter == 0)
            try:
                q = Queue(d).counter
                for i in range(1, 30):
                    j = yield from q.get()
                    self.assertEqual(i, j)
                    yield from sleep(i * 0.003)
            finally:
                yield from task

    @async_tst
    def test_nested(self):
        """test accessing nested properties"""
        remote.nested.val = 3
        remote.nested.nestnest.value = 7
        with (yield from getDevice("remote")) as d:
            self.assertEqual(d.nested.val, 3)
            self.f1 = d.nested.val
            self.f2 = d.nested.nestnest.value
            self.assertEqual(d.nested.nestnest.value, 7)
            d.nested.val = 4
            d.nested.nestnest.value = 5
        yield from sleep(0.1)
        self.assertEqual(remote.nested.val, 4)
        self.assertEqual(remote.nested.nestnest.value, 5)

    @async_tst
    def test_error(self):
        """test error reporting and calling of error methods"""
        remote.done = False
        with self.assertLogs(logger="local", level="ERROR"):
            with (yield from getDevice("local")) as d:
                yield from d.error()
            yield from sleep(0.1)
        self.assertTrue(remote.done)
        remote.done = False
        self.assertIs(local.exc_slot, Local.error)
        self.assertIsInstance(local.exception, RuntimeError)
        local.traceback.tb_lasti  # stupid check whether that is a traceback
        del local.exc_slot
        del local.exception
        del local.traceback

    @async_tst
    def test_task_error(self):
        """test that errors of created tasks are properly reported"""
        remote.done = False
        with self.assertLogs(logger="local", level="ERROR"):
            with (yield from getDevice("local")) as d:
                yield from d.task_error()
                gc.collect()
                yield from sleep(0.1)
        self.assertTrue(remote.done)
        remote.done = False
        self.assertIsNone(local.exc_slot)
        self.assertIsInstance(local.exception, RuntimeError)
        del local.exc_slot
        del local.exception
        del local.traceback


def setUpModule():
    global remote, local, loop
    local = Local({"_deviceId_": "local"})
    remote = Remote({"_deviceId_": "remote"})
    loop = startDevices(local, remote)
    Tests.instance = local


def tearDownModule():
    global remote, local
    stopDevices(local, remote)
    Tests.instance = remote = local = None


if __name__ == "__main__":
    main()
