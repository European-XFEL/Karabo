from unittest.mock import patch

from karabogui.testing import GuiTestCase
from ..configuration import (
    BACKBONE, Configuration, DIRECTORIES, NETWORK, PANEL, PROJECT, USER)


class MockSettings(object):

    @classmethod
    def value(cls, path):
        return getattr(cls, path, None)

    @classmethod
    def setValue(cls, name, value):
        setattr(cls, name, value)


class TestConfiguration(GuiTestCase):
    def test_configuration_namespace(self):
        config = Configuration()
        config['broker_topic'] = 'FXE'
        config['domain'] = 'CAS_INTERNAL'
        config['db_token'] = 'admin'

        self.assertEqual(len(config), 16)
        self.assertEqual(config['db_token'], 'admin')
        self.assertEqual(config['domain'], 'CAS_INTERNAL')

        self.assertEqual(list(config.keys()),
                         ['alarm_panel', 'alarm_service', 'broker_topic',
                          'config_dir', 'console_panel', 'daemon_manager',
                          'db_token', 'documentation', 'domain', 'gui_servers',
                          'log_panel', 'macro_dir', 'project_manager',
                          'scene_dir', 'username', 'wizard'])

    def test_set_wrong_key(self):
        config = Configuration()
        with self.assertRaises(KeyError):
            config['noitem'] = "Beep"

        with self.assertRaises(KeyError):
            config.noitem = "Boom"

    def test_set_bool_value(self):
        target = 'karabogui.singletons.configuration.QSettings'
        with patch(target, new=MockSettings):
            config = Configuration()
            self.assertEqual(config["wizard"], True)
            config["wizard"] = False
            config['db_token'] = "observer"
            self.assertEqual(config["wizard"], False)
            self.assertEqual(config["db_token"], "observer")

    def test_configuration_groups(self):
        config = Configuration()
        groups = config.groups()
        self.assertEqual(len(groups), 6)
        user_group = [item.name for item in groups[USER]]
        self.assertEqual(len(user_group), 1)
        network_group = [item.name for item in groups[NETWORK]]
        self.assertEqual(len(network_group), 2)
        self.assertIn('username', network_group)
        self.assertIn('gui_servers', network_group)
        project_group = [item.name for item in groups[PROJECT]]
        self.assertEqual(len(project_group), 2)
        self.assertIn('db_token', project_group)
        self.assertIn('domain', project_group)
        dir_group = [item.name for item in groups[DIRECTORIES]]
        self.assertEqual(len(dir_group), 3)
        self.assertIn('macro_dir', dir_group)
        self.assertIn('scene_dir', dir_group)
        self.assertIn('config_dir', dir_group)
        bone_group = [item.name for item in groups[BACKBONE]]
        self.assertEqual(len(bone_group), 5)
        self.assertIn('alarm_service', bone_group)
        self.assertIn('project_manager', bone_group)
        self.assertIn('broker_topic', bone_group)
        self.assertIn('documentation', bone_group)
        self.assertIn('daemon_manager', bone_group)
        panel_group = [item.name for item in groups[PANEL]]
        self.assertEqual(len(panel_group), 3)
        self.assertIn('alarm_panel', panel_group)
        self.assertIn('log_panel', panel_group)
        self.assertIn('console_panel', panel_group)
