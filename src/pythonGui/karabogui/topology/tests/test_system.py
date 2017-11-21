from unittest.mock import Mock

from traits.api import push_exception_handler, pop_exception_handler

from karabo.common.api import DeviceStatus
from karabogui.testing import GuiTestCase, singletons
from karabogui.topology.system import SystemTopology
from .utils import system_hash


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


class TestSystemTopology(GuiTestCase):

    def test_get_class_simple(self):
        network = Mock()
        with singletons(network=network):
            topology = SystemTopology()
            topology.update(system_hash())

            klass = topology.get_class('swerver', 'divvy')

            assert klass.status is DeviceStatus.REQUESTED
            network.onGetClassSchema.assert_called_with('swerver', 'divvy')

    def test_get_device_simple(self):
        network = Mock()
        with singletons(network=network):
            topology = SystemTopology()
            topology.update(system_hash())
            klass = topology.get_class('swerver', 'divvy')

            assert klass.status is DeviceStatus.REQUESTED
            network.onGetClassSchema.assert_called_with('swerver', 'divvy')

            klass = topology.get_device('divvy')

            assert klass.status is DeviceStatus.REQUESTED
            network.onGetDeviceSchema.assert_called_with('divvy')

    def test_get_project_device_simple(self):
        network = Mock()
        topology = SystemTopology()
        topology.update(system_hash())
        with singletons(network=network, topology=topology):
            device_id = 'divvy'
            server_id = 'swerver'
            class_id = 'FooClass'

            device = topology.get_project_device(device_id, class_id=class_id,
                                                 server_id=server_id)

            assert not device.proxy.online
            assert device._online_proxy is topology.get_device(device_id)
            assert device._offline_proxy.server_id == server_id
            assert device._offline_proxy.binding.class_id == class_id
            network.onGetDeviceSchema.assert_called_with(device_id)
