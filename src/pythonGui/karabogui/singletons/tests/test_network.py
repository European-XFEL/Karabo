from karabogui.testing import GuiTestCase

from ..network import Network


class TestNetwork(GuiTestCase):
    def test_member_variables(self):
        network = Network()
        self.assertEqual(network.hostname, 'localhost')
        self.assertEqual(network.username, 'operator')
        self.assertEqual(network.password, 'karabo')
        self.assertEqual(network.provider, 'LOCAL')
        self.assertEqual(network.port, 44444)
        self.assertEqual(network.max_servers, 5)
