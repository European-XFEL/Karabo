from unittest import TestCase

from PyQt5.QtCore import Qt

from karabo.common.project.device import DeviceInstanceModel
from karabo.common.project.device_config import DeviceConfigurationModel
from karabo.common.project.server import DeviceServerModel
from karabo.native import Hash
from karabogui.project.controller.device import DeviceInstanceController
from karabogui.project.controller.device_config import (
    DeviceConfigurationController)

STATUS_ICON = 'karabogui.project.controller.' \
              'device.get_project_device_status_icon'


class TestProjectController(TestCase):
    def test_active_configuration(self):
        fconfig = Hash()
        fconfig['fkey'] = 'fvalue'
        fconfig['skey'] = 'svalue'
        fconfig = DeviceConfigurationModel(class_id='BazClass',
                                           configuration=fconfig)
        fconfig.initialized = True

        sconfig = Hash()
        sconfig['fkey'] = 'value_diff'
        sconfig = DeviceConfigurationModel(class_id='BazClass',
                                           configuration=sconfig)
        sconfig.initialized = True

        config_list = [fconfig, sconfig]

        model = DeviceInstanceModel(class_id='BazClass',
                                    instance_id='fooDevice',
                                    configs=config_list)
        model.initialized = True
        model.active_config_ref = fconfig.uuid
        DeviceServerModel(server_id='testServer', host='serverFoo',
                          devices=[model])

        def assert_active_configuration():
            self.assertTrue('fkey' in fconfig.configuration)
            self.assertEqual(fconfig.configuration['fkey'], 'fvalue')

            self.assertTrue('skey' in fconfig.configuration)
            self.assertEqual(fconfig.configuration['skey'], 'svalue')

            self.assertTrue('fkey' in sconfig.configuration)
            self.assertEqual(sconfig.configuration['fkey'], 'value_diff')

            self.assertFalse('skey' in sconfig.configuration)

        controller = DeviceInstanceController(model=model)
        controller.active_config_changed(sconfig)
        assert_active_configuration()

        controller.active_config_changed(fconfig)
        assert_active_configuration()

        controller.active_config_changed(sconfig)
        assert_active_configuration()

    def test_check_mark_configuration(self):
        config = Hash()
        config['fkey'] = 'value'
        model = DeviceConfigurationModel(class_id='BazClass',
                                         configuration=config)
        model.initialized = True
        controller = DeviceConfigurationController(model=model)
        self.assertEqual(controller.initial_check_state, Qt.Checked)
