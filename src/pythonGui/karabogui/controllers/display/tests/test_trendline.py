from karabo.native import Configurable, Bool
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from .data import build_historic_data_float
from ..trendline import DisplayTrendline


class Object(Configurable):
    prop = Bool(defaultValue=True)


class TestDisplayTrendline(GuiTestCase):
    def setUp(self):
        super(TestDisplayTrendline, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayTrendline(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', True)
        # Allow the update to propogate
        self.process_qt_events()

    def test_historic_data(self):
        data = build_historic_data_float()
        t0 = data[0]['v', ...]['sec']
        t1 = data[-1]['v', ...]['sec']
        self.controller._update_x_axis_interval(t0, t1)
        self.proxy.binding.historic_data = data
        self.process_qt_events()
