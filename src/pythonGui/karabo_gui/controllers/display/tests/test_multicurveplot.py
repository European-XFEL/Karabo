from karabo.middlelayer import Configurable, Bool, Int32
from karabo_gui.binding.api import PropertyProxy
from karabo_gui.testing import GuiTestCase, get_class_property_proxy
from ..multicurveplot import MultiCurvePlot


class Object(Configurable):
    reset = Bool()
    index = Int32()
    value = Int32()


class TestMultiCurvePlot(GuiTestCase):
    def setUp(self):
        super(TestMultiCurvePlot, self).setUp()

        schema = Object.getClassSchema()
        self.reset = get_class_property_proxy(schema, 'reset')
        device = self.reset.root_proxy

        self.index = PropertyProxy(root_proxy=device, path='index')
        self.value = PropertyProxy(root_proxy=device, path='value')
        self.controller = MultiCurvePlot(proxy=self.index)
        self.controller.create(None)
        self.controller.visualize_additional_property(self.reset)
        self.controller.visualize_additional_property(self.value)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.index.value = 1
        self.value.value = 42
