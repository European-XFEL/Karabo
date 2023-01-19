import pytest
import pytest_asyncio

from karabo.middlelayer import (
    Device, Int32, Overwrite, Slot, State, getDevice, isSet, setNoWait,
    setWait)
from karabo.middlelayer.testing import (
    AsyncDeviceContext, event_loop, run_test, sleepUntil)


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


@pytest_asyncio.fixture(scope="module")
@pytest.mark.asyncio
async def deviceTest(event_loop: event_loop):
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
