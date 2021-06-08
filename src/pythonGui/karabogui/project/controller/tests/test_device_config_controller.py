from unittest import TestCase
from unittest.mock import Mock, patch

from karabo.common.enums import ProxyStatus
from karabo.common.project.device import DeviceInstanceModel
from karabo.common.project.device_config import DeviceConfigurationModel
from karabo.native import AccessLevel, Hash
from karabogui.project.controller.device import DeviceInstanceController
from karabogui.project.controller.device_config import (
    DeviceConfigurationController)
from karabogui.testing import singletons, system_hash
from karabogui.topology.system_topology import SystemTopology

IPATH = 'karabogui.project.controller.device.get_project_device_status_icon'


class TestDeviceConfigController(TestCase):

    @patch('karabogui.project.controller.device_config.find_parent_object')
    def test_configuration_schema(self, patch_func):
        network = Mock()
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(network=network, topology=topology):
            config = Hash()
            config['value'] = "a"
            config_model = DeviceConfigurationModel(
                class_id='BazClass',
                configuration=config)

            config_model.initialized = True

            config_controller = DeviceConfigurationController(
                model=config_model)

            config_list = [config_model]
            instance_model = DeviceInstanceModel(
                class_id='BazClass',
                instance_id='fooDevice',
                server_id='BazServer',
                configs=config_list)
            instance_model.initialized = True
            instance_model.active_config_ref = config_model.uuid

            with patch(IPATH) as icon_mock:
                icon_mock.return_value = None
                device_controller = DeviceInstanceController(
                    model=instance_model)

            device = device_controller.project_device
            # Click controller, but there is not schema/server available
            assert network.onGetClassSchema.call_count == 0
            assert device.proxy.status == ProxyStatus.NOSERVER
            assert not device.proxy.online

            # We did not request a schema because the server is not there
            patch_func.return_value = device_controller
            config_controller.single_click(None)
            assert network.onGetClassSchema.call_count == 0

            h = Hash()
            h['server.BazServer'] = None
            h['server.BazServer', ...] = {
                'host': 'BIG_IRON',
                'visibility': AccessLevel.OBSERVER,
                'deviceClasses': ['BazClass'],
                'visibilities': [AccessLevel.OBSERVER]}

            changes = Hash("new", h, "update", Hash(), "gone", Hash())
            topology.topology_update(changes)
            # Server and schema are available, we request schema when clicking
            assert device.proxy.status == ProxyStatus.OFFLINE
            assert network.onGetClassSchema.call_count == 0
            config_controller.single_click(None)
            assert network.onGetClassSchema.call_count == 1
