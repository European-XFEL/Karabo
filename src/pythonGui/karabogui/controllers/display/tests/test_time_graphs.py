# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from datetime import datetime
from platform import system

import pytest
from qtpy.QtCore import Qt

from karabo.native import Bool, Configurable, Float, Int32
from karabogui.controllers.trendmodel import UPTIME
from karabogui.testing import get_class_property_proxy, set_proxy_value
from karabogui.util import process_qt_events

from ..time_graphs import DisplayTrendGraph
from .data import build_historic_data_float


class Object(Configurable):
    prop = Bool(defaultValue=True)
    fprop = Float(defaultValue=1.0)
    iprop = Int32(defaultValue=2.1)


def test_trendline(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = DisplayTrendGraph(proxy=proxy)
    controller.create(None)

    set_proxy_value(proxy, "prop", True)
    process_qt_events()

    assert controller._x_detail == UPTIME

    # actions
    actions = controller.widget.actions()
    texts = [action.text() for action in actions]
    assert len(actions) == 7
    assert "Range Y" in texts
    assert "View" in texts

    # historic data
    data = build_historic_data_float()
    t0 = data[0]["v", ...]["sec"]
    t1 = data[-1]["v", ...]["sec"]

    controller.set_time_interval(t0, t1)

    proxy.binding.historic_data = data
    process_qt_events()

    # Range settings
    view_box = controller._plot.plotItem.vb
    view_box.setXRange(1570536895, 1571536895, padding=0.05)

    dt_start = controller.widget.dt_start.dateTime().toUTC()
    dt_end = controller.widget.dt_end.dateTime().toUTC()

    assert dt_start.toString(Qt.ISODate) == "2019-10-07T22:21:35Z"
    assert dt_end.toString(Qt.ISODate) == "2019-10-20T15:54:55Z"

    # Additional proxies
    assert len(controller.proxies) == 1

    float_proxy = get_class_property_proxy(schema, "fprop")
    controller.visualize_additional_property(float_proxy)
    assert len(controller.proxies) == 2

    int_proxy = get_class_property_proxy(schema, "iprop")
    controller.visualize_additional_property(int_proxy)
    assert len(controller.proxies) == 3

    controller.remove_additional_property(float_proxy)
    assert len(controller.proxies) == 2
    controller.remove_additional_property(int_proxy)
    assert len(controller.proxies) == 1

    # Destroy controller
    controller.destroy()
    assert controller.widget is None


@pytest.mark.skipif(system() == "Windows",
                    reason="This tests is Unix specific")
def test_time_graph_range_update(gui_app):
    """When the viewbox change, we must make sure the time axis is
    not overflown"""
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = DisplayTrendGraph(proxy=proxy)
    controller.create(None)

    plot_view = controller._plot
    plot_item = plot_view.plotItem

    view_box = plot_item.vb
    view_box.setXRange(datetime(1378, 12, 31).timestamp(),
                       datetime(2070, 12, 31).timestamp(), update=False)
    process_qt_events()
    # Relax the assertion as the CI is non deterministic
    x_min, x_max = plot_item.getAxis("bottom").range
    assert datetime.utcfromtimestamp(x_min).year == 1970
    assert datetime.utcfromtimestamp(x_max).year == 2038

    controller.destroy()
    assert controller.widget is None
