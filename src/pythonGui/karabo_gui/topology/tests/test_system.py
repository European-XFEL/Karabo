from unittest.mock import Mock

from traits.api import push_exception_handler, pop_exception_handler

from karabo.common.api import DeviceStatus
from karabo_gui.testing import GuiTestCase, singletons
from karabo_gui.topology.system import SystemTopology
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

            klass = topology.get_class('server_id', 'class_id')

            assert klass.status is DeviceStatus.REQUESTED
            network.onGetClassSchema.assert_called_with(
                'server_id', 'class_id')

    def test_get_device_simple(self):
        network = Mock()
        topology = SystemTopology()
        topology.update(system_hash())
        with singletons(network=network, topology=topology):
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

            assert device.online
            assert device.status is DeviceStatus.REQUESTED

            topo_class = topology.get_class(server_id, class_id)
            topo_dev = topology.get_device(device_id)
            assert device._class_config is topo_class
            assert device._online_dev_config is topo_dev

            network.onGetClassSchema.assert_called_with(server_id, class_id)

            # Exercise the cleanup code
            device.destroy()
