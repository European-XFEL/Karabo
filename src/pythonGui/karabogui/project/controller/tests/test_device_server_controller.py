from unittest import TestCase
from unittest.mock import Mock, patch

from karabo.common.project.device import DeviceInstanceModel
from karabo.common.project.device_config import DeviceConfigurationModel
from karabo.common.project.server import DeviceServerModel
from karabo.native import Hash
from karabogui.project.controller.server import DeviceServerController
from karabogui.testing import singletons, system_hash
from karabogui.topology.system_topology import SystemTopology

D_ICON = 'karabogui.project.controller.device.get_project_device_status_icon'
S_ICON = 'karabogui.project.controller.server.get_project_server_status_icon'


class TestServerController(TestCase):

    @patch(D_ICON)
    @patch(S_ICON)
    def test_sort_devices(self, patch_get_device_icon, patch_get_server_icon):
        patch_get_device_icon.return_value = None
        patch_get_server_icon.return_value = None
        network = Mock()
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(network=network, topology=topology):
            # 1. Foo device with config
            foo_config = Hash()
            foo_config['value'] = '1'
            foo_config = DeviceConfigurationModel(class_id='BazClass',
                                                  configuration=foo_config)
            foo_config.initialized = True
            foo_model = DeviceInstanceModel(class_id='BazClass',
                                            instance_id='B/B/fooDevice1',
                                            server_id='testServer',
                                            configs=[foo_config])
            foo_model.initialized = True
            foo_model.active_config_ref = foo_config.uuid

            # 2. Bar device with config
            bar_config = Hash()
            bar_config['value'] = '2'
            bar_config = DeviceConfigurationModel(class_id='BazClass',
                                                  configuration=bar_config)
            bar_config.initialized = True
            bar_model = DeviceInstanceModel(class_id='BazClass',
                                            server_id='testServer',
                                            instance_id='A/A/fooDevice2',
                                            configs=[bar_config])
            bar_model.initialized = True
            bar_model.active_config_ref = bar_config.uuid

            # 3. Server
            server_model = DeviceServerModel(
                server_id='testServer', host='serverFoo',
                devices=[foo_model, bar_model])
            server_model.initialized = True

            # 4. Controllers
            server_controller = DeviceServerController(model=server_model)

            self.assertEqual(server_controller.model.devices[0], foo_model)
            self.assertEqual(server_controller.model.devices[1], bar_model)

            server_controller._sort_alphabetically(None)

            # bar starts with A, foo with B
            self.assertEqual(server_controller.model.devices[0], bar_model)
            self.assertEqual(server_controller.model.devices[1], foo_model)

            server_controller._sort_devices_naming(2, None)

            # Sort by member, foo has 1, bar has 2
            self.assertEqual(server_controller.model.devices[0], foo_model)
            self.assertEqual(server_controller.model.devices[1], bar_model)

            server_controller._sort_devices_naming(1, None)

            # Sort by type, foo has B, bar has A
            self.assertEqual(server_controller.model.devices[0], bar_model)
            self.assertEqual(server_controller.model.devices[1], foo_model)

            server_controller._sort_devices_naming(0, None)

            # Sort by domain, but here nothing happens! bar starts with A
            # and foo with B
            self.assertEqual(server_controller.model.devices[0], bar_model)
            self.assertEqual(server_controller.model.devices[1], foo_model)

            # sort by member again foo has 1, bar has 2
            server_controller._sort_devices_naming(2, None)
            self.assertEqual(server_controller.model.devices[0], foo_model)
            self.assertEqual(server_controller.model.devices[1], bar_model)

            # Then sort by domain, changes are happening.
            server_controller._sort_devices_naming(0, None)
            self.assertEqual(server_controller.model.devices[0], bar_model)
            self.assertEqual(server_controller.model.devices[1], foo_model)
