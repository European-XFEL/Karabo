from datetime import datetime
from platform import system
import unittest

from karabo.native import Configurable, String
from karabogui.controllers.trendmodel import UPTIME
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from .data import build_historic_alarm_string
from ..time_graphs import DisplayAlarmGraph


class Object(Configurable):
    alarm = String(displayType="AlarmCondition",
                   defaultValue="none")


class TestDisplayAlarmGraph(GuiTestCase):
    def setUp(self):
        super(TestDisplayAlarmGraph, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'alarm')
        self.controller = DisplayAlarmGraph(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        super(TestDisplayAlarmGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        set_proxy_value(self.proxy, 'state', 'interlock')
        self.process_qt_events()

    def test_initial_alarm(self):
        self.assertEqual(self.controller._x_detail, UPTIME)

    def test_historic_data(self):
        data = build_historic_alarm_string()
        t0 = data[0]['v', ...]['sec']
        t1 = data[-1]['v', ...]['sec']

        self.controller.set_time_interval(t0, t1)
        self.proxy.binding.historic_data = data
        self.process_qt_events()

    @unittest.skip(reason='The real range is being disturbed in CI')
    def test_datetime_update(self):
        view_box = self.controller._plot.plotItem.vb
        view_box.setXRange(1570536895, 1571536895, padding=0.05)

        # We should've changed the datetimes
        dt_start = self.controller.widget.dt_start.dateTime()
        dt_end = self.controller.widget.dt_end.dateTime()

        self.assertEqual(dt_start.toString(), 'Tue Oct 8 00:21:35 2019')
        self.assertEqual(dt_end.toString(), 'Sun Oct 20 17:54:55 2019')

    @unittest.skipIf(system() == "Windows",
                     reason="This test is Unix specific")
    def test_range_update(self):
        """When the viewbox change, we must make sure the time axis is
        not overflown"""
        plot_view = self.controller._plot
        plot_item = plot_view.plotItem

        view_box = plot_item.vb
        view_box.setXRange(datetime(1378, 12, 31).timestamp(),
                           datetime(2070, 12, 31).timestamp(), update=False)

        # Relax the assertion as the CI is non deterministic
        x_min, x_max = plot_item.getAxis("bottom").range
        self.assertEqual(datetime.utcfromtimestamp(x_min).year, 1970)
        self.assertEqual(datetime.utcfromtimestamp(x_max).year, 2038)
