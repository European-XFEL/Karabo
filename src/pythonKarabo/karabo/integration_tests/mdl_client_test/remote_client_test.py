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

import pytest

from karabo.middlelayer import DeviceClientBase, Hash, State, call, instantiate
from karabo.middlelayer.testing import AsyncServerContext, assert_wait_property


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_discovery():
    """
    Test that the client discovers MDL servers and devices that were started
    by a server (!) before the client got started.
    """
    serverId = "testServerContext"
    deviceId = "TEST/MDL/1"
    config = {deviceId: {"classId": "PropertyTest"}}
    init = json.dumps(config)
    server = AsyncServerContext(
        serverId, [f"init={init}"], verbose=True, api="middlelayer")

    async with server:
        # Wait until device is fully up:
        await assert_wait_property(
            deviceId, "state", State.NORMAL.value, timeout=10)
        # Start client
        clientId = "my-ikarabo"
        dc = DeviceClientBase(dict(_deviceId_=clientId))
        await dc.startInstance()

        servers = dc.systemTopology["server"]
        assert serverId in servers

        devices = list(dc.systemTopology["device"].keys())
        assert clientId in devices
        assert deviceId in devices


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_broadcast():
    config = {"alice": {"classId": "SimpleTopology"}}
    init = json.dumps(config)
    server = AsyncServerContext(
        "testBroad", [f"init={init}", "broadcast=1",
                      "pluginNamespace=karabo.middlelayer_device_test"],
        verbose=True, api="middlelayer")
    async with server:
        await assert_wait_property(
            "alice", "state", State.ON.value, timeout=10)
        await instantiate("testBroad", "SimpleTopology", "bob")
        topo = await call("alice", "getTopology", Hash())
        assert "bob" in topo["device"]

    server = AsyncServerContext(
        "testBroad", [f"init={init}", "broadcast=0",
                      "pluginNamespace=karabo.middlelayer_device_test"],
        verbose=True, api="middlelayer")
    async with server:
        await assert_wait_property(
            "alice", "state", State.ON.value, timeout=10)
        await instantiate("testBroad", "SimpleTopology", "bob")
        topo = await call("alice", "getTopology", Hash())
        assert "bob" not in topo["device"]
