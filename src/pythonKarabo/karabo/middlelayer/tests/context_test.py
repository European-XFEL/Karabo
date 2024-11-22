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
import json
import uuid
from asyncio import ensure_future, get_running_loop, sleep

import pytest
import pytest_asyncio

from karabo.middlelayer import (
    Device, Slot, State, String, connectDevice, getDevice, isSet, lock)
from karabo.middlelayer.testing import (
    AsyncDeviceContext, AsyncServerContext, assert_wait_property,
    create_device_server, run_test)


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
async def test_device_context():
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
async def test_server_context():
    serverId = f"testserver-mdl-{uuid.uuid4()}"
    server = create_device_server(serverId, [WW])
    async with AsyncDeviceContext(server=server) as ctx:
        server_instance = ctx.instances["server"]
        assert server_instance.serverId == serverId
        assert "WW" in server_instance.plugins


@pytest_asyncio.fixture(scope="function")
@pytest.mark.asyncio
async def deviceTest():
    ww = WW({"_deviceId_": f"test-ww-{uuid.uuid4()}"})
    jp = JP({"_deviceId_": f"test-jp-{uuid.uuid4()}"})
    get_running_loop().lead = ww
    async with AsyncDeviceContext(jp=jp, ww=ww) as ctx:
        yield ctx
    get_running_loop().lead = None


@pytest.mark.timeout(30)
@run_test
async def test_loop_lead(deviceTest):
    wwId = deviceTest["ww"].deviceId
    jp = deviceTest["jp"]
    assert get_running_loop().lead is deviceTest["ww"]
    assert get_running_loop().instance() is deviceTest["ww"]
    async with getDevice(jp.deviceId) as proxy:
        with await lock(proxy):
            assert jp.lockedBy == wwId


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_loop_instance():
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

        task = ww.create_instance_task(sleepalot())
        await sleep(0.1)
        assert task.instance() is ww
        task.cancel()
        assert sleep_task is not None
        assert sleep_task.instance() is ww
        sleep_task.cancel()


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_async_server_context():
    config = {"PropertyTestContext": {"classId": "PropertyTest"}}
    init = json.dumps(config)
    server = AsyncServerContext(
        "testServerContext", [f"init={init}"], api="middlelayer")
    async with server:
        await assert_wait_property(
            "PropertyTestContext", "state", State.NORMAL.value, timeout=10)
