import json
import uuid
from asyncio import gather, sleep, wait

import pytest

from karabo.middlelayer.testing import (
    AsyncDeviceContext, create_device_server, event_loop)
from karabo.middlelayer_api.broker import amqp
from karabo.middlelayer_api.device import Device
from karabo.middlelayer_api.device_client import (
    call, getClassSchema, getInstanceInfo, instantiateNoWait, waitUntil)
from karabo.middlelayer_api.tests.eventloop import create_instanceId
from karabo.native import Hash, KaraboError, Schema, Timestamp


class FaultyDevice(Device):

    async def onInitialization(self):
        raise RuntimeError("Not allowed to start")


@pytest.mark.asyncio
@pytest.mark.timeout(90)
@pytest.mark.skipif(amqp, reason="Not working on amqp")
async def test_device_server(event_loop: event_loop, subtests):
    async with AsyncDeviceContext():
        with subtests.test("Test that we can instantiate a server without "
                           "plugins"):
            serverId = f"testMDLServer-{uuid.uuid4()}"
            configuration = {"timeServerId": "KaraboTimeServer",
                             "scanPlugins": False}
            server = create_device_server(serverId, config=configuration)
            assert server is not None
            try:
                server.is_server = True
                await server.startInstance(broadcast=True)
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

        with subtests.test("Test to instantiate a server with "
                           "plugins and instantiate"):
            serverId = f"testMDLServer-{uuid.uuid4()}"

            configuration = {"deviceClasses": ["PropertyTestMDL"],
                             "timeServerId": "KaraboTimeServer",
                             "scanPlugins": False}
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

        with subtests.test("Test that we can autostart a server"):
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

        with subtests.test("Test that we start a faulty "
                           "device via the server"):
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
                with pytest.raises(KaraboError):
                    fut = [server.slotStartDevice(hsh) for _ in range(2)]
                    await gather(*fut)
            finally:
                await finalize_server(server)

        with subtests.test("Test concurrence in the server"):
            try:
                serverId = create_instanceId()
                configuration = {"deviceClasses": ["PropertyTestMDL"]}
                server = create_device_server(serverId, config=configuration)
                await server.startInstance()
                await waitUntil(lambda: server.is_initialized)
                deviceId = create_instanceId()
                hsh = Hash(
                    "classId", "FaultyDevice",
                    "deviceId", deviceId,
                    "configuration", Hash())

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
