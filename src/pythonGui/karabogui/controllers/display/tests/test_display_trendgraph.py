import unittest

from karabo.native import Configurable, Bool
from karabogui.controllers.display import trendline
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from .data import build_historic_data_float
from ..display_trendgraph import DisplayTrendGraph


class Object(Configurable):
    prop = Bool(defaultValue=True)


class TestDisplayTrendGraph(GuiTestCase):
    def setUp(self):
        super(TestDisplayTrendGraph, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayTrendGraph(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        super(TestDisplayTrendGraph, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', True)
        self.process_qt_events()

    def test_initial_state(self):
        self.assertEqual(self.controller._x_detail, trendline.UPTIME)

    def test_historic_data(self):
        data = build_historic_data_float()
        t0 = data[0]['v', ...]['sec']
        t1 = data[-1]['v', ...]['sec']

        self.controller.set_interval(t0, t1)

        self.proxy.binding.historic_data = data
        self.process_qt_events()

    @unittest.skip(reason='The real range is being disturbed in CI')
    def test_range_update(self):
        view_box = self.controller._karabo_plot_view.plotItem.vb
        view_box.setXRange(1570536895, 1571536895, padding=0.05)

        # We should've changed the datetimes
        dt_start = self.controller.widget.dt_start.dateTime()
        dt_end = self.controller.widget.dt_end.dateTime()

        self.assertEqual(dt_start.toString(), 'Tue Oct 8 00:21:35 2019')
        self.assertEqual(dt_end.toString(), 'Sun Oct 20 17:54:55 2019')
