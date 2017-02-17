from asyncio import (async, coroutine, Future, gather, get_event_loop, sleep,
                     wait_for, TimeoutError)
from contextlib import contextmanager
from datetime import datetime
from unittest import main
import time
import weakref

from pint import DimensionalityError

from karabo.middlelayer import (
    AlarmCondition, Bool, Configurable, connectDevice, Device, DeviceNode,
    execute, Float, getDevice, Hash, isSet, Int32, KaraboError,
    lock, MetricPrefix, Node, setNoWait, setWait, Slot, slot, State, String,
    unit, Unit, VectorChar, VectorInt16, VectorString, VectorFloat, waitUntil,
    waitUntilNew)
from karabo.middlelayer_api import openmq
from karabo.middlelayer_api.device_client import Queue
from karabo.middlelayer_api.injectable import Injectable

from .eventloop import DeviceTest, async_tst


class Superslot(Slot):
    @coroutine
    def method(self, device):
        device.value = 22


class SuperInteger(Int32):
    def setter(self, device, value):
        if isSet(value):
            device.value = 2 * value


class NestNest(Configurable):
    value = Int32(unitSymbol=Unit.METER)


class Nested(Configurable):
    val = Int32(unitSymbol=Unit.SECOND, metricPrefixSymbol=MetricPrefix.MILLI)
    nestnest = Node(NestNest)


class Remote(Injectable, Device):
    done = Bool()
    value = Int32(description="The Value", defaultValue=7, tags={"whatever"})
    counter = Int32(defaultValue=-1)

    unit_int = Int32(unitSymbol=Unit.METER)
    unit_float = Float(unitSymbol=Unit.SECOND,
                       metricPrefixSymbol=MetricPrefix.MILLI)
    string = String()

    unit_vector_int = VectorInt16(unitSymbol=Unit.METER_PER_SECOND)
    unit_vector_float = VectorFloat(unitSymbol=Unit.DEGREE)
    string_list = VectorString()

    nested = Node(Nested)

    alarm = Float(warnLow=10, alarmInfo_warnLow="When they go low, we go high",
                  alarmNeedsAck_warnLow=True, alarmHigh=20)

    @Int32()
    def other(self, value):
        self.value = value

    @Int32()
    def once(self, value):
        if isSet(value) and self.once_value is None:
            self.once_value = value

    @Int32()
    def error_value(self, value):
        if isSet(value):
            raise RuntimeError

    @Int32(allowedStates=[State.ON])
    def disallowed_int(self, value):
        self.value = value
        self.state = State.UNKNOWN

    @Slot(allowedStates=[State.UNKNOWN])
    @coroutine
    def allow(self):
        self.state = State.ON
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
    alarmhash = None

    @Slot()
    @coroutine
    def error(self):
        raise RuntimeError

    @Slot()
    @coroutine
    def error_in_error(self):
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
        if slot is Local.error_in_error:
            raise RuntimeError
        else:
            with (yield from getDevice("remote")) as d:
                yield from d.doit()

    @slot
    def slotAlarmUpdate(self, deviceId, alarms):
        self.alarm_future.set_result(None)
        self.alarmhash = alarms


class Tests(DeviceTest):
    """The tests in this class run on behalf of the device "local".
    As they also have a reference to the remote device "remote", they
    can also modify attributes of "remote" directly, as if "remote" had
    done it itself.

    The tests then typically call the remote device via a proxy, which is
    generated, if needed, in every test.
    """
    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.local = Local({"_deviceId_": "local"})
        cls.remote = Remote({"_deviceId_": "remote"})
        with cls.deviceManager(cls.remote, lead=cls.local):
            yield

    @async_tst
    def test_execute(self):
        """test the execution of remote slots"""
        self.remote.done = False
        with (yield from getDevice("remote")) as d:
            yield from d.doit()
        self.assertTrue(self.remote.done)

    @async_tst
    def test_execute_noproxy(self):
        self.remote.done = False
        yield from execute("remote", "doit")
        self.assertTrue(self.remote.done)

    @async_tst
    def test_set_remote(self):
        """test setting of values"""
        with (yield from getDevice("remote")) as d:
            d.string = "bla"
            d.unit_int = 3 * unit.meter
            d.unit_float = 7.5 * unit.millisecond
            d.unit_vector_int = [3, 4, 5] * unit.meter / unit.second
            d.unit_vector_float = [1, 2, 3]
            d.string_list = ["a", "bla"]
            yield from waitUntilNew(d).string_list

            self.assertEqual(d.string, "bla")
            self.assertLess(abs(d.string.timestamp.toTimestamp() -
                                time.time()), 1)
            self.assertEqual(d.unit_int, 3 * unit.meter)
            self.assertEqual(d.unit_float, 7.5 * unit.millisecond)
            for a, b in zip(d.unit_vector_int,
                            [3, 4, 5] * unit.meter / unit.second):
                self.assertEqual(a, b)
            for a, b in zip(d.unit_vector_float, [1, 2, 3] * unit.degree):
                self.assertEqual(a, b)
            for a, b in zip(d.string_list, ["a", "bla"]):
                self.assertEqual(a, b)

        self.assertEqual(self.remote.string, "bla")
        self.assertLess(abs(self.remote.string.timestamp.toTimestamp() -
                            time.time()), 1)
        self.assertEqual(self.remote.unit_int, 3 * unit.meter)
        self.assertEqual(self.remote.unit_float, 7.5 * unit.millisecond)
        for a, b in zip(self.remote.unit_vector_int,
                        [3, 4, 5] * unit.meter / unit.second):
            self.assertEqual(a, b)
        for a, b in zip(self.remote.unit_vector_float,
                        [1, 2, 3] * unit.degree):
            self.assertEqual(a, b)
        for a, b in zip(self.remote.string_list, ["a", "bla"]):
            self.assertEqual(a, b)

        with (yield from getDevice("remote")) as d:
            with self.assertRaises(DimensionalityError):
                d.unit_int = 3 * unit.second
            with self.assertRaises(DimensionalityError):
                d.unit_float = 2 * unit.meter
            with self.assertRaises(DimensionalityError):
                d.unit_vector_int = [1, 2] * unit.meter
            with self.assertRaises(DimensionalityError):
                d.unit_vector_float = [1, 3] * unit.meter
            with self.assertRaises(TypeError):
                d.string_list = [3, 4]

            d.unit_int = 5 * unit.kilometer
            d.unit_float = 2 * unit.millisecond
            d.unit_vector_int = [2, 3, 4] * unit.meter / unit.second
            d.unit_vector_float = [5, 7, 8] * unit.degree
            d.string_list = []
            yield from waitUntilNew(d).string_list

            self.assertEqual(d.unit_int, 5000 * unit.meter)
            self.assertEqual(d.unit_float, 2 * unit.millisecond)
            for a, b in zip(d.unit_vector_int,
                            [2, 3, 4] * unit.meter / unit.second):
                self.assertEqual(a, b)
            for a, b in zip(d.unit_vector_float, [5, 7, 8] * unit.degree):
                self.assertEqual(a, b)
            self.assertEqual(d.string_list, [])

        self.assertEqual(self.remote.unit_int, 5000 * unit.meter)
        self.assertEqual(self.remote.unit_float, 2 * unit.millisecond)
        for a, b in zip(self.remote.unit_vector_int,
                        [2, 3, 4] * unit.meter / unit.second):
            self.assertEqual(a, b)
        for a, b in zip(self.remote.unit_vector_float,
                        [5, 7, 8] * unit.degree):
            self.assertEqual(a, b)
        self.assertEqual(self.remote.string_list, [])

    @async_tst
    def test_set_local(self):
        self.remote.unit_int = 3
        with self.assertRaises(DimensionalityError):
            self.remote.unit_int = 3 * unit.second / unit.second
        self.remote.unit_float = 7.5
        with self.assertRaises(DimensionalityError):
            self.remote.unit_float = 7.5 * unit.m / unit.m
        self.remote.unit_vector_int = [3, 4, 5]
        with self.assertRaises(DimensionalityError):
            self.remote.unit_vector_int = [3, 4, 5] * unit.m / unit.m
        with self.assertRaises(DimensionalityError):
            self.remote.unit_int = 3 * unit.second
        with self.assertRaises(DimensionalityError):
            self.remote.unit_float = 2 * unit.meter
        with self.assertRaises(DimensionalityError):
            self.remote.unit_vector_int = [1, 2] * unit.meter
        with self.assertRaises(DimensionalityError):
            self.remote.unit_vector_float = [1, 3] * unit.meter
        with self.assertRaises(TypeError):
            self.remote.string_list = [3, 4]

        self.remote.string = "blub"
        self.remote.unit_vector_float = [1, 3, 3]
        self.remote.string_list = ["z", "bla"]

        with (yield from getDevice("remote")) as d:
            self.assertEqual(d.string, "blub")
            self.assertLess(abs(d.string.timestamp.toTimestamp() -
                                time.time()), 1)
            for a, b in zip(d.unit_vector_float, [1, 3, 3] * unit.degree):
                self.assertEqual(a, b)
            for a, b in zip(d.string_list, ["z", "bla"]):
                self.assertEqual(a, b)

        self.assertEqual(self.remote.string, "blub")
        self.assertLess(abs(self.remote.string.timestamp.toTimestamp() -
                            time.time()), 1)
        for a, b in zip(self.remote.unit_vector_float,
                        [1, 3, 3] * unit.degree):
            self.assertEqual(a, b)
        for a, b in zip(self.remote.string_list, ["z", "bla"]):
            self.assertEqual(a, b)

        with (yield from getDevice("remote")) as d:
            self.remote.unit_int = 5 * unit.kilometer
            self.remote.unit_float = 2 * unit.millisecond
            self.remote.unit_vector_int = [2, 3, 4] * unit.meter / unit.second
            self.remote.unit_vector_float = [5, 7, 8] * unit.degree
            self.remote.string_list = []
            yield from d

            self.assertEqual(d.unit_int, 5000 * unit.meter)
            self.assertEqual(d.unit_float, 2 * unit.millisecond)
            for a, b in zip(d.unit_vector_int,
                            [2, 3, 4] * unit.meter / unit.second):
                self.assertEqual(a, b)
            for a, b in zip(d.unit_vector_float, [5, 7, 8] * unit.degree):
                self.assertEqual(a, b)
            self.assertEqual(d.string_list, [])

        self.assertEqual(self.remote.unit_int, 5000 * unit.meter)
        self.assertEqual(self.remote.unit_float, 2 * unit.millisecond)
        for a, b in zip(self.remote.unit_vector_int,
                        [2, 3, 4] * unit.meter / unit.second):
            self.assertEqual(a, b)
        for a, b in zip(self.remote.unit_vector_float,
                        [5, 7, 8] * unit.degree):
            self.assertEqual(a, b)
        self.assertEqual(self.remote.string_list, [])

    @async_tst
    def test_change(self):
        """test changing a remote parameter"""
        self.remote.value = 7
        with (yield from getDevice("remote")) as d:
            yield from d.changeit()
        self.assertEqual(self.remote.value, 3)

    @async_tst
    def test_state(self):
        self.remote.state = State.UNKNOWN
        with (yield from getDevice("remote")) as d:
            self.assertEqual(d.state, State.UNKNOWN)
            self.assertEqual(d.alarmCondition, AlarmCondition.NONE)

    @async_tst
    def test_disconnect(self):
        """test values are not updating when disconnected"""
        self.remote.counter = -1
        # there are often still changes on the way that we don't want to see
        yield from sleep(0.1)
        d = yield from getDevice("remote")
        task = async(d.count())
        yield from sleep(0.1)
        self.assertEqual(d.counter, -1,
                         "we're not connected still seeing changes")
        yield from sleep(0.1)
        with (yield from d):
            yield from sleep(0.1)
            tmp = d.counter
            self.assertNotEqual(d.counter, -1,
                                "not seeing changes although connected")
        yield from sleep(0.1)
        # sanity check: the counter should still be running
        self.assertLess(tmp, 20)
        self.assertLess(d.counter - tmp, 2,
                        "to many messages arrived after disconnecting")
        with d:
            yield from sleep(0.5)
            self.assertEqual(d.counter, 29)
        yield from task

    @async_tst
    def test_set(self):
        """test setting of remote values works"""
        self.remote.value = 7
        with (yield from getDevice("remote")) as d:
            self.assertEqual(d.value, 7)
            d.value = 10
            yield from sleep(0.1)
            self.assertEqual(d.value, 10)
            yield from d.changeit()
            self.assertEqual(d.value, 6)

    @async_tst
    def test_generic(self):
        """test calling a generic slot"""
        self.remote.value = 7
        d = yield from getDevice("remote")
        yield from d.generic()
        self.assertEqual(self.remote.value, 22)

    @async_tst
    def test_generic_int(self):
        """test setting a generic property"""
        self.remote.value = 7
        d = yield from getDevice("remote")
        d.generic_int = 33
        yield from d
        self.assertEqual(self.remote.value, 66)

    @async_tst
    def test_setter(self):
        """test setting a property with a setter method"""
        with (yield from getDevice("remote")) as d:
            d.other = 102
        yield from sleep(0.1)
        self.assertEqual(self.remote.value, 102)

    @async_tst
    def test_setwait(self):
        """test the setWait coroutine"""
        d = yield from getDevice("remote")
        yield from setWait(d, value=200, counter=300)
        self.assertEqual(self.remote.value, 200)
        self.assertEqual(self.remote.counter, 300)

    @async_tst
    def test_setnowait(self):
        """test the setNoWait coroutine"""
        self.remote.value = 0
        self.remote.counter = 0
        d = yield from getDevice("remote")
        setNoWait(d, value=200, counter=300)
        self.assertEqual(self.remote.value, 0)
        self.assertEqual(self.remote.counter, 0)
        yield from sleep(0.1)
        self.assertEqual(self.remote.value, 200)
        self.assertEqual(self.remote.counter, 300)

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

                # there are many "while True:" busy loops out there.
                # assure we don't get stuck if the condition is always true
                # it is important to have no other yield from except the
                # waitUntil in the loop to test that it actually gives the
                # event loop a chance to run. This test, if succeeding, is
                # fast, but if failing would kill the entire test suite,
                # that's why there is an upper time limit for it.
                t0 = time.time()
                while d.counter < 15 and time.time() - t0 < 1:
                    yield from waitUntil(lambda: True)
                self.assertLess(time.time() - t0, 1)

                with self.assertRaises(TimeoutError):
                    yield from wait_for(waitUntil(lambda: d.counter > 40),
                                        timeout=0.01)
            finally:
                yield from task

    @async_tst
    def test_waituntilnew_old(self):
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
    def test_waituntilnew_new(self):
        """test the waitUntilNew coroutine for properties"""
        self.remote.counter = None
        with (yield from getDevice("remote")) as d:
            d.counter = 0
            # we test that d.counter is still None (it must be, no yield from
            # since last line). This asserts that waitUntilNew also works
            # with uninitialized values, which had been a bug before.
            self.assertEqual(d.counter, None)
            yield from waitUntilNew(d.counter)
            task = async(d.count())
            try:
                for i in range(30):
                    j = yield from waitUntilNew(d.counter)
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
        self.assertEqual(self.remote.once_value, 10)

    @async_tst
    def test_error_value(self):
        d = yield from getDevice("remote")
        d.error_value = 3
        with self.assertRaises(KaraboError):
            yield from d.doit()

    @async_tst
    def test_disallow(self):
        """test that values cannot be set if in wrong state"""
        self.assertEqual(self.remote.state, State.UNKNOWN)
        with (yield from getDevice("remote")) as d:
            d.value = 7
            yield from waitUntil(lambda: d.value == 7)
            # disallowed_int is normally not allowed
            with self.assertLogs(logger="remote", level="WARNING") as log:
                # no error here as we need to wait for network round-trip
                d.disallowed_int = 333
                yield from sleep(0.02)
            with self.assertRaises(KaraboError):
                # this raises the error from above
                d.value = 8
            with self.assertLogs(logger="remote", level="WARNING") as log,\
                    self.assertRaises(KaraboError):
                d.disallowed_int = 333
                yield from d.allow()
            self.assertEqual(len(log.records), 1)
            self.assertEqual(log.records[0].levelname, "WARNING")
            self.assertEqual(d.value, 7)

            # d.allow() sets d.value to 777 and is changing state such that
            # disallowed_int can be set ...
            yield from d.allow()
            self.assertEqual(d.value, 777)
            d.value = 4
            # ... but it cannot be called itself anymore ...
            with self.assertLogs(logger="remote", level="WARN") as logs, \
                    self.assertRaises(KaraboError):
                yield from d.allow()
            self.assertEqual(len(logs.records), 1)
            self.assertEqual(logs.records[0].levelname, "WARNING")
            self.assertEqual(d.value, 4)
            # ... but disallowed_int sets this back ...
            d.disallowed_int = 444
            yield from waitUntil(lambda: d.value == 444)
            self.assertEqual(d.value, 444)
            # ... so allow can be called again!
            yield from d.allow()
            self.assertEqual(d.value, 777)

    @async_tst
    def test_log(self):
        """test the logging of warnings and exceptions"""
        with (yield from getDevice("remote")) as d:
            t = async(d.read_log())
            yield from sleep(0.1)
            self.local.logger.warning("this is an info")
            yield from t
        hash = Hash.decode(self.remote.logmessage, "Bin")
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
                self.local.logger.exception("expected exception")
            yield from t
        hash = Hash.decode(self.remote.logmessage, "Bin")
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
        self.remote.nested.val = 3 * unit.second
        self.assertLess(
            abs(self.remote.nested.val.timestamp.toTimestamp() - time.time()),
            1)
        ts1 = self.remote.nested.val.timestamp
        self.remote.nested.nestnest.value = 7 * unit.meter
        ts2 = self.remote.nested.nestnest.value.timestamp
        with (yield from getDevice("remote")) as d:
            self.assertEqual(d.nested.val, 3 * unit.second)
            self.assertEqual(d.nested.val.timestamp, ts1)
            self.assertEqual(d.nested.nestnest.value, 7 * unit.meter)
            d.nested.val = 4 * unit.second
            self.assertEqual(d.nested.nestnest.value, 7 * unit.meter)
            self.assertEqual(d.nested.nestnest.value.timestamp, ts2)
            d.nested.val = 4 * unit.second
            d.nested.nestnest.value = 5 * unit.meter
            yield from waitUntilNew(d)
        self.assertEqual(self.remote.nested.val, 4 * unit.second)
        self.assertEqual(self.remote.nested.nestnest.value, 5 * unit.meter)

    @async_tst
    def test_error(self):
        """test error reporting and calling of error methods"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR"):
            with (yield from getDevice("local")) as d, \
                    self.assertRaises(KaraboError):
                yield from d.error()
        self.assertTrue(self.remote.done)
        self.remote.done = False
        self.assertIs(self.local.exc_slot, Local.error)
        self.assertIsInstance(self.local.exception, RuntimeError)
        self.local.traceback.tb_lasti  # check whether that is a traceback
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback

    @async_tst
    def test_error_in_error(self):
        """test what happens if an error happens in an error method"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR") as logs:
            with (yield from getDevice("local")) as d, \
                    self.assertRaises(KaraboError):
                yield from d.error_in_error()
        self.assertEqual(logs.records[-1].msg, "error in error handler")
        self.assertFalse(self.remote.done)
        self.assertIs(self.local.exc_slot, Local.error_in_error)
        self.assertIsInstance(self.local.exception, RuntimeError)
        self.local.traceback.tb_lasti  # check whether that is a traceback
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback

    @async_tst
    def test_task_error(self):
        """test that errors of created tasks are properly reported"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR"):
            with (yield from getDevice("local")) as local, \
                 (yield from getDevice("remote")) as remote:
                yield from local.task_error()
                self.assertFalse(remote.done)
                yield from waitUntil(lambda: remote.done)
        self.assertIsNone(self.local.exc_slot)
        self.assertIsInstance(self.local.exception, RuntimeError)
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback

    @async_tst
    def test_connectDevice(self):
        try:
            d = yield from connectDevice("remote")
            self.assertNotEqual(d.value, 123)
            self.remote.value = 123
            yield from sleep(0.02)
            self.assertEqual(d.value, 123)
        finally:
            # check the proxy gets collected when not used anymore
            weak = weakref.ref(d)
            del d
            self.assertIsNone(weak())

    @async_tst
    def test_getdoc(self):
        # this is usually done by iPython/iKarabo
        d = yield from getDevice("remote")
        self.assertEqual(d.value.getdoc(), "The Value")

    @async_tst
    def test_devicenode(self):
        class A(Device):
            dn = DeviceNode(properties=["value", {"counter": "cntr",
                                                  "other": "othr"}],
                            commands=[{"doit": "do_it"}, "changeit"])

        a = A({"_deviceId_": "devicenode", "dn": "remote"})
        yield from a.startInstance()
        try:
            a.dn.value = 5
            yield from a.dn
            self.assertEqual(self.remote.value, 5)

            with (yield from getDevice("devicenode")) as d:
                self.assertEqual(d.lockedBy, "")
                self.assertEqual(d.dn.value, 5)
                self.assertEqual(d.dn.cntr, -1)
                self.assertEqual(type(d).dn.displayType, "deviceNode")
                d.dn.value = 8
                d.dn.cntr = 12
                self.remote.done = False
                yield from d.dn.do_it()
                self.assertEqual(d.dn.value, 8)
                self.assertEqual(d.dn.cntr, 12)
                self.assertEqual(a.dn.value, 8)
                self.assertEqual(a.dn.counter, 12)
                self.assertEqual(a.dn.state, State.UNKNOWN)
                self.assertEqual(a.dn.alarmCondition, AlarmCondition.NONE)
                self.assertTrue(self.remote.done)
                d.dn.value = 22
                yield from d.dn.changeit()
                self.assertEqual(d.dn.value, 18)
                self.assertEqual(a.dn.value, 18)

                d.dn.othr = 111
                yield from d
                self.assertFalse(isSet(d.dn.othr))
        finally:
            yield from a.slotKillDevice()

    @async_tst
    def test_devicenode_nocopy_lock(self):
        class A(Device):
            dn = DeviceNode(lock=True)

        a = A({"_deviceId_": "devicenode", "dn": "remote"})
        yield from a.startInstance()
        try:
            self.assertEqual(a.dn.lockedBy, "devicenode")
            a.dn.value = 5
            yield from a.dn
            self.assertEqual(self.remote.value, 5)
        finally:
            yield from a.slotKillDevice()
        with (yield from getDevice("remote")) as proxy:
            yield from waitUntil(lambda: proxy.lockedBy == "")

    @async_tst
    def test_devicenode_novalue(self):
        class A(Device):
            dn = DeviceNode()

        a = A({"_deviceId_": "devicenode"})
        yield from a.startInstance()
        self.assertIs(a.dn, None)

    @async_tst
    def test_prenatal_proxy(self):
        task = async(getDevice("prenatal"))
        a = Remote({"_deviceId_": "prenatal"})
        yield from a.startInstance()
        proxy = yield from task
        self.assertEqual(proxy.deviceId, "prenatal")

    @async_tst
    def test_proxy_dead(self):
        a = Remote({"_deviceId_": "moriturus"})
        yield from a.startInstance()
        proxy = yield from getDevice("moriturus")
        yield from a.slotKillDevice()
        with self.assertRaisesRegex(KaraboError, "died"):
            yield from proxy.count()

    @async_tst
    def test_device_schema(self):
        schema, device = yield from self.local.call(
            "remote", "slotGetSchema", False)
        self.assertEqual(device, "remote")
        self.assertEqual(schema.name, "Remote")
        h = schema.hash
        self.assertEqual(h["value", ...], {
            'requiredAccessLevel': 0,
            'metricPrefixSymbol': '',
            'accessMode': 4,
            'description': 'The Value',
            'unitSymbol': 'N_A',
            'assignment': 0,
            'nodeType': 0,
            'defaultValue': 7,
            'valueType': 'INT32',
            'tags': ['whatever'],
            'alarmInfo_alarmHigh': '',
            'alarmInfo_alarmLow': '',
            'alarmInfo_warnHigh': '',
            'alarmInfo_warnLow': '',
            'alarmNeedsAck_alarmHigh': False,
            'alarmNeedsAck_alarmLow': False,
            'alarmNeedsAck_warnHigh': False,
            'alarmNeedsAck_warnLow': False,
            })
        self.assertEqual(h["nested", ...], {
            'requiredAccessLevel': 0,
            'assignment': 0,
            'nodeType': 1,
            'accessMode': 4})
        self.assertEqual(h["doit", ...], {
            'requiredAccessLevel': 0,
            'assignment': 0,
            'displayType': 'Slot',
            'nodeType': 1,
            'accessMode': 4})
        self.assertIn("allow", h)
        self.assertIn("disallowed_int", h)

        schema, device = yield from self.local.call(
            "remote", "slotGetSchema", True)
        self.assertEqual(device, "remote")
        self.assertEqual(schema.name, "Remote")
        h = schema.hash
        self.assertIn("allow", h)
        self.assertNotIn("disallowed_int", h)

    @async_tst
    def test_lock(self):
        with (yield from getDevice("remote")) as d:
            with (yield from lock(d)):
                self.assertEqual(d.lockedBy, "local")
                with (yield from lock(d)):
                    self.assertEqual(d.lockedBy, "local")
                    d.value = 33
                    yield from waitUntil(lambda: d.value == 33)
                yield from d
                self.assertEqual(d.lockedBy, "local")
            yield from waitUntil(lambda: d.lockedBy == "")

    @async_tst
    def test_alarm(self):
        self.remote.signalAlarmUpdate.connect("local", "slotAlarmUpdate")
        self.remote.alarm = 3
        self.remote.update()
        self.local.alarm_future = Future()
        yield from self.local.alarm_future
        ah = self.local.alarmhash
        self.assertFalse(ah["toClear"])
        self.assertEqual(ah["toAdd.alarm.warnLow.type"], "warnLow")
        self.assertEqual(ah["toAdd.alarm.warnLow.description"],
                         "When they go low, we go high")
        self.assertTrue(ah["toAdd.alarm.warnLow.needsAcknowledging"])
        self.remote.alarm = 11
        self.remote.update()
        self.local.alarm_future = Future()
        yield from self.local.alarm_future
        ah = self.local.alarmhash
        self.assertFalse(ah["toAdd"])
        self.assertEqual(ah["toClear.alarm"], ["warnLow"])
        self.remote.signalAlarmUpdate.disconnect("local", "slotAlarmUpdate")

    @async_tst
    def test_double_getDevice(self):
        a, b = yield from gather(getDevice("remote"), getDevice("remote"))
        self.assertIs(a, b)

    @async_tst
    def test_inject(self):
        with (yield from getDevice("remote")) as d:
            self.remote.__class__.injected = String()
            self.remote.injected = "spicer"
            with self.assertRaises(TimeoutError):
                yield from wait_for(waitUntil(lambda: hasattr(d, "injected")),
                                    timeout=0.1)
            self.remote.publishInjectedParameters()
            yield from waitUntil(lambda: hasattr(d, "injected"))
            yield from waitUntil(lambda: d.injected == "spicer")
            self.remote.injected = "bla"
            yield from waitUntil(lambda: d.injected == "bla")
            d.injected = "donald"
            yield from waitUntil(lambda: self.remote.injected == "donald")

if __name__ == "__main__":
    main()
