import json
import uuid
from asyncio import gather, sleep, wait

import pytest

from karabo.middlelayer.testing import (
    create_device_server, create_instanceId, event_loop)
from karabo.middlelayer_api.broker.compat import amqp
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import (
    call, getClassSchema, getInstanceInfo, instantiateNoWait, waitUntil)
from karabo.native import Hash, KaraboError, Schema, Timestamp

SHUTDOWN_TIME = 1

FLAKY_MAX_RUNS = 3
FLAKY_MIN_PASSES = 1


class FaultyDevice(Device):

    async def onInitialization(self):
        raise RuntimeError("Not allowed to start")


@pytest.mark.flaky(max_runs=FLAKY_MAX_RUNS, min_passes=FLAKY_MIN_PASSES)
@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_device_server_no_plugins(event_loop: event_loop):
    serverId = f"testMDLServer-{uuid.uuid4()}"
    configuration = {"timeServerId": "KaraboTimeServer",
                     "scanPlugins": False,
                     "track": True}
    server = create_device_server(serverId, config=configuration)
    assert server is not None
    try:
        server.is_server = True
        await server.startInstance(broadcast=True)

        info = await getInstanceInfo(serverId)
        # Check the serverFlags, None set
        assert info["serverFlags"] == 0

        # And the time info
        th = server.slotGetTime(Hash())
        keys = list(th.keys())
        assert "reference" in keys
        ts_ref = Timestamp.fromHashAttributes(th["reference", ...])
        assert ts_ref.tid == 0

        assert "time" in keys
        assert "timeServerId" in keys
        assert th["timeServerId"] == "KaraboTimeServer"

        t = Timestamp()
        server.slotTimeTick(100, t.time_sec, t.time_frac, 200)
        th = server.slotGetTime(Hash())
        ts_ref = Timestamp.fromHashAttributes(th["reference", ...])
        assert ts_ref.tid == 100
        assert ts_ref.time_sec == t.time_sec
        assert ts_ref.time_frac == t.time_frac

        # Remove start ticks again
        server.slotTimeTick(0, t.time_sec, t.time_frac, 200)
    finally:
        await finalize_server(server)


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_device_server_instantiate_plugins(event_loop: event_loop):
    serverId = f"testMDLServer-{uuid.uuid4()}"

    configuration = {"deviceClasses": ["PropertyTestMDL"],
                     "timeServerId": "KaraboTimeServer",
                     "scanPlugins": False,
                     "serverFlags": ["Development"]}
    server = create_device_server(serverId, config=configuration)
    assert server is not None
    try:
        server.is_server = True
        await server.startInstance(broadcast=True)
        assert server.deviceClasses == ["PropertyTestMDL"]

        assert len(server.deviceInstanceMap.keys()) == 0
        # Test instantiate a device
        deviceId = "test-mdlfake-{}".format(uuid.uuid4())
        await server.startDevice("PropertyTestMDL", deviceId,
                                 Hash())
        assert len(server.deviceInstanceMap.keys()) == 1
        vis = server.getVisibilities()
        assert vis == {"PropertyTestMDL": 3}

        schema, classId, serv_id = server.slotGetClassSchema(
            "PropertyTestMDL")
        assert serv_id == serverId
        assert classId == "PropertyTestMDL"
        assert isinstance(schema, Schema)

        info = await getInstanceInfo(serverId)
        # Check the serverFlags
        assert info["serverFlags"] == 1

        # And the log level
        assert info["log"] == "INFO"
        assert server.log.level == "INFO"
        device = list(server.deviceInstanceMap.values())[0]
        assert device.log.level == "INFO"
        # Change log level
        server.slotLoggerPriority("ERROR")
        assert server.log.level == "ERROR"
        assert device.log.level == "ERROR"

        info = await getInstanceInfo(serverId)
        assert info["log"] == "ERROR"

        logs = await call(serverId, "slotLoggerContent", Hash())
        assert logs["serverId"] == serverId
        assert isinstance(logs["content"], list)
        logs = await call(serverId, "slotLoggerContent",
                          Hash("logs", 20))
        assert isinstance(logs["content"], list)

        schema = await getClassSchema(serverId, "PropertyTestMDL")
        assert schema is not None
        assert isinstance(schema, Schema)
    finally:
        await finalize_server(server)


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_device_server_autostart(event_loop: event_loop):
    deviceId_1 = "test-prop-{}".format(uuid.uuid4())
    deviceId_2 = "test-prop-{}".format(uuid.uuid4())
    serverId = f"testMDLServer-{uuid.uuid4()}"

    init = {deviceId_1: {"classId": "PropertyTestMDL"},
            deviceId_2: {"classId": "PropertyTestMDL"}}
    init = json.dumps(init)

    configuration = {"deviceClasses": ["PropertyTestMDL"],
                     "timeServerId": "KaraboTimeServer"}
    server = create_device_server(serverId, config=configuration)
    assert server is not None
    try:
        server.is_server = True
        server._device_initializer = json.loads(init)
        await server.startInstance(broadcast=True)
        # Wait until a device has been registered in this server
        await waitUntil(lambda: len(server.deviceInstanceMap) == 2)
        assert server.deviceClasses == ["PropertyTestMDL"]
        assert len(server.deviceInstanceMap.keys()) == 2
        assert deviceId_1 in server.deviceInstanceMap
        assert deviceId_2 in server.deviceInstanceMap
        vis = server.getVisibilities()
        assert vis == {"PropertyTestMDL": 3}
        device = server.deviceInstanceMap[deviceId_1]
        assert server == device.device_server
        local = device.getLocalDevice(deviceId_2)
        assert local is not None
        assert local.classId == "PropertyTestMDL"
        local = device.getLocalDevice("Notthere")
        assert local is None
    finally:
        await finalize_server(server)


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_device_server_start_faulty(event_loop: event_loop):
    try:
        serverId = create_instanceId()
        server = create_device_server(serverId,
                                      plugins=[FaultyDevice])
        await server.startInstance()
        await waitUntil(lambda: server.is_initialized)
        deviceId = create_instanceId()
        hsh = Hash(
            "classId", "FaultyDevice",
            "deviceId", deviceId,
            "configuration", Hash())

        # Succesful instantiation although broken device
        ok, *_ = await server.slotStartDevice(hsh)
        assert ok is True
        # Device is not there in server map
        assert deviceId not in server.deviceInstanceMap

        if not amqp:
            # TODO: Find a way to propagate exception
            with pytest.raises(KaraboError):
                fut = [server.slotStartDevice(hsh) for _ in range(2)]
                await gather(*fut)
    finally:
        await finalize_server(server)


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_device_server_concurrence(event_loop: event_loop):
    try:
        serverId = create_instanceId()
        configuration = {"deviceClasses": ["PropertyTestMDL"]}
        server = create_device_server(serverId, config=configuration)
        await server.startInstance()
        await waitUntil(lambda: server.is_initialized)
        deviceId = create_instanceId()
        assert len(server.deviceInstanceMap) == 0
        for _ in range(2):
            instantiateNoWait(serverId, "PropertyTestMDL", deviceId)
        await sleep(2)
        # Only a single device made it
        assert len(server.deviceInstanceMap) == 1
    finally:
        await finalize_server(server)


async def finalize_server(server):
    """Helper function to stop the device server

    Note: Stop the server with slotKillDevice, but not the eventloop
    Make sure all children devices are shutdown!
    """
    futures = [d.slotKillDevice()
               for d in server.deviceInstanceMap.values()]
    if futures:
        await wait(futures, timeout=10)
    await server.slotKillDevice()
    await sleep(SHUTDOWN_TIME)
