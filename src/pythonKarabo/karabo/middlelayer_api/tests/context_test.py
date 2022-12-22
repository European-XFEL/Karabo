import uuid
from asyncio import ensure_future, get_event_loop, sleep

import pytest

from karabo.middlelayer import (
    Device, Slot, String, connectDevice, getDevice, isSet, lock)
from karabo.middlelayer.testing import (
    AsyncDeviceContext, create_device_server, event_loop, run_test)


class Figure(Device):
    name = String()
    figure = ""

    def __init__(self, configuration):
        super().__init__(configuration)
        self.destructed = False

    @Slot()
    async def sayMyName(self):
        self.name = self.figure

    async def onDestruction(self):
        self.destructed = True


class WW(Figure):
    figure = "Heisenberg"


class JP(Figure):
    figure = "PinkMan"


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_device_context(event_loop: event_loop):
    deviceId = f"test-mdl-{uuid.uuid4()}"
    device = WW({"_deviceId_": deviceId})
    ctx_deviceId = f"test-mdl-{uuid.uuid4()}"
    ctx_device = WW({"_deviceId_": ctx_deviceId})
    async with AsyncDeviceContext(device=device) as ctx:
        devices = ctx.instances
        assert not isSet(device.name)
        assert not isSet(devices["device"].name)
        proxy = await connectDevice(deviceId)
        await proxy.sayMyName()
        assert device.name == "Heisenberg"
        assert proxy.name == "Heisenberg"
        assert devices["device"].name == "Heisenberg"
        assert len(devices) == 1
        await ctx.device_context(new=ctx_device)
        assert len(devices) == 2
        assert ctx_device.destructed is False
        assert device.destructed is False

    # Check that the context destroys all devices on exit
    assert ctx_device.destructed is True
    assert device.destructed is True


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_server_context(event_loop: event_loop):
    serverId = f"testserver-mdl-{uuid.uuid4()}"
    server = create_device_server(serverId, [WW])
    async with AsyncDeviceContext(server=server) as ctx:
        server_instance = ctx.instances["server"]
        assert server_instance.serverId == serverId
        assert "WW" in server_instance.plugins


@pytest.fixture(scope="function")
@pytest.mark.asyncio
async def deviceTest(event_loop):
    ww = WW({"_deviceId_": f"test-ww-{uuid.uuid4()}"})
    jp = JP({"_deviceId_": f"test-jp-{uuid.uuid4()}"})
    event_loop.lead = ww
    async with AsyncDeviceContext(jp=jp, ww=ww) as ctx:
        yield ctx
    event_loop.lead = None


@pytest.mark.timeout(30)
@run_test
async def test_loop_lead(deviceTest):
    wwId = deviceTest["ww"].deviceId
    jp = deviceTest["jp"]
    assert get_event_loop().lead is deviceTest["ww"]
    assert get_event_loop().instance() is deviceTest["ww"]
    async with getDevice(jp.deviceId) as proxy:
        with await lock(proxy):
            assert jp.lockedBy == wwId


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_loop_instance(event_loop):
    ww = WW({"_deviceId_": f"test-ww-{uuid.uuid4()}"})
    async with AsyncDeviceContext(ww=ww):
        task = ensure_future(sleep(5))
        await sleep(0.1)
        # instance is signal slotable
        assert task.instance() is not ww
        assert task.instance() is not None
        task.cancel()

        sleep_task = None

        async def sleepalot():
            nonlocal sleep_task
            sleep_task = ensure_future(sleep(5))

        task = event_loop.create_task(sleepalot(), instance=ww)
        await sleep(0.1)
        assert task.instance() is ww
        task.cancel()
        assert sleep_task is not None
        assert sleep_task.instance() is ww
        sleep_task.cancel()
