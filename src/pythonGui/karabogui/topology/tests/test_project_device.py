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

from karabo.common.api import InstanceStatus
from karabo.native import Configurable, Hash, String
from karabogui.binding.api import ProxyStatus
from karabogui.singletons.mediator import Mediator
from karabogui.testing import singletons, system_hash
from karabogui.topology.system_topology import SystemTopology


class FooClass(Configurable):
    val = String()


def test_project_device():
    network = Mock()
    mediator = Mediator()
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(network=network, topology=topology, mediator=mediator):
        device = topology.get_project_device("divvy",
                                             server_id="swerver",
                                             class_id="FooClass")
        offline_proxy = device._offline_proxy
        online_proxy = device._online_proxy

        # mocked network is not providing schema to the online proxy
        online_proxy.status = ProxyStatus.OFFLINE
        config = Hash("val", "foo")
        device.set_project_config_hash(config)
        assert device._offline_config == config

        schema = FooClass.getClassSchema()
        topology.class_schema_updated("swerver", "FooClass", schema)
        # We are lazy and did not request building although we have a
        # class in the topology

        assert len(offline_proxy.binding.value) == 0

        # Now remove the schema for test purposes
        topology._class_schemas.clear()
        # Refresh schema!
        topology.ensure_proxy_class_schema("divvy", "swerver", "FooClass")
        # The request is sent out but no schema is available
        network.onGetClassSchema.assert_called_with("swerver", "FooClass")
        # Schema arrives and we have a proxy!
        topology.class_schema_updated("swerver", "FooClass", schema)
        assert len(offline_proxy.binding.value) == 1
        assert offline_proxy.binding.value.val.value == "foo"

        config = Hash("val", "bar")
        device.set_project_config_hash(config)
        assert offline_proxy.binding.value.val.value == "bar"

        assert repr(offline_proxy) == "<ProjectDeviceProxy deviceId=divvy>"
        device.start_monitoring()
        assert online_proxy._monitor_count == 1
        device.stop_monitoring()
        assert online_proxy._monitor_count == 0

        # Test instanceInfo update
        online_proxy.topology_node.status = InstanceStatus.ERROR
        assert device.instance_status is InstanceStatus.ERROR
