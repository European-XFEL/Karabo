from unittest.mock import Mock

from karabo.common.api import ProxyStatus, KARABO_WARN_HIGH
from karabo.native import Configurable, Hash, String
from karabogui.testing import singletons, system_hash
from karabogui.topology.system_topology import SystemTopology


class FooClass(Configurable):
    val = String()


def test_project_device():
    schema = FooClass.getClassSchema()
    network = Mock()
    with singletons(network=network):
        network = Mock()
        topology = SystemTopology()
        topology.update(system_hash())
        with singletons(network=network, topology=topology):
            device = topology.get_project_device('divvy',
                                                 server_id='swerver',
                                                 class_id='FooClass')
            # mocked network is not providing schema to the online proxy
            device._online_proxy.status = ProxyStatus.OFFLINE

            config = Hash('val', 'foo')
            device.set_project_config_hash(config)
            assert device._offline_config == config

            topology.class_schema_updated('swerver', 'FooClass', schema)
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

            device._online_proxy.status = ProxyStatus.ONLINE
            assert device.status is ProxyStatus.ONLINE
            device.error = True
            assert device.status is ProxyStatus.ERROR
