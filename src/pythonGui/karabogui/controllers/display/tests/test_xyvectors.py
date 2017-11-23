from karabo.middlelayer import Configurable, VectorInt32
from karabogui.binding.api import PropertyProxy
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..xyvectors import XYVector


class Object(Configurable):
    index = VectorInt32()
    value = VectorInt32()


class TestXYVector(GuiTestCase):
    def setUp(self):
        super(TestXYVector, self).setUp()

        schema = Object.getClassSchema()
        self.index = get_class_property_proxy(schema, 'index')
        device = self.index.root_proxy
        self.value = PropertyProxy(root_proxy=device, path='value')

        self.controller = XYVector(proxy=self.index)
        self.controller.create(None)
        self.controller.visualize_additional_property(self.value)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.index.value = [1, 2, 3, 4, 5]
        self.value.value = [42]*5
