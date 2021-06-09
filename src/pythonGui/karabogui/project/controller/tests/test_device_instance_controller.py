from unittest import TestCase
from unittest.mock import Mock, patch

from karabo.common.project.device import DeviceInstanceModel
from karabo.common.project.device_config import DeviceConfigurationModel
from karabo.common.project.server import DeviceServerModel
from karabo.native import Hash
from karabogui.project.controller.device import DeviceInstanceController
from karabogui.project.controller.server import DeviceServerController
from karabogui.testing import singletons, system_hash
from karabogui.topology.system_topology import SystemTopology

D_ICON = 'karabogui.project.controller.device.get_project_device_status_icon'
S_ICON = 'karabogui.project.controller.server.get_project_server_status_icon'
PARENT_PATH = 'karabogui.project.controller.device.find_parent_object'


class TestDeviceInstanceController(TestCase):

    @patch(D_ICON)
    def test_empty_fields_configurator(self, patch_get_device_icon):
        patch_get_device_icon.return_value = None
        network = Mock()
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(network=network, topology=topology):
            foo_config = Hash()
            foo_config['test1'] = 'value1'
            foo_config['test2'] = 'value2'
            foo_config = DeviceConfigurationModel(class_id='BazClass',
                                                  configuration=foo_config)
            foo_config.initialized = True

            bar_config = Hash()
            bar_config['test1'] = 'diff_value1'
            bar_config = DeviceConfigurationModel(class_id='BazClass',
                                                  configuration=bar_config)
            bar_config.initialized = True

            config_list = [foo_config, bar_config]

            device_model = DeviceInstanceModel(class_id='BazClass',
                                               server_id='testServer',
                                               instance_id='fooDevice',
                                               configs=config_list)
            device_model.initialized = True
            device_model.active_config_ref = foo_config.uuid

            def assert_active_configuration():
                self.assertTrue('test1' in foo_config.configuration)
                self.assertEqual(foo_config.configuration['test1'], 'value1')

                self.assertTrue('test2' in foo_config.configuration)
                self.assertEqual(foo_config.configuration['test2'], 'value2')

                self.assertTrue('test1' in bar_config.configuration)
                self.assertEqual(bar_config.configuration['test1'],
                                 'diff_value1')

                self.assertFalse('test2' in bar_config.configuration)

            controller = DeviceInstanceController(model=device_model)
            controller.active_config_changed(bar_config)
            assert_active_configuration()

            controller.active_config_changed(foo_config)
            assert_active_configuration()

            controller.active_config_changed(bar_config)
            assert_active_configuration()

    @patch(D_ICON)
    @patch(S_ICON)
    def test_move_devices(self, patch_get_device_icon, patch_get_server_icon):
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
                                            instance_id='fooDevice',
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
                                            instance_id='fooDevice2',
                                            configs=[bar_config])
            bar_model.initialized = True
            bar_model.active_config_ref = bar_config.uuid

            # 3. Server
            server_model = DeviceServerModel(
                server_id='testServer', host='serverFoo',
                devices=[foo_model, bar_model])
            server_model.initialized = True

            # 4. Controllers
            foo_controller = DeviceInstanceController(model=foo_model)
            bar_controller = DeviceInstanceController(model=bar_model)
            server_controller = DeviceServerController(model=server_model)

            self.assertEqual(server_controller.model.devices[0], foo_model)
            self.assertEqual(server_controller.model.devices[1], bar_model)

            # 5. Fake project class for testing with a model member
            class Project:
                model = None

            project = Project()

            with patch(PARENT_PATH) as find_parent_mock:
                find_parent_mock.return_value = server_model
                foo_controller._move_down(project)

                self.assertEqual(server_controller.model.devices[0], bar_model)
                self.assertEqual(server_controller.model.devices[1], foo_model)

                foo_controller._move_up(project)

                self.assertEqual(server_controller.model.devices[0], foo_model)
                self.assertEqual(server_controller.model.devices[1], bar_model)

                bar_controller._move_up(project)

                self.assertEqual(server_controller.model.devices[0], bar_model)
                self.assertEqual(server_controller.model.devices[1], foo_model)

                # Lets try to crack it and move out of range (< 0)
                bar_controller._move_up(project)

                self.assertEqual(server_controller.model.devices[0], bar_model)
                self.assertEqual(server_controller.model.devices[1], foo_model)

                # Lets try to crack it and move out of range (> len)
                foo_controller._move_down(project)

                self.assertEqual(server_controller.model.devices[0], bar_model)
                self.assertEqual(server_controller.model.devices[1], foo_model)
