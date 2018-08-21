from karabogui.project.dialog.project_handle import LoadProjectDialog
from karabogui.enums import KaraboSettings
from karabogui.testing import GuiTestCase
from karabogui.util import get_setting, set_setting
from karabogui.singletons.api import get_db_conn


class DialogTestCase(GuiTestCase):

    def setUp(self):
        super(DialogTestCase, self).setUp()

    def tearDown(self):
        super(DialogTestCase, self).tearDown()

    def test_set_default_group(self):
        # Use the first available domain, and use it as fake topic,
        # as we can't hard-code a topic, or this test will fail outside EuXFEL
        previous_group = LoadProjectDialog().default_domain

        domains = get_db_conn().get_available_domains()
        domains.remove(previous_group)
        domain = domains[0]

        set_setting(KaraboSettings.BROKER_TOPIC, domain)
        print(get_setting(KaraboSettings.BROKER_TOPIC))
        current_group = LoadProjectDialog().default_domain

        self.assertEqual(current_group, domain)
        self.assertNotEqual(current_group, previous_group)
