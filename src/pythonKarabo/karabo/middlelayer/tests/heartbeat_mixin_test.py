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

import pytest

from karabo.native import Hash

from ..heartbeat_mixin import HeartBeatMixin

TARGET = "karabo.middlelayer.heartbeat_mixin.HeartBeatMixin._topology_changed"  # noqa


def server_devices(hb_server: int | float = 20, hb_device: int | float = 0.1):
    _SERVER_INSTANCES = Hash(
        "karabo_test_1", Hash(
            "heartbeatInterval", hb_server, "type", "server"),
        "karabo_test_2", Hash(
            "heartbeatInterval", hb_server, "type", "server"))
    _DEVICE_INSTANCES = Hash(
        "alice", Hash("heartbeatInterval", hb_device, "type",
                      "device", "serverId", "karabo_test_1"),
        "bob", Hash("heartbeatInterval", hb_device, "type",
                    "device", "serverId", "karabo_test_1"),
        "charlie", Hash("heartbeatInterval", hb_device, "type",
                        "device", "serverId", "karabo_test_1"),
        "delta", Hash("heartbeatInterval", hb_device, "type",
                      "device", "serverId", "karabo_test_1"),
        "echo", Hash("heartbeatInterval", hb_device, "type",
                     "device", "serverId", "karabo_test_1"))
    return _SERVER_INSTANCES, _DEVICE_INSTANCES


@pytest.fixture(scope="function")
def heartbeatMixin():
    """The fixture creates a mixin and fills it with servers and devices

    Here the devices have a significantly smaller heartbeat interval than
    servers.
    """
    mixin = HeartBeatMixin()
    _SERVERS, _DEVICES = server_devices(20, 0.1)
    for server, info in _SERVERS.items():
        mixin.slotDiscoverAnswer(server, info)
    for device, info in _DEVICES.items():
        mixin.slotDiscoverAnswer(device, info)
    yield {"mixin": mixin,
           "server": _SERVERS,
           "device": _DEVICES}


@pytest.fixture(scope="function")
def heartBeatMixinServer():
    """The fixture creates a mixin and fills it with servers and devices

    Here the servers have a significantly smaller heartbeat interval than
    devices.
    """
    mixin = HeartBeatMixin()
    _SERVERS, _DEVICES = server_devices(0.1, 20)
    for server, info in _SERVERS.items():
        mixin.slotDiscoverAnswer(server, info)
    for device, info in _DEVICES.items():
        mixin.slotDiscoverAnswer(device, info)
    yield {"mixin": mixin,
           "server": _SERVERS,
           "device": _DEVICES}


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_topology_init(heartbeatMixin):
    mixin = heartbeatMixin["mixin"]
    topology = mixin.systemTopology
    assert topology["client"] == Hash()
    assert topology["macro"] == Hash()
    assert len(topology["device"]) == len(heartbeatMixin["device"])
    assert len(topology["server"]) == len(heartbeatMixin["server"])


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_topology_add(heartbeatMixin):
    """Test heartbeat mixin add instance"""
    mixin = heartbeatMixin["mixin"]
    topology = mixin.systemTopology
    assert topology["macro"] == Hash()
    instanceId = "macro_1"
    info = Hash("type", "macro", "heartbeatInterval",
                0.1, "serverId", "__none__")
    await mixin._add_instance(instanceId, info)
    assert len(topology["macro"]) == 1


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_topology_add_unknown(heartbeatMixin):
    """Test heartbeat mixin add unknown instance"""
    mixin = heartbeatMixin["mixin"]
    topology = mixin.systemTopology
    instanceId = "outlawdevice"
    info = Hash("type", "notknown", "heartbeatInterval",
                0.1, "serverId", "__none__")
    await mixin._add_instance(instanceId, info)
    assert len(topology["notknown"]) == 1


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_topology_server_already_online(heartbeatMixin, mocker):
    """Test heartbeat mixin with server already online"""
    mixin = heartbeatMixin["mixin"]
    _DEVICE_INSTANCES = heartbeatMixin["device"]
    topology = mixin.systemTopology
    servers = topology["server"]
    assert len(servers) == len(heartbeatMixin["server"])
    assert len(topology["device"]) > 0

    assert "karabo_test_1" in servers
    topo_mock = mocker.patch(TARGET)
    # Server goes online that has a few devices, all devices
    # are removed
    instanceId = "karabo_test_1"
    info = Hash("type", "server", "heartbeatInterval", 0.1)
    await mixin._add_instance(instanceId, info)
    assert len(topology["device"]) == 0
    assert topo_mock.called
    _, kwargs = topo_mock.call_args
    new = kwargs["new"]
    gone = kwargs["gone"]
    devices = [instanceId for (instanceId, _) in gone]
    assert new == []
    assert devices == ["alice", "bob", "charlie", "delta", "echo"]

    topo_mock = mocker.patch(TARGET)
    # A removal of non existing device does not crash
    for device, info in _DEVICE_INSTANCES.items():
        mixin._remove_instance(device, info)
    assert not topo_mock.called

    assert len(topology["device"]) == 0
    assert "karabo_test_2" in servers
    topo_mock = mocker.patch(TARGET)
    # Server goes online that has NO CHILDREN
    instanceId = "karabo_test_2"
    info = Hash("type", "server", "heartbeatInterval", 0.1)
    await mixin._add_instance(instanceId, info)
    assert len(topology["device"]) == 0
    assert not topo_mock.called


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_topology_server_goes_offline(heartbeatMixin):
    """Test heartbeat mixin with server going offline"""
    mixin = heartbeatMixin["mixin"]
    _SERVER_INSTANCES = heartbeatMixin["server"]
    topology = mixin.systemTopology
    servers = topology["server"]
    assert len(servers) == len(_SERVER_INSTANCES)
    assert len(topology["device"]) > 0

    assert "karabo_test_1" in servers
    instanceId = "karabo_test_1"
    info = Hash("type", "server", "heartbeatInterval", 0.1)
    # Server goes offline that has a few devices, all devices
    # are removed
    devices = mixin._remove_instance(instanceId, info)
    assert len(topology["device"]) == 0
    assert len(topology["server"]) == len(_SERVER_INSTANCES) - 1
    assert "karabo_test_1" not in topology["server"]
    devices = [instanceId for (instanceId, _) in devices]
    assert devices == ["alice", "bob", "charlie", "delta", "echo"]


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_device_goes_online(heartbeatMixin, mocker):
    """Test heartbeat mixin with device going online"""
    mixin = heartbeatMixin["mixin"]
    topology = mixin.systemTopology
    instanceId = "eleanor"
    info = Hash("type", "device", "heartbeatInterval", 0.1)

    topo_mock = mocker.patch(TARGET)
    assert len(topology["device"]) == 5
    await mixin._add_instance(instanceId, info)
    assert len(topology["device"]) == 6
    # No call done, we track beats
    assert not topo_mock.called


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_spin_heartbeats(heartbeatMixin, mocker):
    """Spin the tracking until all devices are out due to heartbeats"""
    mixin = heartbeatMixin["mixin"]
    topology = mixin.systemTopology
    devices = topology["device"]
    assert len(devices) == 5
    topo_mock = mocker.patch(TARGET)
    mixin._sigslot = mocker.AsyncMock()

    for _ in range(10):
        await mixin._track_action()
        _, kwargs = topo_mock.call_args
        new = kwargs["new"]
        gone = kwargs["gone"]
        if not new and not gone:
            await asyncio.sleep(0.05)
            continue
        gone = [instanceId for (instanceId, _) in gone]
        assert new == []
        # All devices timed out
        assert gone == ["alice", "bob", "charlie", "delta", "echo"]
        break

    assert len(devices) == 0
    # All devices are gone, but suddenly a zombie raises from the dead!
    topo_mock = mocker.patch(TARGET)
    info = Hash("heartbeatInterval", 20, "type",
                "device", "serverId", "karabo_test_1")
    mixin._sigslot.request.return_value = info
    await mixin.updateHeartBeat("alice", info)
    _, kwargs = topo_mock.call_args
    new = kwargs["new"]
    gone = kwargs["gone"]
    assert gone == []
    new = [instanceId for (instanceId, _) in new]
    assert "alice" in new


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_spin_heartbeats_server(heartBeatMixinServer, mocker):
    """Spin the tracking until all devices are out due to heartbeats"""
    mixin = heartBeatMixinServer["mixin"]
    topology = mixin.systemTopology
    devices = topology["device"]
    assert len(devices) == 5
    topo_mock = mocker.patch(TARGET)
    mixin._sigslot = mocker.AsyncMock()

    for _ in range(10):
        await mixin._track_action()
        _, kwargs = topo_mock.call_args
        new = kwargs["new"]
        gone = kwargs["gone"]
        if not new and not gone:
            await asyncio.sleep(0.05)
            continue
        gone = [instanceId for (instanceId, _) in gone]
        assert new == []
        # All devices and servers timed out
        # Note: servers are last in the list
        assert gone == ["alice", "bob", "charlie", "delta", "echo",
                        "karabo_test_1", "karabo_test_2"]
        break

    assert len(devices) == 0
