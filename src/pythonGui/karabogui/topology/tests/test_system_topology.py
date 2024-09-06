# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from unittest.mock import Mock

from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.api import InstanceStatus
from karabo.native import Configurable, Hash, String
from karabogui.binding.api import ProxyStatus
from karabogui.testing import (
    GuiTestCase, get_device_schema, singletons, system_hash)
from karabogui.topology.system_topology import SystemTopology
from karabogui.topology.util import (
    get_macro_servers, getTopology, is_device_online, is_server_online)


class FooClass(Configurable):
    val = String()


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


class TestSystemTopology(GuiTestCase):

    def test_cleanup_topology(self):
        topology = SystemTopology()
        topology.initialize(system_hash())
        assert topology.online
        topo = getTopology()
        assert isinstance(topo, Hash)
        topology.clear()
        assert not topology.online
        assert isinstance(topo, Hash)

    def test_get_class_simple(self):
        network = Mock()
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(network=network, topology=topology):
            klass = topology.get_class("swerver", "NoClass")
            # server with name "swerver" don"t have the requested class
            assert klass.status is ProxyStatus.NOPLUGIN
            # GUI"s system topology won"t request for schema if the devic
            # class is not in the deviceClasses list of the server, this
            # solves the race condition in MDL
            assert network.onGetClassSchema.call_count == 0

            # Test a class that is available, request multiple times, but
            # only a single schema is requested
            foo_klass = topology.get_class("swerver", "FooClass")
            assert foo_klass.status == ProxyStatus.REQUESTED
            assert network.onGetClassSchema.call_count == 1
            more_foo_klass = topology.get_class("swerver", "FooClass")
            assert more_foo_klass.status == ProxyStatus.REQUESTED
            assert network.onGetClassSchema.call_count == 1

            # Make sure we can request when server leaves topology
            assert "FooClass" in topology._requested_classes["swerver"]
            topology.instance_gone(Hash("server", Hash("swerver", "")))
            assert "swerver" not in topology._requested_classes

    def test_get_device_simple(self):
        network = Mock()
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(network=network, topology=topology):
            klass = topology.get_class("swerver", "divvy")

            assert klass.status is ProxyStatus.NOPLUGIN
            assert network.onGetClassSchema.call_count == 0

            dev = topology.get_device("divvy")

            assert dev.status is ProxyStatus.ONLINEREQUESTED
            network.onGetDeviceSchema.assert_called_with("divvy")
            assert not len(dev.binding.value)
            topology.device_schema_updated("divvy", get_device_schema())
            assert len(dev.binding.value)

    def test_get_project_device_simple(self):
        network = Mock()
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(network=network, topology=topology):
            device_id = "divvy"
            server_id = "swerver"
            class_id = "FooClass"

            device = topology.get_project_device(device_id,
                                                 server_id=server_id,
                                                 class_id=class_id, )
            # Mocked network is not providing online proxy its schema
            device._online_proxy.status = ProxyStatus.OFFLINE
            assert device._online_proxy is topology.get_device(device_id)
            project_proxy = topology.get_project_device_proxy(
                device_id, server_id, class_id)
            assert device._offline_proxy is project_proxy
            assert device._offline_proxy.device_id == device_id
            assert device._offline_proxy.binding.class_id == class_id
            # No schema is requested for the online device
            assert network.onGetDeviceSchema.call_count == 0

            device.rename(device_id="davey")
            key = (server_id, class_id)
            assert device_id not in topology._project_device_proxies[key]
            assert "davey" in topology._project_device_proxies[key]

            network.reset_mock()
            device.rename(device_id="junk", server_id="notthere",
                          class_id="NotValieEither")
            assert network.onGetDeviceSchema.call_count == 0

            assert len(topology._project_device_proxies) == 1
            topology.remove_project_device_proxy(device_id="junk",
                                                 server_id="notthere",
                                                 class_id="NotValieEither")
            assert len(topology._project_device_proxies) == 0

    def test_system_topology_gone(self):
        network = Mock()
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(network=network, topology=topology):
            # 1. Plainly test a faulty changes Hash
            assert topology.get_attributes("device.divvy") is not None
            changes = Hash("new", Hash(), "update", Hash(), "gone", Hash())
            topology.topology_update(changes)
            assert topology.get_attributes("device.divvy") is not None

            # 2. Instance gone device
            dev = topology.get_device("divvy")
            assert dev.status is not ProxyStatus.OFFLINE
            gone_hash = Hash("device", Hash("divvy", None))
            changes = Hash("new", Hash(), "update", Hash(), "gone", gone_hash)
            topology.topology_update(changes)
            assert topology.get_attributes("device.divvy") is None
            # 2.1 A second time to be sure
            topology.topology_update(changes)
            assert topology.get_attributes("device.divvy") is None
            assert dev.status is ProxyStatus.OFFLINE

            # 3. instance gone macro
            assert topology.get_attributes("macro.macdonald") is not None
            gone_hash = Hash("macro", Hash("macdonald", None))
            changes = Hash("new", Hash(), "update", Hash(), "gone", gone_hash)
            topology.topology_update(changes)
            assert topology.get_attributes("macro.macdonald") is None
            # 3.1 A second time to be sure
            topology.topology_update(changes)
            assert topology.get_attributes("macro.macdonald") is None

            # 4. instance gone server
            schema = FooClass.getClassSchema()
            topology.class_schema_updated("swerver", "FooClass", schema)
            schema = topology.get_schema("swerver", "FooClass")
            assert schema is not None
            assert topology.get_attributes("server.swerver") is not None
            gone_hash = Hash("server", Hash("swerver", None))
            changes = Hash("new", Hash(), "update", Hash(), "gone", gone_hash)
            topology.topology_update(changes)
            assert topology.get_attributes("server.swerver") is None
            # 3.1 A second time to be sure
            topology.topology_update(changes)
            assert topology.get_attributes("server.swerver") is None
            assert "swerver" in topology._class_schemas
            # All class schemas are erased
            classes = topology._class_schemas["swerver"]
            assert not len(classes)

            # 5. Instance gone client
            assert topology.get_attributes("client.charlie") is not None
            gone_hash = Hash("client", Hash("charlie", None))
            changes = Hash("new", Hash(), "update", Hash(), "gone", gone_hash)
            topology.topology_update(changes)
            assert topology.get_attributes("client.charlie") is None
            # 4.1 A second time, stress
            topology.topology_update(changes)
            assert topology.get_attributes("client.charlie") is None

    def test_system_topology_update_instance(self):
        network = Mock()
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(network=network, topology=topology):
            attrs = topology.get_attributes("device.divvy")
            assert attrs["status"] != "error"

            h = Hash()
            h["device.divvy"] = None
            h["device.divvy", ...] = {
                "host": "BIG_IRON",
                "serverId": "swerver",
                "classId": "FooClass",
                "status": "error",
                "capabilities": 0,
            }

            dev = topology.get_device("divvy")

            assert dev.status is ProxyStatus.ONLINEREQUESTED
            network.onGetDeviceSchema.assert_called_with("divvy")
            assert dev.topology_node.status is not InstanceStatus.ERROR

            # Instance update device
            changes = Hash("new", Hash(), "update", h, "gone", Hash())
            topology.topology_update(changes)
            attrs = topology.get_attributes("device.divvy")
            assert attrs["status"] == "error"
            assert dev.topology_node.status is InstanceStatus.ERROR

            # A second time to be sure
            h["device.divvy", "status"] = "ok"
            changes = Hash("new", Hash(), "update", h, "gone", Hash())
            topology.topology_update(changes)
            attrs = topology.get_attributes("device.divvy")
            assert attrs["status"] == "ok"
            assert dev.topology_node.status is InstanceStatus.OK

    def test_visit_system_topology(self):
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(topology=topology):
            devices = []

            def device_visitor(node):
                nonlocal devices
                # Check for devices, level 3
                if node.level == 3:
                    devices.append(node)

            topology.visit_system_tree(device_visitor)
            assert len(devices) == 2
            assert devices[0].node_id == "divvy"
            assert devices[1].node_id == "macdonald"

            servers = []

            def server_visitor(node):
                nonlocal servers
                # Check for servers, level 1
                if node.level == 1:
                    servers.append(node)

            topology.visit_system_tree(server_visitor)
            assert len(servers) == 1
            assert servers[0].node_id == "swerver"

            # Visit the device tree
            # The system tree is a bit more strict and validates the Karaob
            # naming convention. Hence, we do not expect any devices

            members = []

            def member_visitor(node):
                nonlocal members
                # Check for member, level 2
                if node.level == 2:
                    members.append(node)

            topology.visit_device_tree(member_visitor)
            assert len(members) == 0

            h = Hash()
            h["device.EG/RR/VIEW"] = None
            h["device.EG/RR/VIEW", ...] = {
                "host": "BIG_IRON",
                "serverId": "swerver",
                "classId": "FooClass",
                "status": "ok",
                "visibility": 4,
                "capabilities": 0,
                "interfaces": 0,
            }

            changes = Hash("new", h, "update", Hash(), "gone", Hash())
            topology.topology_update(changes)

            topology.visit_device_tree(member_visitor)
            assert len(members) == 1
            assert members[0].node_id == "EG/RR/VIEW"

            assert is_device_online("EG/RR/VIEW")

    def test_topology_utils(self):
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(topology=topology):
            assert is_server_online("swerver")
            assert not is_server_online("cppserver/notthere")
            assert not get_macro_servers()

            h = Hash()
            h["server.karabo/macroServer"] = None
            h["server.karabo/macroServer", ...] = {
                "host": "BIG_IRON",
                "deviceClasses": ["Macro", "MetaMacro"],
                "lang": "macro",
                "type": "server",
                "serverId": "karabo/macroServer",
                "heartbeatInterval": 20,
                "karaboVersion": "2.13.0",
                "visibility": 4,
                "log": "INFO",
                "visibilities": [4, 4],
            }

            # adding an example of a macro started by a python server
            h["server.karabo/middlelayer"] = None
            h["server.karabo/middlelayer", ...] = {
                "host": "BIG_IRON",
                "lang": "python",
                "type": "server",
                "serverId": "karabo/middlelayer",
                "heartbeatInterval": 20,
                "karaboVersion": "2.13.0",
                "visibility": 4,
                "log": "INFO",
                "visibilities": [4, 4],
            }
            h["device.rogue_macro"] = None
            h["device.rogue_macro", ...] = {
                "host": "BIG_IRON",
                "classId": "RogueClass",
                "type": "macro",
                "serverId": "karabo/middlelayer",
                "heartbeatInterval": 120,
                "karaboVersion": "2.13.0",
                "visibility": 4,
                "status": "ok",
                "capabilities": 0,
            }

            changes = Hash("new", h, "update", Hash(), "gone", Hash())
            topology.topology_update(changes)
            servers = get_macro_servers()
            assert len(servers) == 1
            assert servers[0] == "karabo/macroServer"

            h = Hash()
            h["server.karabo/macroServer1"] = None
            h["server.karabo/macroServer1", ...] = {
                "host": "BIG_IRON",
                "deviceClasses": ["Macro", "MetaMacro"],
                "lang": "macro",
                "type": "server",
                "serverId": "karabo/macroServer1",
                "heartbeatInterval": 20,
                "karaboVersion": "2.13.0",
                "visibility": 4,
                "log": "INFO",
                "serverFlags": 1,
                "visibilities": [4, 4],
            }
            changes = Hash("new", h, "update", Hash(), "gone", Hash())
            topology.topology_update(changes)

            servers = get_macro_servers(False)
            assert len(servers) == 1
            servers = get_macro_servers(True)
            assert len(servers) == 2
            assert servers[1] == "karabo/macroServer1"

            # And one more macro server
            h = Hash()
            h["server.karabo/macroServer2"] = None
            h["server.karabo/macroServer2", ...] = {
                "host": "BIG_IRON",
                "deviceClasses": ["Macro", "MetaMacro"],
                "lang": "macro",
                "type": "server",
                "serverId": "karabo/macroServer2",
                "heartbeatInterval": 20,
                "karaboVersion": "2.20.0",
                "visibility": 4,
                "log": "INFO",
                "visibilities": [4, 4],
            }
            changes = Hash("new", h, "update", Hash(), "gone", Hash())
            topology.topology_update(changes)
            servers = get_macro_servers(False)
            assert len(servers) == 2
            # Least loaded
            assert servers[0] == "karabo/macroServer"

            # add one more macro
            h["macro.macromotor"] = None
            h["macro.macromotor", ...] = {
                "host": "BIG_IRON",
                "classId": "RogueClass",
                "type": "macro",
                "serverId": "karabo/macroServer",
                "heartbeatInterval": 120,
                "karaboVersion": "2.13.0",
                "visibility": 4,
                "status": "ok",
                "capabilities": 0,
            }
            changes = Hash("new", h, "update", Hash(), "gone", Hash())
            topology.topology_update(changes)
            servers = get_macro_servers(False)
            assert len(servers) == 2
            # Least loaded changed
            assert servers[0] == "karabo/macroServer2"
