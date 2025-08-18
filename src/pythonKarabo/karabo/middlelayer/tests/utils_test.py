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
import asyncio
import time

import pytest

from karabo.common.states import State
from karabo.middlelayer.unitutil import StateSignifier, removeQuantity
from karabo.middlelayer.utils import (
    AsyncTimer, check_broker_scheme, countdown, profiler, suppress)
from karabo.native import EnumValue, Float, QuantityValue, String


@pytest.fixture(scope="module")
def utilsTest():

    class Stamp:
        pass

    obj = Stamp()
    obj.timestamp_1 = 1
    obj.timestamp_2 = 2
    obj.timestamp_3 = 3
    obj.timestamp_4 = 4
    yield obj


@pytest.mark.timeout(30)
def test_signifier(utilsTest):
    a1 = String(enum=State)
    a2 = String(enum=State)
    a3 = String(enum=State)
    a4 = String(enum=State)

    v1 = a1.toKaraboValue(State.MOVING)
    v1.timestamp = utilsTest.timestamp_1
    assert v1 == State.MOVING
    v2 = a2.toKaraboValue(State.ON)
    v2.timestamp = utilsTest.timestamp_2
    assert v2 == State.ON
    v3 = a3.toKaraboValue(State.ON)
    v3.timestamp = utilsTest.timestamp_3
    assert v3 == State.ON
    v4 = a4.toKaraboValue(State.OFF)
    v4.timestamp = utilsTest.timestamp_4
    assert v4 == State.OFF

    signifier = StateSignifier()
    state = signifier.returnMostSignificant([v1, v2, v3, v4])
    assert state == State.MOVING
    assert isinstance(state, EnumValue)
    assert state.timestamp == v4.timestamp

    v4 = a4.toKaraboValue(State.INIT)
    v4.timestamp = utilsTest.timestamp_4
    assert v4 == State.INIT

    state = signifier.returnMostSignificant([v1, v2, v3, v4])
    assert state == State.INIT
    assert isinstance(state, EnumValue)
    assert state.timestamp == v4.timestamp
    assert v1.timestamp == utilsTest.timestamp_1
    assert v2.timestamp == utilsTest.timestamp_2
    assert v3.timestamp == utilsTest.timestamp_3
    assert v4.timestamp == utilsTest.timestamp_4

    v1 = a1.toKaraboValue(State.ON)
    assert v1 == State.ON
    state = signifier.returnMostSignificant([v1, v2, v3, v4])
    assert isinstance(state, EnumValue)
    assert state == State.INIT
    assert state.timestamp == v4.timestamp
    assert state.timestamp == 4
    assert state.timestamp != v1.timestamp
    assert state.timestamp != v2.timestamp
    assert state.timestamp != v3.timestamp
    assert v1.timestamp is None
    assert v2.timestamp == utilsTest.timestamp_2
    assert v3.timestamp == utilsTest.timestamp_3
    assert v4.timestamp == utilsTest.timestamp_4

    # do without timestamp
    v1 = State.ON
    v2 = State.MOVING
    v3 = State.ON
    v4 = State.OFF
    state = signifier.returnMostSignificant([v1, v2, v3, v4])
    assert isinstance(state, EnumValue)
    assert state == State.MOVING
    assert state.timestamp is None

    # timestamp provided for the most significant state
    v1 = a1.toKaraboValue(State.ERROR)
    v1.timestamp = 20
    state = signifier.returnMostSignificant([v1, v2, v3, v4])
    assert isinstance(state, EnumValue)
    assert state == State.ERROR
    assert state.timestamp == v1.timestamp
    assert state.timestamp == 20
    with pytest.raises(AttributeError):
        assert state.timestamp != v2.timestamp
        assert state.timestamp != v3.timestamp
        assert state.timestamp != v4.timestamp

    v1 = a1.toKaraboValue(State.OFF)
    v1.timestamp = utilsTest.timestamp_1
    state = signifier.returnMostSignificant([v1, v2, v3, v4])
    assert isinstance(state, EnumValue)
    assert state == State.MOVING
    assert state.timestamp == v1.timestamp
    assert state.timestamp == 1


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_no_quantity():
    @removeQuantity
    def calculate(a, b, c=5, d=10):
        return a + b + c - d

    assert not asyncio.iscoroutinefunction(calculate)

    a = Float().toKaraboValue(6.0)
    b = Float().toKaraboValue(2.0)
    c = Float().toKaraboValue(1.0)
    d = Float().toKaraboValue(1.0)

    summation = a + b + c - d
    assert isinstance(summation, QuantityValue)
    assert summation == 8.0
    new_summation = calculate(a, b=b, c=c)
    assert not isinstance(new_summation, QuantityValue)
    assert new_summation == -1
    new_summation = calculate(a, b=b, d=c)
    assert not isinstance(new_summation, QuantityValue)
    assert new_summation == 12

    @removeQuantity
    async def async_calculate(a, b, c=5, d=10):
        return a + b + c - d

    assert asyncio.iscoroutinefunction(async_calculate)
    assert isinstance(summation, QuantityValue)
    assert summation == 8.0
    new_summation = await async_calculate(a, b=b, c=c)
    assert not isinstance(new_summation, QuantityValue)
    assert new_summation == -1
    new_summation = await async_calculate(a, b=b, d=c)
    assert not isinstance(new_summation, QuantityValue)
    assert new_summation == 12

    @removeQuantity
    async def async_identity(a, b=False):
        return a is True and b is True

    assert asyncio.iscoroutinefunction(async_identity)
    ident = await async_identity(True)
    assert not ident
    ident = await async_identity(True, b=True)
    assert ident
    ident = await async_identity(True, True)
    assert ident


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_profiler():
    sleep_time = 0.1

    @profiler()
    def slow_func(param, more_param=4):
        time.sleep(sleep_time)
        return param + more_param

    @profiler()
    async def slow_func_async(param, more_param=4):
        await asyncio.sleep(sleep_time)
        return param + more_param

    def slow_func_no_deco():
        time.sleep(sleep_time)

    assert not asyncio.iscoroutinefunction(slow_func)
    before = time.time()
    ret = slow_func(5, more_param=3)
    assert ret == 8
    after = time.time()
    diff = after - before
    pytest.approx(diff, 0.2) == sleep_time

    assert asyncio.iscoroutinefunction(slow_func_async)
    before = time.time()
    ret = await slow_func_async(2, more_param=1)
    assert ret == 3
    after = time.time()
    diff = after - before
    pytest.approx(diff, 0.2) == sleep_time

    with profiler():
        before = time.time()
        slow_func_no_deco()
        after = time.time()
        diff = after - before
        pytest.approx(diff, 0.2) == sleep_time

    with profiler("AName"):
        before = time.time()
        slow_func_no_deco()
        after = time.time()
        diff = after - before
        pytest.approx(diff, 0.2) == sleep_time

    async def inner():
        async with profiler("Test Async with") as p:
            before = time.time()
            slow_func_no_deco()
            after = time.time()
            diff = after - before
            pytest.approx(diff, 0.2) == sleep_time
            assert p.t_start is not None

    await inner()


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_async_timer():
    """Test the async timer class"""
    called = 0

    instance = asyncio.get_event_loop().instance()

    def call():
        nonlocal called
        assert asyncio.get_event_loop().instance() is instance
        # Can create a new timer inside callback
        t = AsyncTimer(timeout=0.2, callback=call)
        assert t.instance() is instance
        del t
        called += 1

    timer = AsyncTimer(timeout=0.2, callback=call)
    # 1. Test the snoozing by always starting the timer
    for _ in range(5):
        assert timer.start()
        assert timer.is_running()
        assert not called
        await asyncio.sleep(0.1)
    await asyncio.sleep(0.2)

    assert called == 1

    # 2. Test continue timer
    await asyncio.sleep(0.3)
    assert called >= 1
    timer.stop()
    await asyncio.sleep(0.2)

    # 3. Test that stop does not call
    called = 0
    assert timer.start()
    await asyncio.sleep(0.1)
    assert called == 0
    assert timer.stop()
    await asyncio.sleep(0.2)
    assert not timer.is_running()
    assert called == 0

    # 4. Multiple stop does not hurt
    for _ in range(5):
        # No need to stop
        assert not timer.stop()

    # 5. Test singleshot
    timer = AsyncTimer(timeout=0.2, callback=call, single_shot=True)
    assert timer.start()
    await asyncio.sleep(1)
    assert called == 1

    # 6. Test async callback
    called = 0

    async def call():
        nonlocal called
        assert asyncio.get_event_loop().instance() is instance
        # Can create a new timer inside callback
        t = AsyncTimer(timeout=0.2, callback=call)
        assert t.instance() is instance
        del t
        called += 1

    timer = AsyncTimer(timeout=0.2, callback=call)
    for _ in range(5):
        assert timer.start()
        assert not called
        await asyncio.sleep(0.1)
    await asyncio.sleep(0.2)
    assert called == 1

    # 7. Test the must flush
    called = 0

    timer = AsyncTimer(timeout=0.2, callback=call,
                       flush_interval=3)
    # We try to snooze here, but after 3 times timeout
    # the timer will call the callback
    with pytest.raises(AssertionError):
        for _ in range(5):
            assert timer.start()
            assert not called
            await asyncio.sleep(0.2)

    assert called > 0
    await asyncio.sleep(0.2)


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_countdown():
    """Test that we can use a countdown error context"""
    # 1. exception is `True`
    try:
        async with countdown(0.1) as ctx:
            assert ctx is not None
            await asyncio.sleep(0.2)
    except TimeoutError:
        # Must have timeout
        pass
    else:
        assert False
    assert ctx.expired

    # 2. exception is `False`, no exception
    async with countdown(0.1, False) as ctx:
        assert ctx is not None
        await asyncio.sleep(0.2)
    assert ctx.expired


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_suppress(utilsTest, caplog):
    loop = asyncio.get_running_loop()
    instanceId = loop.instance().deviceId
    with caplog.at_level("ERROR", logger=instanceId):
        with suppress(RuntimeError):
            raise RuntimeError("Error")
        assert caplog.records


def test_check_broker_scheme():
    # 1. different schemes
    hosts = ["amqp://control:spirit@exflc1:1111",
             "tcp://exfl-broker:7777"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 2. typo in schemes
    hosts = ["amqp://control:spirit@exflc1:1111",
             "amqp//control:spirit@exflc1:1111"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 3.1 correct scheme amqp, also with `-` and `.`
    hosts = ["amqp://control:spirit@exflc1:1111",
             "amqp://control:spirit@exfl-bkr-1:1111"]
    assert not check_broker_scheme(hosts)
    hosts = ["amqp://control:spirit@exflc1:1111"]
    assert not check_broker_scheme(hosts)
    hosts = ["amqp://control:spirit@exflc1.desy.de:1111",
             "amqp://control:spirit@192.168.1.1:1111"]
    assert not check_broker_scheme(hosts)

    # 4.1 incomplete scheme amqp, first missing port
    hosts = ["amqp://control:spirit@exflc1",
             "amqp://control:spirit@exflc1:1111"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)
    # 4.2 incomplete amqp, missing password, etc.
    hosts = ["amqp://control@exflc1:1111",
             "amqp://control:spirit@exflc1"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 4.3 No @ sign
    hosts = ["amqp://control:spiritexflc1:1111",
             "amqp://control:spirit@exflc1:1111"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 4.4 No : second
    hosts = ["amqp://control:spirit@exflc1:1111",
             "amqp://controlspirit@exflc1:1111"]
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)

    # 5. empty list
    hosts = ""
    with pytest.raises(RuntimeError):
        check_broker_scheme(hosts)
