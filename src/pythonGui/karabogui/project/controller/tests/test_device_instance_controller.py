from unittest import TestCase
from unittest.mock import Mock, patch

from karabo.common.project.device import DeviceInstanceModel
from karabo.common.project.device_config import DeviceConfigurationModel
from karabo.common.project.server import DeviceServerModel
from karabo.native import Hash
from karabogui.project.controller.device import DeviceInstanceController
from karabogui.singletons.api import get_topology
from karabogui.testing import singletons, system_hash


class test_device_instance_controller(TestCase):

    @patch(
        'karabogui.project.controller.device.get_project_device_status_icon')
    def test_empty_fields_configurator(self, patch_get_icon):
        patch_get_icon.return_value = None
        network = Mock()
        topology = get_topology()
        topology.clear()
        topology.initialize(system_hash())
        with singletons(network=network, topology=topology):
            config1 = Hash()
            config1['test1'] = 'value1'
            config1['test2'] = 'value2'
            config1 = DeviceConfigurationModel(class_id='BazClass',
                                               configuration=config1)
            config1.initialized = True

            config2 = Hash()
            config2['test1'] = 'diff_value1'
            config2 = DeviceConfigurationModel(class_id='BazClass',
                                               configuration=config2)
            config2.initialized = True

            config_list = [config1, config2]

            model = DeviceInstanceModel(class_id='BazClass',
                                        instance_id='fooDevice',
                                        configs=config_list)
            model.initialized = True
            model.active_config_ref = config1.uuid
            DeviceServerModel(server_id='testServer', host='serverFoo',
                              devices=[model])

            def assert_active_configuration():
                self.assertTrue('test1' in config1.configuration)
                self.assertEqual(config1.configuration['test1'], 'value1')

                self.assertTrue('test2' in config1.configuration)
                self.assertEqual(config1.configuration['test2'], 'value2')

                self.assertTrue('test1' in config2.configuration)
                self.assertEqual(config2.configuration['test1'], 'diff_value1')

                self.assertFalse('test2' in config2.configuration)

            controller = DeviceInstanceController(model=model)
            controller.active_config_changed(config2)
            assert_active_configuration()

            controller.active_config_changed(config1)
            assert_active_configuration()

            controller.active_config_changed(config2)
            assert_active_configuration()
