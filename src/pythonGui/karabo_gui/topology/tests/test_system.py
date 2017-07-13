from unittest.mock import patch, Mock

from traits.api import push_exception_handler, pop_exception_handler

from karabo.common.api import DeviceStatus
from karabo_gui.testing import GuiTestCase
from karabo_gui.topology.system import SystemTopology
from .utils import system_hash


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


class TestSystemTopology(GuiTestCase):

    @patch('karabo_gui.topology.system.get_network')
    def test_get_class_simple(self, get_network):
        network = Mock()
        get_network.return_value = network

        topology = SystemTopology()

        klass = topology.get_class('server_id', 'class_id')

        assert klass.status is DeviceStatus.REQUESTED
        network.onGetClassSchema.assert_called_with('server_id', 'class_id')

    @patch('karabo_gui.topology.system.get_network')
    @patch('karabo_gui.configuration.get_topology')
    def test_get_device_simple(self, get_topology, get_network):
        network = Mock()
        get_network.return_value = network

        topology = SystemTopology()
        topology.update(system_hash())
        get_topology.return_value = topology

        klass = topology.get_device('divvy')

        assert klass.status is DeviceStatus.REQUESTED
        network.onGetDeviceSchema.assert_called_with('divvy')

    @patch('karabo_gui.topology.system.get_network')
    @patch('karabo_gui.topology.project_device.get_topology')
    def test_get_project_device_simple(self, get_topology, get_network):
        network = Mock()
        get_network.return_value = network

        topology = SystemTopology()
        topology.update(system_hash())
        get_topology.return_value = topology

        device_id = 'divvy'
        server_id = 'swerver'
        class_id = 'FooClass'
        device = topology.get_project_device(device_id, class_id=class_id,
                                             server_id=server_id)

        assert not device.online
        assert device.status is DeviceStatus.OFFLINE
        assert device._online_dev_config is topology.get_device(device_id)
        assert device._class_config is topology.get_class(server_id, class_id)
        network.onGetClassSchema.assert_called_with(server_id, class_id)

        # Exercise the cleanup code
        device.destroy()
