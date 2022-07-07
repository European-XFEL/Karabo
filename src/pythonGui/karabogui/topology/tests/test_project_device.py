from unittest.mock import Mock

from karabo.common.api import KARABO_WARN_HIGH, DeviceStatus
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
        device = topology.get_project_device('divvy',
                                             server_id='swerver',
                                             class_id='FooClass')
        # mocked network is not providing schema to the online proxy
        device._online_proxy.status = ProxyStatus.OFFLINE

        config = Hash('val', 'foo')
        device.set_project_config_hash(config)
        assert device._offline_config == config

        schema = FooClass.getClassSchema()
        topology.class_schema_updated('swerver', 'FooClass', schema)
        # We are lazy and did not request building although we have a
        # class in the topology
        assert len(device._offline_proxy.binding.value) == 0

        # Now remove the schema for test purposes
        topology._class_schemas.clear()
        # Refresh schema!
        topology.ensure_proxy_class_schema('divvy', 'swerver', 'FooClass')
        # The request is sent out but no schema is available
        network.onGetClassSchema.assert_called_with('swerver', 'FooClass')
        # Schema arrives and we have a proxy!
        topology.class_schema_updated('swerver', 'FooClass', schema)
        assert len(device._offline_proxy.binding.value) == 1
        assert device._offline_proxy.binding.value.val.value == 'foo'

        config = Hash('val', 'bar')
        config['val', KARABO_WARN_HIGH] = 42
        device.set_project_config_hash(config)
        assert device._offline_proxy.binding.value.val.value == 'bar'
        extracted_config = device.get_user_edited_config_hash()
        assert extracted_config['val', KARABO_WARN_HIGH] == 42

        device.start_monitoring()
        assert device._online_proxy._monitor_count == 1
        device.stop_monitoring()
        assert device._online_proxy._monitor_count == 0

        # Test instanceInfo update
        device._online_proxy.topology_node.status = DeviceStatus.ERROR
        assert device.instance_status is DeviceStatus.ERROR
