import uuid

import pytest

from karabo.middlelayer import Device, Slot, String, connectDevice, isSet
from karabo.middlelayer_api.tests.eventloop import (
    AsyncDeviceContext, create_device_server, event_loop)

# XXX: This is to remove a flake complaint only
event_loop = event_loop


class WW(Device):
    name = String()

    def __init__(self, configuration):
        super().__init__(configuration)
        self.destructed = False

    @Slot()
    async def sayMyName(self):
        self.name = "Heisenberg"

    async def onDestruction(self):
        self.destructed = True


@pytest.mark.asyncio
async def test_device_context():
    deviceId = f"test-mdl-{uuid.uuid4()}"
    device = WW({"_deviceId_": deviceId})
    ctx_deviceId = f"test-mdl-{uuid.uuid4()}"
    ctx_device = WW({"_deviceId_": ctx_deviceId})
    async with AsyncDeviceContext(device=device) as ctx:
        devices = ctx.devices
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


@pytest.mark.asyncio
async def test_server_context():
    serverId = f"testserver-mdl-{uuid.uuid4()}"
    server = create_device_server(serverId, [WW])
    async with AsyncDeviceContext(server=server) as ctx:
        server_instance = ctx.servers["server"]
        assert server_instance.serverId == serverId
        assert "WW" in server_instance.plugins
