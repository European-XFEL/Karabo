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
import pytest
import pytest_asyncio

from karabo.middlelayer import (
    Device, Int32, Overwrite, Slot, State, getDevice, isSet, setNoWait,
    setWait)
from karabo.middlelayer.testing import AsyncDeviceContext, run_test, sleepUntil


class Remote(Device):

    state = Overwrite(options=[State.ON, State.ERROR],
                      defaultValue=State.ON)
    counter = Int32(
        defaultValue=-1)
    targetCounter = Int32(
        defaultValue=-1)

    @Int32(defaultValue=0)
    def monoCounter(self, value):
        previous = self.monoCounter
        self.monoCounter = value
        if isSet(previous) and (value - previous) != 1:
            self.state = State.ERROR

    @Slot()
    async def setCount(self):
        self.counter = self.targetCounter.value

    @Slot()
    def setCountSync(self):
        self.counter = self.targetCounter.value

    @Slot()
    def setSyncError(self):
        self.state = State.ERROR

    @Slot()
    async def setError(self):
        self.state = State.ERROR

    @Slot()
    def setSyncOn(self):
        self.state = State.ON

    @Slot()
    async def setOn(self):
        self.state = State.ON


@pytest_asyncio.fixture(scope="module")
@pytest.mark.asyncio
async def deviceTest():
    remote = Remote({"_deviceId_": "OrderRemote"})
    ctx = AsyncDeviceContext(remote=remote)
    async with ctx:
        yield ctx


@pytest.mark.timeout(30)
@run_test
async def test_async_async_slot_property_order(deviceTest):
    """Test that we maintain order in async case with async slot
    """
    with (await getDevice("OrderRemote")) as d:
        for i in range(100):
            d.targetCounter = i
            await d.setCount()
            assert d.counter == i
            assert d.targetCounter == i
        d.counter = -1
        d.targetCounter = -1


@pytest.mark.timeout(30)
@run_test
async def test_async_sync_slot_property_order(deviceTest):
    """Test that we maintain order in async case with sync slot
    """
    with (await getDevice("OrderRemote")) as d:
        for i in range(100):
            d.targetCounter = i
            await d.setCountSync()
            assert d.counter == i
            assert d.targetCounter == i
        d.counter = -1
        d.targetCounter = -1


@pytest.mark.timeout(30)
@run_test
def test_sync_async_slot_property_order(deviceTest):
    """Test that we maintain order in sync case with async slot
    """
    with getDevice("OrderRemote") as d:
        for i in range(100):
            d.targetCounter = i
            d.setCount()
            assert d.counter == i
            assert d.targetCounter == i
        d.counter = -1
        d.targetCounter = -1


@pytest.mark.timeout(30)
@run_test
def test_sync_sync_slot_property_order(deviceTest):
    """Test that we maintain order in sync case with sync slot
    """
    with getDevice("OrderRemote") as d:
        for i in range(100):
            d.targetCounter = i
            d.setCountSync()
            assert d.counter == i
            assert d.targetCounter == i
        d.counter = -1
        d.targetCounter = -1


@pytest.mark.timeout(30)
@run_test
async def test_async_async_slot_property_nowait(deviceTest):
    """Test that we maintain order in async case with async slot and no wait
    """
    with (await getDevice("OrderRemote")) as d:
        for i in range(100):
            setNoWait("OrderRemote", targetCounter=i)
            await d.setCount()
            assert d.counter == i
            assert d.targetCounter == i
        d.counter = -1
        d.targetCounter = -1


@pytest.mark.timeout(30)
@run_test
async def test_async_sync_slot_property_nowait(deviceTest):
    """Test that we maintain order in async case with sync slot and no wait
    """
    with (await getDevice("OrderRemote")) as d:
        for i in range(100):
            setNoWait("OrderRemote", targetCounter=i)
            await d.setCountSync()
            assert d.counter == i
            assert d.targetCounter == i
        d.counter = -1
        d.targetCounter = -1


@pytest.mark.timeout(30)
@run_test
def test_sync_async_slot_property_nowait(deviceTest):
    """Test that we maintain order in sync case with async slot and no wait
    """
    with getDevice("OrderRemote") as d:
        for i in range(100):
            setNoWait("OrderRemote", targetCounter=i)
            d.setCount()
            assert d.counter == i
            assert d.targetCounter == i
        d.counter = -1
        d.targetCounter = -1


@pytest.mark.timeout(30)
@run_test
def test_sync_sync_slot_property_nowait(deviceTest):
    """Test that we maintain order in sync case with sync slot and no wait
    """
    with getDevice("OrderRemote") as d:
        for i in range(100):
            setNoWait("OrderRemote", targetCounter=i)
            d.setCountSync()
            assert d.counter == i
            assert d.targetCounter == i
        d.counter = -1
        d.targetCounter = -1


@pytest.mark.timeout(30)
@run_test
def test_sync_monoton_nowait(deviceTest):
    """Test that we maintain order in sync case for setNoWait calls
    """
    remote = deviceTest["remote"]
    remote.monoCounter = 0
    remote.state = State.ON
    with getDevice("OrderRemote") as d:
        assert d.state == State.ON
        for i in range(1, 100):
            setNoWait("OrderRemote", monoCounter=i)
        sleepUntil(lambda: d.monoCounter == 99, 2)
        assert d.monoCounter == 99
        assert d.state == State.ON
        setWait(d, monoCounter=25)
        assert d.state == State.ERROR


@pytest.mark.timeout(30)
@run_test
async def test_async_monoton_nowait(deviceTest):
    """Test that we maintain order in async case for setNoWait calls
    """
    remote = deviceTest["remote"]
    remote.monoCounter = 0
    remote.state = State.ON
    with (await getDevice("OrderRemote")) as d:
        assert d.state == State.ON
        for i in range(1, 100):
            setNoWait(d, monoCounter=i)
        await sleepUntil(lambda: d.monoCounter == 99, 2)
        assert d.monoCounter == 99
        assert d.state == State.ON
        await setWait(d, monoCounter=25)
        assert d.state == State.ERROR


@pytest.mark.timeout(30)
@run_test
def test_sync_sync_state_slot_reply(deviceTest):
    with getDevice("OrderRemote") as d:
        for i in range(0, 100):
            if i % 2 == 0:
                state = State.ON
                slot = d.setSyncOn
            else:
                state = State.ERROR
                slot = d.setSyncError
            slot()
            assert d.state == state


@pytest.mark.timeout(30)
@run_test
def test_sync_async_state_slot_reply(deviceTest):
    with getDevice("OrderRemote") as d:
        for i in range(0, 100):
            if i % 2 == 0:
                state = State.ON
                slot = d.setOn
            else:
                state = State.ERROR
                slot = d.setError
            slot()
            assert d.state == state


@pytest.mark.timeout(30)
@run_test
async def test_async_async_state_slot_reply(deviceTest):
    with (await getDevice("OrderRemote")) as d:
        for i in range(0, 100):
            if i % 2 == 0:
                state = State.ON
                slot = d.setOn
            else:
                state = State.ERROR
                slot = d.setError
            await slot()
            assert d.state == state


@pytest.mark.timeout(30)
@run_test
async def test_async_sync_state_slot_reply(deviceTest):
    with (await getDevice("OrderRemote")) as d:
        for i in range(0, 100):
            if i % 2 == 0:
                state = State.ON
                slot = d.setSyncOn
            else:
                state = State.ERROR
                slot = d.setSyncError
            await slot()
            assert d.state == state
