import asyncio
from unittest import mock, main, TestCase

from karabo.native import Hash

from ..heartbeat_mixin import HeartBeatMixin


def run_coro(coro):
    loop = asyncio.get_event_loop()
    return loop.run_until_complete(coro)


TARGET = 'karabo.middlelayer_api.heartbeat_mixin.HeartBeatMixin._topology_changed'  # noqa


_SERVER_INSTANCES = Hash(
    "karabo_test_1", Hash("heartbeatInterval", 20, "type", "server"),
    "karabo_test_2", Hash("heartbeatInterval", 20, "type", "server"),
)

_DEVICE_INSTANCES = Hash(
    "alice", Hash("heartbeatInterval", 0.1, "type",
                  "device", "serverId", "karabo_test_1"),
    "bob", Hash("heartbeatInterval", 0.1, "type",
                "device", "serverId", "karabo_test_1"),
    "charlie", Hash("heartbeatInterval", 0.1, "type",
                    "device", "serverId", "karabo_test_1"),
    "delta", Hash("heartbeatInterval", 0.1, "type",
                  "device", "serverId", "karabo_test_1"),
    "echo", Hash("heartbeatInterval", 0.1, "type",
                 "device", "serverId", "karabo_test_1"),
)


class TestHeartBeats(TestCase):
    @classmethod
    def setUpClass(cls):
        asyncio.set_event_loop(asyncio.new_event_loop())

    def setUp(self):
        """The setup creates a mixin and fills it with servers and devices"""
        self.mixin = HeartBeatMixin()
        for server, info in _SERVER_INSTANCES.items():
            self.mixin.slotPingAnswer(server, info)
        for device, info in _DEVICE_INSTANCES.items():
            self.mixin.slotPingAnswer(device, info)

    def test_topology_init(self):
        topology = self.mixin.systemTopology
        self.assertEqual(topology["client"], Hash())
        self.assertEqual(topology["macro"], Hash())
        self.assertEqual(len(topology["device"]), len(_DEVICE_INSTANCES))
        self.assertEqual(len(topology["server"]), len(_SERVER_INSTANCES))

    def test_topology_add(self):
        """Test heartbeat mixin add instance"""
        topology = self.mixin.systemTopology
        self.assertEqual(topology["macro"], Hash())
        instanceId = "macro_1"
        info = Hash("type", "macro", "heartBeatInterval",
                    0.1, "serverId", "__none__")
        run_coro(self.mixin._add_instance(instanceId, info))
        self.assertEqual(len(topology["macro"]), 1)

    def test_topology_add_unknown(self):
        """Test heartbeat mixin add unknown instance"""
        topology = self.mixin.systemTopology
        instanceId = "outlawdevice"
        info = Hash("type", "notknown", "heartBeatInterval",
                    0.1, "serverId", "__none__")
        run_coro(self.mixin._add_instance(instanceId, info))
        self.assertEqual(len(topology["notknown"]), 1)

    def test_topology_server_already_online(self):
        """Test heartbeat mixin with server already online"""
        topology = self.mixin.systemTopology
        servers = topology["server"]
        self.assertEqual(len(servers), len(_SERVER_INSTANCES))
        self.assertGreater(len(topology["device"]), 0)

        self.assertIn("karabo_test_1", servers)
        with mock.patch(TARGET) as topo_mock:
            # Server goes online that has a few devices, all devices
            # are removed
            instanceId = "karabo_test_1"
            info = Hash("type", "server", "heartBeatInterval", 0.1)
            run_coro(self.mixin._add_instance(instanceId, info))
            self.assertEqual(len(topology["device"]), 0)
            assert topo_mock.called
            _, kwargs = topo_mock.call_args
            new = kwargs["new"]
            gone = kwargs["gone"]
            devices = [instanceId for (instanceId, _) in gone]
            self.assertEqual(new, [])
            self.assertEqual(
                devices, ['alice', 'bob', 'charlie', 'delta', 'echo'])

        with mock.patch(TARGET) as topo_mock:
            # A removal of non existing device does not crash
            for device, info in _DEVICE_INSTANCES.items():
                self.mixin._remove_instance(device, info)
            assert not topo_mock.called

        self.assertEqual(len(topology["device"]), 0)

        self.assertIn("karabo_test_2", servers)
        with mock.patch(TARGET) as topo_mock:
            # Server goes online that has NO CHILDREN
            instanceId = "karabo_test_2"
            info = Hash("type", "server", "heartBeatInterval", 0.1)
            run_coro(self.mixin._add_instance(instanceId, info))
            self.assertEqual(len(topology["device"]), 0)
            assert not topo_mock.called

    def test_topology_server_goes_offline(self):
        """Test heartbeat mixin with server going offline"""
        topology = self.mixin.systemTopology
        servers = topology["server"]
        self.assertEqual(len(servers), len(_SERVER_INSTANCES))
        self.assertGreater(len(topology["device"]), 0)

        self.assertIn("karabo_test_1", servers)
        instanceId = "karabo_test_1"
        info = Hash("type", "server", "heartBeatInterval", 0.1)
        # Server goes offline that has a few devices, all devices
        # are removed
        self.mixin._remove_instance(instanceId, info)
        self.assertEqual(len(topology["device"]), 0)
        self.assertEqual(
            len(topology["server"]), len(_SERVER_INSTANCES) - 1)
        self.assertNotIn("karabo_test_1", topology["server"])

    def test_device_goes_online(self):
        """Test heartbeat mixin with device going online"""
        topology = self.mixin.systemTopology
        instanceId = "eleanor"
        info = Hash("type", "device", "heartBeatInterval", 0.1)

        with mock.patch(TARGET) as topo_mock:
            self.assertEqual(len(topology["device"]), 5)
            run_coro(self.mixin._add_instance(instanceId, info))
            self.assertEqual(len(topology["device"]), 6)
            # No call done, we track beats
            assert not topo_mock.called

    def test_spin_heartbeats(self):
        """Spin the tracking until all devices are out due to heartbeats"""
        topology = self.mixin.systemTopology
        devices = topology["device"]
        self.assertEqual(len(devices), 5)
        with mock.patch(TARGET) as topo_mock:
            for _ in range(10):
                run_coro(self.mixin._track_action())
                _, kwargs = topo_mock.call_args
                new = kwargs["new"]
                gone = kwargs["gone"]
                if not new and not gone:
                    run_coro(asyncio.sleep(0.05))
                    continue
                gone = [instanceId for (instanceId, _) in gone]
                self.assertEqual(new, [])
                # All devices timed out
                self.assertEqual(
                    gone, ['alice', 'bob', 'charlie', 'delta', 'echo'])
                break

        self.assertEqual(len(devices), 0)
        # All devices are gone, but suddenly a zombie raises from the dead!
        with mock.patch(TARGET) as topo_mock:
            info = Hash("heartbeatInterval", 20, "type",
                        "device", "serverId", "karabo_test_1")
            run_coro(self.mixin.updateHeartBeat("alice", info))
            _, kwargs = topo_mock.call_args
            new = kwargs["new"]
            gone = kwargs["gone"]
            self.assertEqual(gone, [])
            new = [instanceId for (instanceId, _) in new]
            self.assertIn("alice", new)


if __name__ == "__main__":
    main()
