from unittest.mock import Mock

from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.api import ProxyStatus
from karabogui.testing import GuiTestCase, singletons, system_hash
from karabogui.topology.system_topology import SystemTopology


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

            # server with name 'swerver' don't have the requested class
            assert klass.status is ProxyStatus.NOPLUGIN
            # GUI's system topology won't request for schema if the devic
            # class is not in the deviceClasses list of the server, this
            # solves the race condition in MDL
            assert network.onGetClassSchema.call_count == 0

    def test_get_device_simple(self):
        network = Mock()
        with singletons(network=network):
            topology = SystemTopology()
            topology.update(system_hash())
            klass = topology.get_class('swerver', 'divvy')

            assert klass.status is ProxyStatus.NOPLUGIN
            assert network.onGetClassSchema.call_count == 0

            dev = topology.get_device('divvy')

            assert dev.status is ProxyStatus.ONLINEREQUESTED
            network.onGetDeviceSchema.assert_called_with('divvy')

    def test_get_project_device_simple(self):
        network = Mock()
        topology = SystemTopology()
        topology.update(system_hash())
        with singletons(network=network, topology=topology):
            device_id = 'divvy'
            server_id = 'swerver'
            class_id = 'FooClass'

            device = topology.get_project_device(device_id,
                                                 server_id=server_id,
                                                 class_id=class_id,)
            # Mocked network is not providing online proxy its schema
            device._online_proxy.status = ProxyStatus.OFFLINE
            assert device._online_proxy is topology.get_device(device_id)
            project_proxy = topology.get_project_device_proxy(
                device_id, server_id, class_id)
            assert device._offline_proxy is project_proxy
            assert device._offline_proxy.device_id == device_id
            assert device._offline_proxy.binding.class_id == class_id
            network.onGetDeviceSchema.assert_called_with(device_id)

            device.rename(device_id='davey')
            key = (server_id, class_id)
            assert device_id not in topology._project_device_proxies[key]
            assert 'davey' in topology._project_device_proxies[key]

            network.reset_mock()
            device.rename(device_id='junk', server_id='notthere',
                          class_id='NotValieEither')
            assert network.onGetDeviceSchema.call_count == 0

            topology.remove_project_device_proxy(device_id='junk',
                                                 server_id='notthere',
                                                 class_id='NotValieEither')
            assert len(topology._project_device_proxies) == 0
