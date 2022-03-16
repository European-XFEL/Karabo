import json
import uuid
from asyncio import get_event_loop, set_event_loop, sleep, wait
from contextlib import ExitStack, contextmanager
from unittest import TestCase, main, skipIf

from karabo.middlelayer_api.compat import amqp
from karabo.middlelayer_api.device_client import (
    call, getClassSchema, getInstanceInfo)
from karabo.middlelayer_api.device_server import DeviceServer
from karabo.middlelayer_api.eventloop import EventLoop
from karabo.middlelayer_api.signalslot import SignalSlotable
from karabo.middlelayer_api.tests.eventloop import async_tst
from karabo.native import Hash, Schema, Timestamp


class ServerTest(TestCase):

    @classmethod
    def setUpClass(cls):
        with ExitStack() as cls.exit_stack:
            cls.loop = EventLoop()
            cls.old_event_loop = get_event_loop()
            cls.exit_stack.enter_context(cls.createComm())
            set_event_loop(cls.loop)
            cls.exit_stack = cls.exit_stack.pop_all()

    @classmethod
    def tearDownClass(cls):
        try:
            with cls.exit_stack:
                pass
        finally:
            del cls.lead
            cls.loop.stop()
            cls.loop.close()
            set_event_loop(cls.old_event_loop)

    @classmethod
    @contextmanager
    def createComm(cls):

        loop = cls.loop
        sigslot = SignalSlotable(
            {"_deviceId_": "test-mdlserver-{}".format(uuid.uuid4())})
        sigslot.startInstance(loop=loop)
        cls.lead = sigslot

        loop.instance = lambda: sigslot
        yield
        loop.run_until_complete(sigslot.slotKillDevice())

    @async_tst
    async def test_device_server_noplugins_time(self):
        """Test that we can instantiate a server without plugins"""
        serverId = f"testMDLServer-{uuid.uuid4()}"
        configuration = {"serverId": serverId,
                         "timeServerId": "KaraboTimeServer",
                         "scanPlugins": False}
        server = DeviceServer(configuration)
        self.assertIsNotNone(server)
        try:
            server.is_server = True
            await server.startInstance(broadcast=True)
            th = server.slotGetTime(Hash())
            keys = list(th.keys())
            self.assertIn("reference", keys)
            ts_ref = Timestamp.fromHashAttributes(th["reference", ...])
            self.assertEqual(ts_ref.tid, 0)

            self.assertIn("time", keys)
            self.assertIn("timeServerId", keys)
            self.assertEqual(th["timeServerId"], "KaraboTimeServer")

            t = Timestamp()
            server.slotTimeTick(100, t.time_sec, t.time_frac, 200)
            th = server.slotGetTime(Hash())
            ts_ref = Timestamp.fromHashAttributes(th["reference", ...])
            self.assertEqual(ts_ref.tid, 100)
            self.assertEqual(ts_ref.time_sec, t.time_sec)
            self.assertEqual(ts_ref.time_frac, t.time_frac)

            # Remove start ticks again
            server.slotTimeTick(0, t.time_sec, t.time_frac, 200)

        finally:
            await self._shutdown_server(server)

    @skipIf(amqp, "fails for amqp")
    @async_tst
    async def test_device_server_plugin_device_logs(self):
        """Test to instantiate a server with plugins and instantiate"""
        serverId = f"testMDLServer-{uuid.uuid4()}"
        configuration = {"serverId": serverId,
                         "deviceClasses": ["PropertyTestMDL"],
                         "timeServerId": "KaraboTimeServer",
                         "scanPlugins": False}
        server = DeviceServer(configuration)
        self.assertIsNotNone(server)
        try:
            server.is_server = True
            await server.startInstance(broadcast=True)
            self.assertEqual(server.deviceClasses, ["PropertyTestMDL"])

            self.assertEqual(len(server.deviceInstanceMap.keys()), 0)
            # Test instantiate a device
            deviceId = "test-mdlfake-{}".format(uuid.uuid4())
            await server.startDevice("PropertyTestMDL", deviceId,
                                     Hash())
            self.assertEqual(len(server.deviceInstanceMap.keys()), 1)
            vis = server.getVisibilities()
            self.assertEqual(vis, {"PropertyTestMDL": 4})

            schema, classId, serv_id = server.slotGetClassSchema(
                "PropertyTestMDL")
            self.assertEqual(serv_id, serverId)
            self.assertEqual(classId, "PropertyTestMDL")
            self.assertIsInstance(schema, Schema)

            info = await getInstanceInfo(serverId)
            self.assertEqual(info["log"], "INFO")
            self.assertEqual(server.log.level, "INFO")
            device = list(server.deviceInstanceMap.values())[0]
            self.assertEqual(device.log.level, "INFO")
            # Change log level
            server.slotLoggerPriority("ERROR")
            self.assertEqual(server.log.level, "ERROR")
            self.assertEqual(device.log.level, "ERROR")

            info = await getInstanceInfo(serverId)
            self.assertEqual(info["log"], "ERROR")

            logs = await call(serverId, "slotLoggerContent", Hash())
            self.assertEqual(logs["serverId"], serverId)
            self.assertIsInstance(logs["content"], list)
            logs = await call(serverId, "slotLoggerContent", Hash("logs", 20))
            self.assertIsInstance(logs["content"], list)

            schema = await getClassSchema(serverId, "PropertyTestMDL")
            self.assertIsNotNone(schema)
            self.assertIsInstance(schema, Schema)
        finally:
            await self._shutdown_server(server)

    @async_tst
    async def test_device_server_autostart(self):
        """Test that we can autostart a server"""
        deviceId = "test-prop-{}".format(uuid.uuid4())
        serverId = f"testMDLServer-{uuid.uuid4()}"

        init = {deviceId: {"classId": "PropertyTestMDL"}}
        init = json.dumps(init)

        configuration = {"serverId": serverId,
                         "deviceClasses": ["PropertyTestMDL"],
                         "timeServerId": "KaraboTimeServer"}
        server = DeviceServer(configuration)
        self.assertIsNotNone(server)
        try:
            server.is_server = True
            server._device_initializer = json.loads(init)
            await server.startInstance(broadcast=True)

            # Wait until a device has been registered in this server
            for _ in range(20):
                if len(server.deviceInstanceMap.keys()):
                    break
                await sleep(0.2)
            self.assertEqual(server.deviceClasses, ["PropertyTestMDL"])
            self.assertEqual(len(server.deviceInstanceMap.keys()), 1)
            self.assertIn(deviceId, server.deviceInstanceMap)
            vis = server.getVisibilities()
            self.assertEqual(vis, {"PropertyTestMDL": 4})

            device = server.deviceInstanceMap[deviceId]
            self.assertEqual(server, device.device_server)
        finally:
            await self._shutdown_server(server)

    async def _shutdown_server(self, server):
        """Helper function to stop the device server

        Note: Stop the server with slotKillDevice, but not the eventloop
        Make sure all children devices are shutdown!
        """
        futures = [d.slotKillDevice()
                   for d in server.deviceInstanceMap.values()]
        if futures:
            await wait(futures, timeout=10)
        await server.slotKillDevice()


if __name__ == '__main__':
    main()
