from karabogui.testing import GuiTestCase
from ..configuration import Configuration


class TestConfiguration(GuiTestCase):
    def test_configuration_namespace(self):
        config = Configuration()
        config['broker_topic'] = 'FXE'
        config['domain'] = 'CAS_INTERNAL'
        config['db_token'] = 'admin'
        config['macro_server'] = 'karabo/macroServer'

        self.assertEqual(len(config), 9)
        self.assertEqual(config['db_token'], 'admin')
        self.assertEqual(config['domain'], 'CAS_INTERNAL')
        self.assertEqual(config['macro_server'], 'karabo/macroServer')

        self.assertEqual(list(config.keys()),
                         ['broker_topic', 'config_dir', 'db_token', 'domain',
                          'guiServers', 'macro_dir', 'macro_server',
                          'scene_dir', 'username'])

        with self.assertRaises(KeyError):
            config['noitem'] = "Beep"
