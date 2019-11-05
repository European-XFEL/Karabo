from karabogui.testing import GuiTestCase
from ..configuration import (
    BACKBONE, Configuration, DIRECTORIES, NETWORK, PROJECT)


class TestConfiguration(GuiTestCase):
    def test_configuration_namespace(self):
        config = Configuration()
        config['broker_topic'] = 'FXE'
        config['domain'] = 'CAS_INTERNAL'
        config['db_token'] = 'admin'
        config['macro_server'] = 'karabo/macroServer'

        self.assertEqual(len(config), 10)
        self.assertEqual(config['db_token'], 'admin')
        self.assertEqual(config['domain'], 'CAS_INTERNAL')
        self.assertEqual(config['macro_server'], 'karabo/macroServer')

        self.assertEqual(list(config.keys()),
                         ['broker_topic', 'config_dir', 'db_token', 'domain',
                          'gui_servers', 'macro_dir', 'macro_server',
                          'project_manager', 'scene_dir', 'username'])

    def test_set_wrong_key(self):
        config = Configuration()
        with self.assertRaises(KeyError):
            config['noitem'] = "Beep"

        with self.assertRaises(KeyError):
            config.noitem = "Boom"

    def test_configuration_groups(self):
        config = Configuration()
        groups = config.groups()
        self.assertEqual(len(groups), 4)
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
        self.assertEqual(len(bone_group), 3)
        self.assertIn('macro_server', bone_group)
        self.assertIn('project_manager', bone_group)
        self.assertIn('broker_topic', bone_group)
