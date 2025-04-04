# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import time
import traceback
import weakref
from asyncio import (
    Future, TimeoutError, ensure_future, gather, sleep, wait_for)
from datetime import datetime

import pytest
import pytest_asyncio
from dateutil.parser import parse as from_isoformat
from pint import DimensionalityError

from karabo.middlelayer import (
    AlarmCondition, Bool, Configurable, Device, DeviceNode, Float, Hash, Int32,
    KaraboError, MetricPrefix, Node, Overwrite, Queue, Slot, State, String,
    Timestamp, Unit, VectorFloat, VectorInt16, VectorString, background, call,
    connectDevice, disconnectDevice, execute, filterByTags, getDevice,
    getInstanceInfo, getSchema, getSystemInfo, getTimeInfo, isAlive, isSet,
    lock, setNoWait, setWait, slot, unit, updateDevice, waitUntil,
    waitUntilNew)
from karabo.middlelayer.logger import CacheLog
from karabo.middlelayer.testing import (
    AsyncDeviceContext, assertLogs, run_test, sleepUntil)

FIXED_TIMESTAMP = Timestamp("2009-04-20T10:32:22 UTC")


@pytest_asyncio.fixture(scope="module")
@pytest.mark.asyncio(loop_scope="module")
async def deviceTest():
    local = Local({"_deviceId_": "local"})
    remote = Remote({"_deviceId_": "remote"})
    import asyncio
    asyncio.get_running_loop().lead = local
    ctx = AsyncDeviceContext(local=local, remote=remote)
    async with ctx:
        yield ctx


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

    incrementNumber = Int32(
        defaultValue=0)

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

    @Slot()
    async def startIncrement(self):
        self.create_instance_task(self.increment())

    @Slot()
    async def startFaultyIncrement(self):
        self.create_instance_task(self.increment)

    @Slot()
    def startIncrementSync(self):
        self.create_instance_task(self.increment())

    @Slot()
    def startFaultyIncrementSync(self):
        self.create_instance_task(self.increment)

    async def increment(self):
        await sleep(0.05)
        self.incrementNumber += 1

    @slot
    def slotRemote(self, a, b, c):
        """A slot that takes three arguments"""
        return True


class Local(Device):

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

    async def onInitialization(self):
        self.state = State.ON

    @slot
    async def useSetWait(self, stringValue):
        """
        'System' slot with argument (slot) that uses
        'setWait' as example from device_client interface
        """
        await setWait("remote", "string", stringValue)


@pytest.mark.timeout(30)
@run_test
async def test_call_remote(deviceTest):
    success = await call("remote", "slotRemote", 1, 2, 3)
    assert success
    # Too many arguments
    with pytest.raises(KaraboError) as exc:
        await call("remote", "slotRemote", 1, 2, 3, 4)
    text = "slotRemote() takes 4 positional arguments but 5 were given"
    assert text in str(exc)
    # not enough argument
    with pytest.raises(KaraboError) as exc:
        await call("remote", "slotRemote", 1, 2)
    text = "slotRemote() missing 1 required positional argument"
    assert text in str(exc)
    # not available slot
    with pytest.raises(KaraboError) as exc:
        await call("remote", "slotRemoteNotThere", 1, 2)
    assert "Slot does not exist" in str(exc)

    # public slots don't take arguments
    with pytest.raises(KaraboError) as exc:
        await call("remote", "startIncrementSync", 1, 2)
    assert "Slot `startIncrementSync` does not take arguments" in str(exc)


@pytest.mark.timeout(30)
@run_test
async def test_execute(deviceTest):
    """test the execution of remote slots"""
    remote = deviceTest["remote"]
    remote.done = False
    with (await getDevice("remote")) as d:
        await d.doit()
    assert remote.done


@pytest.mark.timeout(30)
@run_test
async def test_execute_noproxy(deviceTest):
    remote = deviceTest["remote"]
    remote.done = False
    await execute("remote", "doit")
    assert remote.done


@pytest.mark.timeout(30)
@run_test
async def test_set_remote(deviceTest):
    """test setting of values"""
    with (await getDevice("remote")) as d:
        d.string = "bla"
        d.unit_int = 3 * unit.meter
        d.unit_float = 7.5 * unit.millisecond
        d.unit_vector_int = [3, 4, 5] * unit.meter / unit.second
        d.unit_vector_float = [1, 2, 3]
        d.string_list = ["a", "bla"]
        await waitUntilNew(d.string_list)

        assert d.string == "bla"
        assert abs(d.string.timestamp.toTimestamp() - time.time()) < 1
        assert d.unit_int == 3 * unit.meter
        assert d.unit_float == 7.5 * unit.millisecond
        for a, b in zip(d.unit_vector_int,
                        [3, 4, 5] * unit.meter / unit.second):
            assert a == b
        for a, b in zip(d.unit_vector_float, [1, 2, 3] * unit.degree):
            assert a == b
        for a, b in zip(d.string_list, ["a", "bla"]):
            assert a == b

    remote = deviceTest["remote"]
    assert remote.string == "bla"
    assert abs(remote.string.timestamp.toTimestamp() - time.time()) < 1
    assert remote.unit_int == 3 * unit.meter
    assert remote.unit_float == 7.5 * unit.millisecond
    for a, b in zip(remote.unit_vector_int,
                    [3, 4, 5] * unit.meter / unit.second):
        assert a == b
    for a, b in zip(remote.unit_vector_float,
                    [1, 2, 3] * unit.degree):
        assert a == b
    for a, b in zip(remote.string_list, ["a", "bla"]):
        assert a == b

    with (await getDevice("remote")) as d:
        with pytest.raises(DimensionalityError):
            d.unit_int = 3 * unit.second
        with pytest.raises(DimensionalityError):
            d.unit_float = 2 * unit.meter
        with pytest.raises(DimensionalityError):
            d.unit_vector_int = [1, 2] * unit.meter
        with pytest.raises(DimensionalityError):
            d.unit_vector_float = [1, 3] * unit.meter
        with pytest.raises(TypeError):
            d.string_list = [3, 4]

        d.unit_int = 5 * unit.kilometer
        d.unit_float = 2 * unit.millisecond
        d.unit_vector_int = [2, 3, 4] * unit.meter / unit.second
        d.unit_vector_float = [5, 7, 8] * unit.degree
        d.string_list = []
        await waitUntilNew(d.string_list)

        assert d.unit_int == 5000 * unit.meter
        assert d.unit_float == 2 * unit.millisecond
        for a, b in zip(d.unit_vector_int,
                        [2, 3, 4] * unit.meter / unit.second):
            assert a == b
        for a, b in zip(d.unit_vector_float, [5, 7, 8] * unit.degree):
            assert a == b
        assert d.string_list == []

    assert remote.unit_int == 5000 * unit.meter
    assert remote.unit_float == 2 * unit.millisecond
    for a, b in zip(remote.unit_vector_int,
                    [2, 3, 4] * unit.meter / unit.second):
        assert a == b
    for a, b in zip(remote.unit_vector_float,
                    [5, 7, 8] * unit.degree):
        assert a == b
    assert remote.string_list == []


@pytest.mark.timeout(30)
@run_test
async def test_set_local(deviceTest):
    remote = deviceTest["remote"]
    remote.unit_int = 3
    with pytest.raises(DimensionalityError):
        remote.unit_int = 3 * unit.second / unit.second
    remote.unit_float = 7.5
    with pytest.raises(DimensionalityError):
        remote.unit_float = 7.5 * unit.m / unit.m
    remote.unit_vector_int = [3, 4, 5]
    with pytest.raises(DimensionalityError):
        remote.unit_vector_int = [3, 4, 5] * unit.m / unit.m
    with pytest.raises(DimensionalityError):
        remote.unit_int = 3 * unit.second
    with pytest.raises(DimensionalityError):
        remote.unit_float = 2 * unit.meter
    with pytest.raises(DimensionalityError):
        remote.unit_vector_int = [1, 2] * unit.meter
    with pytest.raises(DimensionalityError):
        remote.unit_vector_float = [1, 3] * unit.meter
    with pytest.raises(TypeError):
        remote.string_list = [3, 4]

    remote.string = "blub"
    remote.unit_vector_float = [1, 3, 3]
    remote.string_list = ["z", "bla"]

    with (await getDevice("remote")) as d:
        assert d.string == "blub"
        assert (abs(d.string.timestamp.toTimestamp() - time.time()) < 1)
        for a, b in zip(d.unit_vector_float, [1, 3, 3] * unit.degree):
            assert a == b
        for a, b in zip(d.string_list, ["z", "bla"]):
            assert a == b

    assert remote.string == "blub"
    assert (abs(remote.string.timestamp.toTimestamp() - time.time()) < 1)
    for a, b in zip(remote.unit_vector_float,
                    [1, 3, 3] * unit.degree):
        assert a == b
    for a, b in zip(remote.string_list, ["z", "bla"]):
        assert a == b

    with (await getDevice("remote")) as d:
        remote.unit_int = 5 * unit.kilometer
        remote.unit_float = 2 * unit.millisecond
        remote.unit_vector_int = [2, 3, 4] * unit.meter / unit.second
        remote.unit_vector_float = [5, 7, 8] * unit.degree
        remote.string_list = []
        await updateDevice(d)

        assert d.unit_int == 5000 * unit.meter
        assert d.unit_float == 2 * unit.millisecond
        for a, b in zip(d.unit_vector_int,
                        [2, 3, 4] * unit.meter / unit.second):
            assert a == b
        for a, b in zip(d.unit_vector_float, [5, 7, 8] * unit.degree):
            assert a == b
        assert d.string_list == []

    assert remote.unit_int == 5000 * unit.meter
    assert remote.unit_float == 2 * unit.millisecond
    for a, b in zip(remote.unit_vector_int,
                    [2, 3, 4] * unit.meter / unit.second):
        assert a == b
    for a, b in zip(remote.unit_vector_float,
                    [5, 7, 8] * unit.degree):
        assert a == b
    assert remote.string_list == []


@pytest.mark.timeout(30)
@run_test
async def test_change(deviceTest):
    """test changing a remote parameter"""
    remote = deviceTest["remote"]
    remote.value = 7
    with (await getDevice("remote")) as d:
        await d.changeit()
    assert remote.value == 3


@pytest.mark.timeout(30)
@run_test
async def test_state(deviceTest):
    remote = deviceTest["remote"]
    remote.state = State.UNKNOWN
    with (await getDevice("remote")) as d:
        assert d.state == State.UNKNOWN
        assert d.alarmCondition == AlarmCondition.NONE


@pytest.mark.timeout(30)
@run_test
async def test_remotetag_proxy(deviceTest):
    with (await getDevice("remote")) as d:
        descriptors = filterByTags(d, "AString")
        paths = [k.longkey for k in descriptors]
        assert ["string"] == paths


@pytest.mark.timeout(30)
@run_test
async def test_disconnect(deviceTest):
    """test values are not updating when disconnected"""
    remote = deviceTest["remote"]
    remote.counter = -1
    # there are often still changes on the way that we don't want to see
    await sleep(0.1)
    d = await getDevice("remote")
    task = ensure_future(d.count())
    msg = "we're not connected still seeing changes"
    assert d.counter == -1, msg
    await sleep(0.1)
    with d:
        await updateDevice(d)
        await sleep(0.1)
        tmp = d.counter
        msg = "not seeing changes although connected"
        assert d.counter != -1, msg

    await sleep(0.01)
    # sanity check: the counter should still be running
    assert tmp < 20
    msg = "too many messages arrived after disconnecting"
    assert d.counter - tmp < 2

    # Disconnecting although disconnected should not harm
    for _ in range(5):
        await disconnectDevice(d)

    with d:
        # Previously, we did sleep 0.8 seconds
        await waitUntil(lambda: d.counter == 29)
        assert d.counter == 29
    await task


@pytest.mark.timeout(30)
@run_test
async def test_set(deviceTest):
    """test setting of remote values works"""
    remote = deviceTest["remote"]
    remote.value = 7
    with (await getDevice("remote")) as d:
        assert d.value == 7
        d.value = 10
        # XXX: Now sleep independent, we wait until it is set
        # We did sleep 0.1 second here
        await waitUntil(lambda: d.value == 10)
        assert d.value == 10
        await d.changeit()
        assert d.value == 6


@pytest.mark.timeout(30)
@run_test
async def test_generic(deviceTest):
    """test calling a generic slot"""
    remote = deviceTest["remote"]
    remote.value = 7
    d = await getDevice("remote")
    await d.generic()
    assert remote.value == 22


@pytest.mark.timeout(30)
@run_test
async def test_generic_int(deviceTest):
    """test setting a generic property"""
    remote = deviceTest["remote"]
    remote.value = 7
    d = await getDevice("remote")
    d.generic_int = 33
    await updateDevice(d)
    assert remote.value == 66


@pytest.mark.timeout(30)
@run_test
async def test_setter(deviceTest):
    """test setting a property with a setter method"""
    remote = deviceTest["remote"]
    with (await getDevice("remote")) as d:
        d.other = 102
    await sleep(0.5)
    assert remote.value == 102


@pytest.mark.timeout(30)
@run_test
async def test_setwait(deviceTest):
    """test the setWait coroutine"""
    remote = deviceTest["remote"]
    d = await getDevice("remote")
    await setWait(d, value=200 * unit.mm, counter=300)
    assert remote.value == 200
    assert remote.counter == 300


@pytest.mark.timeout(30)
@run_test
async def test_setnowait(deviceTest):
    """test the setNoWait coroutine"""
    remote = deviceTest["remote"]
    remote.value = 0
    remote.counter = 0
    d = await getDevice("remote")
    setNoWait(d, value=200, counter=300 * unit.mm)
    assert remote.value == 0
    assert remote.counter == 0
    await sleepUntil(lambda: d.value == 200)
    await sleepUntil(lambda: d.counter == 300)
    assert remote.value == 200
    assert remote.counter == 300


@pytest.mark.timeout(30)
@run_test
async def test_setwait_slot(deviceTest):
    """Test that setwait can be called in @slot"""
    remote = deviceTest["remote"]
    remote.string = "none"
    await call("local", "useSetWait", "string value 42")
    assert remote.string == "string value 42"


@pytest.mark.timeout(30)
@run_test
async def test_waituntil(deviceTest):
    """test the waitUntil coroutine"""
    with (await getDevice("remote")) as d:
        d.counter = 0
        await waitUntil(lambda: d.counter == 0)
        assert d.counter == 0
        task = ensure_future(d.count())
        try:
            with pytest.raises(TimeoutError):
                await wait_for(waitUntil(lambda: d.counter > 10),
                               timeout=0.1)

            await waitUntil(lambda: d.counter > 10)
            assert d.counter >= 11

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
            assert time.time() - t0 < 1

            with pytest.raises(TimeoutError):
                await wait_for(waitUntil(lambda: d.counter > 40),
                               timeout=0.01)
        finally:
            await task


@pytest.mark.timeout(30)
@run_test
async def test_isAlive(deviceTest):
    moriturus = Local({"_deviceId_": "moriturus"})
    async with AsyncDeviceContext(moriturus=moriturus):
        proxy = await getDevice("moriturus")
        await waitUntil(lambda: isAlive(proxy))
        assert isAlive(proxy)
        await moriturus.slotKillDevice()
        await waitUntil(lambda: not isAlive(proxy))
        assert not isAlive(proxy)


@pytest.mark.timeout(30)
@run_test
async def test_isAlive_state(deviceTest):
    moriturus = Local({"_deviceId_": "moriturus"})
    async with AsyncDeviceContext(moriturus=moriturus):
        proxy = await getDevice("moriturus")
        assert isAlive(proxy)
        assert moriturus.state == State.ON
        assert proxy.state == State.ON
        await moriturus.slotKillDevice()
        await waitUntil(lambda: not isAlive(proxy))
        assert not isAlive(proxy)
        # We kill the device to see the desired state transition
        assert proxy.state == State.UNKNOWN


@pytest.mark.timeout(30)
@run_test
async def test_waituntilnew(deviceTest):
    """test the waitUntilNew coroutine for properties"""
    remote = deviceTest["remote"]
    remote.counter = None
    remote.nested.val = None
    # NOTE: Protect against a race condition in getDevice and cycle once!
    # Can be cured with async with which is not possible at the moment.
    await sleep(2)
    with (await getDevice("remote")) as d:
        d.counter = 0
        # we test that d.counter and d.nested.val are still None (it
        # must be, no await since last line). This asserts that
        # waitUntilNew also works with uninitialized values, which
        # had been a bug before
        assert d.counter == None  # noqa
        assert d.nested.val == None  # noqa
        await waitUntilNew(d.value, d.counter, d.nested.val)
        task = ensure_future(d.count())
        try:
            i = 0
            while i < 30:
                await waitUntilNew(d.counter, d.value)
                assert i <= d.counter
                i = d.counter + 1
            assert 29 == d.counter
        finally:
            await task


@pytest.mark.timeout(30)
@run_test
async def test_waituntildevice(deviceTest):
    """test the waitUntilNew coroutine for deviceTest
    """
    remote = deviceTest["remote"]
    remote.counter = -1
    await sleep(2)
    with (await getDevice("remote")) as d:
        d.counter = 0
        await sleep(0.1)
        task = ensure_future(d.count())
        try:
            i = 0
            while i < 30:
                await waitUntilNew(d)
                assert i <= d.counter
                i = d.counter + 1
            assert 29 == d.counter
        finally:
            await task


@pytest.mark.timeout(30)
@run_test
async def test_collect(deviceTest):
    """test that multiple settings are gathered into one"""
    remote = deviceTest["remote"]
    with (await getDevice("remote")) as d:
        d.once = 3
        d.once = 7
        d.once = 10
        await sleep(0.1)
    assert remote.once_value == 10


@pytest.mark.timeout(30)
@run_test
async def test_error_value(deviceTest):
    d = await getDevice("remote")
    d.error_value = 3
    with pytest.raises(KaraboError):
        await d.doit()


@pytest.mark.timeout(30)
@run_test
async def test_disallow(deviceTest):
    """test that values cannot be set if in wrong state"""
    remote = deviceTest["remote"]
    assert remote.state == State.UNKNOWN
    with (await getDevice("remote")) as d:
        d.value = 7
        await waitUntil(lambda: d.value == 7)
        # disallowed_int is normally not allowed
        with assertLogs("remote", "WARNING") as log:
            # no error here as we need to wait for network round-trip
            d.disallowed_int = 333
            await sleep(0.5)
        with pytest.raises(KaraboError):
            # this raises the error from above
            d.value = 8
        with assertLogs("remote", "WARNING") as log, \
                pytest.raises(KaraboError):
            d.disallowed_int = 333
            await d.allow()
        assert len(log.records) == 1
        assert log.records[0].levelname == "WARNING"
        assert d.value == 7

        # d.allow() sets d.value to 777 and is changing state such that
        # disallowed_int can be set ...
        await d.allow()
        assert d.value == 777
        d.value = 4
        # ... but it cannot be called itself anymore ...
        with assertLogs("remote", "WARN") as logs, \
                pytest.raises(KaraboError):
            await d.allow()
        assert len(logs.records) == 1
        assert logs.records[0].levelname == "WARNING"
        assert d.value == 4
        # ... but disallowed_int sets this back ...
        d.disallowed_int = 444
        await waitUntil(lambda: d.value == 444)
        assert d.value == 444
        # ... so allow can be called again!
        await d.allow()
        assert d.value == 777


@pytest.mark.timeout(30)
@run_test
async def test_log(deviceTest):
    """test the logging of warnings and exceptions"""
    remote = deviceTest["remote"]
    local = deviceTest["local"]

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

    remote.logmessage = None
    with (await getDevice("remote")) as d:
        local.logger.warning("this is an info")
        await d.read_log()

    hash = remote.logmessage
    assert hash["message"] == "this is an info"
    assert hash["type"] == "WARN"
    assert hash["category"] == "local"
    assert _absolute_delta_to_now(hash["timestamp"]) <= 10

    remote.logmessage = None
    with (await getDevice("remote")) as d:
        try:
            raise RuntimeError("test_log exception")
        except Exception:
            local.logger.exception("expected exception")
        await d.read_log()

    hash = remote.logmessage
    assert hash["message"] == "expected exception"
    assert hash["type"] == "ERROR"
    assert hash["category"] == "local"
    assert _absolute_delta_to_now(hash["timestamp"]) <= 10
    # Traceback is a joined string of traceback info parts
    assert type(hash["traceback"]) is str


@pytest.mark.timeout(30)
@run_test
async def test_earlylog(deviceTest):
    class A(Device):
        @Int32()
        def logint(self, value):
            self.logger.info("log the int")

        async def onInitialization(self):
            self.logger.info("some test log message")

    with assertLogs("testearlylog", "INFO") as cm:
        a = A({"_deviceId_": "testearlylog"})
    assert cm.records[0].msg == "log the int"
    with assertLogs("testearlylog", "INFO") as cm:
        await a.startInstance()
    assert cm.records[0].msg, "some test log message"
    await a.slotKillDevice()


@pytest.mark.timeout(30)
@run_test
async def test_queue(deviceTest):
    """test queues of properties"""
    with (await getDevice("remote")) as d:
        task = ensure_future(d.count())
        await waitUntil(lambda: d.counter == 0)
        try:
            q = Queue(d.counter)
            for i in range(1, 30):
                j = await q.get()
                assert i == j
                await sleep(i * 0.003)
        finally:
            await task


@pytest.mark.timeout(30)
@run_test
async def test_nested(deviceTest):
    """test accessing nested properties"""
    remote = deviceTest["remote"]
    remote.nested.val = 3 * unit.second
    assert abs(remote.nested.val.timestamp.toTimestamp() - time.time()) < 1
    ts1 = remote.nested.val.timestamp
    remote.nested.nestnest.value = 7 * unit.meter
    ts2 = remote.nested.nestnest.value.timestamp
    with (await getDevice("remote")) as d:
        assert d.nested.val == 3 * unit.second
        assert d.nested.val.timestamp == ts1
        assert d.nested.nestnest.value == 7 * unit.meter
        d.nested.val = 4 * unit.second
        assert d.nested.nestnest.value == 7 * unit.meter
        assert d.nested.nestnest.value.timestamp == ts2
        d.nested.val = 4 * unit.second
        d.nested.nestnest.value = 5 * unit.meter
        await d.nested.nestedSlot()
        # nestedSlot doubles the nested.val
    assert remote.nested.val == 8 * unit.second
    assert remote.nested.nestnest.value == 5 * unit.meter


@pytest.mark.timeout(30)
@run_test
async def test_error(deviceTest):
    """test error reporting and calling of error methods"""
    remote = deviceTest["remote"]
    local = deviceTest["local"]

    remote.done = False
    with assertLogs("local", "ERROR"):
        with (await getDevice("local")) as d:
            with pytest.raises(KaraboError):
                await d.error()

    assert remote.done
    remote.done = False
    assert local.exc_slot is Local.error
    assert isinstance(local.exception, RuntimeError)
    local.traceback.tb_lasti  # check whether that is a traceback
    del local.exc_slot
    del local.exception
    del local.traceback
    del local.traceback_str


@pytest.mark.timeout(30)
@run_test
async def test_error_in_error(deviceTest):
    """test what happens if an error happens in an error method"""
    remote = deviceTest["remote"]
    local = deviceTest["local"]
    remote.done = False

    with assertLogs("local", "ERROR") as logs:
        with (await getDevice("local")) as d:
            with pytest.raises(KaraboError):
                await d.error_in_error()

    assert logs.records[-1].msg == "error in error handler"
    assert not remote.done
    assert local.exc_slot is Local.error_in_error
    assert "On purpose" in local.traceback_str
    assert isinstance(local.exception, RuntimeError)
    local.traceback.tb_lasti  # check whether that is a traceback
    del local.exc_slot
    del local.exception
    del local.traceback
    del local.traceback_str


@pytest.mark.timeout(30)
@run_test
async def test_task_error(deviceTest):
    """test that errors of created tasks are properly reported"""
    remote = deviceTest["remote"]
    remote.done = False
    local = deviceTest["local"]
    with assertLogs("local", "ERROR"):
        with (await getDevice("local")) as loc, \
                (await getDevice("remote")) as rem:
            await loc.task_error()
            assert not rem.done
            await waitUntil(lambda: rem.done)
            assert rem.done

    assert local.exc_slot is None
    assert isinstance(local.exception, RuntimeError)
    assert "On purpose" in local.traceback_str
    del local.exc_slot
    del local.exception
    del local.traceback
    del local.traceback_str


@pytest.mark.timeout(30)
@run_test
async def test_task_error_background_coro(deviceTest):
    """test that errors of background tasks are properly reported"""
    remote = deviceTest["remote"]
    local = deviceTest["local"]
    remote.logmessage = None
    with (await getDevice("local")) as loc, \
            (await getDevice("remote")) as rem:
        await loc.task_background_error_coro()
        await sleep(0.1)
        await rem.read_log()

        # Read out the log message
        hash = remote.logmessage
        message = hash["message"]
        assert "Error" in message
        # assert "Error in background task ..." in message
        assert hash["type"] == "ERROR"
        assert hash["category"] == "local"
        assert "On purpose" in hash["traceback"]

    del local.exc_slot
    del local.exception
    del local.traceback
    del local.traceback_str


@pytest.mark.timeout(30)
@run_test
async def test_task_error_background_no_coro(deviceTest):
    """test that errors of background tasks are properly reported"""
    remote = deviceTest["remote"]
    local = deviceTest["local"]
    remote.logmessage = None
    with (await getDevice("local")) as loc, \
            (await getDevice("remote")) as rem:
        t = ensure_future(rem.read_log())
        await loc.task_background_error_coro()
        await t

        # Read out the log message
        hash = remote.logmessage
        message = hash["message"]
        assert "Error" in message
        # assert "Error in background task ..." in message
        assert hash["type"] == "ERROR"
        assert hash["category"] == "local"
        assert "On purpose" in hash["traceback"]

    del local.exc_slot
    del local.exception
    del local.traceback
    del local.traceback_str


@pytest.mark.timeout(30)
@run_test
async def test_connectDevice(deviceTest):
    remote = deviceTest["remote"]
    d = await connectDevice("remote")
    try:
        assert d.value != 123
        remote.value = 123
        await sleep(1.0)
        assert d.value == 123
    finally:
        # check the proxy gets collected when not used anymore
        weak = weakref.ref(d)
        del d
        # XXX: Added, flaky CI
        import gc
        gc.collect()
        assert weak() is None


@pytest.mark.timeout(30)
@run_test
async def test_nonproper_connectDevice():
    with pytest.raises(KaraboError) as exc:
        await connectDevice(None)
    assert "Need a proper" in str(exc)
    with pytest.raises(KaraboError) as exc:
        await connectDevice("")
    assert "Need a proper" in str(exc)


@pytest.mark.timeout(30)
@run_test
async def test_getdoc(deviceTest):
    # this is usually done by iPython/iKarabo
    d = await getDevice("remote")
    assert d.value.getdoc() == "The Value"


@pytest.mark.timeout(30)
@run_test
async def test_devicenode(deviceTest):
    class A(Device):
        dn = DeviceNode()

        async def onInitialization(self):
            self.state = State.NORMAL

    class B(Device):
        pass

    a = A({"_deviceId_": "devicenode", "dn": "badwienix"})
    b = B({"_deviceId_": "badwienix"})
    async with AsyncDeviceContext(a=a, timeout=0) as ctx:
        with (await getDevice("devicenode")) as d:
            # The device does not pass `onInitialization` as
            # the device badwienix is not online.
            assert d.state == State.UNKNOWN
            assert d.lockedBy == ""
            assert d.dn.value == "badwienix"
            ts_before = d.dn.timestamp
            assert ts_before is not None
            assert type(d).dn.displayType == "deviceNode"
            await ctx.device_context(b=b)
            # wait until it is online, the device will pass
            # the initialization phase
            await waitUntil(lambda: d.state == State.NORMAL)
            assert d.dn.value == "badwienix"
            ts_after = d.dn.timestamp
            assert ts_after is not None
            assert ts_after == ts_before
            assert d.state == State.NORMAL
            # Timestamp is not changed under hood, verify
            await updateDevice(d)
            ts_after = d.dn.timestamp
            assert ts_after == ts_before


@pytest.mark.timeout(30)
@run_test
async def test_devicenode_novalue(deviceTest):
    class A(Device):
        dn = DeviceNode()

    with pytest.raises(KaraboError):
        A({"_deviceId_": "devicenode"})


@pytest.mark.timeout(30)
@run_test
async def test_prenatal_proxy(deviceTest):
    task = ensure_future(getDevice("prenatal"))
    a = Remote({"_deviceId_": "prenatal"})
    await a.startInstance()
    proxy = await task
    assert proxy.deviceId == "prenatal"


@pytest.mark.timeout(30)
@run_test
async def test_proxy_dead():
    a = Remote({"_deviceId_": "moriturus"})
    async with AsyncDeviceContext(a=a):
        proxy = await getDevice("moriturus")
        await a.slotKillDevice()
        with pytest.raises(KaraboError) as exc:
            await proxy.count()
        assert '"moriturus" died' in str(exc)


@pytest.mark.timeout(30)
@run_test
async def test_device_schema():
    remote = Remote({"_deviceId_": "remoteschema"})
    async with AsyncDeviceContext(remote=remote):
        schema = await getSchema("remoteschema", False)
        assert schema.name == "Remote"
        h = schema.hash
        assert h["value", ...] == {
            'requiredAccessLevel': 1,
            'metricPrefixSymbol': '',
            'accessMode': 4,
            'description': 'The Value',
            'unitSymbol': '',
            'assignment': 0,
            'nodeType': 0,
            'defaultValue': 7,
            'valueType': 'INT32',
            'tags': ['whatever'],
        }
        assert h["nested", ...] == {
            'requiredAccessLevel': 0,
            'assignment': 0,
            'nodeType': 1,
            'accessMode': 4}
        assert h["doit", ...] == {
            'requiredAccessLevel': 1,
            'assignment': 0,
            'displayType': 'Slot',
            'classId': 'Slot',
            'nodeType': 1,
            'accessMode': 4}
        assert "allow" in h
        assert "disallowed_int" in h

        schema = await getSchema("remoteschema", True)
        assert schema.name in "Remote"
        h = schema.hash
        assert "allow" in h
        assert "disallowed_int" not in h


@pytest.mark.timeout(30)
@run_test
async def test_lock(deviceTest):
    with (await getDevice("remote")) as d:
        with (await lock(d)):
            assert d.lockedBy == "local"
            with (await lock(d)):
                assert d.lockedBy == "local"
                d.value = 33
                await waitUntil(lambda: d.value == 33)
            await updateDevice(d)
            assert d.lockedBy == "local"
        await waitUntil(lambda: d.lockedBy == "")


@pytest.mark.timeout(30)
@run_test
async def test_lock_concurrence(deviceTest):
    with (await getDevice("remote")) as d:
        await setWait(d, value=40, lockedBy="NoDevice")
        assert d.lockedBy == "NoDevice"
        assert d.value == 40
        with pytest.raises(KaraboError) as e:
            await setWait(d, value=50)
            assert e == "Device locked by NoDevice"
        assert d.value == 40
        await d.slotClearLock()
        await setWait(d, value=50)
        assert d.value == 50
        assert d.lockedBy == ""


@pytest.mark.timeout(30)
@run_test
async def test_device_node_alive(deviceTest):

    class Sub(Configurable):
        dn = DeviceNode()

    class A(Device):
        dn = DeviceNode()
        sub = Node(Sub)

        async def onInitialization(self):
            self.state = State.ON

    node_device = A({"_deviceId_": "devicenodealive",
                     "dn": "scratchy", "sub": {"dn": "itchy"}})
    scratchy = Local({"_deviceId_": "scratchy"})
    itchy = Local({"_deviceId_": "itchy"})
    async with AsyncDeviceContext(node_device=node_device, scratchy=scratchy,
                                  itchy=itchy):
        while True:
            if node_device.state == State.ON:
                break
            await sleep(0.1)
        assert isAlive(node_device.dn)
        assert isAlive(node_device.sub.dn)
        await scratchy.slotKillDevice()
        await itchy.slotKillDevice()
        await waitUntil(lambda: not isAlive(node_device.dn))
        assert not isAlive(node_device.dn)
        await waitUntil(lambda: not isAlive(node_device.sub.dn))
        assert not isAlive(node_device.sub.dn)


@pytest.mark.timeout(30)
@run_test
async def test_double_getDevice(deviceTest):
    a, b = await gather(getDevice("remote"), getDevice("remote"))
    assert a is b

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


@pytest.mark.timeout(30)
@run_test
async def test_inject(deviceTest):
    remote = deviceTest["remote"]
    await sleep(1)
    with (await getDevice("remote")) as d:
        remote.__class__.injected = String()
        remote.__class__.injected_node = Node(Nested)
        remote.__class__.changeit = Overwrite(
            displayedName="ChangeIt")

        @Int32()
        async def tobeinit(myself, value):
            assert value == 100
            myself.tobeinit = 123

        remote.__class__.tobeinit = tobeinit

        @Slot(displayedName="Injected")
        def slot(self):
            nonlocal slotdata
            slotdata = 44

        slotdata = None
        remote.__class__.injected_slot = slot

        with pytest.raises(TimeoutError):
            await wait_for(waitUntil(lambda: hasattr(d, "injected")),
                           timeout=0.1)
        await remote.publishInjectedParameters(tobeinit=100)
        assert remote.changeit.descriptor.displayedName == "ChangeIt"
        remote.injected = "smthng"
        remote.injected_node = Nested({})
        await waitUntil(lambda: hasattr(d, "injected"))
        await waitUntil(lambda: d.injected == "smthng")
        assert d.tobeinit, 123 == "initializer was not called"
        remote.injected = "bla"
        await waitUntil(lambda: d.injected == "bla")
        await d.injected_slot()
        assert remote.injected_slot.descriptor.displayedName == "Injected"
        assert slotdata == 44

        d.injected_node.val = 10
        await d.injected_node.nestedSlot()
        assert d.injected_node.val == 20 * unit.millisecond

        d.injected = "whatever"
        await waitUntil(lambda: remote.injected == "whatever")


@pytest.mark.timeout(30)
@run_test
async def test_earlyinject(deviceTest):

    class A(Device):
        async def onInitialization(self):
            self.__class__.number = Int32()
            await self.publishInjectedParameters()
            self.number = 3

    a = A({"_deviceId_": "testinject"})
    await a.startInstance()
    with (await getDevice("testinject")) as proxy:
        assert proxy.number == 3


@pytest.mark.timeout(30)
@run_test
async def test_device_restart():
    remote = Remote({"_deviceId_": "reconremote"})
    async with AsyncDeviceContext(remote=remote):
        await sleep(0.1)
        proxy = None
        remote.done = False
        future = Future()

        class A(Device):
            async def onInitialization(self):
                nonlocal future, proxy
                proxy = await connectDevice("reconremote")
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
        assert weakproxy() is None
        await proxy.doit()
        assert proxy.done


@pytest.mark.timeout(30)
@run_test
async def test_archive_fails(deviceTest):
    """Test that deviceTest don't have archive in their schema"""
    with (await getDevice("remote")) as d:
        with pytest.raises(AttributeError):
            _ = d.archive.value
    info = await getInstanceInfo("remote")
    assert "archive" not in info


@pytest.mark.timeout(30)
@run_test
async def test_timeinfo_device(deviceTest):
    info = await getTimeInfo("remote")
    assert info["latency"] > 0
    assert isinstance(Timestamp(info["time"]), Timestamp)
    assert isinstance(Timestamp(info["reference"]), Timestamp)
    assert info["timeServerId"] is not None

    sys_info = await getSystemInfo("remote")
    assert sys_info["broker"] is not None
    assert sys_info["user"] is not None
    timeinfo = sys_info["timeInfo"]
    assert isinstance(Timestamp(timeinfo["time"]), Timestamp)
    assert isinstance(Timestamp(timeinfo["reference"]), Timestamp)
    assert timeinfo["timeServerId"] is not None


@pytest.mark.timeout(30)
@run_test
async def test_config_handler(deviceTest):
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
                assert "counter" in h

            # unsubscribe from changes
            d.setConfigHandler(None)
            h.clear()

            await waitUntilNew(d)
            assert "counter" not in h
        finally:
            await task
        d.counter = -1


@pytest.mark.timeout(30)
@run_test
async def test_async_disconnect():
    remote = Remote({"_deviceId_": "newremote"})
    async with AsyncDeviceContext(remote=remote):
        remote.counter = -1
        d = await getDevice("newremote")
        # A proxy is always connected for a few seconds,
        # trigger manual disconnect to speed up
        await disconnectDevice(d)
        task = background(d.count())
        assert d.counter == -1, "we're not connected still seeing changes"
        async with d:
            await waitUntil(lambda: d.counter == 10)
            assert d.counter == 10

        await sleep(0.05)
        # We are not connected anymore!
        counter = d.counter
        for i in range(20):
            # Cycle and spin the eventloop like crazy! But we do not
            # receive updates anymore!
            await sleep(0.01)
            assert d.counter == counter
        await task
        async with d:
            await updateDevice(d)
            assert d.counter == 29


@pytest.mark.timeout(30)
@run_test
async def test_async_with(deviceTest):
    remote = deviceTest["remote"]
    remote.counter = -1
    got_config = remote.got_config
    async with getDevice("remote") as d:
        task = background(d.count())
        await waitUntil(lambda: d.counter == 10)
        assert d.counter == 10

    await sleep(0.05)

    assert remote.got_config == got_config + 1
    assert d.counter < 29
    await task
    async with d:
        await updateDevice(d)
        assert d.counter == 29


@pytest.mark.timeout(30)
@run_test
async def test_crazy_injection_destruction(deviceTest):
    deviceId = "crazy-test-device"
    d = CrazyRemote({"_deviceId_": deviceId})
    await d.startInstance()
    assert not d.destructed
    try:
        await d.injectSlot()
        # Wait very very long, 1 seconds
        await sleep(1)
        assert d.counter == 1
        d.counter = 0
        await d.injectDirect()
        # Wait very very long, 1 seconds
        await sleep(1)
        assert d.counter == 1
    finally:
        await d.slotKillDevice()
        assert d.destructed


@pytest.mark.timeout(30)
@run_test
def test_async_yield_from_connectDevice():
    """Test the old syntax for connectDevice"""
    deviceId = "yield_from_remote-device"
    remote = Remote({"_deviceId_": deviceId})
    with (yield from AsyncDeviceContext(remote=remote)):
        remote.counter = -1
        d = yield from connectDevice(deviceId)
        assert d.counter == -1
        task = background(d.count())
        yield from waitUntil(lambda: d.counter == 10)
        assert d.counter == 10
        yield from task
        assert d.counter == 29


@pytest.mark.timeout(30)
@run_test
def test_async_yield_from_getDevice():
    """Test the old syntax for getDevice"""
    deviceId = "another_yield_from_remote-device"
    remote = Remote({"_deviceId_": deviceId})
    with (yield from AsyncDeviceContext(remote=remote)):
        remote.counter = -1
        d = yield from getDevice(deviceId)
        assert d.counter == -1
        counter = d.counter
        for i in range(20):
            yield from sleep(0.01)
            assert d.counter == counter
        task = background(d.count())
        assert d.counter == -1
        yield from task
        with d:
            yield from updateDevice(d)
            assert d.counter == 29


@pytest.mark.timeout(30)
@run_test
def test_async_with_yield_from_getDevice(deviceTest):
    """Test the old syntax for getDevice"""
    deviceId = "another_with_yield_from_remote-device"
    remote = Remote({"_deviceId_": deviceId})
    try:
        yield from remote.startInstance()
        remote.counter = -1
        with (yield from getDevice(deviceId)) as d:
            assert d.counter == -1
            task = background(d.count())
        counter = d.counter
        for i in range(20):
            assert d.counter == counter
        yield from task
        with d:
            yield from updateDevice(d)
            assert d.counter == 29
    finally:
        yield from remote.slotKillDevice()


@pytest.mark.timeout(30)
@run_test
async def test_instance_task(deviceTest):
    """test setting of values"""
    with (await getDevice("remote")) as d:
        assert d.incrementNumber == 0
        await d.startIncrement()
        assert d.incrementNumber == 0
        await waitUntil(lambda: d.incrementNumber > 0)
        assert d.incrementNumber == 1
        with pytest.raises(KaraboError) as e:
            await d.startFaultyIncrement()
        assert "Input must be a of type coroutine" in str(e)

        # Sync slot case
        assert d.incrementNumber == 1
        await d.startIncrementSync()
        assert d.incrementNumber == 1
        await waitUntil(lambda: d.incrementNumber > 1)
        assert d.incrementNumber == 2
        with pytest.raises(KaraboError) as e:
            await d.startFaultyIncrementSync()
        assert "Input must be a of type coroutine" in str(e)
