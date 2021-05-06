from karabo.common.scenemodel.api import DaemonManagerModel
from karabo.native import Configurable, Hash, String, VectorHash
from karabogui.binding.config import apply_configuration
from karabogui.testing import GuiTestCase, get_class_property_proxy

from ..daemon import DisplayDaemonService


class Row(Configurable):
    name = String(defaultValue="")
    status = String(defaultValue="")
    since = String(defaultValue="")
    duration = String(defaultValue="0.0")
    host = String(defaultValue="")


class Object(Configurable):
    prop = VectorHash(displayType="DaemonManager",
                      rows=Row)


class TestDaemonManager(GuiTestCase):
    def setUp(self):
        super(TestDaemonManager, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
        self.model = DaemonManagerModel()
        self.controller = DisplayDaemonService(proxy=self.proxy,
                                               model=self.model)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        daemon_hash = Hash("prop",
                           [Hash("name", "KaraboDB", "status", "up, running",
                                 "since", "Tue, 05 Nov 2019 22:51:08",
                                 "duration", "22.0", "host", "exflhost")])
        apply_configuration(daemon_hash, self.proxy.root_proxy.binding)

        model = self.controller.table_model
        data = model.index(0, 0).data()
        self.assertEqual(data, "KaraboDB")
        self.assertNotEqual(data, "NoKaraboDB")
        data = model.index(0, 1).data()
        self.assertEqual(data, "up, running")
        data = model.index(0, 2).data()
        self.assertEqual(data, "Tue, 05 Nov 2019 22:51:08")
        data = model.index(0, 3).data()
        self.assertEqual(data, "22.0")
        data = model.index(0, 4).data()
        self.assertEqual(data, "exflhost")
