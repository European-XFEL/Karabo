from karabo.common.scenemodel.api import DirectoryModel
from karabo.middlelayer import Configurable, String
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..filesystem import DisplayDirectory


class Object(Configurable):
    prop = String(displayType='directory')


class TestDisplayDirectory(GuiTestCase):
    def setUp(self):
        super(TestDisplayDirectory, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayDirectory(proxy=self.proxy,
                                           model=DirectoryModel())
        self.controller.create(None)
        assert self.controller.widget is not None

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.proxy.value = 'hello'
        assert self.controller.widget.text() == 'hello'
