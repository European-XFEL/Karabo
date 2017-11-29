from unittest.mock import Mock

from karabo.middlelayer import Configurable, Hash, String
from karabogui.testing import singletons, system_hash
from karabogui.topology.system import SystemTopology


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

            config = Hash('val', 'foo')
            device.set_project_config_hash(config)
            assert device._deferred_update

            topology.class_schema_updated('swerver', 'FooClass', schema)
            assert device._offline_proxy.binding.value.val.value == 'foo'

            config = Hash('val', 'bar')
            device.set_project_config_hash(config)
            assert device._offline_proxy.binding.value.val.value == 'bar'
            assert device.get_current_config_hash() == config
