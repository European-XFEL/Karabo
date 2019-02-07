from karabo.native import Configurable, VectorFloat
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..plot import DisplayPlot


class Object(Configurable):
    prop = VectorFloat()


class TestDisplayPlot(GuiTestCase):
    def setUp(self):
        super(TestDisplayPlot, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayPlot(proxy=self.proxy)
        self.controller.create(None)
        assert self.controller.widget is not None

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        curve = self.controller._curves.get(self.proxy)
        assert curve is not None

        value = [2., 4., 6.]
        set_proxy_value(self.proxy, 'prop', value)
        assert list(curve.data().yData()) == value
