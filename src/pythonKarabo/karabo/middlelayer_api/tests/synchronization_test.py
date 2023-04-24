# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import logging
import time
from asyncio import CancelledError, Future, TimeoutError, ensure_future

import pytest
from pint import DimensionalityError

from karabo.middlelayer import (
    MetricPrefix, QuantityValue, Unit, allCompleted, background,
    firstCompleted, firstException, gather, sleep, synchronous, unit)
from karabo.middlelayer.testing import assertLogs, event_loop, run_test
from karabo.middlelayer_api.synchronization import FutureDict

global called
called = False


@pytest.fixture(scope="function")
def loopTest(event_loop: event_loop):
    global called
    called = False
    deviceId = event_loop.instance().deviceId
    yield deviceId
    called = False


async def coro(param):
    if not param:
        raise RuntimeError
    assert param == "whatever"
    global called
    called = True
    return "coro called"


def func(param):
    if not param:
        raise RuntimeError
    assert param == "something"
    global called
    called = True
    return "func called"


async def coro_sleep():
    try:
        await sleep(1000)
        assert False
    except CancelledError:
        global called
        called = True
        raise


def func_sleep():
    try:
        sleep(1000)
        assert False
    except CancelledError:
        global called
        called = True
        raise


@pytest.mark.timeout(30)
@run_test
async def test_coro_coro_raise(loopTest):
    deviceId = loopTest
    with assertLogs(deviceId, level=logging.ERROR) as log, \
            pytest.raises(RuntimeError):
        await background(coro, False)
    assert log.records[0].exc_info[0] is RuntimeError


@pytest.mark.timeout(30)
@run_test
async def test_coro_coro_direct_raise(loopTest):
    deviceId = loopTest
    with assertLogs(deviceId, level=logging.ERROR) as log, \
            pytest.raises(RuntimeError):
        await background(coro(False))
    assert log.records[0].exc_info[0] is RuntimeError


@pytest.mark.timeout(30)
@run_test
async def test_coro_func_raise(loopTest):
    deviceId = loopTest
    with assertLogs(deviceId, level=logging.ERROR) as log, \
            pytest.raises(RuntimeError):
        await background(func, False)
    assert log.records[0].exc_info[0] is RuntimeError


@pytest.mark.timeout(30)
@run_test
def test_func_coro_raise(loopTest):
    deviceId = loopTest
    with assertLogs(deviceId, level=logging.ERROR) as log, \
            pytest.raises(RuntimeError):
        background(coro, False).wait()
    assert log.records[0].exc_info[0] is RuntimeError


@pytest.mark.timeout(30)
@run_test
def test_func_func_raise(loopTest):
    deviceId = loopTest
    with assertLogs(deviceId, level=logging.ERROR) as log, \
            pytest.raises(RuntimeError):
        background(func, False).wait()
    assert log.records[0].exc_info[0] is RuntimeError


@pytest.mark.timeout(30)
@run_test
async def test_coro_coro(loopTest):
    r = await background(coro, "whatever")
    assert r == "coro called"
    assert called


@pytest.mark.timeout(30)
@run_test
async def test_coro_coro_direct(loopTest):
    r = await background(coro("whatever"))
    assert r == "coro called"
    assert called


@pytest.mark.timeout(30)
@run_test
async def test_coro_func(loopTest):
    r = await background(func, "something")
    assert r == "func called"
    assert called


@pytest.mark.timeout(30)
@run_test
def test_func_coro(loopTest):
    r = background(coro, "whatever").wait()
    assert r == "coro called"
    assert called


@pytest.mark.timeout(30)
@run_test
def test_func_func(loopTest):
    r = background(func, "something").wait()
    assert r == "func called"
    assert called


@pytest.mark.timeout(30)
@run_test
async def test_coro_coro_cancel(loopTest):
    task = background(coro_sleep)
    await sleep(0.01)
    assert not called
    task.cancel()
    with pytest.raises(CancelledError):
        await task
    assert called


@pytest.mark.timeout(30)
@run_test
def test_func_coro_cancel(loopTest):
    task = background(coro_sleep)
    sleep(0.01)
    assert not called
    task.cancel()
    with pytest.raises(CancelledError):
        task.wait()
    assert called


@pytest.mark.timeout(30)
@run_test
async def test_coro_func_cancel(loopTest):
    task = background(func_sleep)
    await sleep(0.01)
    assert not called
    task.cancel()
    with pytest.raises(CancelledError):
        await task
    await sleep(0.01)
    assert called


@pytest.mark.timeout(30)
@run_test
def test_func_func_cancel(loopTest):
    task = background(func_sleep)
    sleep(0.01)
    assert not called
    task.cancel()
    with pytest.raises(CancelledError):
        task.wait()
    assert called


@pytest.mark.timeout(30)
@run_test
def test_gather(loopTest):
    f1 = sleep(0.01, wait=False)
    f2 = background(func, "something")
    g = gather(f1, f2)
    assert g == [None, "func called"]
    assert called


@pytest.mark.timeout(30)
@run_test
def test_gather_raise(loopTest):
    def raise_error():
        sleep(0.01)
        raise RuntimeError

    def fail_late():
        sleep(0.02)
        loopTest.fail()
    f1 = background(raise_error)
    f2 = background(func, "something")
    f3 = background(fail_late)
    with pytest.raises(RuntimeError):
        gather(f1, f2, f3)
    assert called


@pytest.mark.timeout(30)
@run_test
def test_gather_noraise(loopTest):
    def raise_error():
        sleep(0.01)
        raise RuntimeError

    def succeed_late():
        sleep(0.02)
        global called
        called = 2

    f1 = background(raise_error)
    f2 = background(func, "something")
    f3 = background(succeed_late)
    g = gather(f1, f2, f3, return_exceptions=True)
    assert len(g) == 3
    assert isinstance(g[0], RuntimeError)
    assert g[1] == "func called"
    assert g[2] is None
    assert called == 2


@pytest.mark.timeout(30)
@run_test
def test_sleep(loopTest):
    t = time.time()
    sleep(6)
    assert time.time() - t > 6

    t = time.time()
    sleep(1 * unit.ms)
    assert time.time() - t < 0.8
    # UVLoop switch gives value error
    with pytest.raises((DimensionalityError, ValueError)):
        sleep(1 * unit.meter)


@pytest.mark.timeout(30)
@run_test
def test_timeout(loopTest):
    f = sleep(1000, wait=False)
    with pytest.raises(TimeoutError):
        gather(f, timeout=0.01)

    t = time.time()
    f = sleep(1000, wait=False)
    with pytest.raises((AttributeError, TimeoutError)):
        gather(f, timeout=10 * unit.ms)
    assert time.time() - t < 1

    with pytest.raises((TypeError, DimensionalityError)):
        gather(10, timeout=10 * unit.meter)


@pytest.mark.timeout(30)
@run_test
async def test_synchronous_async(loopTest):
    @synchronous
    def f(arg):
        nonlocal done
        assert arg == 3
        done = True
    done = False
    yf = f(3)
    await sleep(0.01)
    assert not done
    await yf
    assert done


@pytest.mark.timeout(30)
@run_test
def test_synchronous_sync(loopTest):
    @synchronous
    def f(arg):
        nonlocal done
        assert arg == 7
        done = True
    done = False
    f(7)
    assert done


@pytest.mark.timeout(30)
@run_test
def test_firstCompleted_sync(loopTest):
    slow = background(sleep, 1000)
    fast = background(sleep, 0.001, "some result")
    done, pending, error = firstCompleted(slow=slow, fast=fast)
    assert done == {"fast": "some result"}
    assert list(pending.keys()) == ["slow"]
    assert not error
    assert pending["slow"] is slow
    pending["slow"].cancel()

    slow = background(sleep, 1000)
    done, pending, error = firstCompleted(slow=slow, timeout=0.001)
    assert "slow" in pending


@pytest.mark.timeout(30)
@run_test
async def test_firstCompleted_async(loopTest):
    done, pending, error = await firstCompleted(
        sleep(100), slow=sleep(1000), fast=sleep(0.001, "result"))
    assert done == {"fast": "result"}
    assert set(pending.keys()) == {0, "slow"}
    assert not error
    try:
        await pending["slow"]
        loopTest.fail()
    except CancelledError:
        pass

    exception = RuntimeError()

    async def raisor():
        raise exception
    done, pending, error = await firstCompleted(
        sleep(100), slow=sleep(1000), err=raisor())

    assert not done
    assert set(pending.keys()) == {0, "slow"}
    assert error == {"err": exception}


@pytest.mark.timeout(30)
@run_test
async def test_allCompleted(loopTest):
    exception = RuntimeError()

    async def raisor():
        raise exception

    timeout = QuantityValue(10, unit=Unit.SECOND,
                            metricPrefix=MetricPrefix.MILLI)
    done, pending, error = await allCompleted(
        sleep(100), slow=sleep(1000), fast=sleep(0.001, "result"),
        err=raisor(), timeout=timeout)

    assert done == {"fast": "result"}
    assert error == {"err": exception}
    assert set(pending.keys()) == {0, "slow"}


@pytest.mark.timeout(30)
@run_test
async def test_allCompleted_cancelled(loopTest):
    running = Future()
    done = Future()
    raises = Future()
    cancelled = Future()

    task = background(allCompleted(running, done, raises, cancelled))
    await sleep(0)
    done.set_result(None)
    raises.set_exception(RuntimeError())
    cancelled.cancel()
    await sleep(0)
    task.cancel()
    with pytest.raises(CancelledError):
        await task

    assert running.cancelled()
    assert cancelled.cancelled()
    assert not done.cancelled()
    assert not raises.cancelled()


@pytest.mark.timeout(30)
@run_test
async def test_firstException(loopTest):
    exception = RuntimeError()

    async def raisor():
        raise exception

    done, pending, error = await firstException(
        sleep(100), slow=sleep(1000), fast=sleep(0.001, "result"),
        err=raisor(), timeout=0.01)

    assert not done
    assert error == {"err": exception}
    assert set(pending.keys()) == {0, "slow", "fast"}


@pytest.mark.timeout(30)
@run_test
async def test_futuredict(loopTest):
    futuredict = FutureDict()

    task1 = ensure_future(futuredict["car"])
    task2 = ensure_future(futuredict["car"])
    task3 = ensure_future(futuredict["power"])
    yield  # let the tasks start
    assert not task1.done()
    futuredict["car"] = "DeLorean"
    assert (await task1) == "DeLorean"
    assert (await task2) == "DeLorean"
    assert task1.done()
    assert task2.done()

    assert not task3.done()
    task3.cancel()
    futuredict["power"] = "lightning"

    futuredict["whatever"] = 3  # should be a no-op
    assert futuredict["whatever"] == 3

    async def find_marty():
        await sleep(0.3)
        futuredict["marty"] = 1955

    ensure_future(find_marty)
    timezone = await futuredict["marty"]
    assert timezone == 1955
