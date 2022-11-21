import time
import traceback
import weakref
from asyncio import (
    Future, TimeoutError, coroutine, ensure_future, gather, sleep, wait_for)
from contextlib import contextmanager
from datetime import datetime
from unittest import main

from dateutil.parser import parse as from_isoformat
from pint import DimensionalityError

from karabo.middlelayer import (
    AlarmCondition, Bool, Configurable, Device, DeviceNode, Float, Hash, Int32,
    KaraboError, MetricPrefix, Node, Overwrite, Queue, Slot, State, String,
    Timestamp, Unit, VectorFloat, VectorInt16, VectorString, background, call,
    connectDevice, coslot, disconnectDevice, execute, filterByTags, getDevice,
    getInstanceInfo, getTimeInfo, isAlive, isSet, lock, setNoWait, setWait,
    slot, unit, updateDevice, waitUntil, waitUntilNew)
from karabo.middlelayer_api.broker import amqp, jms, mqtt, redis
from karabo.middlelayer_api.logger import CacheLog
from karabo.middlelayer_api.tests.eventloop import DeviceTest, async_tst

FIXED_TIMESTAMP = Timestamp("2009-04-20T10:32:22 UTC")


class Superslot(Slot):
    async def method(self, device):
        device.value = 22


class SuperInteger(Int32):
    async def setter(self, device, value):
        if isSet(value):
            device.value = 2 * value


class NestNest(Configurable):
    value = Int32(unitSymbol=Unit.METER)


class Nested(Configurable):
    val = Int32(unitSymbol=Unit.SECOND, metricPrefixSymbol=MetricPrefix.MILLI)
    nestnest = Node(NestNest)

    @Slot()
    async def nestedSlot(self):
        self.val *= 2


class ChannelNode(Configurable):
    data = Int32(defaultValue=0)


class CrazyRemote(Device):

    counter = 0
    destructed = False

    async def injectDirect(self):
        await self._inject_crazy()

    @Slot()
    async def injectSlot(self):
        background(self._inject_crazy)

    async def _inject_crazy(self):
        for i in range(10):
            setattr(self.__class__, str(i), Int32(defaultValue=0))
            await self.publishInjectedParameters()

    def _notifyNewSchema(self):
        self.counter += 1
        super()._notifyNewSchema()

    async def onDestruction(self):
        self.destructed = True
        # This should be cancelled by timelimit
        await sleep(1e6)


class Remote(Device):

    state = Overwrite(options=[State.ON])

    done = Bool()
    value = Int32(description="The Value", defaultValue=7, tags={"whatever"})
    counter = Int32(defaultValue=-1)

    unit_int = Int32(unitSymbol=Unit.METER)
    unit_float = Float(unitSymbol=Unit.SECOND,
                       metricPrefixSymbol=MetricPrefix.MILLI)
    string = String(tags=["AString"])

    unit_vector_int = VectorInt16(unitSymbol=Unit.METER_PER_SECOND)
    unit_vector_float = VectorFloat(unitSymbol=Unit.DEGREE)
    string_list = VectorString()

    useTimestamp = Bool(
        defaultValue=False)
    nested = Node(Nested)

    alarm = Float(
        warnLow=10,
        alarmInfo_warnLow="When they go low, we go high",
        alarmNeedsAck_warnLow=True,
        alarmHigh=20)

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
    async def allow(self):
        self.state = State.ON
        self.value = 777

    @Slot()
    async def doit(self):
        self.done = True

    @Slot(displayedName="Change")
    async def changeit(self):
        self.value -= 4

    def __init__(self, configuration):
        super().__init__(configuration)
        self.done = False
        self.once_value = None

    @Slot()
    async def count(self):
        for i in range(30):
            self.counter = i
            await sleep(0.03)

    generic = Superslot()
    generic_int = SuperInteger()
    logmessage = None

    @Slot()
    async def read_log(self):
        self.logmessage = CacheLog.summary(1)[0]
    got_config = 0

    @slot
    def slotGetConfiguration(self):
        self.got_config += 1
        return super().slotGetConfiguration()


class Local(Device):
    alarmhash = None

    @Slot()
    async def error(self):
        raise RuntimeError("On purpose")

    @Slot()
    async def error_in_error(self):
        raise RuntimeError("On purpose")

    async def error_log_message(self):
        raise RuntimeError("On purpose")

    @Slot()
    async def task_background_error_coro(self):
        background(self.error_log_message())

    @Slot()
    async def task_background_error_no_coro(self):
        background(self.error_log_message())

    @Slot()
    async def task_error(self):
        ensure_future(self.error())

    async def onException(self, slot, exc, tb):
        self.exc_slot = slot
        self.exception = exc
        self.traceback = tb
        self.traceback_str = "".join(traceback.format_tb(tb))
        if slot is Local.error_in_error:
            raise RuntimeError
        else:
            with (await getDevice("remote")) as d:
                await d.doit()

    @slot
    def slotAlarmUpdate(self, deviceId, alarms):
        self.alarm_future.set_result(None)
        self.alarmhash = alarms

    async def onInitialization(self):
        self.state = State.ON

    @coslot
    async def useSetWait(self, stringValue):
        """
        'System' slot with argument (coslot) that uses
        'setWait' as example from device_client interface
        """
        await setWait("remote", "string", stringValue)


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
    async def test_execute(self):
        """test the execution of remote slots"""
        self.remote.done = False
        with (await getDevice("remote")) as d:
            await d.doit()
        self.assertTrue(self.remote.done)

    @async_tst
    async def test_execute_noproxy(self):
        self.remote.done = False
        await execute("remote", "doit")
        self.assertTrue(self.remote.done)

    @async_tst
    async def test_set_remote(self):
        """test setting of values"""
        with (await getDevice("remote")) as d:
            d.string = "bla"
            d.unit_int = 3 * unit.meter
            d.unit_float = 7.5 * unit.millisecond
            d.unit_vector_int = [3, 4, 5] * unit.meter / unit.second
            d.unit_vector_float = [1, 2, 3]
            d.string_list = ["a", "bla"]
            await waitUntilNew(d.string_list)

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

        with (await getDevice("remote")) as d:
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
            await waitUntilNew(d.string_list)

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
    async def test_set_local(self):
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

        with (await getDevice("remote")) as d:
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

        with (await getDevice("remote")) as d:
            self.remote.unit_int = 5 * unit.kilometer
            self.remote.unit_float = 2 * unit.millisecond
            self.remote.unit_vector_int = [2, 3, 4] * unit.meter / unit.second
            self.remote.unit_vector_float = [5, 7, 8] * unit.degree
            self.remote.string_list = []
            await updateDevice(d)

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
    async def test_change(self):
        """test changing a remote parameter"""
        self.remote.value = 7
        with (await getDevice("remote")) as d:
            await d.changeit()
        self.assertEqual(self.remote.value, 3)

    @async_tst
    async def test_state(self):
        self.remote.state = State.UNKNOWN
        with (await getDevice("remote")) as d:
            self.assertEqual(d.state, State.UNKNOWN)
            self.assertEqual(d.alarmCondition, AlarmCondition.NONE)

    @async_tst
    async def test_remotetag_proxy(self):
        with (await getDevice("remote")) as d:
            descriptors = filterByTags(d, "AString")
            paths = [k.longkey for k in descriptors]
            self.assertEqual(["string"], paths)

    @async_tst
    async def test_disconnect(self):
        """test values are not updating when disconnected"""
        self.remote.counter = -1
        # there are often still changes on the way that we don't want to see
        await sleep(0.1)
        d = await getDevice("remote")
        task = ensure_future(d.count())
        self.assertEqual(d.counter, -1,
                         "we're not connected still seeing changes")
        await sleep(0.1)
        with d:
            await updateDevice(d)
            await sleep(0.1)
            tmp = d.counter
            self.assertNotEqual(d.counter, -1,
                                "not seeing changes although connected")
        await sleep(0.01)
        # sanity check: the counter should still be running
        self.assertLess(tmp, 20)
        self.assertLess(d.counter - tmp, 2,
                        "too many messages arrived after disconnecting")

        # Disconnecting although disconnected should not harm
        for _ in range(5):
            await disconnectDevice(d)

        with d:
            await sleep(0.8)
            if amqp:
                await sleep(0.5)
            self.assertEqual(d.counter, 29)
        await task

    @async_tst
    async def test_set(self):
        """test setting of remote values works"""
        self.remote.value = 7
        with (await getDevice("remote")) as d:
            self.assertEqual(d.value, 7)
            d.value = 10
            await sleep(0.1)
            self.assertEqual(d.value, 10)
            await d.changeit()
            self.assertEqual(d.value, 6)

    @async_tst
    async def test_generic(self):
        """test calling a generic slot"""
        self.remote.value = 7
        d = await getDevice("remote")
        await d.generic()
        self.assertEqual(self.remote.value, 22)

    @async_tst
    async def test_generic_int(self):
        """test setting a generic property"""
        self.remote.value = 7
        d = await getDevice("remote")
        d.generic_int = 33
        await updateDevice(d)
        self.assertEqual(self.remote.value, 66)

    @async_tst
    async def test_setter(self):
        """test setting a property with a setter method"""
        with (await getDevice("remote")) as d:
            d.other = 102
        await sleep(0.5)
        self.assertEqual(self.remote.value, 102)

    @async_tst
    async def test_setwait(self):
        """test the setWait coroutine"""
        d = await getDevice("remote")
        await setWait(d, value=200 * unit.mm, counter=300)
        self.assertEqual(self.remote.value, 200)
        self.assertEqual(self.remote.counter, 300)

    @async_tst
    async def test_setnowait(self):
        """test the setNoWait coroutine"""
        self.remote.value = 0
        self.remote.counter = 0
        d = await getDevice("remote")
        setNoWait(d, value=200, counter=300 * unit.mm)
        self.assertEqual(self.remote.value, 0)
        self.assertEqual(self.remote.counter, 0)
        if mqtt:
            await sleep(0.2)
        else:
            await sleep(0.1)
        self.assertEqual(self.remote.value, 200)
        self.assertEqual(self.remote.counter, 300)

    @async_tst
    async def test_setwait_coslot(self):
        """Test that setwait can be called in @coslot"""
        self.remote.string = "none"
        await call("local", "useSetWait", "string value 42")
        self.assertEqual(self.remote.string, "string value 42")

    @async_tst
    async def test_waituntil(self):
        """test the waitUntil coroutine"""
        with (await getDevice("remote")) as d:
            d.counter = 0
            await waitUntil(lambda: d.counter == 0)
            self.assertEqual(d.counter, 0)
            task = ensure_future(d.count())
            try:
                with self.assertRaises(TimeoutError):
                    await wait_for(waitUntil(lambda: d.counter > 10),
                                   timeout=0.1)

                await waitUntil(lambda: d.counter > 10)
                self.assertGreaterEqual(d.counter, 11)

                # there are many "while True:" busy loops out there.
                # assure we don't get stuck if the condition is always true
                # it is important to have no other await except the
                # waitUntil in the loop to test that it actually gives the
                # event loop a chance to run. This test, if succeeding, is
                # fast, but if failing would kill the entire test suite,
                # that's why there is an upper time limit for it.
                t0 = time.time()
                while d.counter < 15 and time.time() - t0 < 1:
                    await waitUntil(lambda: True)
                self.assertLess(time.time() - t0, 1)

                with self.assertRaises(TimeoutError):
                    await wait_for(waitUntil(lambda: d.counter > 40),
                                   timeout=0.01)
            finally:
                await task

    @async_tst
    async def test_isAlive(self):
        moriturus = Local({"_deviceId_": "moriturus"})
        await moriturus.startInstance()
        proxy = await getDevice("moriturus")
        self.assertTrue(isAlive(proxy))
        background(moriturus.slotKillDevice())
        await waitUntil(lambda: not isAlive(proxy))
        self.assertFalse(isAlive(proxy))

    @async_tst
    async def test_isAlive_state(self):
        moriturus = Local({"_deviceId_": "moriturus"})
        await moriturus.startInstance()
        proxy = await getDevice("moriturus")
        await waitUntil(lambda: isAlive(proxy))
        self.assertTrue(isAlive(proxy))
        self.assertEqual(moriturus.state, State.ON)
        self.assertEqual(proxy.state, State.ON)
        # We kill the device to see the desired state transition
        await moriturus.slotKillDevice()
        self.assertFalse(isAlive(proxy))
        self.assertEqual(proxy.state, State.UNKNOWN)

    @async_tst
    # @flaky(max_runs=FLAKY_MAX_RUNS, min_passes=FLAKY_MIN_PASSES)
    async def test_waituntilnew(self):
        """test the waitUntilNew coroutine for properties
        """
        self.remote.counter = None
        self.remote.nested.val = None
        # NOTE: Protect against a race condition in getDevice and cycle once!
        # Can be cured with async with which is not possible at the moment.
        await sleep(2)
        with (await getDevice("remote")) as d:
            d.counter = 0
            # we test that d.counter and d.nested.val are still None (it
            # must be, no await since last line). This asserts that
            # waitUntilNew also works with uninitialized values, which
            # had been a bug before
            self.assertEqual(d.counter, None)
            self.assertEqual(d.nested.val, None)
            if not mqtt:
                await waitUntilNew(d.value, d.counter, d.nested.val)
            task = ensure_future(d.count())
            try:
                i = 0
                while i < 30:
                    await waitUntilNew(d.counter, d.value)
                    self.assertLessEqual(i, d.counter)
                    i = d.counter + 1
                self.assertEqual(29, d.counter)
            finally:
                await task

    @async_tst
    # @flaky(max_runs=FLAKY_MAX_RUNS, min_passes=FLAKY_MIN_PASSES)
    async def test_waituntildevice(self):
        """test the waitUntilNew coroutine for devices
        """
        self.remote.counter = -1
        await sleep(2)
        with (await getDevice("remote")) as d:
            if mqtt:
                await updateDevice(d)
            d.counter = 0
            await sleep(0.1)
            task = ensure_future(d.count())
            try:
                i = 0
                while i < 30:
                    await waitUntilNew(d)
                    self.assertLessEqual(i, d.counter)
                    i = d.counter + 1
                self.assertEqual(29, d.counter)
            finally:
                await task

    @async_tst
    async def test_collect(self):
        """test that multiple settings are gathered into one"""
        with (await getDevice("remote")) as d:
            if mqtt:
                await updateDevice(d)
            d.once = 3
            d.once = 7
            d.once = 10
        if redis:
            await sleep(0.3)
        else:
            await sleep(0.1)
        self.assertEqual(self.remote.once_value, 10)

    @async_tst
    async def test_error_value(self):
        d = await getDevice("remote")
        d.error_value = 3
        with self.assertRaises(KaraboError):
            await d.doit()

    @async_tst
    async def test_disallow(self):
        """test that values cannot be set if in wrong state"""
        self.assertEqual(self.remote.state, State.UNKNOWN)
        with (await getDevice("remote")) as d:
            d.value = 7
            await waitUntil(lambda: d.value == 7)
            # disallowed_int is normally not allowed
            with self.assertLogs(logger="remote", level="WARNING") as log:
                # no error here as we need to wait for network round-trip
                d.disallowed_int = 333
                await sleep(0.5)
            with self.assertRaises(KaraboError):
                # this raises the error from above
                d.value = 8
            with self.assertLogs(logger="remote", level="WARNING") as log, \
                    self.assertRaises(KaraboError):
                d.disallowed_int = 333
                await d.allow()
            self.assertEqual(len(log.records), 1)
            self.assertEqual(log.records[0].levelname, "WARNING")
            self.assertEqual(d.value, 7)

            # d.allow() sets d.value to 777 and is changing state such that
            # disallowed_int can be set ...
            await d.allow()
            self.assertEqual(d.value, 777)
            d.value = 4
            # ... but it cannot be called itself anymore ...
            with self.assertLogs(logger="remote", level="WARN") as logs, \
                    self.assertRaises(KaraboError):
                await d.allow()
            self.assertEqual(len(logs.records), 1)
            self.assertEqual(logs.records[0].levelname, "WARNING")
            self.assertEqual(d.value, 4)
            # ... but disallowed_int sets this back ...
            d.disallowed_int = 444
            await waitUntil(lambda: d.value == 444)
            self.assertEqual(d.value, 444)
            # ... so allow can be called again!
            await d.allow()
            self.assertEqual(d.value, 777)

    @async_tst
    async def test_log(self):
        """test the logging of warnings and exceptions"""

        def _absolute_delta_to_now(isotimestr):
            """Return the delta in seconds between some ISO 8601 timestamp
            and now.
            """
            ts = from_isoformat(isotimestr)
            delta = abs(ts - datetime.now())
            # NOTE: `delta` is a timedelta object, which contains only
            # days, seconds, and microseconds.
            return (delta.days * 3600 * 24 + delta.seconds +
                    delta.microseconds * 1e-6)

        self.remote.logmessage = None
        with (await getDevice("remote")) as d:
            self.local.logger.warning("this is an info")
            await d.read_log()

        hash = self.remote.logmessage
        self.assertEqual(hash["message"], "this is an info")
        self.assertEqual(hash["type"], "WARN")
        self.assertEqual(hash["category"], "local")
        self.assertLessEqual(_absolute_delta_to_now(hash["timestamp"]), 10)

        self.remote.logmessage = None
        with (await getDevice("remote")) as d:
            try:
                raise RuntimeError("test_log exception")
            except Exception:
                self.local.logger.exception("expected exception")
            await d.read_log()

        hash = self.remote.logmessage
        self.assertEqual(hash["message"], "expected exception")
        self.assertEqual(hash["type"], "ERROR")
        self.assertEqual(hash["category"], "local")
        self.assertLessEqual(_absolute_delta_to_now(hash["timestamp"]), 10)
        # Traceback is a joined string of traceback info parts
        self.assertEqual(type(hash["traceback"]), str)

    @async_tst
    async def test_earlylog(self):
        class A(Device):
            @Int32()
            def logint(self, value):
                self.logger.info("log the int")

            async def onInitialization(self):
                self.logger.info("some test log message")

        with self.assertLogs("testearlylog", level="INFO") as cm:
            a = A({"_deviceId_": "testearlylog"})
        self.assertEqual(cm.records[0].msg, "log the int")
        with self.assertLogs("testearlylog", level="INFO") as cm:
            await a.startInstance()
        self.assertEqual(cm.records[0].msg, "some test log message")
        await a.slotKillDevice()

    @async_tst
    async def test_queue(self):
        """test queues of properties"""
        with (await getDevice("remote")) as d:
            task = ensure_future(d.count())
            await waitUntil(lambda: d.counter == 0)
            try:
                q = Queue(d.counter)
                for i in range(1, 30):
                    j = await q.get()
                    self.assertEqual(i, j)
                    await sleep(i * 0.003)
            finally:
                await task

    @async_tst
    async def test_nested(self):
        """test accessing nested properties"""
        self.remote.nested.val = 3 * unit.second
        self.assertLess(
            abs(self.remote.nested.val.timestamp.toTimestamp() - time.time()),
            1)
        ts1 = self.remote.nested.val.timestamp
        self.remote.nested.nestnest.value = 7 * unit.meter
        ts2 = self.remote.nested.nestnest.value.timestamp
        with (await getDevice("remote")) as d:
            self.assertEqual(d.nested.val, 3 * unit.second)
            self.assertEqual(d.nested.val.timestamp, ts1)
            self.assertEqual(d.nested.nestnest.value, 7 * unit.meter)
            d.nested.val = 4 * unit.second
            self.assertEqual(d.nested.nestnest.value, 7 * unit.meter)
            self.assertEqual(d.nested.nestnest.value.timestamp, ts2)
            d.nested.val = 4 * unit.second
            d.nested.nestnest.value = 5 * unit.meter
            await d.nested.nestedSlot()
            # nestedSlot doubles the nested.val
        self.assertEqual(self.remote.nested.val, 8 * unit.second)
        self.assertEqual(self.remote.nested.nestnest.value, 5 * unit.meter)

    @async_tst
    async def test_error(self):
        """test error reporting and calling of error methods"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR"):
            with (await getDevice("local")) as d:
                with self.assertRaises(KaraboError):
                    await d.error()
        self.assertTrue(self.remote.done)
        self.remote.done = False
        self.assertIs(self.local.exc_slot, Local.error)
        self.assertIsInstance(self.local.exception, RuntimeError)
        self.local.traceback.tb_lasti  # check whether that is a traceback
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback
        del self.local.traceback_str

    @async_tst
    async def test_error_in_error(self):
        """test what happens if an error happens in an error method"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR") as logs:
            with (await getDevice("local")) as d:
                with self.assertRaises(KaraboError):
                    await d.error_in_error()
        self.assertEqual(logs.records[-1].msg, "error in error handler")
        self.assertFalse(self.remote.done)
        self.assertIs(self.local.exc_slot, Local.error_in_error)
        self.assertIn("On purpose", self.local.traceback_str)
        self.assertIsInstance(self.local.exception, RuntimeError)
        self.local.traceback.tb_lasti  # check whether that is a traceback
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback
        del self.local.traceback_str

    @async_tst
    async def test_task_error(self):
        """test that errors of created tasks are properly reported"""
        self.remote.done = False
        with self.assertLogs(logger="local", level="ERROR"):
            with (await getDevice("local")) as local, \
                    (await getDevice("remote")) as remote:
                await local.task_error()
                self.assertFalse(remote.done)
                await waitUntil(lambda: remote.done)
                self.assertTrue(remote.done)

        self.assertIsNone(self.local.exc_slot)
        self.assertIsInstance(self.local.exception, RuntimeError)
        self.assertIn("On purpose", self.local.traceback_str)
        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback
        del self.local.traceback_str

    @async_tst
    async def test_task_error_background_coro(self):
        """test that errors of background tasks are properly reported"""
        self.remote.logmessage = None
        with (await getDevice("local")) as local, \
                (await getDevice("remote")) as remote:
            await sleep(0.1)
            await local.task_background_error_coro()
            await remote.read_log()

            # Read out the log message
            hash = self.remote.logmessage
            message = hash["message"]
            self.assertIn("Error", message)
            # self.assertIn("Error in background task ...", message)
            self.assertEqual(hash["type"], "ERROR")
            self.assertEqual(hash["category"], "local")
            self.assertIn("On purpose", hash["traceback"])

        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback
        del self.local.traceback_str

    @async_tst
    async def test_task_error_background_no_coro(self):
        """test that errors of background tasks are properly reported"""
        self.remote.logmessage = None
        with (await getDevice("local")) as local, \
                (await getDevice("remote")) as remote:
            t = ensure_future(remote.read_log())
            await sleep(0.1)
            await local.task_background_error_no_coro()
            await t

            # Read out the log message
            hash = self.remote.logmessage
            message = hash["message"]
            self.assertIn("Error", message)
            # self.assertIn("Error in background task ...", message)
            self.assertEqual(hash["type"], "ERROR")
            self.assertEqual(hash["category"], "local")
            self.assertIn("On purpose", hash["traceback"])

        del self.local.exc_slot
        del self.local.exception
        del self.local.traceback
        del self.local.traceback_str

    @async_tst
    async def test_connectDevice(self):
        d = await connectDevice("remote")
        try:
            self.assertNotEqual(d.value, 123)
            self.remote.value = 123
            await sleep(1.0)
            self.assertEqual(d.value, 123)
        finally:
            # check the proxy gets collected when not used anymore
            weak = weakref.ref(d)
            del d
            self.assertIsNone(weak())

    @async_tst
    async def test_getdoc(self):
        # this is usually done by iPython/iKarabo
        d = await getDevice("remote")
        self.assertEqual(d.value.getdoc(), "The Value")

    @async_tst
    async def test_devicenode(self):
        class A(Device):
            dn = DeviceNode()

            async def onInitialization(self):
                self.state = State.NORMAL

        class B(Device):
            pass

        a = A({"_deviceId_": "devicenode", "dn": "badwienix"})
        b = B({"_deviceId_": "badwienix"})

        a.startInstance()
        try:
            with (await getDevice("devicenode")) as d:
                # A is started without awaiting, we have to wait for the
                # device to come online, however it will not pass
                # initialization, the device badwienix is not online.
                await waitUntil(lambda: d.dn.value == "badwienix")
                self.assertEqual(d.state, State.UNKNOWN)
                self.assertEqual(d.lockedBy, "")
                self.assertEqual(d.dn.value, "badwienix")
                ts_before = d.dn.timestamp
                self.assertIsNotNone(ts_before)
                self.assertEqual(type(d).dn.displayType, "deviceNode")
                await b.startInstance()
                # wait until it is online, the device will pass
                # the initialization phase
                await waitUntil(lambda: d.state == State.NORMAL)
                self.assertEqual(d.dn.value, "badwienix")
                ts_after = d.dn.timestamp
                self.assertIsNotNone(ts_after)
                self.assertEqual(ts_after, ts_before)
                self.assertEqual(d.state, State.NORMAL)
                # Timestamp is not changed under hood, verify
                await updateDevice(d)
                ts_after = d.dn.timestamp
                self.assertEqual(ts_after, ts_before)
        finally:
            await a.slotKillDevice()
            await b.slotKillDevice()

    @async_tst
    async def test_devicenode_novalue(self):
        class A(Device):
            dn = DeviceNode()

        with self.assertRaises(KaraboError):
            A({"_deviceId_": "devicenode"})

    @async_tst
    async def test_prenatal_proxy(self):
        task = ensure_future(getDevice("prenatal"))
        a = Remote({"_deviceId_": "prenatal"})
        await a.startInstance()
        proxy = await task
        self.assertEqual(proxy.deviceId, "prenatal")

    @async_tst
    async def test_proxy_dead(self):
        a = Remote({"_deviceId_": "moriturus"})
        await a.startInstance()
        proxy = await getDevice("moriturus")
        await a.slotKillDevice()
        with self.assertRaisesRegex(KaraboError, "died"):
            await proxy.count()

    @async_tst
    async def test_device_schema(self):
        schema, device = await self.local.call(
            "remote", "slotGetSchema", False)
        self.assertEqual(device, "remote")
        self.assertEqual(schema.name, "Remote")
        h = schema.hash
        self.assertEqual(h["value", ...], {
            'requiredAccessLevel': 1,
            'daqPolicy': -1,
            'metricPrefixSymbol': '',
            'accessMode': 4,
            'description': 'The Value',
            'unitSymbol': '',
            'assignment': 0,
            'nodeType': 0,
            'defaultValue': 7,
            'valueType': 'INT32',
            'tags': ['whatever'],
        })
        self.assertEqual(h["nested", ...], {
            'requiredAccessLevel': 0,
            'assignment': 0,
            'nodeType': 1,
            'accessMode': 4})
        self.assertEqual(h["doit", ...], {
            'requiredAccessLevel': 1,
            'assignment': 0,
            'displayType': 'Slot',
            'classId': 'Slot',
            'nodeType': 1,
            'accessMode': 4})
        self.assertIn("allow", h)
        self.assertIn("disallowed_int", h)

        schema, device = await self.local.call(
            "remote", "slotGetSchema", True)
        self.assertEqual(device, "remote")
        self.assertEqual(schema.name, "Remote")
        h = schema.hash
        self.assertIn("allow", h)
        self.assertNotIn("disallowed_int", h)

    @async_tst
    async def test_lock(self):
        with (await getDevice("remote")) as d:
            with (await lock(d)):
                self.assertEqual(d.lockedBy, "local")
                with (await lock(d)):
                    self.assertEqual(d.lockedBy, "local")
                    d.value = 33
                    await waitUntil(lambda: d.value == 33)
                await updateDevice(d)
                self.assertEqual(d.lockedBy, "local")
            await waitUntil(lambda: d.lockedBy == "")

    @async_tst
    async def test_lock_concurrence(self):
        with (await getDevice("remote")) as d:
            await setWait(d, value=40, lockedBy="NoDevice")
            self.assertEqual(d.lockedBy, "NoDevice")
            self.assertEqual(d.value, 40)
            with self.assertRaises(KaraboError) as e:
                await setWait(d, value=50)
                self.assertEqual(e, "Device locked by NoDevice")
            self.assertEqual(d.value, 40)
            await d.slotClearLock()
            await setWait(d, value=50)
            self.assertEqual(d.value, 50)
            self.assertEqual(d.lockedBy, "")

    @async_tst
    async def test_device_node_alive(self):
        await sleep(0.1)

        class Sub(Configurable):
            dn = DeviceNode()

        class A(Device):
            dn = DeviceNode()
            sub = Node(Sub)

            async def onInitialization(self):
                self.state = State.ON

        node_device = A({"_deviceId_": "devicenode",
                        "dn": "scratchy", "sub": {"dn": "itchy"}})
        scratchy = Local({"_deviceId_": "scratchy"})
        itchy = Local({"_deviceId_": "itchy"})
        try:
            await itchy.startInstance()
            await scratchy.startInstance()
            await node_device.startInstance()
            while True:
                if node_device.state == State.ON:
                    break
                await sleep(0.1)
            self.assertTrue(isAlive(node_device.dn))
            self.assertTrue(isAlive(node_device.sub.dn))
            background(scratchy.slotKillDevice())
            background(itchy.slotKillDevice())
            await waitUntil(lambda: not isAlive(node_device.dn))
            self.assertFalse(isAlive(node_device.dn))
            await waitUntil(lambda: not isAlive(node_device.sub.dn))
            self.assertFalse(isAlive(node_device.sub.dn))
        finally:
            await node_device.slotKillDevice()

    @async_tst
    async def test_alarm(self):
        await self.remote.signalAlarmUpdate.connect("local", "slotAlarmUpdate")
        self.remote.alarm = 3
        self.remote.update()
        self.local.alarm_future = Future()
        await self.local.alarm_future
        ah = self.local.alarmhash
        self.assertFalse(ah["toClear"])
        self.assertEqual(ah["toAdd.alarm.warnLow.type"], "warnLow")
        self.assertEqual(ah["toAdd.alarm.warnLow.description"],
                         "When they go low, we go high")
        self.assertTrue(ah["toAdd.alarm.warnLow.needsAcknowledging"])
        self.remote.alarm = 11
        self.remote.update()
        self.local.alarm_future = Future()
        await self.local.alarm_future
        ah = self.local.alarmhash
        self.assertFalse(ah["toAdd"])
        self.assertEqual(ah["toClear.alarm"], ["warnLow"])

        self.local.alarm_future = Future()
        self.remote.globalAlarmCondition = AlarmCondition.ALARM
        self.remote.update()
        await self.local.alarm_future
        ah = self.local.alarmhash
        self.assertFalse(ah["toClear"])
        self.assertEqual(ah["toAdd.global.alarm.type"], "alarm")
        self.assertEqual(
            ah["toAdd.global.alarm.description"], "")
        self.assertTrue(
            ah["toAdd.global.alarm.needsAcknowledging"])
        await self.remote.signalAlarmUpdate.disconnect("local",
                                                       "slotAlarmUpdate")
        self.remote.globalAlarmCondition = AlarmCondition.NONE

    @async_tst
    async def test_double_getDevice(self):
        a, b = await gather(getDevice("remote"), getDevice("remote"))
        self.assertIs(a, b)

        alive = ensure_future(getDevice("vivro"))
        cancelled = ensure_future(getDevice("vivro"))
        yield  # start the tasks
        cancelled.cancel()
        remote = Remote({"_deviceId_": "vivro"})
        await remote.startInstance()
        try:
            # this used to raise a CancelledError, as we cross-cancelled
            await alive
        finally:
            await remote.slotKillDevice()

    @async_tst
    async def test_inject(self):
        await sleep(1)
        with (await getDevice("remote")) as d:
            self.remote.__class__.injected = String()
            self.remote.__class__.injected_node = Node(Nested)
            self.remote.__class__.changeit = Overwrite(
                displayedName="ChangeIt")

            @Int32()
            async def tobeinit(myself, value):
                self.assertEqual(value, 100)
                myself.tobeinit = 123

            self.remote.__class__.tobeinit = tobeinit

            @Slot(displayedName="Injected")
            def slot(self):
                nonlocal slotdata
                slotdata = 44

            slotdata = None
            self.remote.__class__.injected_slot = slot

            with self.assertRaises(TimeoutError):
                await wait_for(waitUntil(lambda: hasattr(d, "injected")),
                               timeout=0.1)
            await self.remote.publishInjectedParameters(tobeinit=100)
            self.assertEqual(self.remote.changeit.descriptor.displayedName,
                             "ChangeIt")
            self.remote.injected = "smthng"
            self.remote.injected_node = Nested({})
            await waitUntil(lambda: hasattr(d, "injected"))
            await waitUntil(lambda: d.injected == "smthng")
            self.assertEqual(d.tobeinit, 123, "initializer was not called")
            self.remote.injected = "bla"
            await waitUntil(lambda: d.injected == "bla")
            await d.injected_slot()
            self.assertEqual(
                self.remote.injected_slot.descriptor.displayedName, "Injected")
            self.assertEqual(slotdata, 44)

            d.injected_node.val = 10
            await d.injected_node.nestedSlot()
            self.assertEqual(d.injected_node.val, 20 * unit.millisecond)

            d.injected = "whatever"
            await waitUntil(lambda: self.remote.injected == "whatever")

    @async_tst
    async def test_earlyinject(self):
        class A(Device):
            async def onInitialization(self):
                self.__class__.number = Int32()
                await self.publishInjectedParameters()
                self.number = 3

        a = A({"_deviceId_": "testinject"})
        await a.startInstance()
        with (await getDevice("testinject")) as proxy:
            self.assertEqual(proxy.number, 3)

    @async_tst
    async def test_device_restart(self):
        await sleep(0.1)
        proxy = None
        self.remote.done = False
        future = Future()

        class A(Device):
            async def onInitialization(self):
                nonlocal future, proxy
                proxy = await connectDevice("remote")
                future.set_result(None)

        a = A({"_deviceId_": "testrestart"})
        await a.startInstance()
        await future
        firstproxy = proxy
        await a.slotKillDevice()

        future = Future()
        a = A({"_deviceId_": "testrestart"})
        await a.startInstance()
        await future
        weakproxy = weakref.ref(firstproxy)
        del firstproxy
        self.assertIsNone(weakproxy())
        await proxy.doit()
        self.assertTrue(proxy.done)

    @async_tst
    async def test_archive_fails(self):
        """Test that devices don't have archive in their schema"""
        with (await getDevice("remote")) as d:
            with self.assertRaises(AttributeError):
                _ = d.archive.value
        info = await getInstanceInfo("remote")
        self.assertEqual(info["archive"], True)

    @async_tst
    async def test_timeinfo_device(self):
        info = await getTimeInfo("remote")
        self.assertIsNotNone(info["time"])
        self.assertGreater(info["latency"], 0)
        self.assertIsNotNone(info["reference"])

    @async_tst
    async def test_config_handler(self):
        h = Hash()

        def updates(change):
            h.merge(change)

        with (await getDevice("remote")) as d:
            d.counter = -1
            # subscribe to changes
            d.setConfigHandler(updates)
            try:
                await sleep(0.1)
                task = ensure_future(d.count())
                for _ in range(5):
                    h.clear()
                    await waitUntilNew(d)
                    self.assertIn("counter", h)

                # unsubscribe from changes
                d.setConfigHandler(None)
                h.clear()

                await waitUntilNew(d)
                self.assertNotIn("counter", h)
            finally:
                await task
            d.counter = -1

    @async_tst
    async def test_async_disconnect(self):
        self.remote.counter = -1
        d = await getDevice("remote")
        # A proxy is always connected for a few seconds,
        # trigger manual disconnect to speed up
        await disconnectDevice(d)
        task = background(d.count())
        self.assertEqual(d.counter, -1,
                         "we're not connected still seeing changes")
        async with d:
            await waitUntil(lambda: d.counter == 10)
            self.assertEqual(d.counter, 10)

        await sleep(0.05)

        # We are not connected anymore!
        counter = d.counter
        for i in range(20):
            # Cycle and spin the eventloop like crazy! But we do not
            # receive updates anymore!
            await sleep(0.01)
            self.assertEqual(d.counter, counter)
        await task
        async with d:
            await updateDevice(d)
            self.assertEqual(d.counter, 29)

    @async_tst
    async def test_async_with(self):
        self.remote.counter = -1
        got_config = self.remote.got_config
        async with getDevice("remote") as d:
            task = background(d.count())
            await waitUntil(lambda: d.counter == 10)
            self.assertEqual(d.counter, 10)

        if jms:
            await sleep(0.05)

        self.assertEqual(self.remote.got_config, got_config + 1)
        self.assertTrue(d.counter < 29)
        await task
        async with d:
            await updateDevice(d)
            self.assertEqual(d.counter, 29)

    @async_tst
    async def test_crazy_injection_destruction(self):
        deviceId = "crazy-test-device"
        d = CrazyRemote({"_deviceId_": deviceId})
        await d.startInstance()
        self.assertFalse(d.destructed)
        try:
            await d.injectSlot()
            # Wait very very long, 1 seconds
            await sleep(1)
            self.assertEqual(d.counter, 1)
            d.counter = 0
            await d.injectDirect()
            # Wait very very long, 1 seconds
            await sleep(1)
            self.assertEqual(d.counter, 1)
        finally:
            await d.slotKillDevice()
            self.assertTrue(d.destructed)

    @async_tst
    @coroutine
    def test_async_yield_from_connectDevice(self):
        """Test the old syntax for connectDevice"""
        deviceId = "yield_from_remote-device"
        remote = Remote({"_deviceId_": deviceId})
        try:
            yield from remote.startInstance()
            remote.counter = -1
            d = yield from connectDevice(deviceId)
            self.assertEqual(d.counter, -1)
            task = background(d.count())
            yield from waitUntil(lambda: d.counter == 10)
            self.assertEqual(d.counter, 10)
            yield from task
            self.assertEqual(d.counter, 29)
        finally:
            yield from remote.slotKillDevice()

    @async_tst
    @coroutine
    def test_async_yield_from_getDevice(self):
        """Test the old syntax for getDevice"""
        deviceId = "another_yield_from_remote-device"
        remote = Remote({"_deviceId_": deviceId})
        try:
            yield from remote.startInstance()
            remote.counter = -1
            d = yield from getDevice(deviceId)
            self.assertEqual(d.counter, -1)
            counter = d.counter
            for i in range(20):
                yield from sleep(0.01)
                self.assertEqual(d.counter, counter)
            task = background(d.count())
            self.assertEqual(d.counter, -1)
            yield from task
            with d:
                yield from updateDevice(d)
                self.assertEqual(d.counter, 29)
        finally:
            yield from remote.slotKillDevice()

    @async_tst
    @coroutine
    def test_async_with_yield_from_getDevice(self):
        """Test the old syntax for getDevice"""
        deviceId = "another_with_yield_from_remote-device"
        remote = Remote({"_deviceId_": deviceId})
        try:
            yield from remote.startInstance()
            self.remote.counter = -1
            with (yield from getDevice(deviceId)) as d:
                self.assertEqual(d.counter, -1)
                task = background(d.count())
            counter = d.counter
            for i in range(20):
                self.assertEqual(d.counter, counter)
            yield from task
            with d:
                yield from updateDevice(d)
                self.assertEqual(d.counter, 29)
        finally:
            yield from remote.slotKillDevice()


if __name__ == "__main__":
    main()
