# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from datetime import UTC, datetime
from platform import system

import pytest

from karabo.native import Configurable, String
from karabogui.controllers.trendmodel import UPTIME
from karabogui.testing import get_class_property_proxy, set_proxy_value
from karabogui.util import process_qt_events

from ..time_graphs import DisplayStateGraph
from .data import build_historic_state_string


class Object(Configurable):
    state = String(defaultValue="ON")


@pytest.fixture
def display_state_graph_setup(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "state")
    controller = DisplayStateGraph(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None

    yield controller, proxy

    controller.destroy()
    assert controller.widget is None


def test_set_value(display_state_graph_setup):
    controller, proxy = display_state_graph_setup
    set_proxy_value(proxy, "state", "ACQUIRING")
    process_qt_events()


def test_initial_state(display_state_graph_setup):
    controller, _ = display_state_graph_setup
    assert controller._x_detail == UPTIME


def test_actions(display_state_graph_setup):
    controller, _ = display_state_graph_setup
    actions = controller.widget.actions()
    texts = [action.text() for action in actions]
    assert len(actions) == 7
    assert "Range Y" not in texts
    assert "View" in texts


def test_historic_data(display_state_graph_setup):
    controller, proxy = display_state_graph_setup
    data = build_historic_state_string()
    t0 = data[0]["v", ...]["sec"]
    t1 = data[-1]["v", ...]["sec"]

    controller.set_time_interval(t0, t1)
    proxy.binding.historic_data = data
    process_qt_events()


@pytest.mark.skip(reason="The real range is being disturbed in CI")
def test_datetime_update(display_state_graph_setup):
    controller, _ = display_state_graph_setup
    view_box = controller._plot.plotItem.vb
    view_box.setXRange(1570536895, 1571536895, padding=0.05)

    dt_start = controller.widget.dt_start.dateTime()
    dt_end = controller.widget.dt_end.dateTime()

    assert dt_start.toString() == "Tue Oct 8 00:21:35 2019"
    assert dt_end.toString() == "Sun Oct 20 17:54:55 2019"


@pytest.mark.skipif(system() == "Windows",
                    reason="This tests is Unix specific")
def test_range_update(display_state_graph_setup):
    controller, _ = display_state_graph_setup
    plot_view = controller._plot
    plot_item = plot_view.plotItem

    view_box = plot_item.vb
    view_box.setXRange(datetime(1378, 12, 31).timestamp(),
                       datetime(2070, 12, 31).timestamp(), update=False)

    x_min, x_max = plot_item.getAxis("bottom").range
    assert datetime.fromtimestamp(x_min, UTC).year == 1970
    assert datetime.fromtimestamp(x_max, UTC).year == 2038
